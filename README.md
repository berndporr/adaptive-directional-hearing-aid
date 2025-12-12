# Work in Progress

Below is the device tree overlay to enable i2s communication on port 2 of the Rock 5B/5B+

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
