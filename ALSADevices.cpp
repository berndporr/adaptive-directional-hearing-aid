
#include "ALSADevices.h"
#include <cstring>
#include <thread>

bool ALSAPCMDevice::open ()
{
    snd_pcm_hw_params_t *params;

    /* Open PCM device. */
    int rc = snd_pcm_open (&handle, device_name.c_str (),
                           get_pcm_stream_type (), 0);
    if (rc < 0)
        {
            fprintf (stderr, "unable to open pcm device: %s\n",
                     snd_strerror (rc));
            return false;
        }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca (&params);

    snd_pcm_hw_params_any (handle, params);

    snd_pcm_hw_params_set_access (handle, params,
                                  SND_PCM_ACCESS_RW_INTERLEAVED);

    snd_pcm_hw_params_set_format (handle, params, format);

    snd_pcm_hw_params_set_channels (handle, params, channels);

    snd_pcm_hw_params_set_rate (handle, params, sample_rate, 0);

    snd_pcm_hw_params_set_period_size (
        handle, params, static_cast<snd_pcm_uframes_t> (frames_per_period), 0);

    rc = snd_pcm_hw_params (handle, params);
    if (rc < 0)
        {
            fprintf (stderr, "unable to set hw parameters: %s\n",
                     snd_strerror (rc));
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
            capture (buffer);
            if (onPeriod)
                {
                    onPeriod (buffer);
                }
        }
}

void ALSAPCMDevice::close ()
{
    snd_pcm_drain (handle);
    snd_pcm_close (handle);
}

unsigned int ALSAPCMDevice::get_period_time () { return period_time; }

unsigned int ALSAPCMDevice::get_channels () { return channels; }

snd_pcm_sframes_t ALSAPCMDevice::get_frames_per_period ()
{
    return frames_per_period;
}

unsigned int ALSAPCMDevice::get_sample_rate () { return sample_rate; }

size_t ALSAPCMDevice::get_bytes_per_frame () { return bytes_per_frame; }

bool ALSACaptureDevice::open ()
{
    bool b = ALSAPCMDevice::open ();
    if (!b)
        {
            return b;
        }
    thr = std::thread (&ALSACaptureDevice::worker, this);
    return b;
}

void ALSACaptureDevice::close ()
{
    ALSAPCMDevice::close ();
    running = false;
    thr.join ();
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
            if (reportUnderruns) {
                fprintf(stderr,"Playback buffer underrun!\n");
            }
        }

    return frames_written;
}
