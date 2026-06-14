#include "adaptive_filter.h"

float AdaptiveFilter::processSample (std::shared_ptr<Fir1> fir,
                                     std::shared_ptr<DelayLine> delayLine,
                                     float v, float diff)
{
    const double delayed = delayLine->process (v);
    double canceller = fir->filter (diff);
    if (std::abs (canceller) > 1)
    {
        fir->reset ();
        fir->zeroCoeff ();
        canceller = 0;
        fprintf (stderr, "LMS overflow.\n");
    }

    double output = delayed - canceller;

    fir->lms_update (output);

    if (flog)
        fprintf (flog, "%e\t%e\t%e\n", diff, canceller, output);

    output = output * FIR_OUTPUT_GAIN;
    if (output > 1)
        output = 1;
    if (output < -1)
        output = -1;
    return (float)output;
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
        const float diff = left - right;
        const float lOut = processSample (firL,delayLineL,left,diff);
        const float rOut = processSample (firR,delayLineR,right,diff);
        output[2 * i] = (int16_t)(lOut * 32768);
        output[2 * i + 1] = (int16_t)(rOut * 32768);
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