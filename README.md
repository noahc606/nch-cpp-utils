# nch-cpp-utils
Noah's Utilities for C++, SDL2, & FFmpeg.

This repo contains shared C++ code for some of my other projects. It is a collection of 4 modules (math, cpp, sdl, ffmpeg) - some may or may not be included depending on what is needed.

# Using the Library
Copy or symlink "nch" within this repo's "include" directory into your own project's "include" directory. Make sure your CMake/make/whatever setup globs every .cpp and .h file within your include folder. There's also an example CMakeLists.txt here which contains and builds all the dependencies for this project. You are free to use anything within this repo as a base for your own project.

# Inter-Dependencies
- math-utils: none
- cpp-utils: none
- sdl-utils: Depends on cpp-utils
- ffmpeg-utils: Depends on cpp-utils + sdl-utils