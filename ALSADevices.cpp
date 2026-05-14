
#include "ALSADevices.h"
#include <cstring>
#include <mutex>
#include <thread>

bool ALSAPCMDevice::open ()
{
    snd_pcm_hw_params_t *params;

    /* Open PCM device. */
    int rc = snd_pcm_open (&handle, device_name.c_str (),
                           SND_PCM_STREAM_CAPTURE, 0);
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

    thr = std::thread (&ALSAPCMDevice::worker, this);

    return true;
}

void ALSACaptureDevice::worker ()
{
    std::vector<char> buffer;
    buffer.resize (bytes_per_frame * frames_per_period);
    running = true;
    while (running)
        {
            capture (buffer);
            for (int i = 0; i < frames_per_period; i++)
                {
                    auto framedata
                        = reinterpret_cast<const int16_t *> (buffer.data ());
                    const float left = framedata[2 * i] / 32768.0f;
                    const float right = framedata[2 * i + 1] / 32768.0f;
                    if (onFrame)
                        {
                            onFrame (left, right);
                        }
                }
        }
}

void ALSAPCMDevice::close ()
{
    running = false;
    thr.join ();
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

snd_pcm_sframes_t ALSACaptureDevice::capture (std::vector<char> &buffer)
{
    //    fprintf (stderr, "buffer:%ld, frames_per_period:%ld\n", data.size (),
    //             frames_per_period);
    snd_pcm_sframes_t frames_read
        = snd_pcm_readi (handle, buffer.data (),
                         static_cast<snd_pcm_uframes_t> (frames_per_period));

    if (frames_read == 0)
        {
            fprintf (stderr, "End of file.\n");
            return 0;
        }
    if (frames_read == -EINTR)
        {
            return 0;
        }

    if (frames_read != static_cast<snd_pcm_sframes_t> (frames_per_period))
        {
            fprintf (stderr, "Short read: we read <%ld> frames\n",
                     frames_read);
            // A -ve return value means an error.
            if (frames_read < 0)
                {
                    snd_pcm_recover (handle, static_cast<int> (frames_read),
                                     1);
                    fprintf (stderr, "error from readi: %s\n",
                             snd_strerror (static_cast<int> (frames_read)));
                    return 0;
                }
            return frames_read;
        }
    return frames_read;
}

snd_pcm_sframes_t ALSAPlaybackDevice::play ()
{
    mtx.lock();
    snd_pcm_sframes_t frames_written
        = snd_pcm_writei (handle, buffer.data (),
                          static_cast<snd_pcm_uframes_t> (frames_per_period));
    mtx.unlock();

    if (frames_written == -EINTR)
        {
            return 0;
        }

    if (frames_written == -EPIPE)
        {
            /* EPIPE means underrun */
            fprintf (stderr, "underrun occurred\n");
            snd_pcm_prepare (handle);
        }
    else if (frames_written < 0)
        {
            fprintf (stderr, "error from writei: %s\n",
                     snd_strerror (static_cast<int> (frames_written)));
        }
    else if (frames_written != frames_per_period)
        {
            fprintf (stderr, "short write, write %ld frames\n",
                     frames_written);
        }

    return frames_written;
}

void ALSAPlaybackDevice::addFrame (float l, float r)
{
    if (bufferIndex >= frames_per_period)
        {
            return;
        }
    std::lock_guard<std::mutex> guard(mtx);
    buffer[bufferIndex * 2] = (int16_t)(l * 32768);
    buffer[bufferIndex * 2 + 1] = (int16_t)(r * 32768);
    bufferIndex++;
}

void ALSAPlaybackDevice::worker () {
    running = true;
    while (running) {
        play();
        bufferIndex = 0;
    }
}