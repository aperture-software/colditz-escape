#ifndef _COLDITZ_LOW_LEVEL_H
#define _COLDITZ_LOW_LEVEL 1

#ifdef	__cplusplus
extern "C" {
#endif

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

#ifdef	__cplusplus
}
#endif

#endif /* _COLDITZ_LOW_LEVEL_H */
