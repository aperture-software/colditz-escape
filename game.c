/*
 *  Colditz Escape - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2017 Aperture Software
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  ---------------------------------------------------------------------------
 *  game.h: Game runtime functions
 *  ---------------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(WIN32)
#include <windows.h>
#elif defined(PSP)
#include <stdarg.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <psp/psp-printf.h>
#endif

#include "low-level.h"
#include "soundplayer.h"
#include "colditz.h"
#include "graphics.h"
#include "game.h"
#include "cluck.h"
#include "eschew/eschew.h"
#include "conf.h"


/* Some more globals */
uint8_t     obs_to_sprite[NB_OBS_TO_SPRITE];
uint8_t     remove_props[CMP_MAP_WIDTH][CMP_MAP_HEIGHT];
uint8_t     overlay_order[MAX_OVERLAYS];
// Do we need to reload the files on newgame?
bool        game_restart = false;
uint8_t     nb_animations = 0;
s_animation animations[MAX_ANIMATIONS];
s_guybrush  guybrush[NB_GUYBRUSHES];


int	currently_animated[MAX_ANIMATIONS];
uint32_t    exit_flags_offset;
// Pointer to the message ID list of the currently allowed rooms
uint32_t    authorized_ptr;
uint32_t    next_timed_event_ptr = TIMED_EVENTS_INIT;
// We end up using these variables all the time
// making them global allows for more elegant code
uint16_t    room_x, room_y;
int16_t     tile_x, tile_y;
uint32_t    offset;
// And these one are used to break the footprint check into more
// readable functions
// 4 values for upper-left, upper-right, lower-left, lower_right tiles
uint32_t    mask_offset[4];		// tile boundary
uint32_t    exit_offset[4];		// exit boundary
uint8_t     tunexit_tool[4];	// tunnel tool
int16_t     exit_dx[2];
s_sfx       sfx[NB_SFXS];

#if defined(PSP)
// Additional SFX
short*          upcluck;
unsigned long   upcluck_len;
short*          upthrill;
unsigned long   upthrill_len;
#endif

// These are the offsets to the solitary cells doors for each prisoner
// We use them to make sure the doors are closed after leaving a prisoner in
uint32_t solitary_cells_door_offset[NB_NATIONS][2] = { {0x3473, 0x34C1}, {0x3FD1, 0x3F9F},
                                                       {0x92A1, 0x3FA9}, {0x92C3, 0x3FAD} };

// Bummer! The way they setup their animation overlays and the way we
// do it to be more efficient means we need to define a custom table
// to find out animations that loop
/*
ROM:000089EA animation_data: dc.l walk_ani           ; DATA XREF: display_sprites+AEo
ROM:000089EA                                         ; #00
ROM:000089EE                 dc.l run_ani            ; #04
ROM:000089F2                 dc.l emerge_ani         ; #08
ROM:000089F6                 dc.l kneel_ani          ; #0C
ROM:000089FA                 dc.l sleep_ani          ; #10
ROM:000089FE                 dc.l wtf_ani            ; #14
ROM:00008A02                 dc.l ger_walk_ani       ; #18
ROM:00008A06                 dc.l ger_run_ani        ; #1C
ROM:00008A0A                 dc.l fireplace_ani      ; #20
ROM:00008A0E                 dc.l door1_ani          ; #24
ROM:00008A12                 dc.l door2_ani          ; #28
ROM:00008A16                 dc.l door3_ani          ; #2C
ROM:00008A1A                 dc.l door4_ani          ; #30
ROM:00008A1E                 dc.l door5_ani          ; #34
ROM:00008A22                 dc.l door6_ani          ; #38
ROM:00008A26                 dc.l crawl_ani          ; #3C
ROM:00008A2A                 dc.l ger_crawl_ani      ; #40
ROM:00008A2E                 dc.l shot_ani           ; #44
ROM:00008A32                 dc.l ger_shot_ani       ; #48
ROM:00008A36                 dc.l ger_shoot_ani      ; #4C
ROM:00008A3A                 dc.l kneel2_ani         ; #50
ROM:00008A3E                 dc.l kneel3_ani         ; #54
ROM:00008A42                 dc.l ger_kneel_ani      ; #58
*/
const uint8_t looping_animation[NB_ANIMATED_SPRITES] =
    { 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0 };


// Uncompress the PowerPacked LOADTUNE.MUS if needed
void depack_loadtune()
{
    size_t read;
    uint32_t length;
    uint8_t *ppbuffer, *buffer;

    // Don't bother if we already have an uncompressed LOADTUNE
    if ((fd = fopen (mod_name[MOD_LOADTUNE], "rb")) != NULL)
    {
        fclose(fd);
        return;
    }

    // No uncompressed LOADTUNE? Look for the PowerPacked one
    printf("Couldn't find file '%s'\n  Trying to use PowerPacked '%s' instead\n", mod_name[0], PP_LOADTUNE_NAME);

    if ((fd = fopen (PP_LOADTUNE_NAME, "rb")) == NULL)
    {
        printf("  Can't find '%s' - Aborting.\n", PP_LOADTUNE_NAME);
        return;
    }

    if ( (ppbuffer = (uint8_t*) calloc(PP_LOADTUNE_SIZE, 1)) == NULL)
    {
        printf("  Could not allocate source buffer for ppunpack\n");
        fclose(fd);
        return;
    }

    // So far so good
    read = fread (ppbuffer, 1, PP_LOADTUNE_SIZE, fd);
    fclose(fd);

    // Is it the file we are looking for?
    if (read != PP_LOADTUNE_SIZE)
    {
        printf("  '%s': Unexpected file size or read error\n", PP_LOADTUNE_NAME);
        free(ppbuffer); return;
    }

    if (ppbuffer[0] != 'P' || ppbuffer[1] != 'P' ||
        ppbuffer[2] != '2' || ppbuffer[3] != '0')
    {
        printf("  '%s': Not a PowerPacked file\n", PP_LOADTUNE_NAME);
        free(ppbuffer); return;
    }

    // The uncompressed length is given at the end of the file
    length = read24(ppbuffer, PP_LOADTUNE_SIZE-4);

    if ( (buffer = (uint8_t*) calloc(length,1)) == NULL)
    {
        printf("  Could not allocate destination buffer for ppunpack\n");
        free(ppbuffer); return;
    }

    printf("  Uncompressing...");
    // Call the PowerPacker unpack subroutine
    ppDecrunch(&ppbuffer[8], buffer, &ppbuffer[4], PP_LOADTUNE_SIZE-12, length, ppbuffer[PP_LOADTUNE_SIZE-1]);
    free(ppbuffer);

    // We'll play MOD directly from files, so write it
    printf("  OK.\n  Now saving file as '%s'\n", mod_name[0]);
    if ((fd = fopen (mod_name[0], "wb")) == NULL)
    {
        printf("  Couldn't create file '%s'\n", mod_name[0]);
        free(buffer); return;
    }

    read = fwrite (buffer, 1, length, fd);
    if (read != length)
        printf("  '%s': write error.\n", mod_name[0]);

    printf("  DONE.\n\n");

    fclose(fd);
    free(buffer);
}

// free all allocated data
void free_data()
{
    int i;
    free_xml();
    free_gfx();
    for (i=0; i<NB_FILES; i++)
        SAFREE(fbuffer[i]);
    audio_release();
}

// Initial file loader
void load_all_files()
{
    size_t read;
    uint16_t i;
    int compressed_loader = 0;

    // We need a little padding of the loader to keep the offsets happy
    fsize[LOADER] += LOADER_PADDING;

    for (i=0; i<NB_FILES; i++)
    {
        if ( (fbuffer[i] = (uint8_t*) aligned_malloc(fsize[i], 16)) == NULL)
        {
            printf("Could not allocate buffers\n");
            ERR_EXIT;
        }

        if (i==LOADER)
        {
            fbuffer[LOADER] += LOADER_PADDING;
            fsize[LOADER] -= LOADER_PADDING;
        }

        if ((fd = fopen (fname[i], "rb")) == NULL)
        {
            printf("Couldn't find file '%s'\n", fname[i]);

            /* Take care of the compressed loader if present */
            if (i == LOADER)
            {
                // Uncompressed loader was not found
                // Maybe there's a compressed one?
                printf("  Trying to use compressed loader '%s' instead\n", ALT_LOADER);
                if ((fd = fopen (ALT_LOADER, "rb")) == NULL)
                {
                    printf("  '%s' not found - Aborting.\n", ALT_LOADER);
                    ERR_EXIT;
                }
                // OK, file was found - let's allocated the compressed data buffer
                if ((mbuffer = (uint8_t*) aligned_malloc(ALT_LOADER_SIZE, 16)) == NULL)
                {
                    printf("  Could not allocate source buffer for loader decompression\n");
                    ERR_EXIT;
                }

                read = fread (mbuffer, 1, ALT_LOADER_SIZE, fd);
                if ((read != ALT_LOADER_SIZE) && (read != ALT_LOADER_SIZE2))
                {
                    printf("  '%s': Unexpected file size or read error\n", ALT_LOADER);
                    ERR_EXIT;
                }
                compressed_loader = 1;

                printf("  Uncompressing...");
                if (uncompress(fsize[LOADER]))
                {
                    printf("  Error!\n");
                    ERR_EXIT;
                }

                if (read == ALT_LOADER_SIZE2)   // SKR_COLD NTSC FIX, with one byte diff
                    writebyte(fbuffer[LOADER], 0x1b36, 0x67);

                printf("  OK.\n  Now saving file as '%s'\n",fname[LOADER]);
                if ((fd = fopen (fname[LOADER], "wb")) == NULL)
                {
                    printf("  Can't create file '%s'\n", fname[LOADER]);
                    ERR_EXIT;
                }

                // Write file
                read = fwrite (fbuffer[LOADER], 1, fsize[LOADER], fd);
                if (read != fsize[LOADER])
                {
                    printf("  '%s': Unexpected file size or write error\n", fname[LOADER]);
                    ERR_EXIT;
                }
                printf("  DONE.\n\n");
            }
            else
                ERR_EXIT;
        }

        // Read file (except in the case of a compressed loader)
        if (!((i == LOADER) && (compressed_loader)))
        {
            printv("Reading file '%s'...\n", fname[i]);
            read = fread (fbuffer[i], 1, fsize[i], fd);
            if (read != fsize[i])
            {
                printf("'%s': Unexpected file size or read error\n", fname[i]);
                ERR_EXIT;
            }
        }

        fclose (fd);
        fd = NULL;
    }

    // OK, now we can reset our LOADER's start address
    fbuffer[LOADER] -= LOADER_PADDING;
}


// Reload the files for a game restart
void reload_files()
{
    size_t read;
    uint32_t i;

    for (i=0; i<NB_FILES_TO_RELOAD; i++)
    {
        if ((fd = fopen (fname[i], "rb")) == NULL)
        {
            perrv ("fopen()");
            printf("Can't find file '%s'\n", fname[i]);
            return;
        }
        // Read file
        printv("Reloading file '%s'...\n", fname[i]);
        read = fread (fbuffer[i], 1, fsize[i], fd);
        if (read != fsize[i])
        {
            perrv ("fread()");
            printf("'%s': Unexpected file size or read error\n", fname[i]);
            ERR_EXIT;
        }

        fclose (fd);
        fd = NULL;
    }
}

// Reset the variables relevant to a new game
static __inline void init_guard(int i)
{
    int j;

    guard(i).px = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE + 2);
    guard(i).p2y = 2*readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE);
    guard(i).room = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE+ 4);
    guard(i).state = 0;
    guard(i).speed = 1;
    guard(i).direction = 0;
    guard(i).wait = 0;
    guard(i).go_on = 0;
    guard(i).is_dressed_as_guard = true;
    guard(i).reinstantiate = false;
    guard(i).reset_animation = true;
    guard(i).is_onscreen = false;
    guard(i).target = NO_TARGET;
    guard(i).resume_px = GET_LOST_X;
    for (j=0; j<NB_NATIONS; j++)
        guard(i).fooled_by[j] = false;
    // We also need to initialize the current route pos offset for guards (0x0E)
    // simply copy over the route start offset (0x06)
    writelong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE+0x0E,
        readlong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE+0x06));
}

void newgame_init()
{
    uint16_t  i,j;

    // Reset all cheats
    if (opt_thrillerdance)
    {
        thriller_toggle();
        opt_thrillerdance = false;
    }
    for (i=0; i<NB_NATIONS; i++)
        opt_play_as_the_safe[i] = false;
    opt_meh	= false;
    opt_keymaster = false;
    opt_no_guards = false;
    opt_haunted_castle = false;

    if (game_restart)
    {
        reload_files();
        fix_files(true);
    }

    // clear the events array
    for (i=0; i< NB_EVENTS; i++)
        events[i].function = NULL;

    // Set the default nation
    current_nation = BRITISH;

    // Initialize the prisoners
    for (i=0; i<NB_NATIONS; i++)
    {
        guy(i).px = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*i+2)-16;
        guy(i).p2y = 2*readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*i)-8;
        guy(i).room = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*i+4);
        guy(i).state = 0;
        guy(i).speed = 1;
        guy(i).direction = 6;
        guy(i).ext_bitmask = 0x8000001E;
        guy(i).is_dressed_as_guard = false;
        p_event[i].unauthorized = false;
        p_event[i].require_pass = false;
        p_event[i].require_papers = false;
        p_event[i].to_solitary = false;
        p_event[i].display_shot = false;
        p_event[i].killed = false;
        p_event[i].solitary_release = 0;
        p_event[i].escaped = false;
        p_event[i].thrown_stone = false;

        p_event[i].fatigue = 0;
        p_event[i].pass_grace_period_expires = 0;
        for (j=0; j<NB_PROPS; j++)
            props[i][j] = 0;
    }


// FOR DEBUG
/*
    guy(0).p2y += 300;
    guy(0).room = 9;
    guy(0).ext_bitmask = 0x8000001E;
    guy(0).is_dressed_as_guard = true;
    guy(1).px += 100;
    guy(1).p2y += 220;
    guy(1).room = 9;
*/

    // Initialize the guards
    for (i=0;i<NB_GUARDS;i++)
        init_guard(i);

    // Read the number of pickable props and fill in the sprites table
    nb_objects = readword(fbuffer[OBJECTS],0) + 1;
    for (i=0; i<NB_OBS_TO_SPRITE; i++)
        obs_to_sprite[i] = readbyte(fbuffer[LOADER],OBS_TO_SPRITE_START+i);

    // This will be needed to hide the pickable objects on the outside map
    // if the removable walls are set
    for (i=0; i<CMP_MAP_WIDTH; i++)
        for (j=0; j<CMP_MAP_HEIGHT; j++)
            remove_props[i][j] = 0;


    // Setup the start time
    hours_digit_h = 0;
    hours_digit_l = 9;
    minutes_digit_h = 3;
    minutes_digit_l = 0;


