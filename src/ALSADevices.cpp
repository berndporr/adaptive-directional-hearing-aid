
#include "../include/ALSADevices.h"
#include <cstring>


bool ALSAPCMDevice::open() {
    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t* sw;
    /* Open PCM device. */
    int rc = snd_pcm_open(&handle, device_name.c_str(), type, 0);
    if (rc < 0) {
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
        return false;
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);
    
    snd_pcm_hw_params_any(handle, params);

    
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    snd_pcm_hw_params_set_format(handle, params, format);
    
    snd_pcm_hw_params_set_channels(handle, params, channels);
    
    snd_pcm_hw_params_set_rate(handle, params, sample_rate, 0);
    
    snd_pcm_hw_params_set_period_size(handle, params, static_cast<snd_pcm_uframes_t>(frames_per_period), 0);
    
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        return false;
    }

    snd_pcm_hw_params_get_period_time(params,&period_time, 0);

    snd_pcm_sw_params_alloca(&sw);
    snd_pcm_sw_params_current(handle, sw);
    snd_pcm_sw_params_set_start_threshold(handle, sw, period_time);
    snd_pcm_sw_params_set_avail_min(handle, sw, period_time);
    snd_pcm_sw_params(handle, sw);

    bytes_per_frame = static_cast<unsigned>((snd_pcm_format_width(format) / 8)) * channels;
    allocate_buffer();
    

    return true;
}

void ALSAPCMDevice::close() {
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
}

void ALSAPCMDevice::allocate_buffer() {
    size_t total_bytes = static_cast<size_t>(frames_per_period) * bytes_per_frame;
    buffer.resize(total_bytes);
}


unsigned int ALSAPCMDevice::get_period_time(){
    return period_time;
}

unsigned int ALSAPCMDevice::get_channels(){
    return channels;
}

snd_pcm_sframes_t ALSAPCMDevice::get_frames_per_period() {
    return frames_per_period;
}

const std::vector<char>& ALSAPCMDevice::get_buffer() const{
    return buffer;
}

 unsigned int ALSAPCMDevice::get_sample_rate(){
    return sample_rate;
}

size_t ALSAPCMDevice::get_bytes_per_frame() {
    return bytes_per_frame;
}

snd_pcm_sframes_t ALSACaptureDevice::capture_into_buffer() {
    snd_pcm_sframes_t frames_read = snd_pcm_readi(handle, buffer.data(), static_cast<snd_pcm_uframes_t>(frames_per_period));

        
    if(frames_read == 0) {
        fprintf(stderr, "End of file.\n");
        return 0;
    }
    if (frames_read == -EINTR) {
        return 0; 
    }

    if(frames_read != static_cast<snd_pcm_sframes_t>(frames_per_period)) {
        fprintf(stderr, "Short read: we read <%ld> frames\n", frames_read);
        // A -ve return value means an error.
        if(frames_read < 0) {
            snd_pcm_recover(handle, static_cast<int>(frames_read), 1);
            fprintf(stderr, "error from readi: %s\n", snd_strerror(static_cast<int>(frames_read)));
            return 0;
        }
        return frames_read;
    }
    return frames_read;
}

snd_pcm_sframes_t ALSACaptureDevice::capture_into_array(std::vector<char>& data) {
    snd_pcm_sframes_t frames_read = snd_pcm_readi(handle, data.data(), static_cast<snd_pcm_uframes_t>(frames_per_period));

        
    if(frames_read == 0) {
        fprintf(stderr, "End of file.\n");
        return 0;
    }
    if (frames_read == -EINTR) {
        return 0; 
    }

    if(frames_read != static_cast<snd_pcm_sframes_t>(frames_per_period)) {
        fprintf(stderr, "Short read: we read <%ld> frames\n", frames_read);
        // A -ve return value means an error.
        if(frames_read < 0) {
            snd_pcm_recover(handle, static_cast<int>(frames_read), 1);
            fprintf(stderr, "error from readi: %s\n", snd_strerror(static_cast<int>(frames_read)));
            return 0;
        }
        return frames_read;
    }
    return frames_read;
}

snd_pcm_sframes_t ALSAPlaybackDevice::play_from_buffer() {
    snd_pcm_sframes_t frames_written = snd_pcm_writei(handle, buffer.data(), static_cast<snd_pcm_uframes_t>(frames_per_period));

    if (frames_written == -EINTR) {
        return 0;
    }

    if (frames_written == -EPIPE) {
        /* EPIPE means underrun */
        fprintf(stderr, "underrun occurred\n");
        snd_pcm_prepare(handle);
    } else if (frames_written < 0) {
        fprintf(stderr, "error from writei: %s\n", snd_strerror(static_cast<int>(frames_written)));
    }  else if (frames_written != static_cast<snd_pcm_sframes_t>(frames_per_period)) {
        fprintf(stderr, "short write, write %ld frames\n", frames_written);
    }

    return frames_written;
}
snd_pcm_sframes_t ALSAPlaybackDevice::play_from_array(const std::vector<char>& data,snd_pcm_sframes_t frames_to_play) {
    if (frames_to_play != frames_per_period){
        fprintf(stderr, "frames_to_play must equal frames in period <%lu>\n", frames_per_period);
        return 0;
    }
    
    snd_pcm_sframes_t frames_written = snd_pcm_writei(handle, data.data(), static_cast<snd_pcm_uframes_t>(frames_per_period));

    if (frames_written == -EINTR) {
        return 0;
    }

    if (frames_written == -EPIPE) {
        /* EPIPE means underrun */
        fprintf(stderr, "underrun occurred\n");
        snd_pcm_prepare(handle);
    } else if (frames_written < 0) {
        fprintf(stderr, "error from writei: %s\n", snd_strerror(static_cast<int>(frames_written)));
    }  else if (frames_written != frames_per_period) {
        fprintf(stderr, "short write, write %ld frames\n", frames_written);
    }

    return frames_written;
}


void ALSAPlaybackDevice::copy_from_capture(const ALSACaptureDevice& mic){
    const auto& src = mic.get_buffer();

    size_t bytes = static_cast<size_t>(frames_per_period) * bytes_per_frame;

    std::memcpy(buffer.data(), src.data(), bytes);

}

void ALSAPlaybackDevice::copy_from_capture_mono(const ALSACaptureDevice& mic){
    const auto& src = mic.get_buffer();

    auto* in = reinterpret_cast<const int16_t*>(src.data());
    auto* out = reinterpret_cast<int16_t*>(buffer.data());

    for (unsigned int i = 0; i < frames_per_period; i++) {
        int16_t left  = in[2 * i];
        int16_t right = in[2 * i + 1];

        int32_t sum = static_cast<int32_t>(left) + static_cast<int32_t>(right);

        int16_t mono = static_cast<int16_t>(sum / 2);

        out[i]= mono;
    }
}

