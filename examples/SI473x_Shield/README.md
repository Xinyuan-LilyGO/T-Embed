# ESP32 OLED_ALL_IN_ONE sketch from PU2CLR ported to the Lilygo T-Display S3.

This is a porting of [ESP32 OLED_ALL_IN_ONE](https://github.com/pu2clr/SI4735/tree/master/examples/SI47XX_06_ESP32/OLED_ALL_IN_ONE) sketch from [PU2CLR (Ricardo Caratti)](https://github.com/pu2clr/SI4735) using the clean and beatiful [Volos interface](https://github.com/VolosR/TEmbedFMRadio).

I am using a [Lilygo T-Display S3](https://github.com/Xinyuan-LilyGO/T-Display-S3), which is an ESP32S3 board with a 1.9 inch display with a 8-Bit Parallel Interface, but I believe it will also run on [Lilygo T-Embed](https://github.com/Xinyuan-LilyGO/T-Embed) with some minor changes.

# Special instructions for the TFT_eSPI Library

It is extremely important to follow all the steps indicated on Lilygo github:

[Lilygo T-Display S3] (https://github.com/Xinyuan-LilyGO/T-Display-S3#quick-start)

[Lilygo T-Embed] (https://github.com/Xinyuan-LilyGO/T-Embed#quick-start)


# Releases

## [V0.1 - Initial Release]

First release.

    This release implements different behaviors for its single button.
        
        - Rotary encoder:   Normal mode: Change Frequency. Menu Mode: Select and change options
        - Single press:     Change band. Another single press to return to the "frequency mode"
        - Double press:     Menu mode. Turn the rotary encoder to select the desired option and press the button again to select it.

# Component Parts

* Si4732
* [Lilygo T-Display S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)
* Rotary encoder with switch
* External Speaker (I use an old external Spearker)
* External Battery Pack
* Resistors/Capacitors/Wires

# Schematics:

* Si4735
Not avaliable yet

* Si4732
![Si4732](../extras/schematics/schematic_lilygo_tdisplay_s3_Si4732.png)

# Videos:

* https://youtu.be/cxb8L1xAmmo

# Pictures:

## Version V0.1

![Si4732](../extras/images/All_in_One_Lilygo_T-Display_front_view.jpeg)

# References

1. [PU2CLR SI4735 Library for Arduino](https://github.com/pu2clr/SI4735)

2. [Volos Projects](https://github.com/VolosR/TEmbedFMRadio)

3. [Lilygo T-Display S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)

4. [Lilygo T-Embed](https://github.com/Xinyuan-LilyGO/T-Embed)

# DISCLAMER

ATTENTION: The author of this project does not guarantee that procedures shown here will work in your development environment.
Given this, it is at your own risk to continue with the procedures suggested here.
