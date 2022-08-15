#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include <unistd.h>
#include "bw_bridge.h"

int main(int argc, char *argv[])
{
	struct bridge br;

	int opt_i = 0;
	int c;

	char filename[256];
	int numFrames;
	Image img;

	static struct option long_opts[]= //parse arguments to read file name with -f
	{
		{ "filename", required_argument, 0, 'f' },
		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, "f:",
			       long_opts, &opt_i)) != -1)
	{
		switch(c)
		{
		case 'f':
			strcpy(filename,optarg);
			break;
		}
	}

	if (IsFileExtension(filename, ".png")) { // see if we are loading an image or an animation
		img = LoadImage(filename);
		numFrames = 1;
	}
	else if (IsFileExtension(filename, ".gif")) {
		img = LoadImageAnim(filename, &numFrames);
	}
	else {
		printf("ERROR: File type not supported");
		return 1;
	}

	if (bridge_init(&br, BW_BRIDGE_MEM_ADR, BW_BRIDGE_MEM_SIZE) < 0) { //after loading in file, initialize the GPMC interface
		printf("ERROR: GPMC Bridge Init failed");
		return 2;
	}

	int currentFrame = 0;
	int numPixels = 4096;

	printf("Number of Frames: %d\n", numFrames);

	uint16_t matrixData[8192];
	do {
		
		for (int i = 0; i < numPixels; i++)
		{
			(matrixData)[i*2] = (((uint8_t *)img.data)[i*4+1+currentFrame*numPixels*4] >> 2) << 8 | (((uint8_t *)img.data)[i*4+currentFrame*numPixels*4] >> 2);
			(matrixData)[i*2+1] = (((uint8_t *)img.data)[i*4+2+currentFrame*numPixels*4] >> 2);
		}
		currentFrame++;
		if (currentFrame >= numFrames) currentFrame = 0;

		printf("currentFrame %d\n", currentFrame);

		set_fpga_mem(&br, 0x4000, matrixData, 8192);
		
		usleep(16000);
	} while (numFrames > 1);

	bridge_close(&br);
	UnloadImage(img);         // Unload CPU (RAM) image data (pixels)

	return 0;
}