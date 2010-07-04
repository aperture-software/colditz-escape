/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2009 Aperture Software
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
 *  low-level.c: Helper functions for byte/bit/char manipulation & compression
 *  Aligned malloc code from Satya Kiran Popuri:
 *    http://www.cs.uic.edu/~spopuri/amalloc.html
 *  PowerPacker decrunch code from 'amigadepacker' by Heikki Orsila:
 *    http://zakalwe.fi/~shd/foss/amigadepacker/
 *  ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(PSP)
#include <psptypes.h>
#include <psp/psp-printf.h>
#endif
#include "data-types.h"

#include "colditz.h"
#include "low-level.h"


// Some global variables
int  underflow_flag = 0;
u32	compressed_size, checksum;

// For ppdepack
u32 pp_shift_in;
u32 pp_counter = 0;
u8 *pp_source;


// dammit %b should have been made a C standard by now!
// <sigh> converts a 32 bit number to binary string then...
const char *to_binary(u32 x)
{
    static char b[33];
    u8 i;
    u32 m;
    for (i=0,m=0x80000000; m!=0; i++,m>>=1)
        b[i] = (x&m)?'1':'0';
    b[i] = 0;
    return b;
}


// Power-of-two... err... ize!
// We need this to change a dimension to the closest greater power of two
// as pspgl can only deal with power of two dimensionned textures
u16 powerize(u16 n)
{
    u16 retval;
    int i, first_one, last_one;

    retval = n;	// left unchanged if already power of two
                // also works if n == 0
    first_one = -1;
    last_one = -1;

    for (i=0; i<16; i++)
    {
        if (n & 0x0001)
        {
            if (first_one == -1)
                first_one = i;
            last_one = i;
        }
        n >>= 1;
    }
    if (first_one != last_one)
        retval = 1<<(last_one+1);

    return retval;
}



/*
 * Custom SKR_COLD compression functions (Bytekiller 1.3)
 */

// Get one bit and read ahead if needed
u32 getbit(u32 *address, u32 *data)
{
    // Read one bit and rotate data
    u32 bit = (*data) & 1;
    (*data) >>= 1;
    if ((*data) == 0)
    {	// End of current bitstream? => read another longword
        (*data) = readlong(mbuffer, *address);
        checksum ^= (*data);
//        printb("(-%X).l = %08X\n",(uint)(compressed_size-*address+LOADER_DATA_START+8), (uint)*data);
        (*address)-=4;
        // Lose the 1 bit marker on read ahead
        bit = (*data) & 1;
        // Rotate data and introduce a 1 bit marker as MSb
        // This to ensure that zeros in high order bits are processed too
        (*data) = ((*data)>>1) | 0x80000000;
    }
    return bit;
}

// Get sequence of streamsize bits (in reverse order)
u32 getbitstream(u32 *address, u32 *data, u32 streamsize)
{
    u32 bitstream = 0;
    u32 i;
    for (i=0; i<streamsize; i++)
        bitstream = (bitstream<<1) | getbit (address, data);
    return bitstream;
}

// Decrement address by one byte and check for buffer underflow
void decrement(u32 *address)
{
    if (underflow_flag)
        perr("uncompress(): Buffer underflow error.\n");
    if ((*address)!=0)
        (*address)--;
    else
        underflow_flag = 1;
}

// Duplicate nb_bytes from address+offset to address
void duplicate(u32 *address, u32 offset, u32 nb_bytes)
{
    u32 i;
    if (offset == 0)
        perr("uncompress(): WARNING - zero offset value found for duplication\n");
    for (i=0; i<nb_bytes; i++)
    {
        writebyte(fbuffer[LOADER], (*address), readbyte(fbuffer[LOADER],(*address)+offset));
        decrement(address);
    }
}

/*
 *	Colditz loader uncompression. Algorithm is Bytekiller 1.3
 */