// MORE DEBUG
/*
    minutes_digit_h = 5;
    minutes_digit_l = 4;
*/

    // Setup our initial event state
    next_timed_event_ptr = TIMED_EVENTS_INIT;
    // Start event is #3 (confined to quarters)
    authorized_ptr = readlong(fbuffer[LOADER],AUTHORIZED_BASE+4*3);

    if (game_restart)
    {	// Reset the palette
        palette_index = INITIAL_PALETTE_INDEX;
        to_16bit_palette(palette_index, 0xFF, PALETTES);
        cells_to_wGRAB(fbuffer[CELLS],rgbCells);
        sprites_to_wGRAB();
    }

    // Reset the room props & animations
    init_animations = true;
    set_room_props();

    // Reinit the time markers;
    // NB: to have the clock go at full throttle, set ctime to 0 and game_time to a high value
    t_last = mtime();
    game_time = 0;
    last_ctime = 0;
    last_atime = 0;
    last_ptime = 0;

    // Notify that we'll need a reload next time around
    game_restart = true;
}

/*
 *	SAVE AND LOAD FUNCTIONS
 */
#define SAVE_SINGLE(el)  if (fwrite(&el, sizeof(el), 1, fd) != 1) return false
#define SAVE_ARRAY(ar)   if (fwrite(ar, sizeof(ar[0]), SIZE_A(ar), fd) != SIZE_A(ar)) return false;
#define SAVE_BUFFER(buf) if (fwrite(fbuffer[buf], 1, fsize[buf], fd) !=  fsize[buf]) return false;
#define LOAD_SINGLE(el)  if (fread(&el, sizeof(el), 1 , fd) != 1) return false
#define LOAD_ARRAY(ar)   if (fread(ar, sizeof(ar[0]), SIZE_A(ar), fd) != SIZE_A(ar)) return false;
#define LOAD_BUFFER(buf) if (fread(fbuffer[buf], 1, fsize[buf], fd) !=  fsize[buf]) return false;
bool save_game(char* save_name)
{
    int i;

    if ((fd = fopen(save_name, "wb")) == NULL)
        return false;

    // Save the current nation
    SAVE_SINGLE(current_nation);
    SAVE_SINGLE(palette_index);
    SAVE_SINGLE(hours_digit_h);
    SAVE_SINGLE(hours_digit_l);
    SAVE_SINGLE(minutes_digit_h);
    SAVE_SINGLE(minutes_digit_l);
    SAVE_SINGLE(next_timed_event_ptr);
    SAVE_SINGLE(authorized_ptr);
    SAVE_SINGLE(game_time);
    SAVE_SINGLE(last_ctime);
    SAVE_SINGLE(last_atime);
    SAVE_SINGLE(last_ptime);

    SAVE_ARRAY(guybrush);
    SAVE_ARRAY(p_event);
    SAVE_ARRAY(selected_prop);
    for (i=0; i<NB_NATIONS; i++)
        SAVE_ARRAY(props[i])
    for (i=0; i<NB_FILES_TO_SAVE; i++)
        SAVE_BUFFER(i);

    fclose(fd);
    return true;

}

bool load_game(char* load_name)
{
    int i,j;
    if ((fd = fopen(load_name, "rb")) == NULL)
        return false;

    LOAD_SINGLE(current_nation);
    LOAD_SINGLE(palette_index);
    LOAD_SINGLE(hours_digit_h);
    LOAD_SINGLE(hours_digit_l);
    LOAD_SINGLE(minutes_digit_h);
    LOAD_SINGLE(minutes_digit_l);
    LOAD_SINGLE(next_timed_event_ptr);
    LOAD_SINGLE(authorized_ptr);
    LOAD_SINGLE(game_time);
    LOAD_SINGLE(last_ctime);
    LOAD_SINGLE(last_atime);
    LOAD_SINGLE(last_ptime);

    LOAD_ARRAY(guybrush);
    LOAD_ARRAY(p_event);
    LOAD_ARRAY(selected_prop);
    for (i=0; i<NB_NATIONS; i++)
        LOAD_ARRAY(props[i])
    for (i=0; i<NB_FILES_TO_SAVE; i++)
        LOAD_BUFFER(i);


    // clear a few arrays
    for (i=0; i< NB_EVENTS; i++)
        events[i].function = NULL;
    for (i=0; i<CMP_MAP_WIDTH; i++)
        for (j=0; j<CMP_MAP_HEIGHT; j++)
            remove_props[i][j] = 0;

    // Restore the palette
    to_16bit_palette(palette_index, 0xFF, PALETTES);
    cells_to_wGRAB(fbuffer[CELLS],rgbCells);
    sprites_to_wGRAB();

    // Reset the room props & animations
    init_animations = true;
    set_room_props();

    // Update time
    t_last = mtime();

    fclose(fd);

    return true;
}


// Simple event handler
void enqueue_event(void (*f)(uint32_t), uint32_t p, uint64_t delay)
{
    uint8_t i;

    // find an empty event to use
    for (i=0; i< NB_EVENTS; i++)
        if (events[i].function == NULL)
            break;

    if (i == NB_EVENTS)
    {
        perr("Couldn't enqueue event!!!\n");
        return;
    }

    events[i].function = f;
    events[i].parameter = p;
    events[i].expiration_time = game_time + delay;
}

// Returns the last frame of an animation (usually the centered position)
int get_stop_animation_sid(uint8_t ani_index, bool is_guybrush)
{
    uint8_t frame;
    int sid;
    uint32_t ani_base;
    int16_t dir;
    s_animation* p_ani;

    // Pointer to the animation structure
    p_ani = is_guybrush?&guybrush[ani_index].animation:&animations[ani_index];
    // Our index will tell us which animation sequence we use (walk, run, kneel, etc.)
    ani_base = readlong(fbuffer[LOADER], ANIMATION_OFFSET_BASE + 4*p_ani->index);
    // Guybrushes animations need to handle a direction, others do not
    dir = is_guybrush?guybrush[ani_index].direction:0;
    // With the direction and animation base, we can get to the base SID of the ani sequence
    sid = readbyte(fbuffer[LOADER], ani_base + 0x0A + dir);
    // find out the index of the last animation frame
    frame = readbyte(fbuffer[LOADER], ani_base) - 1;
    sid += readbyte(fbuffer[LOADER], readlong(fbuffer[LOADER], ani_base + 0x06) + frame);
    return sid;
}

// Returns an animation frame
// index is either the animation[] array index (standard overlays) or the guybrush[] array index
int get_animation_sid(uint8_t ani_index, bool is_guybrush)
{
    uint8_t sid_increment;
    int sid;
    uint32_t ani_base;
    int32_t frame, nb_frames;
    int16_t dir;
    s_animation* p_ani;
    uint8_t sfx_id;

    // Pointer to the animation structure
    p_ani = is_guybrush?&guybrush[ani_index].animation:&animations[ani_index];
    // read the base sid
    ani_base = readlong(fbuffer[LOADER], ANIMATION_OFFSET_BASE + 4*p_ani->index);
    dir = is_guybrush?guybrush[ani_index].direction:0;
    sid = readbyte(fbuffer[LOADER], ani_base + 0x0A + dir);
    nb_frames = readbyte(fbuffer[LOADER], ani_base);	// offset 0 is nb frames max
//  printb("sid base = %04X, p_ani->framecount = %d\n", sid, p_ani->framecount);
//  printb("p_ani = %p, ani_index = %d, p_ani->index = %d, dir = %d\n", p_ani, ani_index, p_ani->index, dir);

    if ( (!(looping_animation[p_ani->index])) && (p_ani->framecount >= nb_frames) )
    {	// end of one shot animations
        frame = nb_frames - 1;	// 0 indexed
        if (p_ani->end_of_ani_function != NULL)
        {	// execute the end of animation function (toggle exit)
            p_ani->end_of_ani_function(p_ani->end_of_ani_parameter);
            p_ani->end_of_ani_function = NULL;
        }
    }
    else
    {	// one shot (non end) or loop
        frame = p_ani->framecount % nb_frames;
    }
    sid_increment = readbyte(fbuffer[LOADER],
        readlong(fbuffer[LOADER], ani_base + 0x06) + frame);
//  printb("frame = %d/%d, increment = %x\n", frame, nb_frames, sid_increment);
    if (sid_increment == 0xFF)
    {	// play a sound
        sfx_id = readbyte(fbuffer[LOADER],
            readlong(fbuffer[LOADER], ani_base + 0x06) + frame + 1);
        play_sfx(sfx_id);
        sid_increment = readbyte(fbuffer[LOADER],
            readlong(fbuffer[LOADER], ani_base + 0x06) + frame + 2);
        p_ani->framecount += 2;
    }
    if (sid_increment & 0x80)
        sid = REMOVE_ANIMATION_SID;
    else
        sid += sid_increment;

    return sid;
}


// This function sets the room_x, room_y as well as the offset global variables
// usually called before checking some room properties
void set_room_xy(uint16_t room)
{
    if (room == ROOM_OUTSIDE)
    {	// on the compressed map
        room_x = CMP_MAP_WIDTH;
        room_y = CMP_MAP_HEIGHT;
        offset = 0;
    }
    else
    {	// in a room (inside)
        offset = CRM_ROOMS_START + readlong((uint8_t*)fbuffer[ROOMS], CRM_OFFSETS_START+4*room);
        // Note that offset might be 0xFFFFFFFF for some room indexes as there
        // is a gap in the middle of the CRM file. We do not check this here
        room_y = readword((uint8_t*)fbuffer[ROOMS], offset);
        offset +=2;
        room_x = readword((uint8_t*)fbuffer[ROOMS], offset);
        offset +=2;		// remember offset is used in readtile/readexit and needs
                        // to be constant from there on
    }
}


// Populates the tile overlays, if we are on Colditz Rooms Map
void crm_set_overlays(int16_t x, int16_t y, uint16_t current_tile)
{
    uint16_t tile2_data;
    uint16_t i;
    int16_t sx, sy;
    uint16_t sid;
    int animated_sid;	// sprite index

    animated_sid = 0;	// 0 is a valid sid, but not for overlays, so we
                        // can use it as "false" flag
    // read current tile
    for (i=0; i<(12*NB_SPECIAL_TILES); i+=12)
    {
        if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i) != current_tile)
            continue;

        if (current_tile == FIREPLACE_TILE)
        {	// The fireplace is the only animated overlay we need to handle beside exits
            if (init_animations)
            {	// Setup animated tiles, if any
                currently_animated[0] = nb_animations;
                animations[nb_animations].index = FIREPLACE_ANI;
                animations[nb_animations].framecount = 0;
                animations[nb_animations].end_of_ani_function = NULL;
                safe_nb_animations_increment();
            }
            // Even if there's more than one fireplace per room, their sids will match
            // so we can use currently_animated[0] for all of them. Other room animations
            // will go at currently_animated[1+]
            animated_sid = get_animation_sid(currently_animated[0], false);
        }

        sx = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+8);
//		printb("  match: %04X, direction: %04X\n", current_tile, sx);
        if (i >= (12*(NB_SPECIAL_TILES-4)))
        // The four last special tiles are exits. We need to check is they are open
        {
            // Get the exit data (same tile if tunnel, 2 rows down if door)
            tile2_data = readword(fbuffer[ROOMS], offset +
            // careful that ? take precedence over +, so if you don't put the
            // whole ?: increment in parenthesis, you have a problem
                ((i==(12*(NB_SPECIAL_TILES-1)))?0:(4*room_x)));

            // Validity check
            if (!(tile2_data & 0x000F))
                // This is how I know that we can use the exit # as ani index
                // and leave index 0 for the fireplace ani
                perr("set_overlays: Integrity check failure on exit tile\n");

            // The door might be in animation
            if ((currently_animated[tile2_data & 0x000F] >= 0) &&
                (currently_animated[tile2_data & 0x000F] < 0x70))
                // the trick of using the currently_animated table to find the door
                // direction works because the exit sids are always > 0x70
                animated_sid = get_animation_sid(currently_animated[tile2_data & 0x000F], false);
            else
                currently_animated[tile2_data & 0x000F] = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);

            // if the tile is an exit and the exit is open
            if (tile2_data & 0x0010)
            {	// door open
//				printb("    exit open: ignoring overlay\n");
                // The second check on exits is always an FA00, thus we can safely
                break;
            }
        }

        if (sx < 0)
            tile2_data = readword(fbuffer[ROOMS], offset-2) & 0xFF80;
        else
            tile2_data = readword(fbuffer[ROOMS], offset+2) & 0xFF80;
        // ignore if special tile that follows is matched
        if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2) == tile2_data)
        {
//			printb("    ignored as %04X matches\n", tile2_data);
            continue;
        }

        if (animated_sid == REMOVE_ANIMATION_SID)
        // ignore
            continue;

        sid = (animated_sid)?animated_sid:readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);
        overlay[overlay_index].sid = (uint8_t)sid;

//		printb("    overlay as %04X != %04X => %X\n", tile2_data,
//			readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2), sid);
        sy = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+6);
//		printb("    sx: %04X, sy: %04X\n", sx, sy);

        overlay[overlay_index].x = x + sx - sprite[sid].w + sprite[sid].x_offset;
        overlay[overlay_index].y = y + sy - sprite[sid].h + 1;

        // No need to bother if the overlay is offscreen (with generous margins)
        if ((overlay[overlay_index].x < -64) || (overlay[overlay_index].x > (PSP_SCR_WIDTH+64)))
            continue;
        if ((overlay[overlay_index].y < -64) || (overlay[overlay_index].y > (PSP_SCR_HEIGHT+64)))
            continue;

        // Update the z index according to our current y pos
        if (sprite[sid].z_offset == MIN_Z)
            overlay[overlay_index].z = MIN_Z;
        else
            // PSP_SCR_HEIGHT/2 is our actual prisoner position on screen
            overlay[overlay_index].z = overlay[overlay_index].y - sprite[sid].z_offset
                - PSP_SCR_HEIGHT/2 + NORTHWARD_HO - 2;
        safe_overlay_index_increment();
        // No point in looking for overlays any further if we met our match
        // UNLESS this is a double bed overlay, in which case the same tile
        // needs to be checked for a double match (in both in +x and -x)
        if (current_tile != 0xEF00)
            break;
    }
}


