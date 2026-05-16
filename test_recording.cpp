#include "ALSADevices.h"
#include "constants.h"
#include <cmath>
#include <cstdio>
#include <unistd.h>

const std::string datFilename = "/tmp/test_recording.dat";

struct SOXdatafile
{
    FILE *f;
    long int n = 0;
    SOXdatafile (std::string filename)
    {
        f = fopen (filename.c_str (), "wt");
        if (!f)
            {
                fprintf (stderr, "Can't open %s\n", filename.c_str ());
                throw;
            }
        fprintf (f, "; Sample Rate %d\n", SAMPLING_RATE);
        fprintf (f, "; Channels %d\n", CHANNELS);
    }
    ~SOXdatafile () { fclose (f); }
    void addSample (const int16_t l, const int16_t r)
    {
        fprintf (f, "%f\t%f\t%f\n", (double)n / SAMPLING_RATE,
                 (float)l / 32768.0f, (float)r / 32768.0f);
        n++;
    }
};

int main ()
{

    ALSACaptureDevice microphone ("hw:memsmiccard,0", SAMPLING_RATE,
                                  CHANNELS, FRAMES_PER_PERIOD);

    SOXdatafile soxDatafile (datFilename);

    microphone.registerCallback ([&] (const std::vector<int16_t> &period) {
        for (long unsigned int i = 0;
             i < (period.size () / CHANNELS); i++)
            {
                soxDatafile.addSample (period[i * 2], period[i * 2 + 1]);
            }
    });

    microphone.open ();

    printf ("Bear with me!\n");

    sleep (2);

    microphone.close ();

    printf ("Audio written to: %s\n", datFilename.c_str ());

    return 0;
}
