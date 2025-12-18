#include "ALSADevices.hpp"
#include "constants.h"
#include "Fir1.h"
#include <signal.h>
#include <atomic>

ALSACaptureDevice microphone("plughw:5,0", SAMPLING_RATE, 2, FRAMES_PER_PERIOD, FORMAT);
ALSAPlaybackDevice speaker("default", SAMPLING_RATE, 1, FRAMES_PER_PERIOD, FORMAT);

std::atomic<bool> running(true);

void signal_handler(int signo) {
    running = false;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    microphone.open();
    speaker.open();
    unsigned int frames_captured, frames_played;
    
    while (running) {
        frames_captured = microphone.capture_into_buffer();
        speaker.copy_from_capture_mono(microphone);
        frames_played = speaker.play_from_buffer();
    }

    microphone.close();
    speaker.close();
    return 0;
}
