#ifndef _COLDITZ_LOW_LEVEL_H
#define _COLDITZ_LOW_LEVEL 1

#ifdef	__cplusplus
extern "C" {
#endif

// concatenate 2 words into a long 
#define to_long(msw,lsw)			\
	((((u32)(msw))<<16) | ((u16)(lsw)))

// check if a sprite (s) is overflowing on a mask (m)
#define collision(s,m)				\
	( (((m)^s)&(m)) != (m) )

// rerurn true if a bit of a sprite (s) is set over a clear bit of the mask (m)
#define inverted_collision(s,m)		\
	collision(s,~m)

u32 readlong(u8* buffer, u32 addr);
void writelong(u8* buffer, u32 addr, u32 value);
u16 readword(u8* buffer, u32 addr);
void writeword(u8* buffer, u32 addr, u16 value);
u8 readbyte(u8* buffer, u32 addr);
void writebyte(u8* buffer, u32 addr, u8 value);
u16 powerize(u16 n);
int uncompress(u32 expected_size);
void *aligned_malloc(size_t bytes, size_t alignment);
void aligned_free(void *ptr);
const char *to_binary(u32 x);

#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_LOW_LEVEL_H */
