#define _XOPEN_SOURCE 500 // needed for nanosleep
#define __USE_POSIX199309 // needed for nanosleep
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "bw_bridge.h"

#define FPS 60
#define FRAMETIME_US ((int)(1.0/FPS * 1e9)) // 10 ms / 100Hz

void *FrameTimerThread(void *vargp);

int main(int argc, char *argv[])
{
	struct bridge br;

	int opt_i = 0;
	int c;

	char filename[256];
	int numFrames;
	Image img;

	// Handle input arguments
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

	//Handle image loading
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

	// Set up a timer to have a controllable frame time
	volatile bool change_frame = 0;
	pthread_t frame_timer_thread_id;
	pthread_create(&frame_timer_thread_id, NULL, FrameTimerThread, (bool*)&change_frame);
    
	// display our frames
	uint16_t matrixData[8192];
	do {
		
		for (int i = 0; i < numPixels; i++)
		{
			(matrixData)[i*2] = (((uint8_t *)img.data)[i*4+1+currentFrame*numPixels*4]) << 8 | (((uint8_t *)img.data)[i*4+currentFrame*numPixels*4]);
			(matrixData)[i*2+1] = (((uint8_t *)img.data)[i*4+2+currentFrame*numPixels*4]);
		}
		currentFrame++;
		if (currentFrame >= numFrames) currentFrame = 0;

		// printf("Frame %d\n", currentFrame);

		if (change_frame) printf("Frame not ready!");
		while (!change_frame){
		};
		
		set_fpga_mem(&br, 0x4000, matrixData, 8192);
		change_frame = 0;

	} while (numFrames > 1);

	bridge_close(&br);
	UnloadImage(img);         // Unload CPU (RAM) image data (pixels)

	return 0;
}

void *FrameTimerThread(void *vargp) {
	const struct timespec frametimer = {0 , FRAMETIME_US};
	while(1) {
		nanosleep(&frametimer, NULL); //frame time
		*(bool*)(vargp) = 1; //new character ready
	}
	return NULL;
}