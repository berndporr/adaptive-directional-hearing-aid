#include "ALSADevices.h"
#include "adaptive_filter.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    ALSACaptureDevice microphone;
    ALSAPlaybackDevice speaker;
    AdaptiveFilter adaptive_filter (FIR_NTAPS, SAMPLING_RATE);

    // switch on learning after one second
    int startupCountdown = SAMPLING_RATE / FRAMES_PER_PERIOD;

    microphone.registerCallback ([&] (const std::vector<int16_t> &period) {
        adaptive_filter.processAsync (period);
        if (startupCountdown > 0)
        {
            startupCountdown--;
            if (0 == startupCountdown) {
                adaptive_filter.setLearningrate (FIR_LEARNING_RATE);
                fprintf(stderr,"Enabling learning at mu=%f\n",FIR_LEARNING_RATE);
            }
        }
    });

    adaptive_filter.registerCallback (
        [&] (const std::vector<int16_t> &period) {
            speaker.onPeriod (period);
        });

    adaptive_filter.enableLogging ("/tmp/log.dat");

    speaker.open ("plughw:rockchipes8316,0", SAMPLING_RATE, CHANNELS,
                  FRAMES_PER_PERIOD);
    adaptive_filter.start ();
    microphone.open ("hw:memsmiccard,0", SAMPLING_RATE, CHANNELS,
                     FRAMES_PER_PERIOD);

    printf ("Up and running. Press any key to stop.\n");
    // do nothing
    getchar ();

    printf ("Shutting down...\n");
    microphone.close ();
    adaptive_filter.stop ();
    speaker.close ();

    return 0;
}