int uncompress(u32 expected_size)
{
    u32 source = LOADER_DATA_START;
    u32 uncompressed_size, current;
    u32 dest, offset;
    u32 bit, nb_bits_to_process, nb_bytes_to_process;
    u32 j;
    compressed_size = readlong(mbuffer, source);
    source +=4;
    uncompressed_size = readlong(mbuffer, source);
    source +=4;
    if (uncompressed_size != expected_size)
    {
        perr("uncompress(): uncompressed data size does not match expected size\n");
        return -1;
    }
    checksum = readlong(mbuffer, source);	// There's a compression checksum
    source +=4;	// Keeping this last +/- 4 on source for clarity

    perrv("  Compressed size=%X, uncompressed size=%X\n",
        (uint)compressed_size, (uint)uncompressed_size);

    source += (compressed_size-4);	// We read compressed data (long) starting from the end
    dest = uncompressed_size-1;		// We fill the uncompressed data (byte) from the end too

    current = readlong(mbuffer, source);
    source -= 4;
    // Note that the longword above (last one) will not have the one bit marker
    // => Unlike other longwords, we might read ahead BEFORE all 32 bits
    // have been rotated out of the last longword of compressed data
    // (i.e., as soon as rotated long is zero)

    checksum ^= current;
//    printb("(-%X).l = %08X\n", (uint)(compressed_size-source+LOADER_DATA_START+8), (uint)current);

    while (dest != 0)
    {
        // Read bit 0 of multiplier
        bit = getbit (&source, &current);
        if (bit)
        {	// bit0 = 1 => 3 bit multiplier
            // Read bits 1 and 2
            bit = getbitstream(&source, &current, 2);
            // OK, this is no longer a bit, but who cares?
            switch (bit)
            {
            case 2:	// mult: 011 (01 reversed  = 10)
                // Read # of bytes to duplicate (8 bit value)
                nb_bytes_to_process = getbitstream(&source, &current, 8)+1;
                // Read offset (12 bit value)
                offset = getbitstream(&source, &current, 12);
                duplicate(&dest, offset, nb_bytes_to_process);
//                printb("  o mult=011: duplicated %d bytes at (start) offset %X to address %X\n",
//                        (uint)nb_bytes_to_process, (int)offset, (uint)dest+1);
                break;
            case 3:	// mult: 111
                // Read # of bytes to read and copy (8 bit value)
                nb_bytes_to_process = getbitstream(&source, &current, 8) + 9;
                // We add 8 above, because a [1-9] value
                // would be taken care of by a 3 bit bitstream
                for (j=0; j<nb_bytes_to_process; j++)
                {	// Read and copy nb_bytes+1
                    writebyte(fbuffer[LOADER], dest, (u8)getbitstream(&source, &current, 8));
                    decrement(&dest);
                }
//                printb("  o mult=111: copied %d bytes to address %X\n", (int)nb_bytes_to_process, (uint)dest+1);
                break;
            default: // mult: x01
                // Read offset (9 or 10 bit value)
                nb_bits_to_process = bit+9;
                offset = getbitstream(&source, &current, nb_bits_to_process);
                // Duplicate 2 or 3 bytes
                nb_bytes_to_process = bit+3;
                duplicate(&dest, offset, nb_bytes_to_process);
//                printb("  o mult=%d01: duplicated %d bytes at (start) offset %X to address %X\n",
//                        (int)bit&1, (int)nb_bytes_to_process, (uint)offset, (uint)dest+1);
                break;
            }
        }
        else
        {	// bit0=0 => 2 bit multiplier
            bit = getbit (&source, &current);
            if (bit)
            {	// mult: 10
                // Read 8 bit offset
                offset = getbitstream(&source, &current, 8);
                // Duplicate 1 byte
                duplicate(&dest, offset, 2);
//                printb("  o mult=10: duplicated 2 bytes at (start) offset %X to address %X\n",
//                        (uint)offset, (uint)dest+1);
            }
            else
            {	// mult: 00
                // Read # of bytes to read and copy (3 bit value)
                nb_bytes_to_process = getbitstream(&source, &current, 3) + 1;
                for (j=0; j<nb_bytes_to_process; j++)
                {	// Read and copy nb_bytes+1
                    writebyte(fbuffer[LOADER], dest, (u8)getbitstream(&source, &current, 8));
                    decrement(&dest);
                }
//                printb("  o mult=00: copied 2 bytes to address %X\n", (uint)dest+1);

            }
        }
    }

    if (checksum != 0)
    {
        perr("uncompress(): checksum error\n");
        return -1;
    }
    return 0;
}


/* Returns a piece of memory aligned to the given
 * alignment parameter. Alignment must be a power of
 * 2.
 * This function returns memory of length 'bytes' or more
 */
