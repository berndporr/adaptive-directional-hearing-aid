#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>


#define SAMPLING_RATE 44100
#define FRAMES_PER_PERIOD 32
#define MICROPHONE_CHANNELS 2
#define SPEAKER_CHANNELS 1
#define NTAPS 200
#define FIR_LEARNING_RATE 0.0000001
#define LEARNING_RATE 100
#define GAIN 30
#define NEURAL_NETWORK_LAYERS 4
#endif
