#include "ALSADevices.h"
#include "constants.h"
#include "adaptive_filter.h"
#include <cmath>
#include <cstdio>

int main ()
{
    ALSACaptureDevice microphone ("hw:memsmiccard,0", FIR_SAMPLING_RATE,
                                  MICROPHONE_CHANNELS, FRAMES_PER_PERIOD);
    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", FIR_SAMPLING_RATE,
                                SPEAKER_CHANNELS, FRAMES_PER_PERIOD);

    AdaptiveFilter adaptive_filter(FIR_NTAPS);

    microphone.registerCallback([&](const std::vector<int16_t> &period){adaptive_filter.processAsync(period);});
    adaptive_filter.registerCallback([&](const std::vector<int16_t> &period){speaker.onPeriod(period);});

    adaptive_filter.start();
    microphone.open ();
    speaker.open ();

    // do nothing
    getchar();

    microphone.close ();
    speaker.close ();
    adaptive_filter.stop();
    return 0;
}
