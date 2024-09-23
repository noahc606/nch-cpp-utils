# nch-cpp-utils
Noah's Utilities for C++, SDL2, & FFmpeg.

This repo contains shared C++ code for some of my other projects. Much of the code here originally came from <a href="https://github.com/noahc606/Back-to-Earth">Back to Earth's codebase</a>.

# Using the Library
Copy or symlink "nch" within this repo's "include" directory into your own project's "include" directory. I assume your CMake/make/whatever setup globs every .cpp and .h file within your include folder. If so, simple as that! There's also an example CMakeLists.txt here which contains all the dependencies for this project. You are free to use anything within this repo as a base for your own project.

# Note on ffmpeg stuff
'include/nch/ffmpeg-utils' is experimental and currently contains deprecated/badly written code. You may need to delete this module if the build fails.