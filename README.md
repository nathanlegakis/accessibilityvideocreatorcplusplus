# accessibilityvideocreatorcplusplus

C++ starter project for an accessibility-focused video creator pipeline.

## Goals (MVP)

- ingest audio/video
- run speech recognition (planned with [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx))
- generate captions/transcripts
- support accessibility-first outputs

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run binary from your build output directory.

## Roadmap

1. Add sherpa-onnx dependency
2. Add audio input + ASR pipeline
3. Generate subtitle files (SRT/VTT)
4. Add CLI options and config
