#pragma once
#include <cstdint>
#include <string>
#include <vector>

class NCH_Color {
public:
    NCH_Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    NCH_Color(uint8_t r, uint8_t g, uint8_t b);
    NCH_Color(uint32_t rgba);
	NCH_Color(std::string p_value);
    NCH_Color();
    ~NCH_Color();

    /**/
    static uint32_t getRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static uint32_t getRGBA(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t getRGB(uint8_t r, uint8_t g, uint8_t b);
    static uint32_t getRGB(uint32_t rgba);
    static uint32_t getA(uint32_t rgba);
    uint32_t getRGBA();
    uint32_t getRGB();
    std::vector<double> getHSV();
    double getHSV2();
	NCH_Color getInterpolColor(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a, double weight);
	NCH_Color getInterpolColor(NCH_Color& c, double weight);

	std::string toStringB10();
	std::string toStringB16(bool transparency);

	/* Color setting & combining */
	//Linear interpolation between two colors
	//Additional blending: formulas based off of those at https://wiki.libsdl.org/SDL2/SDL_BlendMode
	void add(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a);
    void add(NCH_Color& c);
    void blend(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a);
    void blend(NCH_Color& c);
    void mod(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a);
    void mod(NCH_Color& c);
    void brighten(int val);
    void transpare(int dA);
    void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void set(uint8_t r, uint8_t g, uint8_t b);
    void set(uint32_t rgba);
    void setFromB10Str(std::string decimal);
    void setFromB16Str(std::string hexadecimal);
    void setFromHSV(double h, double s, double v);
    void setBrightness(int val);

    NCH_Color& operator=( const NCH_Color& other );

    uint8_t r=0, g=0, b=0, a=255;
private:
};