#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    const int fs = 8000;

    ALSACaptureDevice microphone ("hw:memsmiccard,0", fs,
                                  CHANNELS, FRAMES_PER_PERIOD);

    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", fs,
                                CHANNELS, FRAMES_PER_PERIOD);

    std::vector<int16_t> boostbuffer;
    boostbuffer.resize (microphone.get_frames_per_period() * microphone.get_channels());

    microphone.registerCallback ([&] (const std::vector<int16_t> &period) {
        int i = 0;
        for(const int16_t& v:period ) {
            boostbuffer[i] = v * 10;
            i++;
        }
        long int r = speaker.onPeriod(boostbuffer);
        if (speaker.isUnderrunErrorCode(r)) {
            fprintf(stderr,"Playback underrun.\n");
        }
    });

    bool r = speaker.open ();
    if (!r) {
        fprintf(stderr,"Speaker init failed. Bailing out.\n");
        return -1;
    }
    printf("Speaker: buffer size = %ld, period size = %ld\n",speaker.get_buffer_size(),speaker.get_frames_per_period());
    r = microphone.open ();
    if (!r) {
        fprintf(stderr,"Mic init failed. Bailing out.\n");
        return -1;
    }

    printf("Latency in us reported by ALSA: %ld\n",microphone.get_period_time());

    printf("Up and running. Press any key to stop.\n");
    // do nothing
    getchar ();

    printf("Shutting down...\n");
    microphone.close ();
    speaker.close ();

    return 0;
}
