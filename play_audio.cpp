#include "ALSADevices.hpp"
#include "constants.h"
#include <cstdint>
#include <iostream>
#include <signal.h>
#include <atomic>
#include "Fir1.h"

ALSACaptureDevice microphone("plughw:5,0", SAMPLING_RATE, 2, FRAMES_PER_PERIOD, FORMAT);
ALSAPlaybackDevice speaker("default", SAMPLING_RATE, 2, FRAMES_PER_PERIOD, FORMAT);

std::atomic<bool> running(true);

void signal_handler(int signo) {
    running = false;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    microphone.open();
    speaker.open();
    char* buffer = microphone.allocate_buffer();
    unsigned int frames_captured, frames_played;
    
    Fir1 fir(NTAPS);
    fir.setLearningRate(LEARNING_RATE);

    while (running) {
        frames_captured = microphone.capture_into_buffer(buffer, microphone.get_frames_per_period());
        int16_t* samples = reinterpret_cast<int16_t*>(buffer);

        for (unsigned int i = 0; i < frames_captured; i++) {
            int16_t left  = samples[i * 2 + 0];
            int16_t right = samples[i * 2 + 1];
            
            int32_t L = left;
            int32_t R = right;

            // Mid/Mono calculation
            int32_t mono32 = (L + R) / 2;

            // Difference/Side calculation (if needed)
            int32_t diff32 = (L - R) / 2;

            int16_t mono = (int16_t)mono32;

            samples[i * 2 + 0] = mono;  
            samples[i * 2 + 1] = mono;

            left  = samples[i * 2 + 0];
            right = samples[i * 2 + 1];
            printf("Frame %u: L=%d  R=%d\n", i, left, right);

        }
        frames_played = speaker.play_from_buffer(buffer, speaker.get_frames_per_period());
    }

    microphone.close();
    speaker.close();
    return 0;
}