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
#include <math.h>
#include "bw_bridge.h"

#define pi 3.14159265

// Frame timer
#define FPS 100
#define FRAMETIME_US ((int)(1.0/FPS * 1e9)) // 10 ms / 100Hz

void *FrameTimerThread(void *vargp);

// SW rendering
typedef struct shape2d {
	int numLines;
	Vector2* startPos;
	Vector2* endPos;
} shape2d;

void drawShape2d (Image* Image, shape2d* shape, int xoffset, int yoffset, int rotationAngle, Color color);


int main(int argc, char *argv[])
{
	struct bridge br;

	int opt_i = 0;
	int opt;

	char filename[256];
	int numFrames;
	Image img;

	int mode = 0; // choose what function is being displayed

	// Handle input arguments
	static struct option long_opts[]= //parse arguments to read file name with -f
	{
		{ "filename", required_argument, 0, 'f' }, // Filename
		{ "mode"    , optional_argument, 0, 'm' }, // mode 0 = gif/image, 1 = software rendering
		{ 0, 0, 0, 0 },
	};

	while((opt = getopt_long(argc, argv, "m:f:", long_opts, &opt_i)) != -1)
	{
		switch(opt)
		{
		case 'f':
			strcpy(filename,optarg);
			mode = 0;
			break;
		case 'm':
			mode = atoi(optarg);
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

	// for SW rendering make a frame buffer
	Image fbuf = GenImageColor(64, 64, BLACK); 
	// Other SW rendering objects
	//lets represent a wireframe shape as some vectors
	Vector2 startPts[] = {{-16.0, -8.0}, {16.0,-8.0}, {0.0, 8.0}};
	Vector2 endPts[]   = {{16.0,-8.0},   {0.0, 8.0},  {-16.0, -8.0}};
	shape2d triangle = {
		.numLines = 3,
		.startPos = startPts,
		.endPos   = endPts,
	};
	int angle = 0;
    
	// display our frames
	uint16_t matrixData[8192];
	do {
		switch (mode) {
			case 0: // display image/gif
				for (int i = 0; i < numPixels; i++)
				{
					(matrixData)[i*2] = (((uint8_t *)img.data)[i*4+1+currentFrame*numPixels*4]) << 8 | (((uint8_t *)img.data)[i*4+currentFrame*numPixels*4]);
					(matrixData)[i*2+1] = (((uint8_t *)img.data)[i*4+2+currentFrame*numPixels*4]);
				}
				currentFrame++;
				if (currentFrame >= numFrames) currentFrame = 0;
				break;

			case 1: // render arbitrary shape

				// make a 2d shape and draw it
				ImageClearBackground(&fbuf,BLACK);
				drawShape2d(&fbuf, &triangle, 31, 31, angle, BLUE);
				angle += 2;
				if (angle > 360) angle = 0;

				for (int i = 0; i < numPixels; i++)
				{
					(matrixData)[i*2] = (((uint8_t *)fbuf.data)[i*4+1]) << 8 | (((uint8_t *)fbuf.data)[i*4]);
					(matrixData)[i*2+1] = (((uint8_t *)fbuf.data)[i*4+2]);
				}
				break;
		}

		// printf("Frame %d\n", currentFrame);

		// At this point we should have our data ready in matrixData
		if (change_frame) printf("Frame not ready!\n");
		while (!change_frame){
		};
		
		set_fpga_mem(&br, 0x4000, matrixData, 8192);
		change_frame = 0;

	} while (1);

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

void drawShape2d (Image* Image, shape2d* shape, int xoffset, int yoffset, int rotationAngle, Color color) {
	float costheta = cos(rotationAngle * pi / 180);
	float sintheta = sin(rotationAngle * pi / 180);
	for (int i=0; i < shape->numLines; i++) {
		int xStartRotated = shape->startPos[i].x * costheta - shape->startPos[i].y * sintheta;
		int yStartRotated = shape->startPos[i].y * costheta + shape->startPos[i].x * sintheta;
		int xEndRotated   = shape->endPos[i].x   * costheta - shape->endPos[i].y   * sintheta;
		int yEndRotated   = shape->endPos[i].y   * costheta + shape->endPos[i].x   * sintheta;
		ImageDrawLine(Image, xoffset + xStartRotated, yoffset + yStartRotated, xoffset + xEndRotated, yoffset + yEndRotated, color);
	}
}