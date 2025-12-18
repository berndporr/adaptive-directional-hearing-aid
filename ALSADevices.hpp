#ifndef __ALSADevices_H
#define __ALSADevices_H

#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <string>
#include <vector>

class ALSAPCMDevice {
    protected:
    snd_pcm_t* handle;
    std::string device_name;
    unsigned int sample_rate, channels;             
    snd_pcm_uframes_t frames_per_period;            
    snd_pcm_format_t format;                        
    _snd_pcm_stream type; 
    std::vector<char> buffer;
    size_t bytes_per_frame;
                         
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
    void allocate_buffer();
    snd_pcm_uframes_t get_frames_per_period();
    size_t get_bytes_per_frame();
    unsigned int get_channels();
    const std::vector<char>& get_buffer() const;
};


class ALSACaptureDevice : public ALSAPCMDevice {
    public:
    ALSACaptureDevice(
        std::string device_name,
        unsigned int sample_rate,
        unsigned int channels,
        unsigned int frames_per_period,
        snd_pcm_format_t format
    ) : ALSAPCMDevice( 
        device_name,
        sample_rate, 
        channels, 
        frames_per_period, 
        format, 
        SND_PCM_STREAM_CAPTURE) 
    {}

    snd_pcm_sframes_t capture_into_buffer();
};


class ALSAPlaybackDevice : public ALSAPCMDevice {
    public:
    ALSAPlaybackDevice(
        std::string device_name,
        unsigned int sample_rate,
        unsigned int channels,
        unsigned int frames_per_period,
        snd_pcm_format_t format
    ) : ALSAPCMDevice( 
        device_name,
        sample_rate, 
        channels, 
        frames_per_period, 
        format, 
        SND_PCM_STREAM_PLAYBACK) 
    {}

    snd_pcm_sframes_t play_from_buffer();
    void copy_from_capture(const ALSACaptureDevice& mic);
    void copy_from_capture_mono(const ALSACaptureDevice& mic);
};

#endif