// Populates the tile overlays, if we are on the CoMPressed map
void cmp_set_overlays()
{
    uint16_t i;
    uint32_t bitset;
    short sx, sy;
    uint8_t	exit_nr;
    int sid;	// sprite index
    uint8_t io_file = ROOMS;	// We'll need to switch to TUNNEL_IO midway through

    // We're on the compressed map
    room_x = CMP_MAP_WIDTH;

    for (i=0; i<(4*OUTSIDE_OVL_NB+4*TUNNEL_OVL_NB); i+=4)
    {
        if (i==(4*OUTSIDE_OVL_NB))
            io_file = TUNNEL_IO;	// switch IO file

        // The relevant bit (byte[0]) from the bitmask must be set
        bitset = 1 << (readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i));
        if (!(rem_bitmask & bitset))
            continue;
        // But only if the bit identified by byte[1] is not set
        bitset = 1 << (readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i+1));
        if (rem_bitmask & bitset)
            continue;

        // OK, now we know that our removable section is meant to show an exit

        // First, let's grab the base sid
        offset = CMP_OVERLAYS + (readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i+3) << 3);
        sid = readword(fbuffer[LOADER], offset+4);

        // Next read the pixel shifts on the tile
        sx = readword(fbuffer[LOADER], offset+2);
        sy = readword(fbuffer[LOADER], offset);

        // Then add the tile position, as identified in the 8 bytes data at the beginning of
        // the Colditz Rooms Map or Tunnel_IO files,
        offset = readbyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+i+2) << 3;
        // check if the exit is open. This is indicated with bit 12 of the first word
        if (readword(fbuffer[io_file],offset) & 0x1000)
            continue;

        tile_x = readword(fbuffer[io_file], offset+6);
        tile_y = readword(fbuffer[io_file], offset+4);

        sx += tile_x * 32;
        sy += tile_y * 16;

        // Don't forget the displayable area offset
        overlay[overlay_index].x = gl_off_x + sx - sprite[sid].w + sprite[sid].x_offset;
        ignore_offscreen_x(overlay_index);	// Don't bother if offscreen
        overlay[overlay_index].y = gl_off_y + sy - sprite[sid].h + 1;
        ignore_offscreen_y(overlay_index);	// Don't bother if offscreen

        // OK, now let's deal with potential door animations
        if (i<(4*OUTSIDE_OVL_NB))
        {	// we're dealing with a door overlay, possibly animated
            // Get the exit_nr (which we need for animated overlays)
            exit_nr = readexit(tile_x, tile_y);

            if ((currently_animated[exit_nr] >= 0) && (currently_animated[exit_nr] < 0x70))
            // get the current animation frame on animated overlays
                sid = get_animation_sid(currently_animated[exit_nr], false);
            else
            // if it's not animated, set the sid in the table, so we can find out
            // our type of exit later on
                currently_animated[exit_nr] = sid;
        }

        if (sid == REMOVE_ANIMATION_SID)	// ignore doors that have ended their animation cycle
            continue;

        // Now we have our definitive sid
        overlay[overlay_index].sid = sid;

        // Update the z index according to our current y pos
        if (sprite[sid].z_offset == MIN_Z)
            overlay[overlay_index].z = MIN_Z;
        else
            // PSP_SCR_HEIGHT/2 is our actual prisoner position on screen
            overlay[overlay_index].z = overlay[overlay_index].y - sprite[sid].z_offset
                - PSP_SCR_HEIGHT/2 + NORTHWARD_HO -3;

        safe_overlay_index_increment();
    }
}


// Read the props (pickable objects) from obs.bin
// For efficiency reasons, this is only done when switching room
void set_room_props()
{
    uint16_t prop_offset;

    nb_room_props = 0;
    for (prop_offset=2; prop_offset<(8*nb_objects+2); prop_offset+=8)
    {
        if (readword(fbuffer[OBJECTS],prop_offset) != current_room_index)
            continue;

        room_props[nb_room_props] = prop_offset;
        nb_room_props++;
    }
}


// Set the props overlays
void set_props_overlays()
{
    uint8_t u;
    uint32_t prop_offset;
    uint16_t x, y;

    // reset the stand over prop
    over_prop = 0;
    over_prop_id = 0;
    for (u=0; u<nb_room_props; u++)
    {
        prop_offset = room_props[u];

        if (prop_offset == 0)
        // we might have picked the prop since last time
            continue;

        overlay[overlay_index].sid = obs_to_sprite[readword(fbuffer[OBJECTS],prop_offset+6)];

        // Man, this positioning of sprites sure is a bleeping mess,
        // with weird offsets having to be manually added everywhere!
        x = readword(fbuffer[OBJECTS],prop_offset+4) - 15;
        y = readword(fbuffer[OBJECTS],prop_offset+2) - 4;

        overlay[overlay_index].x = gl_off_x + x;
        ignore_offscreen_x(overlay_index);
        overlay[overlay_index].y = gl_off_y + y + sprite[overlay[overlay_index].sid].y_offset;
        ignore_offscreen_y(overlay_index);

        // We also take this oppportunity to check if we stand over a prop
        if ( (prisoner_x >= x-9) && (prisoner_x < x+8) &&
             (prisoner_2y/2 >= y-9) && (prisoner_2y/2 < y+8) )
        {
            over_prop = u+1;	// 1 indexed
            over_prop_id = readbyte(fbuffer[OBJECTS],prop_offset+7);
            // The props message takes precedence
            set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER],
                PROPS_MESSAGE_BASE + 4*(over_prop_id-1)), 1, PROPS_MESSAGE_TIMEOUT);
//			printb("over_prop = %x, over_prop_id = %x\n", over_prop, over_prop_id);
        }

        // all the props should appear behind overlays, expect the ones with no mask
        // (which are always set at MIN_Z)
        overlay[overlay_index].z = MIN_Z+1;

        // Because of the removable walls we have a special case for the CMP_MAP
        if (!(is_outside && (remove_props[x/32][y/16])))
            // Don't add overlay if covered by a wall
            safe_overlay_index_increment();
    }
}

// We need a sort to reorganize our overlays according to z
// We'll use "merge" sort (see: http://www.sorting-algorithms.com/) here,
// but we probably could have gotten away with "shell" sort
void sort_overlays(uint8_t a[], uint8_t n)
{
    uint8_t m,i,j,k;
    static uint8_t b[MAX_OVERLAYS/2+1];

    if (n < 2)
        return;

    // split in half
    m = n >> 1;

    // recursive sorts
    sort_overlays(a, m);
    sort_overlays(a+m, n-m);

    // merge sorted sub-arrays using temp array
    // b = copy of a[1..m]
    for (i=0; i<m; i++)
        b[i] = a[i];

    i = 0; j = m; k = 0;
    while ((i < m) && (j < n))
        a[k++] = (overlay[a[j]].z < overlay[b[i]].z) ? a[j++] : b[i++];
        // => invariant: a[1..k] in final position
    while (i < m)
        a[k++] = b[i++];
        // => invariant: a[1..k] in final position
}

// The compressed map (outside) uses a series of overlaid tiles to bring some walls
// up or down, according to a bitmask
void removable_walls()
{	// Get the current removable walls mask to apply

    uint16_t tile_data;
    uint32_t cmp_data;
    uint32_t tmp_bitmask;
    uint8_t  bit_index;
    uint32_t dir_offset;
    int register rdx, rdy;
    uint8_t  register cmp_x, cmp_y;

    rdx = prisoner_x - last_p_x;
    rdy = (prisoner_2y/2) - last_p_y;
    // If no motion, exit
    if ((rdx == 0) && (rdy == 0))
        return;

    // Compute the tile on which we stand
    tile_y = prisoner_2y / 32;
    tile_x = prisoner_x / 32;

    // Read a longword in the first part of the compressed map
    // The compressed map elements are of the form
    // OOOO OOOO OOOO OOOT TTTT TTTT IIII II DD
    // where:
    // OOO OOOO OOOO OOOO is the index for overlay tile (or 0 for tiles without cmp map overlays)
    // T TTTT TTTT        is the base tile index (tile to display with all overlays removed)
    // II IIII            is the removable_mask index to use when positionned on this tile
    //                    (REMOVABLES_MASKS_LENGTH possible values)
    // DD                 is the index for the direction subroutine to pick
    cmp_data = readlong((uint8_t*)fbuffer[COMPRESSED_MAP], (tile_y*CMP_MAP_WIDTH+tile_x)*4);

    tile_data = (uint16_t)((uint32_t)(cmp_data & 0x1FF00) >> 1);

    bit_index = ((uint8_t)cmp_data) & 0xFC;
    if (bit_index == 0)
        return;

    // direction "subroutine" to use (diagonal, horizontal...)
    dir_offset = ((uint8_t)cmp_data) & 0x03;

    // read the mask with relevant removable turned on, associated to the tile
    tmp_bitmask = readlong((uint8_t*)fbuffer[LOADER],  REMOVABLES_MASKS_START + bit_index);

    if ((tile_data <= 0x480) || (tile_data == 0x5100) || (tile_data == 0x5180) ||
        (tile_data == 0x6100) || (tile_data == 0x6180))
    // ignore if blank or exit (including tunnel exit)
    // nb: if it is an exit, lower 5 bits are the exit number
        return;

    // direction "subroutines":
    switch(dir_offset)
    {
    case 0:
        if (rdy > 0)
        // moving down and having crossed the horizontal
        // boundary (set at the tile top)
        // => turn the relevant removable visible
            rem_bitmask = tmp_bitmask;
        break;
    case 1:
        if (rdy < 0)
        {	// moving up and having crossed the horizontal
            // boundary (set at tile bottom)
            // => turn the relevant removable invisible
            tmp_bitmask &= ~(1 << (bit_index >> 2));	// don't forget to rotate dammit!
            rem_bitmask = tmp_bitmask;
        }
        break;
    case 2:
        // check the crossing of a bottom-left to top-right diagonal
        cmp_x = ~(prisoner_x/2) & 0xF;	// need to invert x
        cmp_y = (prisoner_2y/2) & 0xF;
        if ( ((rdx > 0) && (rdy == 0)) || (rdy > 0) )
        {	// into the bottom "quadrant"
            if (cmp_x <= cmp_y)
            {	// turn removable on
                rem_bitmask = tmp_bitmask;
            }
        }
        else
        {	// into the top "quadrant"
            if (cmp_x >= cmp_y)
            {	// turn removable off
                tmp_bitmask &= ~(1 << (bit_index >> 2));
                rem_bitmask = tmp_bitmask;
            }
        }
        break;
    case 3:
        // check the crossing of a top-left to bottom-right diagonal
        cmp_x = (prisoner_x/2) & 0xF;
        cmp_y = (prisoner_2y/2) & 0xF;
        if ( ((rdx < 0) && (rdy == 0)) || (rdy > 0) )
        {	// into the bottom "quadrant"
            if (cmp_x <= cmp_y)
            {	// turn removable on
                rem_bitmask = tmp_bitmask;
            }
        }
        else
        {	// into the top "quadrant"
            if (cmp_x >= cmp_y)
            {	// turn removable off
                tmp_bitmask &= ~(1 << (bit_index >> 2));
                rem_bitmask = tmp_bitmask;
            }
        }
        break;
    }
}


// Display the 8 bit debug index n at overlay pos (x,y,z), using the clock's digits
void debug_index(int16_t x, int16_t y, int16_t z, uint8_t n)
{	// Display guard numbers, using the clock's digits

    // units
    overlay[overlay_index].x = x;
    overlay[overlay_index].y = y;
    overlay[overlay_index].z = z;
    overlay[overlay_index].sid = PANEL_CLOCK_DIGITS_BASE + n%10;
    safe_overlay_index_increment();

    if (n > 10)
    {
        overlay[overlay_index].x = x - 8;
        overlay[overlay_index].y = y;
        overlay[overlay_index].z = z;
        overlay[overlay_index].sid = PANEL_CLOCK_DIGITS_BASE + (n%100)/10;
        safe_overlay_index_increment();
    }

    if (n > 100)
    {
        overlay[overlay_index].x = x - 16;
        overlay[overlay_index].y = y;
        overlay[overlay_index].z = z;
        overlay[overlay_index].sid = PANEL_CLOCK_DIGITS_BASE + (n%1000)/100;
        safe_overlay_index_increment();
    }
}

static __inline uint32_t get_base_ani_index(int i)
{
    if (guy(i).state & STATE_TUNNELING)
        return (guy(i).is_dressed_as_guard)?GUARD_CRAWL_ANI:CRAWL_ANI;
    else if (guy(i).state & STATE_SHOT)
        return (guy(i).is_dressed_as_guard)?GUARD_SHOT_ANI:SHOT_ANI;
    else if (guy(i).state & STATE_SLEEPING)
        return SLEEP_ANI;
    else if (guy(i).state & STATE_AIMING)
        return GUARD_SHOOTS_ANI;
    else if (guy(i).speed == 1)
        return (guy(i).is_dressed_as_guard)?GUARD_WALK_ANI:WALK_ANI;
    else
        return (guy(i).is_dressed_as_guard)?GUARD_RUN_ANI:RUN_ANI;
}


