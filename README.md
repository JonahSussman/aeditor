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

## Gallery

**No video loaded**
![](./readme/none_loaded.png)

**Example video loaded**
![](./readme/paf_loaded.png)

## License

This software is released under the MIT License. See the [LICENSE](LICENSE) file for more information.
