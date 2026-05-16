#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    const int fs = 8000;

    ALSACaptureDevice microphone ("hw:memsmiccard,0", fs,
                                  MICROPHONE_CHANNELS, FRAMES_PER_PERIOD);

    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", fs,
                                SPEAKER_CHANNELS, FRAMES_PER_PERIOD);

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

    speaker.open ();
    microphone.open ();

    printf("Latency in ms requested: %d\n",1000*FRAMES_PER_PERIOD/fs);
    printf("Latency in ms reported by ALSA: %d\n",microphone.get_period_time()/1000);

    printf("Up and running. Press any key to stop.\n");
    // do nothing
    getchar ();

    printf("Shutting down...\n");
    microphone.close ();
    speaker.close ();

    return 0;
}
