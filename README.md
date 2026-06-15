# C++ Video to GIF

中文 | [English](#english)

一个基于 C++ 构建的本地视频转 GIF 工具，带有浏览器端可视化界面。

这个项目运行一个轻量级原生 HTTP 服务，让你可以在浏览器里上传视频、裁剪时间范围、调整 GIF 参数，并在本地导出结果。

## 项目亮点

- 原生 C++ 后端，内置 HTTP 服务器
- 浏览器端可视化界面，支持上传、预览、裁剪和转换
- 本地运行，无需云服务依赖
- 支持查看 GIF 生成历史
- 自带测试视频，方便快速体验

## 使用流程

这个工具的核心流程很直接：

1. 上传视频文件
2. 预览视频并选择时间范围
3. 调整 GIF 输出参数
4. 开始转换并下载结果

如果你愿意，后面还可以继续补一张截图或演示 GIF，让仓库首页更有展示感。

## 快速开始

### 环境要求

- macOS 或 Linux
- 可用的 C++ 编译器
- 环境中已安装 `ffmpeg`

### 编译

```bash
g++ -std=c++11 -pthread main.cpp -o video-to-gif-server
```

### 运行

```bash
./video-to-gif-server
```

然后在浏览器中打开：

```text
http://localhost:8080
```

如果你的程序使用了其他端口，请以终端启动时打印出来的地址为准。

## 工作原理

- C++ 服务端负责处理 HTTP 请求、文件上传、转换请求和历史记录查询。
- 前端页面由 [public/index.html](/Users/howard/Desktop/vedio-editor/cpp-video-to-gif/public/index.html:1) 提供。
- 上传的视频会保存在本地 `uploads/` 目录。
- 生成的 GIF 会保存在本地 `output/` 目录。
- 视频转换逻辑由本地 C++ 后端驱动，并结合 FFmpeg 工作流完成。

## 项目结构

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

## 核心功能

- 浏览器上传视频
- 视频预览
- 片段时间范围选择
- GIF 生成
- 转换历史管理
- 本地 GIF 文件访问与预览

## 技术栈

- C++
- 基于 socket 的原生 HTTP 服务
- HTML / CSS / JavaScript
- 本地文件存储
- 基于 FFmpeg 的视频处理流程

## 说明

- 这个项目主要面向本地使用场景。
- 视频文件越大，上传和转换时间通常越长。
- 最终输出质量和转换速度取决于本机性能与 FFmpeg 环境。

## 后续计划

- 补充项目截图或演示动图
- 将前端样式和脚本拆分为独立文件
- 增加更清晰的 Windows / Linux 运行说明
- 提供更多 GIF 输出预设
- 改进错误处理和进度反馈

## 仓库简介建议

建议填写为：

```text
A lightweight C++ video-to-GIF converter with a built-in local web UI for upload, trimming, and export.
```

## English

A lightweight local video-to-GIF tool built with C++ and a browser-based UI.

This project runs a small native HTTP server, lets you upload a video in the browser, trim the time range, configure GIF options, and export the result locally.

## Highlights

- Native C++ backend with a built-in HTTP server
- Clean browser UI for upload, preview, trimming, and conversion
- Local workflow with no cloud dependency
- GIF history view for generated results
- Sample video included for quick testing

## Workflow

The app is built around a simple flow:

1. Upload a video file
2. Preview and select the clip range
3. Adjust GIF settings
4. Convert and download the result

You can later add a screenshot or demo GIF here to make the repository page even stronger.

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
- The frontend is served from [public/index.html](/Users/howard/Desktop/vedio-editor/cpp-video-to-gif/public/index.html:1).
- Uploaded videos are stored locally in `uploads/`.
- Generated GIF files are stored locally in `output/`.
- Video conversion is performed through the local C++ backend with an FFmpeg-based workflow.

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