// Places individuals on the map
void add_guybrushes()
{
uint8_t i, sid;

    // Add our current prisoner's animation (DO NOT RESET if kneeling!)
    if (prisoner_reset_ani && !(prisoner_state & STATE_KNEELING))
    {	// Set the animation for our prisoner
        if (in_tunnel)
            prisoner_speed = 1;
        prisoner_ani.index = get_base_ani_index(current_nation);
        prisoner_ani.framecount = 0;
        prisoner_ani.end_of_ani_function = NULL;
        guy(current_nation).reset_animation = false;
    }

    // Always display our main guy
    if (!opt_thrillerdance)
        sid = get_guybrush_sid(current_nation);
    else
    {	// Might as well moonwalk while we're at it!
        prisoner_dir = invert_dir[prisoner_dir];
        sid = get_guybrush_sid(current_nation);
        prisoner_dir = invert_dir[prisoner_dir];
    }

    overlay[overlay_index].sid = (opt_sid == -1)?sid:opt_sid;

    // If you uncomment the lines below, you'll get confirmation that our position
    // computations are right to position our guy to the middle of the screen
//	overlay[overlay_index].x = gl_off_x + guybrush[PRISONER].px + sprite[sid].x_offset;
    overlay[overlay_index].y = gl_off_y + guybrush[current_nation].p2y/2 - sprite[sid].h + (in_tunnel?11:5);
    overlay[overlay_index].x = PSP_SCR_WIDTH/2 + sprite[sid].x_offset - (in_tunnel?24:0);
//	overlay[overlay_index].y = PSP_SCR_HEIGHT/2 - NORTHWARD_HO - 32;

    // Our guy's always at the center of our z-buffer
    overlay[overlay_index].z = 0;
    // Who cares about optimizing for one guy!
    if(!(p_event[current_nation].escaped))
        // Ignore this overlay if our guy is free
        safe_overlay_index_increment();

    // Now add all the other guys
    for (i=0; i< (opt_no_guards?NB_NATIONS:NB_GUYBRUSHES); i++)
    {
        // Our current guy has already been taken care of above
        if (i==current_nation)
            continue;

        // Everybody is offscreen by default. NB: this is only used for guards,
        // so don't care if the prisoner's onscreen status is wrong
        guy(i).is_onscreen = false;

        // Guy already on the loose?
        if ((i<NB_NATIONS) && (p_event[i].escaped))
            continue;

        // Guybrush's probably blowing his foghorn in the library again
        if (guy(i).room != current_room_index)
            continue;

        // How I wish there was an easy way to explain these small offsets we add
        // NB: The positions we compute below are still missing the sprite dimensions
        // which we will only add at the end. They are just good enough for ignore_offscreen()
        overlay[overlay_index].x = gl_off_x + guy(i).px; // + sprite[sid].x_offset;
        ignore_offscreen_x(overlay_index);	// Don't bother if offscreen
        overlay[overlay_index].y = gl_off_y + guy(i).p2y/2 + 5; //  - sprite[sid].h + 5;
        ignore_offscreen_y(overlay_index);	// Don't bother if offscreen

        // If the guy's under a removable wall, we ignore him too
        if (is_outside && (remove_props[guy(i).px/32][(guy(i).p2y+4)/32]))
            // TO_DO: check for an actual props SID?
            continue;

        // Ideally, we would remove animations that have gone offscreen here, but
        // there's little performance to be gained in doing so, so we don't
        // We do set the onscreen flag though
        guy(i).is_onscreen = true;

//		printb("guard(%x).is_onscreen\n", i-4);

        // First we check if the relevant guy's animation was ever initialized
        if (guy(i).reset_animation)
        {	// We need to initialize that guy's animation
            guy(i).animation.index = get_base_ani_index(i);
            guy(i).animation.framecount = 0;
            guy(i).animation.end_of_ani_function = NULL;
            guy(i).reset_animation = false;
        }

        // OK, now we're good to add the overlay sprite
        if (!opt_thrillerdance)
            sid = get_guybrush_sid(i);

        // And now that we have the sprite attributes, we can add the final position adjustments
        if (i < NB_NATIONS)
        {	// prisoners
            overlay[overlay_index].x += sprite[sid].x_offset - ((guy(i).state&STATE_TUNNELING)?24:0);
            overlay[overlay_index].y -= sprite[sid].h - ((guy(i).state&STATE_TUNNELING)?6:0);
        }
        else
        {	// guards
            overlay[overlay_index].x += sprite[sid].x_offset - 16;
            overlay[overlay_index].y -= sprite[sid].h + 4;
        }
        overlay[overlay_index].sid = sid;

        overlay[overlay_index].z = overlay[overlay_index].y - sprite[sid].z_offset
                - PSP_SCR_HEIGHT/2 + NORTHWARD_HO - 3;
        safe_overlay_index_increment();
        if (guy(i).fooled_by[current_nation])
        {
            overlay[overlay_index].x = overlay[overlay_index-1].x + 12;
            overlay[overlay_index].y = overlay[overlay_index-1].y - 10;
            overlay[overlay_index].z = overlay[overlay_index-1].z;
            overlay[overlay_index].sid = FOOLED_BY_SPRITE;
            safe_overlay_index_increment();
        }
#if defined(DEBUG_ENABLED)
        // DEBUG: Display guard numbers, using the clock's digits
        if ((opt_onscreen_debug) && (i > NB_NATIONS))
        {
            debug_index(overlay[overlay_index-1].x - (guy(i).fooled_by[current_nation]?20:8),
                overlay[overlay_index-1].y - (guy(i).fooled_by[current_nation]?0:10),
                overlay[overlay_index-1].z, i-NB_NATIONS);
        }
#endif
    }

    if (opt_play_as_the_safe[current_nation])
    {
        overlay[overlay_index].sid = 0x91;
        overlay[overlay_index].x = PSP_SCR_WIDTH/2 - 9;
        overlay[overlay_index].y = PSP_SCR_HEIGHT/2 - NORTHWARD_HO - 32 - ((prisoner_state&STATE_MOTION)?10:6);
        overlay[overlay_index].z = 0;
        safe_overlay_index_increment();
    }

}

// Called after the shot animation has finished playing
void prisoner_killed(uint32_t p)
{
    p_event[p].display_shot = true;
    // Prevent the sprite from being animated
    guy(p).state = STATE_SHOT;
}


void stop_guards_in_pursuit(uint32_t p)
{
    int g;

    for (g=0; g<NB_GUARDS; g++)
        if (guard(g).target == p)
            guard(g).state &= ~(STATE_MOTION|STATE_ANIMATED);
}

void reinstantiate_guards_in_pursuit(uint32_t p)
{
    int g;
    for (g=0; g<NB_GUARDS; g++)
        if (guard(g).target == p)
        {
            init_guard(g);
//			printb("reinstantiate_guards_in_pursuit %d\n", g);
        }
}

void reinstantiate_guard_delayed(uint32_t g)
{
    guard(g).state = 0;
    guard(g).reinstantiate = true;
    guard(g).wait = RESET_GUARD_MAX_TIMEOUT;
    guard(g).target = NO_TARGET;
//	printb("reinstantiate_single_delayed %d\n", g);
}


void reset_guard_delayed(uint32_t g)
{
    guard(g).state = STATE_RESUME_ROUTE_WAIT;
    guard(g).speed = 1;
    guard(g).wait = RESET_GUARD_MIN_TIMEOUT + rand()%RESET_GUARD_MAX_TIMEOUT;
    guard(g).target = NO_TARGET;
}

void reset_guards_in_pursuit(uint32_t p)
{
    int g;
//	printb("reset_guards_in_pursuit() called\n");
    for (g=0; g<NB_GUARDS; g++)
        if (guard(g).target == p)
            reset_guard_delayed(g);
}

// This one will clear the in pursuit state for the prisonner if he kept quiet for a while
void clear_pursuit(uint32_t p)
{
    int j;
//	printb("in clear_pursuit for prisoner %d\n", p);

    if (!(guy(p).state & STATE_IN_PURSUIT))
        return;
    for (j=0; j<NB_GUARDS; j++)
        if ((guard(j).target == p))
        {
//			printb("clear_pursuit: guard %d still in chase\n", j);
            return;
        }
    guy(p).state &= ~STATE_IN_PURSUIT;
//	printb("clear_pursuit: cleared!\n");
}





// These helper functions are used by guard_in_pursuit() & move_guards()
static __inline bool guard_close_by(i, pos_x, pos_2y)
{
int16_t dx, dy;
    dx = pos_x+16 - guard(i).px;
    dy = pos_2y+8 - guard(i).p2y;
    if ( ((dx>=0 && dx<=144) || (dx<0 && dx>=-144)) &&
         ((dy>=0 && dy<=160)  || (dy<0 && dy>-160)) )
         return true;
    return false;
}


static __inline bool guard_collision(i, pos_x, pos_2y)
{
int16_t dx, dy;
    dx = pos_x+16 - guard(i).px;
    dy = pos_2y+8 - guard(i).p2y;
    if ( ((dx>=0 && dx<=10) || (dx<0 && dx>=-10)) &&
         ((dy>=0 && dy<=8)  || (dy<0 && dy>-8)) )
         return true;
    return false;
}


// Reposition a guard according to its route data (or continue in the same direction)
void route_guard(int i)
{
    int dir_x, dir_y;
    uint32_t route_pos;
    uint16_t route_data;
    uint16_t g_px, g_py, g_room;
//	bool route_reset;

    if (guard(i).reinstantiate)
    {	// Attempt to reset to first pos of route

        // As long as we're onscreen, we'll reset the wait to reinstantiate counter
        // This prevents the player to see guards disappear who were there just a second ago
        if (guard(i).is_onscreen)
        {
            guard(i).wait = RESET_GUARD_MAX_TIMEOUT;
            return;	// Can't do anything for now
        }

        // If we have a countdown running (for reinstantiation), decrement it
        if (guard(i).wait > 0)
            guard(i).wait--;

        if (guard(i).wait == 0)
        {	// Reset route
            route_data = 0xFFFF;
//			route_reset = true;
        }
        else
            return;	// Don't do anything for now
    }
    // No reinstantiation request
    // Go_on > 0 indicates that we don't need to read any new route data
    else if (guard(i).go_on > 0)
    {
        guard(i).go_on--;
        // This counter is used to make sure a guard follows at least a portion
        // of its planned route after entering a room, before engaging in pursuits
        guard(i).spent_in_room++;

        // If it's not a pause in route
        if (guard(i).state & STATE_MOTION)
        {
            dir_x = guard(i).speed * dir_to_dx[guard(i).direction];
            dir_y = guard(i).speed * dir_to_d2y[guard(i).direction];
            guard(i).px += dir_x;
            guard(i).p2y += dir_y;

        }

        return;
    }
    else
    {
        // Change in route => get our current route position
        route_pos = readlong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE + 0x0E);

        // Read the first word
        route_data = readword(fbuffer[ROUTES], route_pos);
    }

    if (route_data == 0xFFFF)
    {	// repeat => back to start of route
        g_px = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE + 2);
        g_py = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE);
        g_room = readword(fbuffer[GUARDS],i*MENDAT_ITEM_SIZE + 4);

        // Only reinstantiate if destination is offscreen
        if ( (guard(i).reinstantiate) && (g_room == current_room_index) &&
             ((!is_offscreen_x(gl_off_x + g_px)) || (!is_offscreen_y(gl_off_y + g_py))) )
             return;

        guard(i).px = g_px;
        guard(i).p2y = 2*g_py;
        guard(i).room = g_room;
        guard(i).state = 0;
        guard(i).speed = 1;
        // Reset variables
        guard(i).reset_animation = true;	// reset the animation
        guard(i).reinstantiate = false;
        guard(i).state = 0;
        guard(i).target = NO_TARGET;

        route_pos = readlong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE + 0x06);
        route_data = readword(fbuffer[ROUTES], route_pos);

//		if (route_reset)
//			printb("route: reset for guard %d\n", i);
    }

    if (route_data & 0x8000)
    {	// absolute positioning (eg. when switching rooms)
        g_room = readword(fbuffer[ROUTES], route_pos + 2);
        // The spent in room counter is used only for pre-pursuit anti blocking
        if (g_room != guard(i).room)
            guard(i).spent_in_room = 0;
        guard(i).room = g_room;
        guard(i).px = readword(fbuffer[ROUTES], route_pos + 6);
        guard(i).p2y = 2*readword(fbuffer[ROUTES], route_pos + 4);
        route_pos +=10;
    }
    else
    {	// standard route action
        guard(i).go_on = route_data; // How long do we need to keep at it
        route_data =  readword(fbuffer[ROUTES], route_pos + 2);
        if (route_data == 0xFFFF)
        {	// stopped state (pause)
            guard(i).state &= ~STATE_MOTION;
        }
        else
        {	// motion state
            guard(i).direction = route_data;
            guard(i).state |= STATE_MOTION;
            // Change our position
            guard(i).px += guard(i).speed * dir_to_dx[guard(i).direction];
            guard(i).p2y += guard(i).speed * dir_to_d2y[guard(i).direction];
        }
        route_pos += 4;
    }

    // save the new current position
    writelong(fbuffer[GUARDS], i*MENDAT_ITEM_SIZE+0x0E, route_pos);
}


