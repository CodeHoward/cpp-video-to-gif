# C++ Video to GIF

A lightweight local video-to-GIF tool built with C++ and a browser-based UI.

This project runs a small native HTTP server, lets you upload a video in the browser, trim the time range, configure GIF options, and export the result locally.

## Highlights

- Native C++ backend with a built-in HTTP server
- Clean browser UI for upload, preview, trimming, and conversion
- Local workflow with no cloud dependency
- GIF history view for generated results
- Sample video included for quick testing

## Preview

The app focuses on a simple flow:

1. Upload a video file
2. Preview and select the clip range
3. Adjust GIF settings
4. Convert and download the result

If you want, you can later add a screenshot or demo GIF here to make the repository page even stronger.

## Quick Start

### Requirements

- macOS or Linux
- A C++ compiler
- `ffmpeg` available in your environment

### Build

```bash
g++ -std=c++11 -pthread main.cpp -o video-to-gif-server
```

### Run

```bash
./video-to-gif-server
```

Then open:

```text
http://localhost:8080
```

If your build uses a different default port, use the address printed in the terminal when the server starts.

## How It Works

- The C++ server handles HTTP requests, file uploads, conversion requests, and history queries.
- The frontend is served from [`public/index.html`](/Users/howard/Desktop/vedio-editor/cpp-video-to-gif/public/index.html:1).
- Uploaded videos are stored locally in `uploads/`.
- Generated GIF files are stored locally in `output/`.
- Video conversion is performed through local processing logic in the C++ backend.

## Project Structure

```text
cpp-video-to-gif/
├── main.cpp
├── public/
│   └── index.html
├── uploads/
├── output/
├── test_video.mp4
└── .gitignore
```

## Core Features

- Video upload from the browser
- Video preview before conversion
- Clip range selection
- GIF generation
- Conversion history management
- Local file serving for generated GIF outputs

## Tech Stack

- C++
- Native socket-based HTTP server
- HTML / CSS / JavaScript
- Local file storage
- FFmpeg-based video processing workflow

## Notes

- This project is designed for local use.
- Large video files may take longer to upload and convert.
- Output quality and speed depend on your local machine and FFmpeg setup.

## Roadmap

- Add a dedicated demo screenshot or animated preview
- Split frontend assets into separate CSS and JS files
- Add clearer build instructions for Windows and Linux
- Add preset export profiles for different GIF sizes
- Improve error handling and progress feedback

## Repository Description

Suggested GitHub repo description:

```text
A lightweight C++ video-to-GIF converter with a built-in local web UI for upload, trimming, and export.
```
