#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>

//constants for the fir filter
#define SAMPLING_RATE 8000
#define FIR_NTAPS 50
#define FIR_LEARNING_RATE 10.0f
#define FIR_OUTPUT_GAIN 50

//general constants
#define FRAMES_PER_PERIOD 64
#define CHANNELS 2

//
#define SPEED_OF_SOUND 343
#define AVERAGE_DISTANCE_FROM_EAR_TO_EAR_CM 18
//multiplier to increase the delay of the signal so that it is more
//than the minimum
#define DELAY_LINE_MULTIPLIER 2

#endif
