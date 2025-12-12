#ifndef __ALSADevices_H
#define __ALSADevices_H

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <string>

class ALSAPCMDevice {
    protected:
    snd_pcm_t* handle;
    std::string device_name;
    unsigned int sample_rate, channels;             // Quality of the recorded audio.
    snd_pcm_uframes_t frames_per_period;            // Latency - lower numbers will decrease latency and increase CPU usage.
    snd_pcm_format_t format;                        // Bit depth - Quality.
    _snd_pcm_stream type;                      // SND_PCM_STREAM_CAPTURE | SND_PCM_STREAM_PLAYBACK

    public:
    ALSAPCMDevice(
        std::string device_name,
        unsigned int sample_rate,
        unsigned int channels,
        unsigned int frames_per_period,
        snd_pcm_format_t format,
        _snd_pcm_stream type
    ) : 
        device_name(device_name),
        sample_rate(sample_rate),
        channels(channels),
        frames_per_period(frames_per_period),
        format(format),
        type(type)
    {}

    bool open();
    void close();
    char* allocate_buffer();
    unsigned int get_frames_per_period();
    unsigned int get_bytes_per_frame();
    unsigned int get_channels();
};


class ALSACaptureDevice : public ALSAPCMDevice {
    public:
    ALSACaptureDevice(
        std::string device_name,
        unsigned int sample_rate,
        unsigned int channels,
        unsigned int frames_per_period,
        snd_pcm_format_t format
    ) : ALSAPCMDevice( // Calling super constructor
        device_name,
        sample_rate, 
        channels, 
        frames_per_period, 
        format, 
        SND_PCM_STREAM_CAPTURE) 
    {}

    unsigned int capture_into_buffer(char* buffer, snd_pcm_uframes_t frames_to_capture);
};


class ALSAPlaybackDevice : public ALSAPCMDevice {
    public:
    ALSAPlaybackDevice(
        std::string device_name,
        unsigned int sample_rate,
        unsigned int channels,
        unsigned int frames_per_period,
        snd_pcm_format_t format
    ) : ALSAPCMDevice( // Calling super constructor
        device_name,
        sample_rate, 
        channels, 
        frames_per_period, 
        format, 
        SND_PCM_STREAM_PLAYBACK) 
    {}

    unsigned int play_from_buffer(char* buffer, snd_pcm_uframes_t frames_to_play);
};

#endif