#include "ALSADevices.hpp"
#include "constants.h"

ALSACaptureDevice microphone("plughw:5,0", SAMPLING_RATE, 2, FRAMES_PER_PERIOD, FORMAT);
ALSAPlaybackDevice speaker("default", SAMPLING_RATE, 1, FRAMES_PER_PERIOD, FORMAT);



int main() {

    microphone.open();
    speaker.open();
    snd_pcm_sframes_t frames_captured, frames_played;
    
    (void)frames_captured;
    (void)frames_played;

    while (1) {
        frames_captured = microphone.capture_into_buffer();
        speaker.copy_from_capture_mono(microphone);
        frames_played = speaker.play_from_buffer();
    }

    microphone.close();
    speaker.close();
    return 0;
}
