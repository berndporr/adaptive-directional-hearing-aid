#include "adaptive_filter.h"

void AdaptiveFilter::processPeriod (std::vector<int16_t> &output)
{
    unsigned long frames_per_period = shared_input.value ().size () / 2;
    if (shared_input.value ().size () != output.size ())
        {
            output.resize (shared_input.value ().size ());
        }
    for (unsigned long i = 0; i < frames_per_period; i++)
        {
            const float left = shared_input.value ()[2 * i] / 32768.0f;
            const float right = shared_input.value ()[2 * i + 1] / 32768.0f;
            const float y = processSample (left, right);
            output[2 * i] = (int16_t)(y * 32768);
            output[2 * i + 1] = (int16_t)(y * 32768);
        }
}

float AdaptiveFilter::processSample (const float l, const float r)
{
    float sum = r + l;
    float diff = r - l;

    double delayed_sum = delayLine->process (sum);
    double canceller = fir->filter (diff);
    if (std::abs (canceller) > 1)
        {
            fir->reset ();
            fir->zeroCoeff ();
            canceller = 0;
        }

    double output = delayed_sum - canceller;

    fir->lms_update (output);

    float y = (float)(output * GAIN);

    if (y > 1)
        y = 1;
    if (y < -1)
        y = -1;

    return y;
}

void AdaptiveFilter::processAsync (const std::vector<int16_t> &period)
{
    std::lock_guard lk (m);
    if (shared_input.has_value())
        {
            fprintf (stderr, "Congestion in the adaptive filter.");
            return;
        }
    shared_input = std::move (period);
    cv.notify_all ();
}

void AdaptiveFilter::worker ()
{
    std::vector<int16_t> output;

    running = true;
    while (running)
        {
            std::unique_lock lock (m);
            cv.wait (lock,
                     [&] { return shared_input.has_value () || !running; });
            if (!running)
                break;
            processPeriod (output);
            if (onPeriod)
                {
                    onPeriod (output);
                }
            shared_input.reset ();
            lock.unlock ();
        }
}