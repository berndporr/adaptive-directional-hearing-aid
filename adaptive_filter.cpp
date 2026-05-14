#include "adaptive_filter.h"

float AdaptiveFilter::processSync (const float l, const float r)
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

void AdaptiveFilter::processAsync (const float l, const float r) {
    if (shared_left.has_value() || shared_right.has_value()) {
        fprintf(stderr,"Dropping frame.\n");
        return;
    }
    shared_left = l;
    shared_right = r;
    cv.notify_all();
}

void AdaptiveFilter::worker ()
{
    running = true;
    while (running)
        {
            std::unique_lock lock (m);
            cv.wait (lock);
            if (!running)
                break;
            float y = processSync(shared_left.value(), shared_right.value());
            if (onFrame) {
                onFrame(y,y);
            }
            shared_left.reset();
            shared_right.reset();
        }
}