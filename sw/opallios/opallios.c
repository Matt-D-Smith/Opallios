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

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 64
static uint8_t fire[SCREEN_WIDTH * SCREEN_HEIGHT];


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

	// Case 1 shapes
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

	// Case 2 fire effect data
	Color colors[numPixels];

	for (int i = 0; i < 32; ++i) {
		/* black to mid red, 32 values*/
		// colors[i].r = i << 2; // make the last red section decay linearly
		// colors[i].r = (int)(4/32.0*pow(i,2)); //make the last red section decay exponentially
		colors[i].r = (int)(4/1024.0*pow(i,3)); //make the last red section decay as a 3rd order exponent

		/* mid red to orange, 32 values*/
		// colors[i + 32].r = 128 + (i << 2);

		colors[i + 32].r = 128 + (i << 2);
		colors[i + 32].g = (i << 2);

		/*yellow to orange, 32 values*/
		colors[i + 64].r = 255;
		colors[i + 64].g = 128 + (i << 2);

		/* yellow to white, 162 */
		colors[i + 96].r = 255;
		colors[i + 96].g = 255;
		colors[i + 96].b = i << 2;
		colors[i + 128].r = 255;
		colors[i + 128].g = 255;
		colors[i + 128].b = 64 + (i << 2);
		colors[i + 160].r = 255;
		colors[i + 160].g = 255;
		colors[i + 160].b = 128 + (i << 2);
		colors[i + 192].r = 255;
		colors[i + 192].g = 255;
		colors[i + 192].b = 192 + i;
		colors[i + 224].r = 255;
		colors[i + 224].g = 255;
		colors[i + 224].b = 224 + i;
    } 

	int i,j; 
	uint16_t temp;
  	uint8_t index;
    
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

			case 2: // fire effect
				// credit to https://demo-effects.sourceforge.net/ for this algorithm, I just modified the color palette

				/* draw random bottom line in fire array*/
				j = SCREEN_WIDTH * (SCREEN_HEIGHT- 1);
				for (i = 0; i < SCREEN_WIDTH - 1; i++)
				{
				int random = 1 + (int)(16.0 * (rand()/(RAND_MAX+1.0)));
				if (random > 9) /* the lower the value, the intenser the fire, compensate a lower value with a higher decay value*/
					fire[j + i] = 255; /*maximum heat*/
				else
					fire[j + i] = 0;
				}  
				
				/* move fire upwards, start at bottom*/
				
				for (index = 0; index < 63 ; ++index) {
					for (i = 0; i < SCREEN_WIDTH - 1; ++i) {
						if (i == 0) { /* at the left border*/
							temp = fire[j];
							temp += fire[j + 1];
							temp += fire[j - SCREEN_WIDTH];
							temp /=3;
						}
						else if (i == SCREEN_WIDTH - 1) { /* at the right border*/
							temp = fire[j + i];
							temp += fire[j - SCREEN_WIDTH + i];
							temp += fire[j + i - 1];
							temp /= 3;
						}
						else {
							temp = fire[j + i];
							temp += fire[j + i + 1];
							temp += fire[j + i - 1];
							temp += fire[j - SCREEN_WIDTH + i];
							temp >>= 2;
						}
						if (temp > 1) {
							temp -= 1; /* decay */
							if (temp%10 == 0) temp -= 1; // scale it down slightly so it doesn't hit the top edge
						}
						else temp = 0;

						fire[j - SCREEN_WIDTH + i] = temp;
					}
					j -= SCREEN_WIDTH;
				}

				for (int i = 0; i < numPixels; i++) {
					(matrixData)[i*2] = (colors[fire[i]].g) << 8 | (colors[fire[i]].r);
					(matrixData)[i*2+1] = (colors[fire[i]].b);
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