// Go through the various pursuit states
// Returns true if the prisoner has been caught
bool guard_in_pursuit(int i, int p)
{
    int j, dir_x, dir_y;

    // 0. Pre-pursuit delay
    // The spent_in_room counter is used only for the pre-pursuit feature, that is
    // to make sure that a guard that just went in the same room as a prisoner in chase
    // is given some time to fully enter the room. This prevents instant blocking of guards
    if  (guard(i).spent_in_room < PRE_PURSUIT_TIMEOUT)
        return false;

    // 1. Walking pursuit
    if ((!(guard(i).state & STATE_IN_PURSUIT)) || p_event[p].thrown_stone)
    {	// Start walking towards prisoner
        // Indicate that we deviate from the normal flight path
//		printb("guard %d walk starts\n", i);
        // save the start of pursuit position (if blank)
        if (opt_enhanced_guards && (guard(i).resume_px == GET_LOST_X))
        {
            guard(i).resume_px = guard(i).px;
            guard(i).resume_p2y = guard(i).p2y;
            guard(i).resume_motion = guard(i).state & STATE_MOTION;
            guard(i).resume_direction = guard(i).direction;
//			printb("%d left route at (%d,%d), go_on = %d, direction = %d, motion = %d\n", i,
//				guard(i).px, guard(i).p2y, guard(i).go_on, guard(i).direction, guard(i).resume_motion);
        }
        guard(i).state = STATE_IN_PURSUIT|STATE_MOTION;
        // This might be a guard that was waiting to reinstantiate
        guard(i).reinstantiate = false;
        // Set our target
        guard(i).target = p;
        // Set guard to walk
        guard(i).speed = 1;
        guard(i).wait = p_event[p].thrown_stone?STONE_THROWN_TIMEOUT:WALKING_PURSUIT_TIMEOUT;
        guard(i).reset_animation = true;
    }
    // 2. Running pursuit
    else if ((guard(i).state & STATE_MOTION) && (guard(i).speed == 1) && (guard(i).wait == 0))
    {	// Start running towards prisoner
//		printb("guard %d run starts\n", i);
        guard(i).speed = 2;
        guard(i).wait = RUNNING_PURSUIT_TIMEOUT;
        guard(i).reset_animation = true;
    }
    // 3. Aiming
    else if ((guard(i).state & STATE_MOTION) && (guard(i).speed == 2) && (guard(i).wait == 0))
    {	// We were running, now we're pissed off => License to kill
        // ALL guards in pursuit (that were not already about to shoot) take aim,
        // but we'll give the prisoner one last small chance before we shoot
        for (j=0; j<NB_GUARDS; j++)
        {
            if ((guard(j).target == p) && (!(guard(j).state & STATE_AIMING)))
            {
//				printb("guard %d aiming\n", j);
                guard(j).state &= ~STATE_MOTION;
                guard(j).state |= STATE_AIMING|STATE_ANIMATED;
                guard(j).wait = SHOOTING_GUARD_TIMEOUT;
                guard(j).animation.index = GUARD_SHOOTS_ANI;
                guard(j).animation.framecount = 0;
            }
        }
    }
    // 4. Shooting (prisoner still moving) or repeat aiming (prisoner motionless)
    else if ((guard(i).state & STATE_AIMING) && (guard(i).wait == 0))
    {	// End of the pause for aiming => Unless you stopped, you're dead man
        if (guy(p).state & STATE_MOTION && !opt_play_as_the_safe[p])
        {	// Moving prisoners make good targets
//			printb("guard %d shoots\n", i);
            // Stop the prisoner and set shot animation
            guy(p).state = STATE_SHOT|STATE_ANIMATED;
            guy(p).animation.end_of_ani_parameter = p;
            guy(p).animation.end_of_ani_function = prisoner_killed;
            guy(p).animation.index = guy(p).is_dressed_as_guard?GUARD_SHOT_ANI:SHOT_ANI;
            guy(p).animation.framecount = 0;
            // Set the event flags
            p_event[p].unauthorized = false;
            // Play the relevant SFX
            play_sfx(SFX_SHOOT);
            // Stop all the guards in pursuit
            stop_guards_in_pursuit(p);
        }
        else
        {	// The prisoner has stopped => give him another chance
            // Restart the run counter
//			printb("new chance from %d\n", i);
            guard(i).wait = RUNNING_PURSUIT_TIMEOUT;
        }
    }

    // 5. Check catching up and update motion
    if (guard_collision(i, guy(p).px, guy(p).p2y))
    {
//		printb("gotcha! from %d\n", i);
        if (guy(p).is_dressed_as_guard)
        // Ask for a pass
            p_event[p].require_pass = true;
        else
            p_event[p].to_solitary = true;

        stop_guards_in_pursuit(p);

        // Remember who caught us, so that we can nicely
        // tell him to go to hell later on
        guy(p).target = i;

        // As we don't move the guard, we...
        return true;
    }

    // Update the guard's direction (also applies when shooting)
    dir_x = (guard(i).px - guy(p).px - 16)/2;
    // If we don't divide by 2 here, we'll have jerky motion on pursuit
    dir_y = (guard(i).p2y - guy(p).p2y - 8)/2;

    if (dir_x != 0)
        dir_x = (dir_x>0)?-1:1;
    dir_x++;

    if (dir_y !=0)
        dir_y = (dir_y>0)?-1:1;
    dir_y++;

//	if ((guard(i).state & STATE_IN_PURSUIT) && (guard(i).direction != directions[dir_y][dir_x]))
//		printb("changing direction for %d from %d to %d\n", i,guard(i).direction, directions[dir_y][dir_x]);

    guard(i).direction = directions[dir_y][dir_x];

    return false;
}


// Handle the repositioning of guards
// Returns true if we need to stop the prisoner's motion
bool move_guards()
{
    int  i, p;
    bool continue_parent;
    bool but_i_just_got_out;
    bool do_i_know_you;
    int	 kill_motion;
    int	 dir_x, dir_y;

    kill_motion = false;
    for (i=0; i<NB_GUARDS; i++)
    {
        if (opt_thrillerdance && guard(i).is_onscreen)
        {
            dir_x = (prisoner_state & STATE_MOTION)?dir_to_dx[prisoner_dir]:0;
            dir_y = (prisoner_state & STATE_MOTION)?dir_to_d2y[prisoner_dir]:0;
            if (check_guard_footprint(i, dir_x, dir_y))
            {	// true means there's no obstacle in the way
                guard(i).px += dir_x;
                guard(i).p2y += dir_y;
            }
            continue;
        }

        // We'll use this variable to break this loop from a child loop if needed
        continue_parent = false;

        // This one is to make sure that we execute at least one more step from
        // the route at the end a the blocked timeout
        // (to prevent the blocking of guards by unattended prisoners)
        but_i_just_got_out = false;

        // If we have a wait active, decrement it
        if (guard(i).wait != 0)
            guard(i).wait--;

        // 1. Check if we have a collision between our current prisoner and the guard
        //    and kill our motion as a result...
        if ((guard(i).room == current_room_index) && guard_collision(i, prisoner_x, prisoner_2y) &&
            // ...unless we're trying to get out
            (prisoner_dir != DIRECTION_STOPPED) &&
            guard_collision(i, prisoner_x+2*dir_to_dx[prisoner_dir], prisoner_2y+2*dir_to_d2y[prisoner_dir]) )
                kill_motion = true;

        // 2. Deal with guards that are currently being blocked by a prisoner
        if ((guard(i).state & STATE_BLOCKED) && guard(i).blocked_by_prisoner)
        {
            // Did our blocking counter just reach zero
            if (guard(i).wait == 0)
            {
                // Is our prisoner blocked but still trying to get out at the end of the guard's pause?
                if ((kill_motion) && (prisoner_dir != DIRECTION_STOPPED))
                {
                    // Prevent blocking (butter guard!)
                    kill_motion = false;
                    continue;
                }

                // Nicely restore to STATE_MOVE or STATE_STOP
                guard(i).state ^= STATE_BLOCKED;
                but_i_just_got_out = true;

                // Not issuing a continue here allows us to progress one step further
                // even if blocked
            }
            else
            // ignore motion
                continue;
        }

        // 3. Check for an event with one of the prisoners
        for (p = 0; p<NB_NATIONS; p++)
        {
            // Don't bother with prisoners that are dead, escaped or being handled by a guard
            if ( (guy(p).state & STATE_SHOT) || (p_event[p].escaped) ||
                 (p_event[p].require_pass) || (p_event[p].to_solitary) )
                continue;

            // Also don't bother with this guy if we're in pursuit of another prisoner
            if ((guard(i).state & STATE_IN_PURSUIT) && (guard(i).target != p))
                continue;

            // Alrighty, do we have our prisoner in sight then?
            if ( (guard(i).room == guy(p).room) && guard_close_by(i, guy(p).px, guy(p).p2y) )
            {
                // Handle stooge
                if (guy(p).state & STATE_STOOGING)
                {	// Stooge tripwire => set our stooge as the active guy
                    guy(p).state ^= STATE_STOOGING;
                    if (p != current_nation)
                        switch_nation(p);
                    return 0;
                }

                // For clarity purposes
                do_i_know_you = opt_enhanced_guards && guy(p).is_dressed_as_guard &&
                    guard(i).fooled_by[p];

                // 3d. If that prisoner is not suspicious yet, should he be?
                if ( (!(guy(p).state & STATE_IN_PURSUIT)) && p_event[p].unauthorized &&
                     // there's a grace period after handling a pass
                     (game_time > p_event[p].pass_grace_period_expires) &&
                     // And, in the enhanced version, guards remember when they've seen a pass
                     (!do_i_know_you)
                   )
                {
//					printb("in pursuit set for prisoner %d by %d\n", p, i);
//					printb("by the way, %d's state is %X\n", i, guy(i).state);
                    guy(p).state |= STATE_IN_PURSUIT;
                }

                if ((guy(p).state & STATE_IN_PURSUIT) && (!do_i_know_you) && !(opt_meh))
                    // Act on pursuit
                    guard_in_pursuit(i, p);
                else
                {	// Prisoner is not suspicious. Just check if he's in our way
                    if (guard_collision(i, guy(p).px, guy(p).p2y)
                    // Allow us to do one step if we just exited a blocked timeout loop
                          && (!but_i_just_got_out))
                    {
                        // Setup the blocked counter
                        guard(i).wait = BLOCKED_GUARD_TIMEOUT;
                        // And indicate that we are stopped (and who's blocking us)
                        guard(i).state |= STATE_BLOCKED;
                        guard(i).blocked_by_prisoner = true;
                        // Ah shoot, we need to continue the parent "for" loop
                        continue_parent = true;
                        // Break this loop then
                        break;
                    }
                }
            }
            else
            {	// Did we just lose track of our prisoner?
                if ((guard(i).state & STATE_IN_PURSUIT) && (guard(i).target == p))
                {
//					printb("LOS on %d\n", i);
                    if (opt_enhanced_guards)
                    {
//						printb("delayed pursuit reset from %d for %d\n", i, p);
                        // Clear the in pursuit flag if our prisoner behaved in the next minute
                        enqueue_event(clear_pursuit, p, 60000);
                        reset_guard_delayed(i);
                    }
                    else
                        reinstantiate_guard_delayed(i);
                    continue_parent = true;
                    break;
                }
            }
        }

        // Messy, but works
        if (continue_parent)
            continue;

        if (opt_enhanced_guards)
        {	// 4. Return to our route if we finished a pursuit
            if (guard(i).state & STATE_RESUME_ROUTE_WAIT)
            {
                if (guard(i).wait == 0)
                {
                    guard(i).state = STATE_RESUME_ROUTE|STATE_MOTION;
                    guard(i).reset_animation = true;
//					printb("resume route wait end for %d\n", i);
                }
                continue;
            }

            if (guard(i).state & STATE_RESUME_ROUTE)
            {
                if ((guard(i).px == guard(i).resume_px) && (guard(i).p2y == guard(i).resume_p2y))
                {	// Back on track
                    guard(i).state = 0;
                    guard(i).resume_px = GET_LOST_X;
                    guard(i).wait = 0;
                    if (guard(i).resume_motion)
                        guard(i).state = STATE_MOTION;
                    guard(i).reset_animation = true;
                    guard(i).direction = guard(i).resume_direction;
//					printb("%d resumed route at (%d,%d), go_on = %d, direction = %d, motion = %d\n", i,
//						guard(i).px, guard(i).p2y, guard(i).go_on, guard(i).direction, guard(i).resume_motion);
                }
                else
                {	// Return to route
                    dir_x = guard(i).px - guard(i).resume_px;
                    dir_y = guard(i).p2y - guard(i).resume_p2y;

                    if (dir_x != 0)
                        dir_x = (dir_x>0)?-1:1;
                    dir_x++;

                    if (dir_y !=0)
                        dir_y = (dir_y>0)?-1:1;
                    dir_y++;

                    guard(i).direction = directions[dir_y][dir_x];
                }
            }
        }

        if ((guard(i).state & DEVIATED_FROM_ROUTE) && (!guard(i).reinstantiate))
        {	// Not using the standard route
            if (guard(i).state & STATE_MOTION)
            {	// Find out if there's an obstacle in the way
                dir_x = guard(i).speed * dir_to_dx[guard(i).direction];
                dir_y = guard(i).speed * dir_to_d2y[guard(i).direction];
                if (check_guard_footprint(i, dir_x, dir_y))
                {	// true means there's no obstacle in the way
                    guard(i).state &= ~STATE_BLOCKED;
                    guard(i).px += dir_x;
                    guard(i).p2y += dir_y;
                }
                else
                {	// roadblock
//					printb("guard %d blocked\n", i);
                    guard(i).state |= STATE_BLOCKED;
                    // Indicate that we're blocked by a non-prisoner obstacle
                    guard(i).blocked_by_prisoner = false;
                    // Alright, we're not gonna use an A star algorithm to route oursevles around obstacles
                    // if our resume route is blocked. Just reinstantiate when offscreen and be done with it
                    if (guard(i).state & STATE_RESUME_ROUTE)
                    {
                        reinstantiate_guard_delayed(i);
                        // We take this opportunity to switch the direction we're facing to the opposite of
                        // where we were headed, so that we don't look too out of place until reinstantiation
                        guard(i).direction = invert_dir[guard(i).direction];
                    }
                }
            }
        }
        else
        // Use standard guard routing
            route_guard(i);

    }
    return kill_motion;
}




// Handle timed game events (palette change, rollcalls, ...)
void timed_events(uint16_t hours, uint16_t minutes_high, uint16_t minutes_low)
{
    uint16_t event_data;
    uint16_t p, iff_id;

    // Read the hour (or reset marker)
    event_data = readword(fbuffer[LOADER], next_timed_event_ptr);

    // Negative => reset
    if (event_data & 0x8000)
        next_timed_event_ptr = TIMED_EVENTS_BASE;
        // NB: we'll use the next comparison to safely return on above event

    if (event_data != hours)
        return;

    // Read the minutes tens
    event_data = readword(fbuffer[LOADER], next_timed_event_ptr+2);
    if (event_data != minutes_high)
        return;

    // Read the minutes units
    event_data = readword(fbuffer[LOADER], next_timed_event_ptr+4);
    if (event_data != minutes_low)
        return;

    // Time match => deal with the event
    event_data = readword(fbuffer[LOADER], next_timed_event_ptr+6);

    // Change the palette
    if (event_data == TIMED_EVENT_PALETTE)
    {
        palette_index = readbyte(fbuffer[LOADER], next_timed_event_ptr+9);
        to_16bit_palette(palette_index, 0xFF, PALETTES);
        cells_to_wGRAB(fbuffer[CELLS],rgbCells);
        sprites_to_wGRAB();
        next_timed_event_ptr += 10;
    }
    else
    {	// Rollcall, etc.
///		printb("got event %04X\n", event_data);
        // Each event changes the list of authorized rooms
        authorized_ptr = readlong(fbuffer[LOADER],AUTHORIZED_BASE+4*event_data);

        // Get the relevant picture index (for events with static images)
        // according to the IFF_INDEX_TABLE in the loader
        iff_id = readword(fbuffer[LOADER], IFF_INDEX_TABLE + 2*event_data);
///		printb("iff to load = %x\n", current_iff);
        if (iff_id != 0xFFFF)
            static_screen((uint8_t)iff_id, NULL, 0);
        else if (event_data == TIMED_EVENT_ROLLCALL_CHECK)
        {	// This is the actual courtyard rollcall check
            for (p=0; p<NB_NATIONS; p++)
            {
                if ( (!(guy(p).state & STATE_IN_PRISON)) && (guy(p).room != ROOM_OUTSIDE) )
                    // neither outside nor in prison => catch him!
                    guy(p).state |= STATE_IN_PURSUIT;
            }
        }
        next_timed_event_ptr += 8;
    }
}


