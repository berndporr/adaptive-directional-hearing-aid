#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <alsa/asoundlib.h>
#include </usr/include/alsa/pcm.h>

const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_LE;
#define SAMPLING_RATE 44100
#define FRAMES_PER_PERIOD 32
#define MICROPHONE_CHANNELS 2
#define SPEAKER_CHANNELS 1

#endif
