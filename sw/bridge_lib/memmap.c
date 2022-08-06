#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include "bw_bridge.h"
#define word_access

void print_usage()
{
	printf("USAGE:\tmemmap -a ADDR [-w VAL]\n");
	printf("\t-a, --address   ADDR\t16 bit word address (hex)\n");
	printf("\t-w, --write     VAL\tValue to write (hex)\n");
	printf("\nEXAMPLE: memmap -a 0 -w DEAD\n");
}

int main(int argc, char *argv[])
{
	struct bridge br;

	int opt_i = 0;
	int c;

	int address = 0;
	int value = 0;
	int is_write = 0;

	static struct option long_opts[]=
	{
		{ "address", required_argument, 0, 'a' },
		{ "write", required_argument, 0, 'w' },
		{ 0, 0, 0, 0 },
	};

	while((c = getopt_long(argc, argv, "a:w:",
			       long_opts, &opt_i)) != -1)
	{
		switch(c)
		{
		case 'a':
			address = strtoul(optarg, NULL, 16);
			address = address*2; 
			break;
		case 'w':
			value = strtoul(optarg, NULL, 16);
			is_write = 1;
			break;
		case '?':
			print_usage();
			return 0;
			break;
		}
	}

	if (bridge_init(&br, BW_BRIDGE_MEM_ADR, BW_BRIDGE_MEM_SIZE) < 0)
		return 1;

	void *ptr = br.virt_addr;

	if (is_write)
	{
		*(uint16_t *)(ptr + address) = value;
	}

	printf("mem[%x] = %x\n",address/2, *(uint16_t *)(ptr + address));

	bridge_close(&br);

	return 0;
}
