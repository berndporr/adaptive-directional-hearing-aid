#ifndef _ALSADevices_H
#define _ALSADevices_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#include <string>
#include <vector>

/**
 * Generic ALSA Device for both capture and playback.
 */
class ALSAPCMDevice
{
  public:
    ALSAPCMDevice () = default;

    /**
     * Opens the ALSA device with the requested paramters.
     * @param _device_name ALSA device as listed for example by "aplay -l" or "arecord -l".
     * @param _sample_rate Exact sampling rate of the device. Nn deviation is allowed.
     * @param _channels Exact number of channels. No deviation is allowed.
     * @param _frames_per_period Desired number of frames per period. This might be chnanged by ALSA.
     * @return True on success. False if any of the parameters won't work.
     */
    virtual bool open (const std::string _device_name,
                       const unsigned int _sample_rate,
                       const unsigned int _channels,
                       const unsigned int _frames_per_period);

    /**
     * Closes the ALSA device.
     */
    virtual void close ();

    /**
     * Gets the actual number of frames per period.
     */
    long unsigned int get_frames_per_period () const;

    /**
     * Gets the period time in us. That's basically the
     * max latency from submitting to playing as internall ALSA
     * buffering one period.
     */
    long unsigned int get_period_time () const;

    /**
     * Gets the internal kernel buffer size which
     * is two times larger than a period so that the kernel
     * can do double buffering.
     */
    long unsigned int get_buffer_size () const;

    /**
     * Needs to be overridden by its children and either return
     * SND_PCM_STREAM_CAPTURE or SND_PCM_STREAM_PLAYBACK.
     */
    virtual _snd_pcm_stream get_pcm_stream_type () const = 0;

    /**
     * The dataformat of a sample. It's always signed 16 bit little endian.
     */
    static constexpr snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

  protected:
    snd_pcm_t *handle = nullptr;
    std::string device_name;
    unsigned int sample_rate = 0;
    snd_pcm_sframes_t frames_per_period = 0;
    unsigned int period_time = 0;
    unsigned int channels = 0;
    long unsigned int buffer_size = 0;
};

/**
 * ALSA capture device with callback. Samples are of type int16_t and
 * are interleaved in the buffer for stereo.
 */
class ALSACaptureDevice : public ALSAPCMDevice
{
  public:
    /**
     * Callback type which returns a period with frames inside.
     * For mono it's simply a stream of int16. For stereo they
     * are interleaved: LRLRLR etc.
     */
    using OnPeriod = std::function<void (const std::vector<int16_t> &)>;

    /**
     * Registers the callback. Note it needs to be faster than the
     * duration of one period. Ideally it should be a lot shorter
     * and hand it over to a non-blocking subscriber.
     */
    void registerCallback (OnPeriod of) { onPeriod = of; }

    /**
     * Starts audio capture of the ALSA device with the requested paramters.
     * If this method is successful on return it will start calling the
     * callback at the rate of the period.
     * @param _device_name ALSA device as listed for example by "arecord -l".
     * @param _sample_rate Requested exact sampling rate of the device. No deviation is allowed.
     * @param _channels Requested exact number of channels. No deviation is allowed.
     * @param _frames_per_period Desired number of frames per period. This might be chnanged by ALSA.
     * @return Is true on success. False if any of the parameters won't work.
     */
    bool open (const std::string _device_name, const unsigned int _sample_rate,
               const unsigned int _channels,
               const unsigned int _frames_per_period) override;

    /**
     * Stops capture and closes the device. The callback won't be called any
     * longer.
     */
    void close () override;

  private:
    snd_pcm_sframes_t capture (std::vector<int16_t> &buffer);
    OnPeriod onPeriod;
    virtual void worker ();
    std::thread thr;
    bool running = false;
    virtual _snd_pcm_stream get_pcm_stream_type () const override
    {
        return SND_PCM_STREAM_CAPTURE;
    };
};

/**
 * ALSA playback device. Samples are of type int16_t and
 * are always interleaved in the buffer for stereo.
 */
class ALSAPlaybackDevice : public ALSAPCMDevice
{
  public:
    /**
     * Closes the device. Sound output stops.
     */
    void close () override;

    /**
     * Plays a period. For mono audio it's just a stream of int16.
     * For stereo it needs to be interleaved LRLRLR.
     * @param period The audio to play. Mono: LLLLLL..., stereo: LRLRLR...
     * @return The number of frames delivered or an error code.
     */
    snd_pcm_sframes_t onPeriod (const std::vector<int16_t> &period);

    /**
     * Checks if onPeriod has returned the "Unterrun error code" which
     * means that not enough data has been supplied.
     */
    inline bool isUnderrunErrorCode (const long int r) { return -EPIPE == r; }

  private:
    virtual _snd_pcm_stream get_pcm_stream_type () const override
    {
        return SND_PCM_STREAM_PLAYBACK;
    };
};

#endif