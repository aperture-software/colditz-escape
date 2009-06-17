#if defined(WIN32)
// Disable the _CRT_SECURE_DEPRECATE warnings of VC++
#pragma warning(disable:4996)
#endif

#include <stdio.h>
#include <stdlib.h>	
#include "data-types.h"
#include "low-level.h"


/* Here we go! */
int main (int argc, char *argv[])
{

	u32  i,size;
	u32  rgba;
	u16  grab4;
	char dstname[256];
	FILE* fd;


	// Let's clean up our buffers
	fflush(stdin);
	void* buffer    = NULL;

	if (argc != 2)
	{
		printf("Usage: %s <rgba_file.raw>\n", argv[0]);
		exit(1);
	}

	size = strlen(argv[1]);
	strncpy(dstname, argv[1], size-4);
	dstname[size-4] = '4';
	strncpy(dstname+size-3, argv[1]+size-4, 4);
	dstname[size+1] = 0;

	printf("%s\n", dstname);

	if ((fd = fopen (argv[1], "rb")) == NULL)
	{
		printf("Can't find file '%s'\n", argv[1]);
		exit(1);
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	buffer = malloc(size);
	fread(buffer, 1, size, fd);
	fclose(fd);

	for (i=0; i<size; i+=4)
	{
		rgba = readlong((u8*)buffer, i);
		// Red component
		grab4 = (rgba >> 20) & 0x0F00;
		// Green component
		grab4 |= (rgba >> 8) & 0xF000;
		// Blue component
		grab4 |= (rgba>>12) & 0x000F;
		// Alpha component
		grab4 |= rgba & 0x00F0;
		writeword((u8*)buffer, i/2, grab4);
	}

	if ((fd = fopen (dstname, "wb")) == NULL)
	{
		printf("Can't open file '%s'\n", dstname);
		exit(1);
	}
	fwrite(buffer, 1, size/2, fd);
	fclose(fd);

}



