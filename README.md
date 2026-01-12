# NCH-CPP-Utils
Noah's Utilities for C++, SDL2, FFmpeg, XML, and OpenGL.

This repo contains shared C++ code for some of my other projects. It is a collection of 7 modules (math, cpp, sdl, ffmpeg, xml, json, opengl) - some may or may not be included depending on what is needed.

# NCH Library Modules & Inter-Dependencies
- math-utils: none
- cpp-utils: none
- sdl-utils: Depends on cpp-utils
- ffmpeg-utils: Depends on cpp-utils + sdl-utils
- rmlui-utils (<a href="https://github.com/noahc606/NCH-RmlUi-Utils">separate repo</a>): Depends on cpp-utils + math-utils + sdl-utils
- xml-utils: Depends on cpp-utils
- json-utils: none
- opengl-utils: Depends on cpp-utils + math-utils + sdl-utils

# Using the Library
Copy or symlink "nch" within this repo's "include" directory into your own project's "include" directory. Make sure your CMake/make/whatever setup globs every .cpp and .h file within your include folder. Then, use "#include <nch/...-utils/...>" and "using namespace nch;" within your C++ code.

There's a CMakeLists.txt here which you can use to build the example nch::MainLoopDriver graphical program. You are free to use anything within this repo as a base for your own project (you may need to look at "Manually linked libraries" for more info).

Building with Emscripten is possible (especially with the first 3 modules) but is not fully supported. <a href="https://noahc606.github.io/nch/emscripten/apps/nch-cpp-utils/">Here</a> is a browser-viewable Emscripten demo of the example program.

# Third-party Dependencies
This only applies to the modules after the first 3. Module 'ffmpeg-utils' is dynamically linked to via linker flags - FFmpeg is not included within this repo. Copies of <a href="https://github.com/g-truc/glm">glm</a> and <a href="https://github.com/nlohmann/json">nlohmann-json</a> are included within /include, and both are MIT licensed. They are used by 'opengl-utils' and 'json-utils' respectively. libxml2 is manually linked via CMakeLists.txt from /usr/include/libxml2 if it exists and is used by 'xml-utils'.

If you do not need these, you don't need to include them within your 'include' folder. You can also remove the line that says /usr/include/libxml2 from the CMakeLists.txt if it is not needed. Similarly, you can remove linker flags you don't need within target_link_libraries:
- "-lavutil, ...everything after..." -> FFmpeg, ffmpeg-utils
- "-lxml2" -> libxml2, xml-utils
- "-lGL -lGLU -lGLEW" -> OpenGL, opengl-utils