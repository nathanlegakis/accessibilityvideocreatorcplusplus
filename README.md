# accessibilityvideocreatorcplusplus

Windows 11 C++ text-to-speech (TTS) starter app using Microsoft SAPI.

## Features

- Speak text out loud
- Save speech to WAV file
- Configure speaking rate and volume

## Requirements

- Windows 11
- CMake 3.20+
- Visual Studio Build Tools / MSVC

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Usage

```bash
# Speak text using default system voice
.\\build\\Release\\accessibility_video_creator.exe --text "Hello from Windows 11"

# Save output to WAV
.\\build\\Release\\accessibility_video_creator.exe --text "This is saved to file" --out speech.wav

# Set rate and volume
.\\build\\Release\\accessibility_video_creator.exe --text "Custom speech" --rate 1 --volume 90
```

### Arguments

- `--text` (required): text to speak
- `--out` (optional): path to output WAV file
- `--rate` (optional): range `-10..10` (default `0`)
- `--volume` (optional): range `0..100` (default `100`)

## Next Steps

- Add voice selection by name
- Add batch mode from text files
- Add subtitle generation (SRT/VTT)
- Integrate `sherpa-onnx` for speech and accessibility pipelines
