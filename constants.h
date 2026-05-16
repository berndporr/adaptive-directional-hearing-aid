#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>

//constants for the fir filter
#define SAMPLING_RATE 8000
#define FIR_NTAPS 200
#define FIR_LEARNING_RATE 0.000001
#define FIR_OUTPUT_GAIN 30

//general constants
#define FRAMES_PER_PERIOD 32
#define MICROPHONE_CHANNELS 2
#define SPEAKER_CHANNELS 1

//
#define SPEED_OF_SOUND 343
#define AVERAGE_DISTANCE_FROM_EAR_TO_EAR_CM 18
//multiplier to increase the delay of the signal so that it is more
//than the minimum
#define DELAY_LINE_MULTIPLIER 2

#endif
