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

class DelayLine
{
  public:
    explicit DelayLine (size_t delay) : buffer (delay, 0.0), size (delay) {}

    inline double process (double x)
    {
        double y = buffer[index];
        buffer[index] = x;

        index++;
        if (index == size)
            index = 0;

        return y;
    }

  private:
    std::vector<double> buffer;
    size_t size;
    size_t index = 0;
};

class AdaptiveFilter
{
  public:
    AdaptiveFilter (const int ntaps)
    {
        fir = std::make_shared<Fir1> (ntaps, 0.00000);
        delayLine = std::make_shared<DelayLine> (delay_line_length);
    }
    void setLearningrate (float mu) { fir->setLearningRate (mu); }
    float processSync (const float l, const float r);
    void processAsync (const float l, const float r);

    using OnFrame = std::function<void (const float l, const float r)>;
    void registerCallback (OnFrame of) { onFrame = of; }

    void start() {
      if (running) return;
      thr = std::thread(&AdaptiveFilter::worker,this);
    }

    void stop() {
      if (!running) return;
      running = false;
      cv.notify_all();
      if (thr.joinable()) thr.join();
    }

    private:
    std::shared_ptr<Fir1> fir;
    std::shared_ptr<DelayLine> delayLine;
    std::thread thr;
    bool running = false;

    std::mutex m;
    std::condition_variable cv;
    std::optional<float> shared_left;
    std::optional<float> shared_right;

    void worker ();
    OnFrame onFrame;
    const size_t delay_line_length = static_cast<size_t> (std::round (
        ((AVERAGE_DISTANCE_FROM_EAR_TO_EAR_CM / 100.0) / SPEED_OF_SOUND)
        * FIR_SAMPLING_RATE * DELAY_LINE_MULTIPLIER));
};

#endif
