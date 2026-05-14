#ifndef __ALSADevices_H
#define __ALSADevices_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <string>
#include <vector>

class ALSAPCMDevice
{
  protected:
    const snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
    snd_pcm_t *handle = nullptr;
    std::string device_name;
    unsigned int sample_rate;
    snd_pcm_sframes_t frames_per_period;
    size_t bytes_per_frame;
    unsigned int period_time;
    unsigned int channels;
    virtual void worker () = 0;

  public:
    ALSAPCMDevice (const std::string device_name,
                   const unsigned int sample_rate, const unsigned int channels,
                   const unsigned int frames_per_period)
        : device_name (device_name), sample_rate (sample_rate),
          frames_per_period (frames_per_period), channels (channels)
    {
    }

    bool open ();
    void close ();
    snd_pcm_sframes_t get_frames_per_period ();
    size_t get_bytes_per_frame ();
    unsigned int get_channels ();
    unsigned int get_period_time ();
    unsigned int get_sample_rate ();
    std::thread thr;
    bool running = false;
};

class ALSACaptureDevice : public ALSAPCMDevice
{
  public:
    ALSACaptureDevice (const std::string device_name,
                       const unsigned int sample_rate,
                       const unsigned int channels,
                       unsigned int frames_per_period)
        : ALSAPCMDevice (device_name, sample_rate, channels, frames_per_period)
    {
    }

    using OnFrame = std::function<void (const float l, const float r)>;
    void registerCallback (OnFrame of) { onFrame = of; }

  private:
    snd_pcm_sframes_t capture (std::vector<char> &buffer);
    OnFrame onFrame;
    virtual void worker() override;
};

class ALSAPlaybackDevice : public ALSAPCMDevice
{
  public:
    ALSAPlaybackDevice (std::string device_name, unsigned int sample_rate,
                        unsigned int channels, unsigned int frames_per_period)
        : ALSAPCMDevice (device_name, sample_rate, channels, frames_per_period)
    {
        buffer.resize (bytes_per_frame * frames_per_period);
    }

    void addFrame (float l, float r);

  private:
    virtual void worker();
    snd_pcm_sframes_t play ();
    std::atomic<int> bufferIndex = 0;
    std::vector<int16_t> buffer;
    std::mutex mtx;
};

#endif