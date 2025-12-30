#include "../include/ALSADevices.h"
#include "Fir1.h"
#include "../constants.h"
#include <cstddef>
#include <cstdint>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>


struct AudioMessage {
    std::vector<char> data;
    snd_pcm_sframes_t frames;

    AudioMessage(snd_pcm_sframes_t frames,size_t bytes_per_frame) : 
        frames(frames) 
        {
            data.resize(static_cast<size_t>(frames) * bytes_per_frame);
        }

};

class AudioQueue {
    std::queue<AudioMessage> queue;
    std::mutex mtx;
    std::condition_variable cv;
    size_t MAX_QUEUE_SIZE;


public:
    AudioQueue(size_t MAX_QUEUE_SIZE):
        MAX_QUEUE_SIZE(MAX_QUEUE_SIZE)
    {}

    void push(AudioMessage msg) {
        std::unique_lock<std::mutex> lock(mtx);
        if (queue.size() >= MAX_QUEUE_SIZE)
            return;

        queue.push(std::move(msg));
        lock.unlock();
        cv.notify_one();
    }

    AudioMessage pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty(); });
        AudioMessage msg = std::move(queue.front());
        queue.pop();
        return msg;
    }
};

void capture_thread_func(ALSACaptureDevice* mic, AudioQueue* audio_queue) {
    while (1) {
        AudioMessage msg(mic->get_frames_per_period(),mic->get_bytes_per_frame());
        snd_pcm_sframes_t frames = mic->capture_into_array(msg.data);
        if (frames > 0) {
            msg.frames = frames;
            audio_queue->push(std::move(msg));
        }
    }
}

void playback_thread_func(ALSAPlaybackDevice* speaker, AudioQueue* audio_queue) {
    while (1) {
        AudioMessage msg = audio_queue->pop();
        if (!msg.data.empty()) {
            speaker->play_from_array(msg.data,msg.frames);
        }
    }
}

class DelayLine {
public:
    explicit DelayLine(size_t delay)
        : buffer(delay, 0.0), size(delay) {}

    inline double process(double x) {
        double y = buffer[index];
        buffer[index] = x;

        index++;
        if (index == size) index = 0;

        return y;
    }

private:
    std::vector<double> buffer;
    size_t size;
    size_t index = 0;
};



int main() {
    const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_LE;
    ALSACaptureDevice microphone("plughw:5,0", SAMPLING_RATE, MICROPHONE_CHANNELS, FRAMES_PER_PERIOD, FORMAT);
    ALSAPlaybackDevice speaker("default", SAMPLING_RATE, SPEAKER_CHANNELS, FRAMES_PER_PERIOD, FORMAT);

    microphone.open();
    speaker.open();
    
    AudioQueue DSP_receiving_queue(500);
    AudioQueue DSP_transmission_queue(500);

    std::thread capture_thread(capture_thread_func, &microphone, &DSP_receiving_queue);
    std::thread playback_thread(playback_thread_func, &speaker, &DSP_transmission_queue);
    
    Fir1 fir(NTAPS,0.00000);
    fir.setLearningRate(LEARNING_RATE);
    size_t delay_line_length = NTAPS/2;

    DelayLine delay_line(delay_line_length);

    while (1) {
        AudioMessage in_msg = DSP_receiving_queue.pop();
        AudioMessage out_msg(speaker.get_frames_per_period(),speaker.get_bytes_per_frame());
        if (!in_msg.data.empty()) {
            const auto& src = in_msg.data;

            auto* in = reinterpret_cast<const int16_t*>(src.data());
            auto* out = reinterpret_cast<int16_t*>(out_msg.data.data());

            for (unsigned int i = 0; i < in_msg.frames; i++) {
                int16_t left  = in[2 * i];
                int16_t right = in[2 * i + 1];

                int32_t sum32 = static_cast<int32_t>(left) + static_cast<int32_t>(right);
                int32_t diff32 = static_cast<int32_t>(right) - static_cast<int32_t>(left);
                double sum  = static_cast<double>(sum32 >> 1);
                double diff = static_cast<double>(diff32 >> 1);
                
                double delayed_sum = delay_line.process(sum);
                double canceller = fir.filter(diff);
                if(std::abs(canceller)>500){
                    fir.reset();
                    fir.zeroCoeff();
                    out[i]= static_cast<int16_t>(sum)*GAIN;
                    continue;
                }

                double output = delayed_sum - canceller;
                if (output<-32768){
                    output = -32768;
                }
                if(output>32767){
                    output = 32767;
                }

                fir.lms_update(output);
                
                int16_t out16 = static_cast<int16_t>(output);

                out[i]= out16*GAIN;

                
                }
        }
        DSP_transmission_queue.push(std::move(out_msg));
    }

    capture_thread.join();
    playback_thread.join();

    microphone.close();
    speaker.close();
    return 0;
}