// Open a closed door, or close an open door
// Makes use of exit_flags_offset which is a global variable
void toggle_exit(uint32_t exit_nr)
{
    uint8_t	ROOMS_TUNIO;
    uint16_t exit_index;	// exit index in destination room
    uint16_t tile_data;
    bool found;
    uint8_t	exit_flags;
    uint16_t target_room_index;
    // Don't use the globals here, or exit handling will go screwie!!!
    uint16_t _room_x, _room_y;
    int16_t _tile_x, _tile_y;
    uint32_t _offset;

    // Restore the ability to consume a key (prevents double key consumption issue)
    can_consume_key = true;

    // On the compressed map, we use either the ROOMS or TUNNEL_IO file
    // depending on the exit type.
    // Exit indexes for tunnel IO are offset by 0x100, thus
    ROOMS_TUNIO = (exit_nr>=0x100)?TUNNEL_IO:ROOMS;

    if (is_outside)
    {
        // Toggle the exit we are facing
        exit_flags = readbyte(fbuffer[ROOMS_TUNIO], exit_flags_offset);
        toggle_open_flag(exit_flags);
        writebyte(fbuffer[ROOMS_TUNIO], exit_flags_offset, exit_flags);

        // Get target:
        // If we are on the compressed map, we need to read 2 words (out of 4)
        // from beginning of the ROOMS_MAP file or from TUNNEL_IO if tunnelling
        _offset = (exit_nr&0xFF) << 3;	// skip 8 bytes
        target_room_index = readword((uint8_t*)fbuffer[ROOMS_TUNIO], _offset) & 0x7FF;
        exit_index = readword((uint8_t*)fbuffer[ROOMS_TUNIO], _offset+2);
    }
    else
    {
        // Toggle the exit we are facing
        exit_flags = readbyte(fbuffer[ROOMS], exit_flags_offset);
        toggle_open_flag(exit_flags);
        writebyte(fbuffer[ROOMS], exit_flags_offset, exit_flags);

        // Get target by reading from the ROOMS_EXIT_BASE data
        exit_index = (exit_nr&0xF)-1;
        _offset = current_room_index << 4;
        // Now the real clever trick here is that the exit index of the room you
        // just left and the exit index of the one you go always match.
        // Thus, we know where we should get positioned on entering the room
        target_room_index = readword((uint8_t*)fbuffer[ROOMS], ROOMS_EXITS_BASE + _offset
            + 2*exit_index);
    }

    exit_index++;	// zero based to one based

    if (target_room_index & 0x8000)
    {	// outside destination (compressed map)
        _room_x = CMP_MAP_WIDTH;	// keep our readtile macros happy
        // NB: The ground floor rooms are in [00-F8]
        _offset = target_room_index & 0xF8;
        // set the mirror door to open
        exit_flags = readbyte(fbuffer[ROOMS_TUNIO], _offset);
        toggle_open_flag(exit_flags);
        writebyte(fbuffer[ROOMS_TUNIO], _offset, exit_flags);
    }
    else
    {	// inside destination (colditz_room_map)
        // Get the room dimensions
        _offset = readlong((uint8_t*)fbuffer[ROOMS], CRM_OFFSETS_START+4*target_room_index);
        _room_y = readword((uint8_t*)fbuffer[ROOMS], CRM_ROOMS_START+_offset);
        _offset +=2;
        _room_x = readword((uint8_t*)fbuffer[ROOMS], CRM_ROOMS_START+_offset);
        _offset +=2;

        // Read the tiles data
        found = false;	// easier this way, as tile_x/y won't need adjusting
        for (_tile_y=0; (_tile_y<_room_y)&&(!found); _tile_y++)
        {
            for(_tile_x=0; (_tile_x<_room_x)&&(!found); _tile_x++)
            {
                tile_data = readword((uint8_t*)fbuffer[ROOMS], CRM_ROOMS_START+_offset);
                if ((tile_data & 0xF) == exit_index)
                {
                    found = true;
                    // open exit
                    exit_flags = tile_data & 0xFF;
                    toggle_open_flag(exit_flags);
                    writebyte(fbuffer[ROOMS], CRM_ROOMS_START+_offset+1, exit_flags);
                    break;
                }
                _offset +=2;		// Read next tile
            }
            if (found)
                break;
        }
    }
}


// Helper function for check_footprint() below:
// populates relevant properties for one of the 4 quadrant's tile
static __inline void get_tile_props(int16_t _tile_x, int16_t _tile_y, int index_nr)
{
    uint8_t register u;
    uint32_t tile;

    // Set the left mask offset index, converted to a long offset
    // Be mindful that the _tile_x/y used here are not the global variables
    // as me might be doing a lookup right/down
    tile = readtile(_tile_x, _tile_y) + (in_tunnel?TUNNEL_TILE_ADDON:0);
//	printb("got tile %04X\n", tile<<7);
    // Dunno why they reset the tile index for tunnels in the original game

    // Get the exit mask, if we stand on an exit
    // If we are not on an exit tile we'll use the empty mask from TILE_MASKS
    // NB: This is why we add the ####_MASKS_STARTs here, as we might mix EXIT and TILE
    exit_offset[index_nr] = MASK_EMPTY;

    for (u=0; u<NB_EXITS; u++)
    {	// Check if the tile is a regular exit
        if (readword((uint8_t*)fbuffer[LOADER], EXIT_TILES_LIST + 2*u) == tile)
        {
            exit_offset[index_nr] = EXIT_MASKS_START + readword((uint8_t*)fbuffer[LOADER], EXIT_MASKS_OFFSETS+2*u);
            exit_dx[index_nr/2] = index_nr%2;
            break;
        }
    }

    // Check for tunnel exits, and set the appropriate tool
    // NB: we need to do that even when not specifically checking for tunnel exits
    //     to make sure tunnel I/O tiles are walkable (MASK_FULL) as their default
    //     mask is not
    tunexit_tool[index_nr] = ITEM_NONE;
    for (u=0; u<NB_TUNNEL_EXITS; u++)
    {	// Check if the tile is a tunnel exit
        if (readword((uint8_t*)fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST + 2*u) == tile)
        {
            tunexit_tool[index_nr] = readbyte((uint8_t*)fbuffer[LOADER], TUNNEL_EXIT_TOOLS_LIST + 2*u + 1);
//			printb("tunnexit_tool[%d] set\n", index_nr);
            break;
        }
    }

    if (u<IN_TUNNEL_EXITS_START)	// means we had a match (above ground) above
    // a tunnel exit is always walkable (even open), as per the original game
    // NB: This is not necessary for inside tunnel exits, where the default mask works fine
        mask_offset[index_nr] = MASK_FULL;
    else
        // Regular
        mask_offset[index_nr] = TILE_MASKS_START + readlong((uint8_t*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));
}


// Checks if the prisoner can go to (px,p2y) and initiates door/tunnel I/O
// Returns non zero if allowed (-1 if not an exit, or the exit number), 0 if not allowed
// Be mindful that the dx, d2y used here are not the same as the global values from main!
int16_t check_footprint(int16_t dx, int16_t d2y)
{
    uint32_t tile_mask, exit_mask;
    uint32_t ani_offset;
    uint32_t footprint;
    uint16_t mask_y;
    uint8_t i, u, sid;
    int16_t px, p2y;
    uint8_t exit_flags;
    uint8_t exit_nr;
    char* debug_message = "WHY DO I HAVE TO INITIALIZE THIS CRAP?";

    // Initialize a few values
    if (in_tunnel)
        footprint = TUNNEL_FOOTPRINT;
    else
        footprint = SPRITE_FOOTPRINT;
    offset = 0;
    set_room_xy(current_room_index);

    // Compute the tile on which we try to stand
    px = prisoner_x + dx - (in_tunnel?16:0);
    p2y = prisoner_2y + 2*d2y - 1;
    tile_y = p2y / 32;
    tile_x = px / 32;
    // check if we are trying to overflow our room left or up
    if ((px<0) || (p2y<0))
        return 0;

    //
    // Now, to check the footprint, we need to set 4 quadrants
    // in case our rectangular footprint spans more than a single tile
    //
    for (i=0; i<2; i++)
    {	// y and y+1
        // Populate the various props arrays
        get_tile_props(tile_x, tile_y, 2*i);

        // Set the upper right mask offset
        if ((px&0x1F) < 16)
        {	// we're on the left handside of the tile
            mask_offset[2*i+1] = mask_offset[2*i] + 2;	// Just shift 16 bits on the same tile
            exit_offset[2*i+1] = exit_offset[2*i] + 2;
        }
        else
        {	// right handside = > need to lookup the adjacent
            // (tile_x+1, tile_y(+1)) mask
            mask_offset[2*i] += 2;	// first, we need to offset our first quadrant
            exit_offset[2*i] += 2;

            if ((tile_x+1) < room_x)
            // only read adjacent if it exists (i.e. < room_x)
                get_tile_props(tile_x+1, tile_y, 2*i+1);
            else
            {
                exit_offset[2*i+1] = MASK_EMPTY;
                mask_offset[2*i+1] = MASK_EMPTY;
            }
        }
        tile_y++;	// process lower tiles
    }

    if ((dx == 0) && (d2y == 0))
    // no motion means we were only checking for tunnel exits
    // => no need to push it further
        return 0;

    // OK, now we have our 4 mask offsets
    mask_y = (p2y & 0x1E)<<1;	// one mask line is 4 bytes (and p2y is already 2*py)

    mask_offset[0] += mask_y;	// start at the right line
    mask_offset[1] += mask_y;

    exit_offset[0] += mask_y;	// start at the right line
    exit_offset[1] += mask_y;

    footprint >>= (px & 0x0F);	// rotate our footprint according to our x pos
///	printb("%s %s [%d]\n", to_binary(footprint), to_binary(footprint), (px&0x1f));


    // Not tunnel I/O => we check collisions and room IO for multiple py's
    for (i=0; i<FOOTPRINT_HEIGHT; i++)
    {
        tile_mask = to_long(readword((uint8_t*)fbuffer[LOADER], mask_offset[0]),
            readword((uint8_t*)fbuffer[LOADER], mask_offset[1]));

        exit_mask = to_long(readword((uint8_t*)fbuffer[LOADER], exit_offset[0]),
            readword((uint8_t*)fbuffer[LOADER], exit_offset[1]));

///	printb("%s ",to_binary(exit_mask));
///	printb("%s\n", to_binary(tile_mask));

        // see low_level.h for the collisions macros
        if inverted_collision(footprint,tile_mask)
        {
            // we have an exit perhaps
            if (collision(footprint,exit_mask))
            {
                // We need to spare the exit offset value
                exit_flags_offset = get_exit_offset(tile_x+exit_dx[0],tile_y-2);
                exit_flags = readbyte(fbuffer[ROOMS], exit_flags_offset);

                if (opt_onscreen_debug)
                {	// Override the message
                    exit_nr = (uint8_t) readexit(tile_x+exit_dx[0],tile_y-2) & 0x1F;
                    sprintf(debug_message, "EXIT #%d (GRADE %d)", exit_nr & (is_inside?0x0F:0xFF), ((exit_flags & 0x60) >> 5) - 1);
                    set_status_message(debug_message, 3, 3000); //NO_MESSAGE_TIMEOUT);
                }

                // Is the exit open?
                if ((!(exit_flags & 0x10)) && (exit_flags & 0x60))
                {	// exit is closed
                    if (is_fire_pressed && can_consume_key)
                    {
                        if ((opt_keymaster) ||
                            // do we have the right key selected
                            (selected_prop[current_nation] == ((exit_flags & 0x60) >> 5)))
                        {
                            // Play the door SFX
                            play_sfx(SFX_DOOR);
                            // enqueue the door opening animation
                            exit_nr = (uint8_t) readexit(tile_x+exit_dx[0],tile_y-2) & 0x1F;
                            // The trick is we use currently_animated[] to store our door sids
                            // even if not yet animated, so that we can quickly access the
                            // right animation data, rather than exhaustively compare tiles
                            sid = currently_animated[exit_nr];
                            ani_offset = 0;
                            switch(sid)
                            {	// let's optimize this a bit
                            case 0x76:	// door_left
                                ani_offset += 0x02;		// +2 because of door close ani
                            case 0x78:	// door right
                                ani_offset += 0x02;
                            case 0x71:	// horizontal door
                                ani_offset += DOOR_HORI_OPEN_ANI;
                                currently_animated[exit_nr] = nb_animations;
                                animations[nb_animations].index = ani_offset;
                                animations[nb_animations].framecount = 0;
                                animations[nb_animations].end_of_ani_function = &toggle_exit;
                                animations[nb_animations].end_of_ani_parameter = exit_nr;
                                can_consume_key = false;	// Don't consume any more keys till door opened
                                safe_nb_animations_increment();
                                break;
                            default:	// not an exit we should animate
                                // just enqueue the toggle exit event
                                enqueue_event(&toggle_exit, exit_nr, 3*ANIMATION_INTERVAL);
                                can_consume_key = false;	// Don't consume any more keys till door opened
                                break;
                            }
                            consume_prop();
                        }
                        // For now, the exit is still closed, so we return failure to progress further
                        return 0;
                    }
                    else
                    {
                        // Display the key grade message
                        set_status_message(fbuffer[LOADER] + readlong(fbuffer[LOADER], EXIT_MESSAGE_BASE +
                            ((exit_flags & 0x60) >> 3)), 2, NO_MESSAGE_TIMEOUT);
                        // Return failure if we can't exit
                        return 0;
                    }
                }

                // +1 as exits start at 0
                return(readexit(tile_x+exit_dx[0],tile_y-2)+1);
            }
            return 0;
        }
        mask_y+=4;

        // Do we need to change tile in y?
        for (u=0;u<2;u++)
        {
            if (mask_y == 0x40)
            {	// went over the tile boundary
                // => replace upper mask offsets with lower
                mask_offset[u] = mask_offset[u+2];
                exit_offset[u] = exit_offset[u+2];
                // We need an array for dx as we may have 2 exits on opposite quadrants (room 118 for instance)
                exit_dx[0] = exit_dx[1];
            }
            else
            {	// just scroll one mask line down
                mask_offset[u] +=4;
                exit_offset[u] +=4;
            }
        }
    }
    return -1;
}


