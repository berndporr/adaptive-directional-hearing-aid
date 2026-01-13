#include "../include/ALSADevices.h"
#include "../constants.h"
#include "../include/dnf_torch.h"
#include <cstddef>
#include <cstdint>
#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

struct AudioMessage {
    std::vector<char> data;
    snd_pcm_sframes_t frames;

    AudioMessage(snd_pcm_sframes_t frames,size_t bytes_per_frame) : 
        frames(frames) 
        {
            data.resize(static_cast<size_t>(frames) * bytes_per_frame);
        }

};

struct WavHeader {
    char riff[4] = {'R','I','F','F'};
    uint32_t chunkSize = 0;
    char wave[4] = {'W','A','V','E'};

    char fmt[4] = {'f','m','t',' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1; 
    uint32_t sampleRate = 0;
    uint32_t byteRate = 0;
    uint16_t blockAlign = 0;
    uint16_t bitsPerSample = 16;

    char data[4] = {'d','a','t','a'};
    uint32_t dataSize = 0;
};


struct WavFormat {
    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;
};

bool read_wav_format(const std::string& path, WavFormat& fmt) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    char riff[4];
    uint32_t chunkSize;
    char wave[4];

    f.read(riff, 4);
    f.read(reinterpret_cast<char*>(&chunkSize), 4);
    f.read(wave, 4);

    if (std::strncmp(riff, "RIFF", 4) != 0 ||
        std::strncmp(wave, "WAVE", 4) != 0) {
        return false;
    }

    while (f) {
        char id[4];
        uint32_t size;

        f.read(id, 4);
        f.read(reinterpret_cast<char*>(&size), 4);
        if (!f) break;

        if (std::strncmp(id, "fmt ", 4) == 0) {
            f.read(reinterpret_cast<char*>(&fmt.audioFormat), 2);
            f.read(reinterpret_cast<char*>(&fmt.numChannels), 2);
            f.read(reinterpret_cast<char*>(&fmt.sampleRate), 4);

            uint32_t byteRate;
            uint16_t blockAlign;
            f.read(reinterpret_cast<char*>(&byteRate), 4);
            f.read(reinterpret_cast<char*>(&blockAlign), 2);
            f.read(reinterpret_cast<char*>(&fmt.bitsPerSample), 2);

            f.seekg(size - 16, std::ios::cur); // skip any extras
        }
        else if (std::strncmp(id, "data", 4) == 0) {
            fmt.dataOffset = static_cast<uint32_t>(f.tellg());
            fmt.dataSize = size;
            f.seekg(size, std::ios::cur);
        }
        else {
            f.seekg(size, std::ios::cur);
        }
    }

    return true;
}

bool validate_stereo_s16le(const WavFormat& f) {
    return f.audioFormat == 1 &&   
           f.numChannels == 2 &&
           f.bitsPerSample == 16;
}

int16_t filter_stereo_frame_and_convert_to_mono(int16_t left,int16_t right,DNF& dnf){
    int32_t sum32 = static_cast<int32_t>(left) + static_cast<int32_t>(right);
    int32_t diff32 = static_cast<int32_t>(right) - static_cast<int32_t>(left);
    
    float sum  = static_cast<float>(sum32 >> 1);
    float diff = static_cast<float>(diff32 >> 1);
                
                
    float output = dnf.filter(sum,diff);
                
    float y = output;

    if (y >  32767.0f) y =  32767.0f;
    if (y < -32768.0f) y = -32768.0f;
                
    int16_t out16 = static_cast<int16_t>(std::lrintf(y));
    
    return out16;
}

void print_progress(size_t current, size_t total) {
    const int barWidth = 50;

    float progress = static_cast<float>(current) / static_cast<float>(total);
    int pos = static_cast<int>(barWidth * progress);

    std::cout << "\r[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] "
              << std::setw(3) << static_cast<int>(progress * 100.0f)
              << "%";

    std::cout.flush();
}