void *aligned_malloc(size_t bytes, size_t alignment)
{
    size_t size;
    size_t delta;
    void *malloc_ptr;
    void *new_ptr;
    void *aligned_ptr;

    /* Check if alignment is a power of 2
     * as promised by the caller.
     */
    if ( alignment & (alignment-1)) /* If not a power of 2 */
        return NULL;

    /* Determine how much more to allocate
     * to make room for the alignment:
     *
     * We need (alignment - 1) extra locations
     * in the worst case - i.e., malloc returns an
     * address off by 1 byte from an aligned
     * address.
     */
    size = bytes + alignment - 1;

    /* Additional storage space for storing a delta. */
    size += sizeof(size_t);

    /* Allocate memory using malloc() */
    malloc_ptr = calloc(size, 1);

    if (NULL == malloc_ptr)
        return NULL;

    /* Move pointer to account for storage of delta */
    new_ptr = (void *) ((char *)malloc_ptr + sizeof(size_t));

    /* Make ptr a multiple of alignment,
     * using the standard trick. This is
     * used everywhere in the Linux kernel
     * for example.
     */
    aligned_ptr = (void *) (((size_t)new_ptr + alignment - 1) & ~(alignment -1));

    delta = (size_t)aligned_ptr - (size_t)malloc_ptr;

    /* write the delta just before the place we return to user */
    *((size_t *)aligned_ptr - 1) = delta;

    return aligned_ptr;
}


/* Frees a chunk of memory returned by aligned_malloc() */
void aligned_free(void *ptr)
{
    size_t delta;
    void *malloc_ptr;

    if (NULL == ptr)
        return;

    /* Retrieve delta */
    delta = *( (size_t *)ptr - 1);

    /* Calculate the original ptr returned by malloc() */
    malloc_ptr = (void *) ( (size_t)ptr - delta);

    free(malloc_ptr);
}

u32 get_bits(u32 n)
{
    u32 result = 0;
    u32 i;

    for (i = 0; i < n; i++)
    {
        if (pp_counter == 0)
        {
            pp_counter = 8;
            pp_shift_in = *--pp_source;
        }
        result = (result<<1) | (pp_shift_in & 1);
        pp_shift_in >>= 1;
        pp_counter--;
    }
    return result;
}

/*
 * PowerPacker decruncher functions, from 'amigadepacker' by Heikki Orsila
 */

#define PP_READ_BITS(nbits, var) do {                          \
  bit_cnt = (nbits);                                           \
  while (bits_left < bit_cnt) {                                \
    if (buf_src < src) return 0; /* out of source bits */      \
    bit_buffer |= (*--buf_src << bits_left);                   \
    bits_left += 8;                                            \
  }                                                            \
  (var) = 0;                                                   \
  bits_left -= bit_cnt;                                        \
  while (bit_cnt--) {                                          \
    (var) = ((var) << 1) | (bit_buffer & 1);                   \
    bit_buffer >>= 1;                                          \
  }                                                            \
} while(0)

#define PP_BYTE_OUT(byte) do {                                 \
  if (out <= dest) return 0; /* output overflow */             \
  *--out = ((u8)byte);                                         \
  written++;                                                   \
} while (0)


int ppDecrunch(u8 *src, u8 *dest, u8 *offset_lens, u32 src_len, u32 dest_len, u8 skip_bits)
{
  u8 *buf_src, *out, *dest_end, bits_left = 0, bit_cnt;
  u32 bit_buffer = 0, x, todo, offbits, offset, written=0;

  if (src == NULL || dest == NULL || offset_lens == NULL) return 0;

  /* set up input and output pointers */
  buf_src = src + src_len;
  out = dest_end = dest + dest_len;

  /* skip the first few bits */
  PP_READ_BITS(skip_bits, x);

  /* while there are input bits left */
  while (written < dest_len) {
    PP_READ_BITS(1, x);
    if (x == 0) {
      /* 1bit==0: literal, then match. 1bit==1: just match */
      todo = 1; do { PP_READ_BITS(2, x); todo += x; } while (x == 3);
      while (todo--) { PP_READ_BITS(8, x); PP_BYTE_OUT(x); }

      /* should we end decoding on a literal, break out of the main loop */
      if (written == dest_len) break;
    }

    /* match: read 2 bits for initial offset bitlength / match length */
    PP_READ_BITS(2, x);
    offbits = offset_lens[x];
    todo = x+2;
    if (x == 3) {
      PP_READ_BITS(1, x);
      if (x==0) offbits = 7;
      PP_READ_BITS((u8)offbits, offset);
      do { PP_READ_BITS(3, x); todo += x; } while (x == 7);
    }
    else {
      PP_READ_BITS((u8)offbits, offset);
    }
    if ((out + offset) >= dest_end) return 0; /* match overflow */
    while (todo--) { x = out[offset]; PP_BYTE_OUT(x); }
  }

  /* all output bytes written without error */
  return 1;
  /* return (src == buf_src) ? 1 : 0; */
}
