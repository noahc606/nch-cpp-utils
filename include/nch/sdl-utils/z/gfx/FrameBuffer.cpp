#include "FrameBuffer.h"

using namespace nch;

FrameBuffer::FrameBuffer(){}
FrameBuffer::~FrameBuffer() { free(); }

/*
    Build a new frame buffer with the given width and height. All color and depth values are initialized to 0
    Also safely cleans up the previous frame buffer color and depth arrays.
    Not called upon object construction.
*/
void FrameBuffer::build(int width, int height)
{
    free();

    if(depths==nullptr) { depths = new double[width*height](); }
    if(colors==nullptr) { colors = new uint32_t[width*height](); }

    FrameBuffer::width = width;
    FrameBuffer::height = height;
}

/*
    Clean up the frame buffer's color and depth arrays.
    Called upon object destruction.
*/
void FrameBuffer::free()
{
    if(depths!=nullptr) {
        delete[] depths;
        depths = nullptr;
    }

    if(colors!=nullptr) {
        delete[] colors;
        colors = nullptr;
    }
}

/* Get the depth value of a pixel */
double FrameBuffer::getPixelDepth(int x, int y) { return depths[y*width+x]; }
/* Get the color value of a pixel */
uint32_t FrameBuffer::getPixelColor(int x, int y) { return colors[y*width+x]; }
/* Return pixel color data for use in SDL_UpdateTexture([...], [...], const void* pixels, [...]). */
void* FrameBuffer::getPixelColorData() { return colors; }
/* Return pitch value for use in SDL_UpdateTexture([...], [...], [...], int pitch). */
int FrameBuffer::getPitch() { return (int)width*sizeof(uint32_t); }

/* Set a pixel's color AND depth. */
void FrameBuffer::setPixel(int x, int y, uint32_t color, double depth)
{
    colors[y*width+x] = color;
    depths[y*width+x] = depth;
}
/* Set a pixel's color. */
void FrameBuffer::setPixel(int x, int y, uint32_t color)
{
    colors[y*width+x] = color;
}