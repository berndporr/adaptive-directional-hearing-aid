#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    ALSAPlaybackDevice speaker ("plughw:rockchipes8316,0", SAMPLING_RATE,
                                SPEAKER_CHANNELS, FRAMES_PER_PERIOD);

    std::vector<int16_t> buffer;
    buffer.resize (speaker.get_frames_per_period () * speaker.get_channels ());
    for (auto &v : buffer)
        {
            v = (int16_t)(random () % 0x8000) - 0x4000;
        }

    speaker.open ();

    for (int i = 0; i < 1000; i++)
        {
            long int r = speaker.onPeriod (buffer);
            if (r < 0)
                {
                    fprintf (stderr, "\nALSA error: %ld\n", r);
                    break;
                }
            else
                {
                    printf (".");
                }
        }
    printf ("\n");

    speaker.close ();

    return 0;
}
