#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include "bw_bridge.h"

int main(int argc, char *argv[])
{
	struct bridge br;

	int opt_i = 0;
	int c;

	char filename[256];
	uint16_t matrixData[8192];

	static struct option long_opts[]=
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

	Image img = LoadImage(filename);

	Color *imgColors = LoadImageColors(img);

	for (int i = 0; i < img.width * img.height; i++)
	{
		(matrixData)[i*2] = (imgColors[i].g >> 2) << 8 | (imgColors[i].r >> 2);
		(matrixData)[i*2+1] = (imgColors[i].b >> 2);
	}

	if (bridge_init(&br, BW_BRIDGE_MEM_ADR, BW_BRIDGE_MEM_SIZE) < 0)
		return -1;

	set_fpga_mem(&br, 0x4000, matrixData, 8192);

	bridge_close(&br);
	UnloadImage(img);         // Unload CPU (RAM) image data (pixels)

	return 0;
}