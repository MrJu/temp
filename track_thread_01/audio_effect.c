#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

int local_gain = 0;
float gain_filter = 1.0f;

//this mehtod allows media player to normalize loudness for pcm buffer 
//gain as x dB , if u want to +6dB , set the gain as 6.0f
//only use for 16bit pcm buffer
int bufferFilter(char *buf , int length , int sample_rate , float gain){

    if (local_gain != gain)
    {
        
        local_gain = gain;
        
        if (local_gain > 0) {
            gain_filter = (float)pow(10, gain / 20);
        }else{
            gain_filter = (float)pow(10, -gain / 20);
        }
    }

    short *ps;
    int i = 0;
    ps = (short *)buf;
    for (i = 0; i < length; i = i + 2)
    {
        if (gain > 0)
        {
            *ps = *ps * local_gain;
        }
        else if (gain < 0)
        {
            *ps = *ps / local_gain;
        }

        if (unlikely(*ps > 32767))
            *ps = 32767;
        else if (unlikely(*ps < -32768))
            *ps = -32768;

        ps++;
    }
    return length;
}

//may be something more later :)