#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", FIR_SAMPLING_RATE,
                                SPEAKER_CHANNELS, FRAMES_PER_PERIOD);

    std::vector<int16_t> buffer;
    buffer.resize (speaker.get_frames_per_period() * speaker.get_channels());

    speaker.open ();

    for(int i = 0; i < 1000; i++) {
        printf(".");
        speaker.onPeriod(buffer);
    }
    printf("\n");

    speaker.close ();

    return 0;
}
