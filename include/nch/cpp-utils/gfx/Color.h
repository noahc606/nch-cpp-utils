#pragma once
#include <cstdint>
#include <vector>

class Color {
public:
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    Color(uint8_t r, uint8_t g, uint8_t b);
    Color(uint32_t rgba);
    Color();
    ~Color();

    std::vector<double> getHSV();

    void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void set(uint8_t r, uint8_t g, uint8_t b);
    void set(uint32_t rgba);
    void brighten(int val);
    void transpare(int dA);
    void setFromHSV(double h, double s, double v);
    void setBrightness(int val);

    uint8_t r=0, g=0, b=0, a=255;
private:
};