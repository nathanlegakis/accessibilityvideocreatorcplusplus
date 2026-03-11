# accessibilityvideocreatorcplusplus

Windows 11 C++ text-to-speech (TTS) app using Microsoft SAPI + SQLite.

## Features

- Speak ad-hoc text
- Save speech to WAV
- Store `.txt` files in a local SQLite database
- Read pending items aloud and mark them as spoken

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

### 1) Speak direct text

```bash
.\build\Release\accessibility_video_creator.exe --text "Hello from Windows 11"
.\build\Release\accessibility_video_creator.exe --text "Save this" --out speech.wav --rate 1 --volume 90
```

### 2) Import text files into DB

```bash
.\build\Release\accessibility_video_creator.exe --db queue.db --import .\texts
```

Imports all `.txt` files recursively from the folder.

### 3) List pending items

```bash
.\build\Release\accessibility_video_creator.exe --db queue.db --list
```

### 4) Speak next pending item

```bash
.\build\Release\accessibility_video_creator.exe --db queue.db --speak-next
.\build\Release\accessibility_video_creator.exe --db queue.db --speak-next --out next.wav
```

When spoken successfully, status changes from `pending` to `spoken`.

## CLI Args

- `--text` text to speak directly
- `--out` optional output WAV path
- `--rate` `-10..10` (default `0`)
- `--volume` `0..100` (default `100`)
- `--db` SQLite DB file path
- `--import` folder containing `.txt` files
- `--list` list pending records
- `--speak-next` read the next pending record aloud
