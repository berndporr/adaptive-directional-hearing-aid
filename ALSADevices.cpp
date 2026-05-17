#include "ALSADevices.h"
#include <alsa/asoundlib.h>
#include <cstring>
#include <thread>

bool ALSAPCMDevice::open (const std::string _device_name,
                          const unsigned int _sample_rate,
                          const unsigned int _channels,
                          const unsigned int _frames_per_period)
{
    device_name = _device_name;
    sample_rate = _sample_rate;
    channels = _channels;
    frames_per_period = _frames_per_period;

    /* Open PCM device. */
    int rc = snd_pcm_open (&handle, device_name.c_str (),
                           get_pcm_stream_type (), 0);
    if (rc < 0)
    {
        fprintf (stderr, "unable to open pcm device: %s\n", snd_strerror (rc));
        return false;
    }

    snd_pcm_hw_params_t *params;

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca (&params);

    rc = snd_pcm_hw_params_any (handle, params);
    if (rc < 0)
    {
        fprintf (stderr, "Broken configuration: %s.\n", snd_strerror (rc));
        return false;
    }

    rc = snd_pcm_hw_params_set_access (handle, params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0)
    {
        fprintf (stderr, "Can't set access: %s.\n", snd_strerror (rc));
        return false;
    }

    rc = snd_pcm_hw_params_set_format (handle, params, format);
    if (rc < 0)
    {
        fprintf (stderr, "Can't set format: %s.\n", snd_strerror (rc));
        return false;
    }

    rc = snd_pcm_hw_params_set_channels (handle, params, channels);
    if (rc < 0)
    {
        fprintf (stderr, "Can't set channels: %s.\n", snd_strerror (rc));
        return false;
    }

    rc = snd_pcm_hw_params_set_rate (handle, params, sample_rate, 0);
    if (rc < 0)
    {
        fprintf (stderr, "Can't set sampling rate: %s.\n", snd_strerror (rc));
        return false;
    }

    snd_pcm_uframes_t fpp = static_cast<snd_pcm_uframes_t> (frames_per_period);
    rc = snd_pcm_hw_params_set_period_size_near (handle, params, &fpp, 0);
    if (rc < 0)
    {
        fprintf (stderr, "Can't set period size: %s.\n", snd_strerror (rc));
        return false;
    }
    frames_per_period = fpp;

    snd_pcm_uframes_t bufsz
        = static_cast<snd_pcm_uframes_t> (frames_per_period);
    rc = snd_pcm_hw_params_set_buffer_size_near (handle, params, &bufsz);
    if (rc < 0)
    {
        fprintf (stderr, "Can't set buffer size: %s.\n", snd_strerror (rc));
        return false;
    }
    buffer_size = bufsz;

    rc = snd_pcm_hw_params (handle, params);
    if (rc < 0)
    {
        fprintf (stderr, "unable to set hw parameters: %s\n",
                 snd_strerror (rc));
        return false;
    }

    unsigned int ppbuf;
    snd_pcm_hw_params_get_periods_min (params, &ppbuf, NULL);
    if (ppbuf > 2)
    {
        fprintf (stderr,
                 "Playback device does not support 2 periods per buffer.\n");
        return false;
    }

    snd_pcm_hw_params_get_period_time (params, &period_time, 0);

    bytes_per_frame
        = static_cast<unsigned> ((snd_pcm_format_width (format) / 8))
          * channels;

    return true;
}

void ALSACaptureDevice::worker ()
{
    std::vector<int16_t> buffer;
    buffer.resize (frames_per_period * channels);
    running = true;
    while (running)
    {
        snd_pcm_sframes_t n = capture (buffer);
        if (n < 0)
        {
            fprintf (stderr, "Capture error: %s\n", snd_strerror ((int)n));
        }
        if ((n > 0) && onPeriod)
        {
            onPeriod (buffer);
            if ((n * channels) != (long int)buffer.size ())
            {
                fprintf (stderr, "Capture: %ld != %ld\n", n * channels,
                         buffer.size ());
            }
        }
    }
}

void ALSAPCMDevice::close ()
{
    snd_pcm_drain (handle);
    snd_pcm_close (handle);
}

long unsigned int ALSAPCMDevice::get_period_time () { return period_time; }

long unsigned int ALSAPCMDevice::get_channels () { return channels; }

long unsigned int ALSAPCMDevice::get_frames_per_period ()
{
    return frames_per_period;
}

long unsigned int ALSAPCMDevice::get_sample_rate () { return sample_rate; }

long unsigned int ALSAPCMDevice::get_buffer_size () { return buffer_size; }

long unsigned int ALSAPCMDevice::get_bytes_per_frame ()
{
    return bytes_per_frame;
}

bool ALSACaptureDevice::open (const std::string _device_name,
                              const unsigned int _sample_rate,
                              const unsigned int _channels,
                              const unsigned int _frames_per_period)
{
    bool b = ALSAPCMDevice::open (_device_name, _sample_rate, _channels,
                                  _frames_per_period);
    if (!b)
    {
        return b;
    }
    thr = std::thread (&ALSACaptureDevice::worker, this);
    return b;
}

void ALSACaptureDevice::close ()
{
    running = false;
    thr.join ();
    snd_pcm_drop(handle);
    ALSAPCMDevice::close ();
}

snd_pcm_sframes_t ALSACaptureDevice::capture (std::vector<int16_t> &buffer)
{
    //    fprintf (stderr, "buffer:%ld, frames_per_period:%ld\n", data.size (),
    //             frames_per_period);
    snd_pcm_sframes_t frames_read
        = snd_pcm_readi (handle, buffer.data (),
                         static_cast<snd_pcm_uframes_t> (frames_per_period));

    if (frames_read < 0)
    {
        snd_pcm_recover (handle, static_cast<int> (frames_read), 1);
    }

    return frames_read;
}

snd_pcm_sframes_t
ALSAPlaybackDevice::onPeriod (const std::vector<int16_t> &period)
{
    //    fprintf(stderr,"period.size()=%ld,
    //    frames_per_period=%ld\n",period.size(),frames_per_period);
    snd_pcm_sframes_t frames_written
        = snd_pcm_writei (handle, period.data (),
                          static_cast<snd_pcm_uframes_t> (frames_per_period));

    if (frames_written == -EPIPE)
    {
        /* EPIPE means underrun */
        snd_pcm_prepare (handle);
    }

    return frames_written;
}

void ALSAPlaybackDevice::close ()
{
	snd_pcm_drain(handle);
    ALSAPCMDevice::close ();
}
