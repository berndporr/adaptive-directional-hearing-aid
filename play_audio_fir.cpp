#include "ALSADevices.h"
#include "constants.h"
#include <cmath>

int main ()
{
    ALSACaptureDevice microphone ("hw:memsmiccard,0", FIR_SAMPLING_RATE,
                                  MICROPHONE_CHANNELS, FRAMES_PER_PERIOD);
    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", FIR_SAMPLING_RATE,
                                SPEAKER_CHANNELS, FRAMES_PER_PERIOD);

    microphone.open ();
    speaker.open ();

    microphone.close ();
    speaker.close ();
    return 0;
}
