// soundplayer.h: headers for Colditz MOD and SFX audio engine
//
//////////////////////////////////////////////////////////////////////
#ifndef _SOUNDPLAYER_H
#define _SOUNDPLAYER_H

// This #define is used to convert an Amiga period number to a frequency. 
// The frequency returned is the frequency that the sample should be played at.
//  3579545.25f / 428 = 8363.423 Hz for Middle C (PAL)
#define Period2Freq(period) (3579545.25f / (period))

// These next few lines determine how the sound will be mixed. 
// Set PLAYBACK_FREQ to whatever you want. 
#define PLAYBACK_FREQ 44100

// OVERSAMPLE can be commented out to disable that function 
// (takes up less CPU time, but doesnt sound as good). 
#define OVERSAMPLE

#ifdef __cplusplus
extern "C" {
#endif


	bool audio_init();
	bool audio_release();
    bool mod_init(char *filename);
	void mod_release();
	bool mod_play();
    void mod_pause();
    bool mod_stop();
	bool play_sample(int channel, unsigned int volume, void *address, unsigned int length, 
		unsigned int frequency, unsigned int bits_per_sample);
#if defined(PSP)
	void psp_upsample(short **dst_address, unsigned long *dst_length, char *src_sample, 
					  unsigned long src_numsamples, unsigned short src_frequency);
#endif
#ifdef __cplusplus
}
#endif
#endif
