// modplayer.h: headers for psp modplayer code
//
// All public functions for modplayer
//
//////////////////////////////////////////////////////////////////////
#ifndef _MODPLAYER_GENERAL_H
#define _MODPLAYER_GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif

//  Function prototypes for private functions
    bool Mod_Init();
    bool Mod_Play();
    void Mod_Pause();
    bool Mod_Stop();
    void Mod_End();
    bool Mod_Load(char *filename);
    void Mod_Tick();
    void Mod_Close();
    bool Mod_EndOfStream();

    void Mod_GetTimeString(char *);

#ifdef __cplusplus
}
#endif
#endif
