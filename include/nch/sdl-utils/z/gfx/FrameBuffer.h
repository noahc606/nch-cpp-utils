#pragma once
#include <cstdint>

namespace nch { class FrameBuffer {
public:
    FrameBuffer();
    ~FrameBuffer();
    void build(int width, int height);
    void free();

    double getPixelDepth(int x, int y);
    uint32_t getPixelColor(int x, int y);
    void* getPixelColorData();
    int getPitch();

    void setPixel(int x, int y, uint32_t color, double depth);
    void setPixel(int x, int y, uint32_t color);

private:
    int width = 0, height = 0;  /* Value(s) representing the dimension(s) (size) of each of the frame buffer arrays. */
    
    double* depths = nullptr;   /* Decimal "depth" values which exist for every pixel. Useful for 3D graphics. */
    uint32_t* colors = nullptr; /* Colors are in a 32-bit unsigned-int RGBA format (e.g. value 0xFF0000FF -> R=255 , G=0, B=0, A=255). */
}; }