Before running you need to install libtorch which you will be able to do by following the below:

Libtorch

Intel architectures: Get libtorch from the PyTorch homepage. Create an environment variable CMAKE_PREFIX_PATH=/path/to/libtorch pointing to the libtorch directory.

ARM Debian (Raspberry PI): just do apt install libtorch-dev and you are all set!



Command to record audio to overwrite training_audio file or test microphones are working

arecord -D hw:5,0 -f S16_LE -c 2 -r 44100 training_audio.wav

Below is the device tree overlay to enable i2s communication on port 2 of the Rock 5B/5B+. The way to attach this device tree overlay depends on the operating system you are using, however for Armbian, which is what I used, the steps are shown here https://docs.armbian.com/User-Guide_Armbian_overlays/

```dts
/dts-v1/;
/plugin/;

/ {
    metadata {
        title = "Enable I2S2-M1 2-channel dummy sound card";
        exclusive = "GPIO3_B5", "GPIO3_B6", "GPIO3_B2", "GPIO3_B3", "i2s2_2ch";
    };
};

&{/} {
    i2s2_dummy_codec: i2s2-dummy-codec {
        compatible = "rockchip,dummy-codec";
        #sound-dai-cells = <0>;
    };

    i2s2_dummy_sound: i2s2-dummy-sound {
        #address-cells = <1>;
        #size-cells = <0>;
        compatible = "simple-audio-card";
        simple-audio-card,format = "i2s";
        simple-audio-card,name = "dummy-card";
        simple-audio-card,mclk-fs = <256>;
        status = "okay";

        simple-audio-card,dai-link@0 {
            reg = <0>;
            format = "i2s";
            cpu {
                sound-dai = <&i2s2_2ch>;
            };
            codec {
                sound-dai = <&i2s2_dummy_codec>;
            };
        };
    };
};

&i2s2_2ch {
    pinctrl-0 = <&i2s2m1_lrck &i2s2m1_sclk &i2s2m1_sdi &i2s2m1_sdo>;
    status = "okay";
};

