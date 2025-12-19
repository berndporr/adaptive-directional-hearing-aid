#include "ALSADevices.hpp"
#include "constants.h"
#include <cstddef>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>

struct AudioMessage {
    std::vector<char> data;
    snd_pcm_uframes_t frames;

    AudioMessage(snd_pcm_uframes_t frames,size_t bytes_per_frame) : 
        frames(frames) 
        {
            data.resize(frames * bytes_per_frame);
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
        if (queue.size()<MAX_QUEUE_SIZE){        
            {
                std::lock_guard<std::mutex> lock(mtx);
                queue.push(std::move(msg));
            }
            cv.notify_one();
        }
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
        snd_pcm_uframes_t frames = mic->capture_into_array(msg.data);
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


int main() {
    ALSACaptureDevice microphone("plughw:5,0", SAMPLING_RATE, MICROPHONE_CHANNELS, FRAMES_PER_PERIOD, FORMAT);
    ALSAPlaybackDevice speaker("default", SAMPLING_RATE, SPEAKER_CHANNELS, FRAMES_PER_PERIOD, FORMAT);

    microphone.open();
    speaker.open();
    
    AudioQueue audio_queue(1);

    std::thread capture_thread(capture_thread_func, &microphone, &audio_queue);
    std::thread playback_thread(playback_thread_func, &speaker, &audio_queue);
    
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    capture_thread.join();
    playback_thread.join();

    microphone.close();
    speaker.close();
    return 0;
}
