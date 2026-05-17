#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>

int main ()
{
    const int fs = 8000;
    ALSAPlaybackDevice speaker;

    std::vector<int16_t> buffer;
    buffer.resize (speaker.get_frames_per_period () * speaker.get_channels ());
    // 1kHz in normalised freq.
    float fn = 1000.0f / (float)fs;

    speaker.open ("plughw:rockchipes8316,0", fs, CHANNELS, FRAMES_PER_PERIOD);

    printf ("Latency in us reported by ALSA: %ld\n",
            speaker.get_period_time ());

    long int n = 0;
    for (int i = 0; i < 1000; i++)
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
