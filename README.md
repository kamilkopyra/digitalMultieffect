# Multieffect

A real-time guitar multi-effect processor built in C++ with PortAudio,
controlled from the keyboard or from physical rotary encoders wired to an
Arduino over serial.

## Features

- Chain of effects — toggle any of them on/off, signal runs through all
  active ones in order
- Control via keyboard or rotary encoders (Arduino over UART)
- One effect at a time is "focused" for editing; its 3 parameters are
  adjusted with the encoders (or keyboard fallback for testing on PC)
- Tap tempo for modulation effects
- Tuner (pitch detection, McLeod method)
- Recording to WAV

## Effects

| Effect | Category |
|---|---|
| Fuzz | distortion |
| Rat | distortion |
| TubeScreamer | distortion |
| Boost | clean gain + tone |
| Compressor | dynamics |
| NoiseGate | dynamics |
| Delay | time-based |
| Tremolo | modulation (LFO on volume) |
| Vibrato | modulation (LFO on pitch) |
| Flanger | modulation |
| Phaser | modulation |
| AutoWah | envelope-driven filter |
| Equalizer | static filter (3-band, biquad) |
| Tuner | pitch detection |

## Controls

| Key | Action |
|---|---|
| `1`-`8`, `z x c v b n` | toggle an effect in/out of the chain |
| `0` | clear the chain |
| `Tab` | switch which effect's parameters are being edited |
| `a s` / `d f` / `g h` | adjust parameter 1 / 2 / 3 of the focused effect |
| `t` | tap tempo (focused modulation effect) |
| `r` | start/stop recording to WAV |
| `q` | quit |

The same actions are available from Arduino rotary encoders sending
`E <index> <dir>` (turn) and `B <index>` (button press) lines over serial.

## Arduino

`arduino/Encoders/Encoders.ino` reads 3 rotary encoders (with push-button)
and forwards turns/presses to the PC over serial at 9600 baud, in the
`E <index> <dir>` / `B <index>` format the app expects. Needs the
[Encoder](https://www.pjrc.com/teensy/td_libs_Encoder.html) library.

Wiring: encoder 1 on pins 2/3 (button on 4), encoder 2 on pins 5/6 (button
on 7), encoder 3 on pins 8/9 (button on 10); button pins use `INPUT_PULLUP`.

## Building

Windows, Visual Studio (`Multieffect.slnx` / `.vcxproj`).

Requirements:
- [PortAudio](http://www.portaudio.com/)
- [Boost.Asio](https://www.boost.org/) (serial communication with the Arduino)

Open the solution in Visual Studio, point the include/library paths at
your PortAudio and Boost installs, and build.

## GUI version

A Qt Quick desktop GUI (effect chain slots, sliders, tuner, oscilloscope,
presets, WAV recording) lives on the `qt-gui` branch.