// This function MUST immediately follow a call to check_footprint(0,0)
// Checks if the prisoner can do tunnel I/O
// Returns >0 if went through a tunnel, <0 if opened a tunnel or 0 if no tunnel action occured
int16_t check_tunnel_io()
{
    uint8_t u;
    int16_t px, p2y;
    uint8_t exit_flags;
    uint8_t exit_nr;

    // Compute the tile on which we try to stand
    // tile_x and tile_y from previous check_footprint() call are still relevant
    px = prisoner_x - (in_tunnel?16:0);
    p2y = prisoner_2y - 1;

    // Check if we are standing on a tunnel exit and set the global variables accordingly
    // If a tunnel exit tool is set, we have a winner

    for (u=0; u<3; u++)
    {
        if (tunexit_tool[u] != ITEM_NONE)
        {
            // The line below defines the boundaries we'll use for tunnel exits
            // It may not look like it, but I'd say we do a much better job than
            // the original game, as exiting tunnels was a complete pain there

            if ( ( (!in_tunnel) && (((u == 0) && (px%32 <24 )) || ((u == 1) && (px%32 >=24))) ) ||
                   ( ( in_tunnel) && ((u == 0) || (u == 2)) ) )
            {
                // We need to spare the exit offset value
                // NB1: tile_y was incremented twice in the previous check_footprint() call
                // NB2: exit_flags_offset is a global that will be used in toggle_exit()
                exit_flags_offset = get_exit_offset(tile_x+(u%2),tile_y-2+(u/2));
                exit_flags = readbyte(fbuffer[is_inside?ROOMS:TUNNEL_IO], exit_flags_offset);
                // as for regular exits, because we use tunexit_nr as a boolean, we start at +1
                exit_nr = readexit(tile_x+(u%2),tile_y-2+(u/2)) + 1;
                if (exit_nr == 1)
                {	// tunexit_tool[] and tile(u%2,u/2) should always match...
                    perr("check_tunnel_io: Exit tile mismatch.\n");
                    continue;
                }

                if (!(exit_flags & 0x10))
                {	// Exit is closed => check for the right prop
                    if ( (opt_keymaster) || (selected_prop[current_nation] == tunexit_tool[u]) )
                    {	// Toggle the exit open and consume the relevant item
                        // Play the relevant SFX if needed
                        if (selected_prop[current_nation] == ITEM_SAW)
                            play_sfx(SFX_SAW);
                        else if (!in_tunnel)
                            play_sfx(SFX_WTF);

                        consume_prop();		// doesn't consume if opt_keymaster
                        show_prop_count();
                        // We offset exits by 0x100 for toggle_exit to know it's a tunnel
                        toggle_exit(exit_nr-1 + 0x100);

                        if (in_tunnel)
                            // If we're in a tunnel and used the shovel, we exit directly
                            return exit_nr;
                        else
                            // return -1 to flag the fact that we used a prop (but don't switch room just yet)
                            return -1;
                    }
                }
                else if ( (opt_keymaster) || (in_tunnel) ||
                          (selected_prop[current_nation] == ITEM_CANDLE) )
                {	// Exit is open and we're all set to get through it
                    if (!in_tunnel)
                    {
                        // Only consume the candle (because of the if !opt_keymaster in fn)
                        consume_prop();
                        show_prop_count();
                    }
                    return exit_nr;
                }
            }
            break;
        }
    }
    return 0;
}


// Simplified (faster) check_footprint for guards => only checks wall collisions
bool check_guard_footprint(uint8_t g, int16_t dx, int16_t d2y)
{
    uint32_t tile, tile_mask;
    uint32_t footprint;
    uint16_t mask_y;
    uint8_t i,u;
    int16_t gx, g2y;


    footprint = SPRITE_FOOTPRINT;
    offset = 0;

    // Won't work unless we're in the active room
    if (guard(g).room != current_room_index)
        return true;

    set_room_xy(guard(g).room);

    // Compute the tile on which we try to stand
    gx = guard(g).px + dx - 16;
    g2y = guard(g).p2y + 2*d2y - 5;
    tile_y = g2y / 32;
    tile_x = gx / 32;

    // y and y+1
    for (i=0; i<2; i++)
    {
        // Set the left mask offset (tile_x, tile_y(+1)) index, converted to a long offset
        tile = readtile(tile_x, tile_y);
        mask_offset[2*i] = TILE_MASKS_START + readlong((uint8_t*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));

        // Set the upper right mask offset
        if ((gx&0x1F) < 16)
        // we're on the left handside of the tile
            mask_offset[2*i+1] = mask_offset[2*i] + 2;	// Just shift 16 bits on the same tile
        else
        {	// right handside = > need to lookup the adjacent
            // (tile_x+1, tile_y(+1)) mask
            mask_offset[2*i] += 2;	// first, we need to offset our first quadrant

            if ((tile_x+1) < room_x)
            {	// only read adjacent if it exists (i.e. < room_x)
                tile = readtile(tile_x+1, tile_y);
                mask_offset[2*i+1] = TILE_MASKS_START + readlong((uint8_t*)fbuffer[LOADER], TILE_MASKS_OFFSETS+(tile<<2));
            }
            else
                mask_offset[2*i+1] = MASK_EMPTY;
        }
        tile_y++;	// process lower tiles
    }

    // OK, now we have our 4 mask offsets
    mask_y = (g2y & 0x1E)<<1;	// one mask line is 4 bytes (and g2y is already 2*gy)

    mask_offset[0] += mask_y;	// start at the right line
    mask_offset[1] += mask_y;

    footprint >>= (gx & 0x0F);	// rotate our footprint according to our x pos

    // Not tunnel I/O => we check collisions and regular exits for multiple py's
    for (i=0; i<FOOTPRINT_HEIGHT; i++)
    {
        tile_mask = to_long(readword((uint8_t*)fbuffer[LOADER], mask_offset[0]),
            readword((uint8_t*)fbuffer[LOADER], mask_offset[1]));

        // see low_level.h for the collisions macros
        if inverted_collision(footprint,tile_mask)
            return false;

        mask_y+=4;
        // Do we need to change tile in y?
        for (u=0;u<2;u++)
        {
            if (mask_y == 0x40)
                // went over the tile boundary
                // => replace upper mask offsets with lower
                mask_offset[u] = mask_offset[u+2];
            else
                // just scroll one mask line down
                mask_offset[u] +=4;
        }
    }
    return true;
}


// Called when switching prisoner
void switch_nation(uint8_t new_nation)
{
    // if there was any end of ani function, execute it
    if (prisoner_ani.end_of_ani_function != NULL)
    {	// execute the end of animation function (toggle exit)
        prisoner_ani.end_of_ani_function(prisoner_ani.end_of_ani_parameter);
        prisoner_ani.end_of_ani_function = NULL;
    }
    // Clear flags for old nation
    prisoner_state &= ~(STATE_MOTION|STATE_ANIMATED);
    current_nation = new_nation;
    // Clear the stooge flag for the new nation
    prisoner_state &= ~(STATE_MOTION|STATE_ANIMATED|STATE_STOOGING);
    prisoner_reset_ani = true;
    t_status_message_timeout = 0;
    status_message_priority = 0;
    set_room_props();
}

// Called when changing rooms
void switch_room(int16_t exit_nr, bool tunnel_io)
{
    uint16_t exit_index;	// exit index in destination room
    uint16_t tile_data = 0;
    uint32_t u;
    bool found;
    int16_t pixel_x, pixel_y;
    uint8_t bit_index;

    // Let's get through
    if (is_outside)
    {	// If we're on the compressed map, we need to read 2 words (out of 4)
        // from beginning of the ROOMS_MAP file
        offset = exit_nr << 3;	// skip 8 bytes
        current_room_index = readword((uint8_t*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset) & 0x7FF;
        exit_index = readword((uint8_t*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset+2);
    }
    else
    {	// indoors => read from the ROOMS_EXIT_BASE data
        exit_index = (exit_nr&0xF) - 1;
        offset = current_room_index << 4;
        // Now the real clever trick here is that the exit index of the room you
        // just left and the exit index of the one you go always match.
        // Thus, we know where we should get positioned on entering the room
        current_room_index = readword((uint8_t*)fbuffer[ROOMS], ROOMS_EXITS_BASE + offset
            + 2*exit_index);
    }

    // Since we're changing room, reset all animations...
    init_animations = true;
    // and to be safe, make sure we can use keys
    can_consume_key = true;

    exit_index++;	// zero based to one based
//	printb("          to room[%X] (exit_index = %d)\n", current_room_index, exit_index);

    // OK, we have now officially changed room, but we still need to position our guy
    if (current_room_index & 0x8000)	// MSb from ROOMS_EXIT_BASE data means going out
                                        // anything else is inside
    {	// going outside
        room_x = CMP_MAP_WIDTH;		// keep our readtile macros happy

        // If we're outside, we need to set the removable mask according to our data's MSB
        bit_index = (current_room_index >> 8) & 0x7C;
        rem_bitmask = readlong((uint8_t*)fbuffer[LOADER],  REMOVABLES_MASKS_START + bit_index);

        // Now, use the tile index (LSB) as an offset to our (x,y) pos
        // NB: The ground floor rooms are in [00-F8]
        offset = current_room_index & 0xF8;
        tile_y = readword((uint8_t*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset+4);
        tile_x = readword((uint8_t*)fbuffer[tunnel_io?TUNNEL_IO:ROOMS], offset+6);

        // Now that we're done, switch to our actual outbound marker
        current_room_index = ROOM_OUTSIDE;

        // Finally, we need to adjust our pos, through the rabbit offset table
        // But only if we're not doing tunnel_io
        if (!tunnel_io)
        {
            tile_data = ((readtile(tile_x,tile_y) & 0xFF) << 1) - 2;	// first exit tile is 1, not 0
            offset = readword((uint8_t*)fbuffer[LOADER], CMP_RABBIT_OFFSET + tile_data);
        }
    }
    else
    {	// going inside, or still inside
        // Get the room dimensions
        offset = CRM_ROOMS_START + readlong((uint8_t*)fbuffer[ROOMS], CRM_OFFSETS_START+4*current_room_index);
        room_y = readword((uint8_t*)fbuffer[ROOMS], offset);
        offset +=2;
        room_x = readword((uint8_t*)fbuffer[ROOMS], offset);
        offset +=2;

        // Read the tiles data
        found = false;	// easier this way, as tile_x/y won't need adjusting
        for (tile_y=0; (tile_y<room_y)&&(!found); tile_y++)
        {
            for(tile_x=0; (tile_x<room_x)&&(!found); tile_x++)
            {
                tile_data = readword((uint8_t*)fbuffer[ROOMS], offset);
                if ((tile_data & 0xF) == exit_index)
                {
                    found = true;
                    break;
                }
                offset +=2;		// Read next tile
            }
            if (found)
                break;
        }

        if (!found)
        {	// Better exit than go LHC and create a black hole
            perr("switch_room(): Exit lookup failed\n");
            ERR_EXIT;
        }

        // We have our exit position in tiles. Now, depending
        // on the exit type, we need to add a small position offset
        if (!tunnel_io)
        {	// but only if we're not doing a tunnel io
            tile_data &= 0xFF80;
            offset = 0;		// Should never fail (famous last words). Zero in case it does
            for (u=0; u<NB_CELLS_EXITS; u++)
            {
                if (readword((uint8_t*)fbuffer[LOADER], EXIT_CELLS_LIST + 2*u) == tile_data)
                {
                    offset = readword((uint8_t*)fbuffer[LOADER], HAT_RABBIT_OFFSET + 2*u);
                    break;
                }
            }
        }
    }

    // Read the pixel adjustment
    if (!tunnel_io)
    {
        pixel_x = (int16_t)(readword((uint8_t*)fbuffer[LOADER], HAT_RABBIT_POS_START + offset+2));
        pixel_y = (int16_t)(readword((uint8_t*)fbuffer[LOADER], HAT_RABBIT_POS_START + offset)) + 32;
    }
    else if (prisoner_state&STATE_TUNNELING)
    {	// Entering a tunnel
        pixel_x = 20;
        pixel_y = 0;
    }
    else
    {	// Exiting a tunnel
        pixel_x = 16;
        pixel_y = 0;
    }

    prisoner_x = tile_x*32 + pixel_x;
    prisoner_2y = tile_y*32 + 2*pixel_y - 2;

    // Don't forget to (re)set the room props
    set_room_props();
}


// The next 2 functions are called while displaying the go to / released from
// solitary static screen, so that the prisoner position has been switched
// when we fade in on the game
void go_to_jail(uint32_t p)
{
    int prop;

    guy(p).state &= ~STATE_IN_PURSUIT;
    guy(p).state |= STATE_IN_PRISON;
    p_event[p].unauthorized = false;

    // Make sure the jail doors are closed when we leave the prisoner in!
    writebyte(fbuffer[ROOMS], solitary_cells_door_offset[p][0],
        readbyte(fbuffer[ROOMS], solitary_cells_door_offset[p][0]) & 0xEF);
    writebyte(fbuffer[ROOMS], solitary_cells_door_offset[p][1],
        readbyte(fbuffer[ROOMS], solitary_cells_door_offset[p][1]) & 0xEF);

    // Set our guy in the cell
    guy(p).room = readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p);
    guy(p).p2y = 2*readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p+2)-2;
    guy(p).px = readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p+4)-2;
    if (!opt_keymaster)
    {	// Bye bye props!
        for (prop = 0; prop<NB_PROPS; prop++)
            props[p][prop] = 0;
        // display the empty box
        selected_prop[p] = 0;
        // Strip of guard uniform if any
        guy(p).is_dressed_as_guard = false;
        guy(p).reset_animation = true;
    }

    // Reset all the guards that were in pursuit
    if (opt_enhanced_guards)
        reset_guards_in_pursuit(p);
    else
        reinstantiate_guards_in_pursuit(p);

    // Don't forget to (re)set the room props
    set_room_props();
}

