#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace nch { class Color
{
public:
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    Color(uint8_t r, uint8_t g, uint8_t b);
    Color(uint32_t rgba);
	Color(std::string p_value);
    Color();
    ~Color();

    /* Getters */
    static uint32_t getRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static uint32_t getRGBA(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t getRGB(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t getRGB(uint32_t rgba);
    static uint32_t getA(uint32_t rgba);
    uint32_t getRGBA();
    uint32_t getRGB();
    std::vector<double> getHSV();
    std::vector<uint8_t> getRGBTriple();
    double getHSV2();
	Color getInterpolColor(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a, double weight);
	Color getInterpolColor(const Color& c, double weight);

	std::string toStringB10();
	std::string toStringB16(bool transparency);

    bool operator==(const Color& other) {
        return (
            r==other.r &&
            g==other.g &&
            b==other.b &&
            a==other.a
        );
    }
    bool operator!=(const Color& other) {
        return !((*this)==other);
    }

	/* Color setting & combining */
	//Linear interpolation between two colors
	//Additional blending: formulas based off of those at https://wiki.libsdl.org/SDL2/SDL_BlendMode
	void add(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a);
    void add(Color& c);
    void blend(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a);
    void blend(Color& c);
    void mod(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a);
    void mod(Color& c);
    void brighten(int val);
    void saturate(int val);
    void transpare(int dA);
    void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void set(uint8_t r, uint8_t g, uint8_t b);
    void set(uint32_t rgba);
    void setFromB10Str(std::string decimal);
    void setFromB16Str(std::string hexadecimal);
    void setFromHSV(double h, double s, double v);
    void setBrightness(int val);

    Color& operator=( const Color& other );

    uint8_t r=0, g=0, b=0, a=255;
private:
};
}