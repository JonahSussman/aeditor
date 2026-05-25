# aeditor
A video editor designed to aid in labeling and alphabetizing videos. Compiles on both Windows and Linux.

## Requirements

- C++17 compiler (GCC, Clang)
- CMake 3.16+
- SFML 2.6+
- libVLC 3.x (development headers)
- OpenGL

### Fedora

```bash
sudo dnf install gcc-c++ cmake SFML-devel vlc-devel mesa-libGL-devel
```

### Ubuntu/Debian

```bash
sudo apt install g++ cmake libsfml-dev libvlc-dev libgl1-mesa-dev
```

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Running

```bash
cd build
./aeditor
```

## Usage

Open a video with **Ctrl+O**, enter the filename without extension, and click **Auto-find**. This loads both the video (`.mkv`) and its script (`.csv`) if one exists.

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Play / Pause |
| Left / Right | Seek ±5 seconds |
| Up / Down | Jump to previous / next line |
| S | Seek to specific timestamp |
| V | Toggle video window |
| L | Toggle loop on current line |
| C | Set current line's timestamp to playback position |
| \\ | Edit current line |
| Shift+\\ | Set current line's timestamp without opening editor |
| [ / ] | Adjust line timestamp ±50ms |
| Alt+[ / Alt+] | Adjust line timestamp ±10ms |
| Shift+[ / Shift+] | Adjust next line's timestamp ±50ms |
| Delete | Delete current line |
| Ctrl+O | Open file |
| Ctrl+S | Save CSV |
| Ctrl+N | New line at current timestamp |
| Ctrl+L | Toggle line selector |
| Ctrl+A | Align line (Montreal Forced Aligner) |
| Ctrl+Z | Undo last delete |

## Gallery

**No video loaded**
![](./readme/none_loaded.png)

**Example video loaded**
![](./readme/paf_loaded.png)

## License

This software is released under the MIT License. See the [LICENSE](LICENSE) file for more information.
