#include "sine.h"

void fill_16_bits_sine_wav(char* buf, int channels, int frames)
{
	int i, j, count;
	short *index = (short *)buf;
	short sine[] = {
		0,      6393,   12539,  18204,
		23170,  27245,  30273,  32137,
		32767,  32137,  30273,  27245,
		23170,  18204,  12539,  6393,
		0,      -6393,  -12539, -18204,
		-23170, -27245, -30273, -32137,
		-32767, -32137, -30273, -27245,
		-23170, -18204, -12539, -6393,
	};

	count = sizeof(sine) / sizeof(sine[0]);

	for (i = 0; i < frames; i++)
		for (j = 0; j < channels; j++)
			*index++ = sine[i % count];
}
