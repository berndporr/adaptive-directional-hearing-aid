#ifndef _ADAPTIVE_FILTER
#define _ADAPTIVE_FILTER

#include "Fir1.h"
#include "constants.h"
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <functional>
#include <memory>
#include <optional>
#include <thread>

/**
 * Delay line is double values.
 */
class DelayLine
{
  public:
    /**
     * Constructs the delay line.
     * @param delay The number of delay steps
     */
    explicit DelayLine (size_t delay) : buffer (delay, 0.0) {}

    /**
     * Processes a sample.
     * @param x Input to the delay line.
     * @return Output of the delay line.
     */
    inline double process (double x)
    {
        double y = buffer[index];
        buffer[index] = x;

        index++;
        if (index == buffer.size ())
            index = 0;

        return y;
    }

  private:
    std::vector<double> buffer;
    size_t index = 0;
};

/**
 * LMS Adaptive Noise canceller.
 * It tries to cancel out anything that's in l-r.
 * It's working asynchronously by subscribing to an audio stream
 * and publishing the results.
 */
class AdaptiveFilter
{
  public:
    /**
     * Constructs the adaptive FIR filter.
     * @param ntaps Number of taps of the adaptive FIR filter.
     * @param sampling_rate The sampling rate.
     */
    AdaptiveFilter (const int ntaps, const int sampling_rate)
        : sampling_rate (sampling_rate)
    {
        fir = std::make_shared<Fir1> (ntaps, 0.00000);
        delayLine = std::make_shared<DelayLine> (delay_line_length);
        fir->setLearningRate (0);
    }

    /**
     * Sets the learning rate of the FIR filter.
     */
    void setLearningrate (float mu) { fir->setLearningRate (mu); }

    /**
     * Subscriber to the microphone of a sound card.
     * @param period is the chunk of data processed: stereo interleaved LRLR.
     */
    void processAsync (const std::vector<int16_t> &period);

    /**
     * Callback when a new chunk is ready.
     */
    using OnPeriod = std::function<void (const std::vector<int16_t> &)>;

    /**
     * Registers the callback for the subscriber.
     */
    void registerCallback (OnPeriod op) { onPeriod = op; }

    /**
     * Starts the thread processing the audio chunks async.
     */
    void start ()
    {
        if (running)
            return;
        thr = std::thread (&AdaptiveFilter::worker, this);
        shared_input.reset ();
    }

    /**
     * Stops the thread processing the async audio chunks.
     */
    void stop ()
    {
        if (!running)
            return;
        running = false;
        // Waking up the worker thread and finish it.
        cv.notify_all ();
        // Waiting for the worker thread to finish.
        if (thr.joinable ())
            thr.join ();
    }

    /**
     * Enables logging sample by sample
     */
    void enableLogging (const char *filename)
    {
        flog = fopen (filename, "wt");
    }

  private:
    int sampling_rate = 0;
    std::shared_ptr<Fir1> fir;
    std::shared_ptr<DelayLine> delayLine;
    std::thread thr;
    bool running = false;

    std::mutex m;
    std::condition_variable cv;
    std::optional<std::vector<int16_t> > shared_input;

    void processPeriod (std::vector<int16_t> &output);
    float processSample (const float l, const float r);
    void worker ();
    OnPeriod onPeriod;
    const size_t delay_line_length = static_cast<size_t> (std::round (
        ((AVERAGE_DISTANCE_FROM_EAR_TO_EAR_CM / 100.0) / SPEED_OF_SOUND)
        * sampling_rate * DELAY_LINE_MULTIPLIER));

    FILE *flog = nullptr;
};

#endif