void out_of_jail(uint32_t p)
{
    p_event[p].solitary_release = 0;
    guy(p).state &= ~STATE_IN_PRISON;

    guy(p).px = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*p+2)-16;
    guy(p).p2y = 2*readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*p)-8;
    guy(p).room = readword(fbuffer[LOADER],INITIAL_POSITION_BASE+10*p+4);

    // Don't forget to (re)set the room props
    set_room_props();
}

// This next one is necessary so that we don't send the guard away after a pass request
// until after we faded the game screen
void require_pass(uint32_t p)
{
    int g;

    // Clear the event
    p_event[p].require_pass = false;

    if (props[p][ITEM_PASS] != 0)
    {
        guy(p).state &= ~STATE_IN_PURSUIT;
//		selected_prop[p] = ITEM_PASS;		// Doing this is bothersome if
        props[p][ITEM_PASS]--;				// we have to re-cycle in a hurry
//		show_prop_count();					// => let's comment these lines out

        // Reset all the guards that were in pursuit
        if (opt_enhanced_guards)
        {
            // Guards remember when they've seen a pass
            for (g=0; g<NB_GUARDS; g++)
                if (guard(g).target == p)
                {
                    guard(g).fooled_by[p] = true;
                    // Lower your gun, please
                    guard(g).state &= ~STATE_AIMING;
                    guard(g).reset_animation = true;
                }
            reset_guards_in_pursuit(p);
        }
        else
            reinstantiate_guards_in_pursuit(p);

        p_event[p].pass_grace_period_expires = game_time + PASS_GRACE_PERIOD;
    }
    else
        p_event[p].to_solitary = true;
}

// Have a look at what our prisoner are doing
void check_on_prisoners()
{
    static int nb_escaped = 0;
    int p;
    uint16_t i;
    uint16_t authorized_id;
    uint8_t room_desc_id;
    int game_over_count, game_won_count;

    // Check escape condition (for current prisoner only)
    if ( (!has_escaped) && (current_room_index == ROOM_OUTSIDE) &&
         ( (prisoner_x < ESCAPE_MIN_X) || (prisoner_x > ESCAPE_MAX_X) ||
           (prisoner_2y < (2*ESCAPE_MIN_Y)) || (prisoner_2y > (2*ESCAPE_MAX_Y)) ) )
    {
        if (props[current_nation][ITEM_PAPERS])
        {
            nb_escaped++;
            if (nb_escaped >= NB_NATIONS)
            {
                static_screen(PRISONER_FREE_ALL_TEXT, NULL, 0);
            }
            else
            {
                static_screen(PRISONER_FREE, NULL, 0);
                p_event[current_nation].escaped = true;
            }
        }
        else
        {
            static_screen(REQUIRE_PAPERS, NULL, 0);
            p_event[current_nation].to_solitary = true;
            return;
        }
    }

    game_over_count = 0;
    game_won_count  = 0;
    // Game over condition
    for(p=0; p<NB_NATIONS; p++)
    {
        if ( (p_event[p].killed) || (guy(p).state & STATE_IN_PRISON) )
            game_over_count++;
        if (p_event[p].escaped)
            game_won_count++;
    }
    if (game_over_count == NB_NATIONS)
    {
        game_state = GAME_STATE_GAME_OVER;
        static_screen(GAME_OVER_TEXT, NULL, 0);
        return;
    }
    if (game_won_count == NB_NATIONS)
    {
        game_state = GAME_STATE_GAME_WON;
        static_screen(PRISONER_FREE_ALL_TEXT, NULL, 0);
        return;
    }

    // Other conditions
    for(p=0; p<NB_NATIONS; p++)
    {
        // Decrease fatigue if sleeping
        if ((guy(p).state & STATE_SLEEPING) && (p_event[p].fatigue >= 2))
            p_event[p].fatigue -= 2;
        // At this stage, we've processed user input & guards actions so we
        // reset the thrown_stone flag if set
        p_event[p].thrown_stone = false;
        if ((p_event[p].escaped) || (p_event[p].killed))
            continue;
        else if (p_event[p].to_solitary)
        {	// Sent to jail
            static_screen(TO_SOLITARY, go_to_jail, p);
            p_event[p].to_solitary = false;
            // Start solitary countdown
            p_event[p].solitary_release = game_time + SOLITARY_DURATION;
        }
        else if (p_event[p].require_pass)
        {	// Pass request
            static_screen(REQUIRE_PASS, require_pass, p);
        }
        else if (p_event[p].display_shot)
        {	// End of the shot animation
            if (opt_enhanced_guards)
            {
                // Keep the guard onscreen in aiming position for a little while longer
                static_screen(PRISONER_SHOT, NULL, 0);
                enqueue_event(reset_guards_in_pursuit, p, 5000);
            }
            else
                static_screen(PRISONER_SHOT, reinstantiate_guards_in_pursuit, p);
            p_event[p].display_shot = false;
            p_event[p].killed = true;
        }
        else if (p_event[p].solitary_release != 0)
        {	// Guy's in solitary => check on him

            if (game_time >= p_event[p].solitary_release)
            {	// Still in his cell?
                if (guy(p).room == readword(fbuffer[LOADER],SOLITARY_POSITION_BASE+8*p))
                {	// "Freeeeeeeedom!"
                    static_screen(FROM_SOLITARY, out_of_jail, p);
                }
                else
                {	// Jailbreak!
                    // We'll keep showing the guy behind bars until the guards
                    // come to release, in which case pursuit mode is activated
                    // unless the prisoner is dressed as a guard
                    guy(p).state &= ~STATE_IN_PRISON;
                    p_event[p].solitary_release = 0;
                    // In the enhanced version, we'll fool the guards if dressed as one
                    if ((!opt_enhanced_guards) || (!guy(p).is_dressed_as_guard))
                        guy(p).state |= STATE_IN_PURSUIT;
                }
            }
        }
        else
        {	// Check if we are authorised in our current pos
            if (guy(p).room == ROOM_OUTSIDE)
                room_desc_id = COURTYARD_MSG_ID;
            else if (guy(p).room < ROOM_TUNNEL)
                room_desc_id = readbyte(fbuffer[LOADER], ROOM_DESC_BASE	+ guy(p).room);
            else
                room_desc_id = TUNNEL_MSG_ID;

            // Now that we have the room desc ID, we can check if it's in the
            // currently authrorized list
            p_event[p].unauthorized = true;
            for (i=1; i<=readword(fbuffer[LOADER], authorized_ptr)+1; i++)
            {
                authorized_id = readword(fbuffer[LOADER], authorized_ptr+2*i);
                if (authorized_id == 0xFFFF)
                    // prisoner's quarters
                    authorized_id = readbyte(fbuffer[LOADER], AUTHORIZED_NATION_BASE+p);

                if (authorized_id == room_desc_id)
                {	// if there's a match, we're allowed here
                    p_event[p].unauthorized = false;
                    // Additional boundary check for courtyard
                    if ( (guy(p).room == ROOM_OUTSIDE) && (
                         (guy(p).px < COURTYARD_MIN_X) || (guy(p).px > COURTYARD_MAX_X) ||
                         (guy(p).p2y < (2*COURTYARD_MIN_Y)) || (guy(p).p2y > (2*COURTYARD_MAX_Y)) ) )
                        p_event[p].unauthorized = true;
                    break;
                }
            }
        }
    }
}

// Looks like the original programmer found that some of the data files had issues,
// but rather than fixing the files, they patched them in the loader... go figure!
void fix_files(bool reload)
{
    uint8_t i;
    uint32_t mask;

    //
    // Original game patch
    //
    //////////////////////////////////////////////////////////////////
    //00001C10                 move.b  #$30,(fixed_crm_vector).l ; '0'
    writebyte(fbuffer[ROOMS],FIXED_CRM_VECTOR,0x30);
    //00001C18                 move.w  #$73,(exits_base).l ; 's' ; fix room #0's exits
    writeword(fbuffer[ROOMS],ROOMS_EXITS_BASE,0x0073);
    //00001C20                 move.w  #1,(exits_base+2).l
    writeword(fbuffer[ROOMS],ROOMS_EXITS_BASE+2,0x0001);
    //00001C28                 move.w  #$114,(r116_exits+$E).l ; fix room #116's last exit (0 -> $114)
    writeword(fbuffer[ROOMS],ROOMS_EXITS_BASE+(0x116<<4)+0xE,0x0114);

    if (reload)
    // On a reload, no need to fix the other files again
        return;

    // DEBUG: I've always wanted to have the stethoscope!
    // (replaces the stone in the courtyard)
//	writeword(fbuffer[OBJECTS],32,0x000E);

    // OK, because we're not exactly following the exact exit detection routines from the original game
    // (we took some shortcuts to make things more optimized) upper stairs landings are a major pain in
    // the ass to handle, so we might as well take this opportunity to do a little patching of our own...
    for (i=9; i<16; i++)
    {	// Offset 0x280 is the intermediate right stairs landing
        mask = readlong((uint8_t*)fbuffer[LOADER], EXIT_MASKS_START + 0x280 + 4*i);
        // eliminating the lower right section of the exit mask seems to do the job
        mask &= 0xFFFF8000;
        writelong((uint8_t*)fbuffer[LOADER], EXIT_MASKS_START + 0x280 + 4*i, mask);
    }

    // The 3rd tunnel removable masks for overlays, on the compressed map, were not designed
    // for widescreen, and as such the tunnel will show open if we don't patch it.
    writebyte(fbuffer[LOADER], OUTSIDE_OVL_BASE+0x38+1, 0x05);

    // In the original, they reset the tile index for tunnels, instead of starting at
    // 0x1E0 (TUNNEL_TILE_ADDON) as they should have done. Because we use the latter
    // more logical approach for efficiency, we'll patch a couple of words
    writeword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START,
        readword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START) + TUNNEL_TILE_ADDON);
    writeword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START+2,
        readword(fbuffer[LOADER], TUNNEL_EXIT_TILES_LIST+2*IN_TUNNEL_EXITS_START+2) + TUNNEL_TILE_ADDON);

    // Don't want to be picky, but when you include an IFF Sample to be played as an SFX, you might want
    // to make sure that either you remove the IFF header, or you start playing the actual BODY. The SFX
    // for the door SFX was screwed up in the original game because of the 0x68 bytes header. We fix it:
    writeword(fbuffer[LOADER], SFX_TABLE_START, readword(fbuffer[LOADER], SFX_TABLE_START) + 0x68);
    writeword(fbuffer[LOADER], SFX_TABLE_START+6, readword(fbuffer[LOADER], SFX_TABLE_START+6) - 0x68);

}

// Initalize the SFXs
void set_sfxs()
{
    uint16_t i,j;

    // Initialize SFXs
    for (i=0; i<NB_SFXS; i++)
    {
        sfx[i].address   = SFX_ADDRESS_START + readword(fbuffer[LOADER], SFX_TABLE_START + 8*i);
        sfx[i].volume    = readbyte(fbuffer[LOADER], SFX_TABLE_START + 8*i+3);
        sfx[i].frequency = (uint16_t)(Period2Freq((float)readword(fbuffer[LOADER], SFX_TABLE_START + 8*i+4))/1.0);
        sfx[i].length    = readword(fbuffer[LOADER], SFX_TABLE_START + 8*i+6);
#if defined(WIN32)
        // Why, of course Microsoft had to use UNSIGNED for 8 bit WAV data but SIGNED for 16!
        // (Whereas Commodore et al. did the LOGICAL thing of using signed ALWAYS)
        // We need to sign convert our 8 bit mono samples on Windows
        for (j=0; j<sfx[i].length; j++)
            writebyte(fbuffer[LOADER], sfx[i].address+j,
                (uint8_t) (readbyte(fbuffer[LOADER], sfx[i].address+j) + 0x80));
#elif defined(PSP)
        // On the PSP on the other hand, we must upconvert our 8 bit mono samples @ whatever frequency
        // to 16bit/44.1 kHz. The psp_upsample() routine from soundplayer is there for that.
        psp_upsample(&(sfx[i].upconverted_address), &(sfx[i].upconverted_length),
            (char*)(fbuffer[LOADER] + sfx[i].address), sfx[i].length, sfx[i].frequency);
#else
#error No SFX playout for this platform
#endif
    }

    // The footstep's SFX volume is a bit too high for my taste
    sfx[SFX_FOOTSTEPS].volume /= 3;

#if defined(PSP)
    // Let's upconvert us some chicken
    for (j=0; j<sizeof(cluck_sfx); j++)
        cluck_sfx[j] += 0x80;
    psp_upsample(&upcluck, &upcluck_len, (char*)cluck_sfx, sizeof(cluck_sfx), 8000);
    // "'coz this is thriller!..."
    for (j=0; j<sizeof(thriller_sfx); j++)
        thriller_sfx[j] += 0x80;
    psp_upsample(&upthrill, &upthrill_len, (char*)thriller_sfx, sizeof(thriller_sfx), 8000);
#endif
}

// Play one of the 5 game SFXs
void play_sfx(int sfx_id)
{
    if (opt_thrillerdance)
        return;
#if defined(WIN32)
    play_sample(-1, sfx[sfx_id].volume, fbuffer[LOADER] + sfx[sfx_id].address,
        sfx[sfx_id].length, sfx[sfx_id].frequency, 8, false);
#elif defined(PSP)
    play_sample(-1, sfx[sfx_id].volume, sfx[sfx_id].upconverted_address,
        sfx[sfx_id].upconverted_length, PLAYBACK_FREQ, 16, false);
#else
#error No SFX playout for this platform
#endif
}

// Play one of the non game SFX
void play_cluck()
{
    if (opt_thrillerdance)
        return;
    msleep(120);
#if defined(WIN32)
    play_sample(-1, 60, cluck_sfx, sizeof(cluck_sfx), 8000, 8, false);
#elif defined(PSP)
    play_sample(-1, 60, upcluck, upcluck_len, PLAYBACK_FREQ, 16, false);
#else
#error No SFX playout for this platform
#endif
}

// Play one of the non game SFX
void thriller_toggle()
{
static bool is_playing = false;

    if (is_playing)
        stop_loop();
    else
    {
#if defined(WIN32)
        play_sample(-1, 60, thriller_sfx, sizeof(thriller_sfx), 8000, 8, true);
#elif defined(PSP)
        play_sample(-1, 60, upthrill, upthrill_len, PLAYBACK_FREQ, 16, true);
#else
#error No SFX playout for this platform
#endif
    }
    is_playing = !is_playing;
}
