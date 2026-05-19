#include "adaptive_filter.h"

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
        fprintf (stderr, "LMS overflow.\n");
    }

    double output = delayed_sum - canceller;

    fir->lms_update (output);

    if (flog)
        fprintf (flog, "%e\t%e\t%e\t%e\n", sum, diff, canceller, output);

    float y = (float)(output * FIR_OUTPUT_GAIN);

    if (y > 1)
        y = 1;
    if (y < -1)
        y = -1;

    return y;
}

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

void AdaptiveFilter::processAsync (const std::vector<int16_t> &period)
{
    std::lock_guard lk (m);
    if (shared_input.has_value ())
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
        // We get a unique lock to protect the shared input
        std::unique_lock lock (m);
        // Get woken up by the subscriber but might also gets
        // woken up spuriously so need to also check that shared_input
        // has a value. Gets also woken up if the user sets running to false.
        // cv.wait also temporary unlocks the lock so that the other thread
        // can write to shared_input!
        cv.wait (lock, [&] { return shared_input.has_value () || !running; });
        // In case we are no longer running get out of here.
        if (!running)
        {
            lock.unlock ();
            break;
        }
        // This processes the shared_input and generates the output synchronously.
        processPeriod (output);
        // Discarding the shared_input as it's no longer needed.
        shared_input.reset ();
        // Releasing the lock on shared_input now so that we can receive
        // new data without blocking.
        lock.unlock ();
        // Publish results
        if (onPeriod)
        {
            onPeriod (output);
        }
    }
}