int main() {
    std::string filename = "../training_audio.wav";
    std::string output_filename = "../result.wav";
    //check file is in desired format
    WavFormat fmt;
    if (!read_wav_format(filename, fmt)) {
        std::cerr << "Invalid WAV file\n";
        return 1;
    }
    if (!validate_stereo_s16le(fmt)) {
        std::cerr << "WAV is NOT stereo 16-bit PCM LE\n";
        return 1;
    }

    //initialise speaker
    const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_LE;
    ALSAPlaybackDevice speaker("default", fmt.sampleRate, SPEAKER_CHANNELS, FRAMES_PER_PERIOD, FORMAT);
    speaker.open();

    //set up neural network for training
    int nTaps = NTAPS;
    int nLayers = NEURAL_NETWORK_LAYERS;

    DNF dnf(nLayers,nTaps);

    dnf.setLearningRate(static_cast<float>(LEARNING_RATE));
    
    //reading data from file
    std::ifstream f(filename, std::ios::binary);
    f.seekg(fmt.dataOffset);
    
    std::vector<int16_t> samples(fmt.dataSize / sizeof(int16_t));
    f.read(reinterpret_cast<char*>(samples.data()), fmt.dataSize);
        
    size_t totalFrames = samples.size() / 2;

    std::queue<AudioMessage> queue;
    std::cout << "reading the file and training/filtering"<<"\n";
    size_t frame_index = 0;

    AudioMessage msg(speaker.get_frames_per_period(),speaker.get_bytes_per_frame());
    auto* out = reinterpret_cast<int16_t*>(msg.data.data());
    size_t message_index =0;
    const size_t totalMessages = ((fmt.dataSize / sizeof(int16_t)) / 2) / 32;
    for (size_t i = 0; i < totalFrames; ++i) {
        int16_t left  = samples[2 * i];
        int16_t right = samples[2 * i + 1];

        out[frame_index] = filter_stereo_frame_and_convert_to_mono(left,right,dnf); 
        
        frame_index++;
        
        if(frame_index==static_cast<size_t>(speaker.get_frames_per_period())){
            msg.frames =speaker.get_frames_per_period() ;
            queue.push(std::move(msg));
            
            
            msg = AudioMessage(speaker.get_frames_per_period(), speaker.get_bytes_per_frame());
            out = reinterpret_cast<int16_t*>(msg.data.data());
            frame_index = 0;
            message_index++;
            
            print_progress(message_index, totalMessages);
            
        }
    }

    //open output wav file for filtered result
    std::ofstream wavOut(output_filename, std::ios::binary);
    if (!wavOut) {
        std::cerr << "Failed to open result.wav\n";
        return 1;
    }

    WavHeader header;
    header.sampleRate   = fmt.sampleRate;
    header.byteRate     = fmt.sampleRate * 1 * sizeof(int16_t);
    header.blockAlign   = 1 * sizeof(int16_t);

    wavOut.write(reinterpret_cast<char*>(&header), sizeof(header));

    uint32_t totalSamplesWritten = 0;

    std::cout << "\n" << "playback and writing to file" << "\n";

    while(queue.size()!=0){
        AudioMessage msg = std::move(queue.front());
        queue.pop();

        if (!msg.data.empty()) {
            auto* out = reinterpret_cast<int16_t*>(msg.data.data());
            
            for (unsigned int i = 0; i < msg.frames; i++) {
                int16_t sum  = out[i];
                
                int32_t mono32 = sum;
                mono32 *= GAIN;
                mono32 = std::clamp(mono32, -32768, 32767);

                int16_t mono  = static_cast<int16_t>(mono32);

                out[i] = mono;
            }
            wavOut.write(reinterpret_cast<const char*>(out), static_cast<std::streamsize>(static_cast<snd_pcm_uframes_t>(msg.frames) * sizeof(int16_t)));

            totalSamplesWritten += static_cast<uint32_t>(msg.frames);
            speaker.play_from_array(msg.data,msg.frames);
            
        }
        
    }

    //Fix the wav header at the end
    header.dataSize  = totalSamplesWritten * sizeof(int16_t);
    header.chunkSize = 36 + header.dataSize;

    wavOut.seekp(0, std::ios::beg);
    wavOut.write(reinterpret_cast<char*>(&header), sizeof(header));

    wavOut.close();
    

    speaker.close();
    return 0;
}