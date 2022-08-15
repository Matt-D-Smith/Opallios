#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <raylib.h>
#include <rlgl.h>
#include <string.h>
#include <unistd.h>
#include "bw_bridge.h"

#define DEFAULT_GRAPHIC_DEVICE_DRM "/dev/dri/card0"

int main(int argc, char *argv[])
{
	struct bridge br;

	int opt_i = 0;
	int c;

	char filename[256];
	int numFrames;
	Image img;

	const int screenWidth = 64;
    const int screenHeight = 64;

	InitWindow(screenWidth, screenHeight, "opallios");

	static struct option long_opts[]= //parse arguments to read file name with -f
	{
		{ "filename", required_argument, 0, 'f' },
		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, "f:", long_opts, &opt_i)) != -1)
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
	int numPixels = img.width * img.height;

	Texture2D tex = LoadTextureFromImage(img);
	SetTargetFPS(60);               // Set our display to run at 60 frames-per-second

	printf("Number of Frames: %d\n", numFrames);

	uint16_t matrixData[8192];
	unsigned char *frameData = (unsigned char *)RL_CALLOC(numPixels*4, sizeof(unsigned char));
	while (!WindowShouldClose()) {

		// UpdateTexture(tex, ((unsigned char *)img.data) + currentFrame * numPixels * 4);

		BeginDrawing();
		// ClearBackground(BLUE);
		// DrawText("HELLO", 32, 32, 10, GREEN);

		DrawTexture(tex, 0, 0, WHITE);

		EndDrawing();

			frameData = rlReadScreenPixels(screenWidth, screenHeight);

			for (int i = 0; i < numPixels; i++)
			{
				(matrixData)[i*2] = (((uint8_t *)frameData)[i*4+1+numPixels*4] >> 2) << 8 | (((uint8_t *)frameData)[i*4+numPixels*4] >> 2);
				(matrixData)[i*2+1] = (((uint8_t *)frameData)[i*4+2+numPixels*4] >> 2);
			}

			set_fpga_mem(&br, 0x4000, matrixData, 8192);

		

		printf("Current Frame: %d\n", currentFrame);
		// printf("Data dump:\n");
		// for (int j=0; j < 8192; j++) printf("%hi\n", (short int)matrixData[j]);

		// EndDrawing();

		currentFrame++;
		if (currentFrame >= numFrames) currentFrame = 0;
		// usleep(16000);
		if (numFrames == 1) break;
	};

	CloseWindow();
	bridge_close(&br);
	UnloadImage(img);         // Unload CPU (RAM) image data (pixels)

	return 0;
}