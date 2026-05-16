#include "ALSADevices.h"
#include "adaptive_filter.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    ALSACaptureDevice microphone ("hw:memsmiccard,0", SAMPLING_RATE,
                                  MICROPHONE_CHANNELS, FRAMES_PER_PERIOD);

    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", SAMPLING_RATE,
                                SPEAKER_CHANNELS, FRAMES_PER_PERIOD);

    AdaptiveFilter adaptive_filter (FIR_NTAPS,SAMPLING_RATE);

    microphone.registerCallback ([&] (const std::vector<int16_t> &period) {
        adaptive_filter.processAsync (period);
    });

    adaptive_filter.registerCallback (
        [&] (const std::vector<int16_t> &period) {
            speaker.onPeriod (period);
        });

    speaker.open ();
    adaptive_filter.start ();
    microphone.open ();

    printf("Up and running. Press any key to stop.\n");
    // do nothing
    getchar ();

    printf("Shutting down...\n");
    microphone.close ();
    adaptive_filter.stop ();
    speaker.close ();

    return 0;
}
