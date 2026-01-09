#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>


#define SAMPLING_RATE 8000
#define FRAMES_PER_PERIOD 32
#define MICROPHONE_CHANNELS 2
#define SPEAKER_CHANNELS 1
#define NTAPS 200
#define LEARNING_RATE 0.00001
#define GAIN 30
#define NEURAL_NETWORK_LAYERS 2
#define DELAY_LINE_LENGTH 100
#endif
