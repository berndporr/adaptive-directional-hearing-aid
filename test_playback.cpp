#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    // sampling rate
    const int fs = 8000;

    // We play a sine wave of 1kHz
    // In normalised freq:
    const float fn = 1000.0f / (float)fs;

    // 3 seconds
    // In ALSA Periods:
    const int nPeriods = (int)(3.0*fs/FRAMES_PER_PERIOD);

    // Sample buffer for one Period
    std::vector<int16_t> buffer;
    buffer.resize (CHANNELS * FRAMES_PER_PERIOD);

    // Our Loudspeaker
    ALSAPlaybackDevice speaker;
    speaker.open ("plughw:rockchipes8316,0", fs, CHANNELS, FRAMES_PER_PERIOD);

    printf ("Latency in us reported by ALSA: %ld\n",
            speaker.get_period_time ());

    long int n = 0;
    for (int i = 0; i < nPeriods; i++)
    {
        for (long unsigned int j = 0; j < (buffer.size () / 2); j++)
        {
            int16_t v = (int16_t)(sin ((float)n * 2 * M_PI * fn) * 10000);
            buffer[j * 2] = v;
            buffer[j * 2 + 1] = v;
            n++;
        }
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
