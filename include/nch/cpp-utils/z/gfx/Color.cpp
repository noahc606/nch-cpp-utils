#include "Color.h"
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>

using namespace nch;

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { set(r, g, b, a); }
Color::Color(uint8_t r, uint8_t g, uint8_t b) { set(r, g, b); }
Color::Color(uint32_t rgba) { set(rgba); }
Color::Color(std::string p_value)
{
	try {
		uint32_t num = std::stoul(p_value);
		set(num);
	} catch(...) {
		//Log::error(__PRETTY_FUNCTION__, "Failed to parse string as a color", "setting RGBA=(0,0,0,0)");
		set(0);
	}
}
Color::Color(): Color(0, 0, 0){}
Color::~Color(){}

uint32_t Color::getRGBA(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a) { return 16777216*p_r+65536*p_g+256*p_b+p_a; }
uint32_t Color::getRGBA(uint8_t p_r, uint8_t p_g, uint8_t p_b) { return getRGBA(p_r, p_g, p_b, 255); }
uint32_t Color::getRGBA() const { return getRGBA(r, g, b, a); }
uint32_t Color::getRGB(uint8_t p_r, uint8_t p_g, uint8_t p_b) { return 65536*p_r+256*p_g+p_b; }
uint32_t Color::getRGB(uint32_t p_rgba) { return p_rgba>>8; }
uint32_t Color::getRGB() const { return getRGB(r, g, b); }
uint32_t Color::getA(uint32_t p_rgba) { return p_rgba&0xFF; }

/**
 * H = [0-360)
 * S = [0-100]
 * V = [0-100]
 */
std::vector<double> Color::getHSV() const
{
	double rp = ((double)r)/255.0;
	double gp = ((double)g)/255.0;
	double bp = ((double)b)/255.0;
	double cmax = std::max(rp, std::max(gp, bp));
	double delta = cmax-std::min(rp, std::min(gp, bp));
	
	//Hue
	double h;
	if(cmax==rp) {
		h = 60.0*((gp-bp)/delta+0.0);
	} else if(cmax==gp) {
		h = 60.0*((bp-rp)/delta+2.0);
	} else {
		h = 60.0*((rp-gp)/delta+4.0);
	}
	if( h<0.0 ) {
		h += 360.0;
	}
	
	//Saturation
	double s;
	if(cmax==0) {
		s = 0;
		//H is Nan when max is zero
	} else {
		s = 100.0*delta/cmax;
	}
	
	//Value
	double v = 100.0*cmax;
	
	std::vector<double> res;
	if(std::isnan(h)) { h = 0; }
	res.push_back(h); res.push_back(s); res.push_back(v);
	return res;
}

std::vector<uint8_t> Color::getRGBTriple() const
{
	std::vector<uint8_t> res;
	res.push_back(r);
	res.push_back(g);
	res.push_back(b);
	return res;
}

/**
 * Get the V in the HSV value
*/
double Color::getHSV2() const
{
	double rp = ((double)r)/255.0;
	double gp = ((double)g)/255.0;
	double bp = ((double)b)/255.0;
	double cmax = std::max(rp, std::max(gp, bp));
	
	//Value
	double v = 100.0*cmax;
	return v;
}

/**
 * Return the 32bit RGBA value, interpreted as a base 10 number, as a string.
 */
std::string Color::toStringB10() const
{
	std::stringstream ss; ss << getRGBA();
	return ss.str();
}

std::string Color::toStringB16(bool transparency) const
{
	std::stringstream ss;
	ss << "#";
	ss << std::setfill('0') << std::setw(2) << std::hex << (int)r;
	ss << std::setfill('0') << std::setw(2) << std::hex << (int)g;
	ss << std::setfill('0') << std::setw(2) << std::hex << (int)b;
	if(transparency) {
		ss << std::hex << (int)a;
	}

	std::string ssStr = ss.str();
	std::stringstream res;
	for(int i = 0; i<ssStr.size(); i++) {
		res << (char)std::toupper(ssStr[i]);
	}
	return res.str();
}

std::string Color::toStringReadable(bool transparency) const
{
	std::stringstream ss;
	ss << "(" << (int)r << ", " << (int)g << ", " << (int)b;
	if(transparency) {
		ss << ", " << (int) a;
	}
	ss << ")";
	return ss.str();
}

/*
 * Return a new color that is the weighted average between two colors. 'weight' is a value from 0.0 to 1.0.
 * 
 * A weight close to 0.0 would return a color "closer to" this object.
 * A weight close to 1.0 would return a color "closer to" the specified color (within the parameters).
 */ 
Color Color::getInterpolColor(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a, double weight) const
{
	uint8_t r1 = r; 	uint8_t g1 = g; 	uint8_t b1 = b;		uint8_t a1 = a;
	uint8_t r2 = p_r; 	uint8_t g2 = p_g; 	uint8_t b2 = p_b;	uint8_t a2 = p_a;
	
	double dR = ((double)(r1-r2))*weight;
	double dG = ((double)(g1-g2))*weight;
	double dB = ((double)(b1-b2))*weight;
	double dA = ((double)(a1-a2))*weight;
	
	Color res;
	
	return Color(r1-dR, g1-dG, b1-dB, a1-dA);
}
Color Color::getInterpolColor(const Color& c, double weight) const
{
	return getInterpolColor(c.r, c.g, c.b, c.a, weight);
}

/**
    Additive color blending.
    Destination color (current) is mixed with source color (parameters).
*/
void Color::add(uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa)
{
	r = sr*sa+r;
	g = sg*sa+g;
	b = sb*sa+b;
}
void Color::add(Color& c) { add(c.r, c.g, c.b, c.a); }

void Color::blend(uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa)
{
    r = sr*sa/255+r*(255-a)/255;
    g = sg*sa/255+g*(255-a)/255;
    b = sb*sa/255+b*(255-a)/255;
    a = sa+       a*(255-a)/255;
}
void Color::blend(Color& c) { blend(c.r, c.g, c.b, c.a); }

void Color::mod(uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa)
{
    r = sr*r/255;
    g = sg*g/255;
    b = sb*b/255;
    a = sa*a/255;
}
void Color::mod(Color& c) { mod(c.r, c.g, c.b, c.a); }

/*
    Brighten the current color by 'val' (added to 'value' in HSV).
	When HSV's 'value' exceeds 100, HSV's 'saturation' is decreased by the remaining amount to increase visual brightness even more.
    Use negative 'val' for darkening.
*/
void Color::brighten(int val)
{
    auto hsv = getHSV();
    
	hsv[2] += val;
	if(hsv[2]<=0) { hsv[2] = 0; }
	if(hsv[2]>=100.) {
		double remaining = hsv[2]-100.;
		hsv[2] = 100;

		hsv[1] -= remaining;
		if(hsv[1]<0.) {
			hsv[1] = 0.;
		}
	}

	setFromHSV(hsv[0], hsv[1], hsv[2]);
}
void Color::saturate(int val)
{
    auto hsv = getHSV();
    
	hsv[1] += val;
	if(hsv[1]<=0) { hsv[1] = 0; }
	if(hsv[1]>=100.) {
		hsv[1] = 100;
	}

	setFromHSV(hsv[0], hsv[1], hsv[2]);
}

void Color::transpare(int dA)
{
	int ra = a;
	ra += (-dA);
	if(ra<0) ra = 0;
	a = ra;
}

void Color::set(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    Color::r = r; Color::g = g; Color::b = b; Color::a = a;
}
void Color::set(uint8_t r, uint8_t g, uint8_t b) { set(r, g, b, 255); }
void Color::set(uint32_t rgba)
{
    r = (rgba>>24)&0xFF;
    g = (rgba>>16)&0xFF;
    b = (rgba>> 8)&0xFF;
    a = (rgba>> 0)&0xFF;
}

/**
	Take in a uint32_t number represented as a string, and set the rgba to that number.
*/
void Color::setFromB10Str(std::string decimal)
{
	uint32_t rgba = std::stoul(decimal);
	set(rgba);
}

void Color::setFromB16Str(std::string hexadecimal)
{
	//Sanitize input
	std::string allowedChars = "0123456789ABCDEF";
	std::stringstream ss;
	for(int i = 0; i<hexadecimal.size(); i++) {
		if( allowedChars.find(hexadecimal[i])!=-1 ) {
			ss << hexadecimal[i];
		}
	}
	std::string sanitizedHex = ss.str();
	
	//Convert hex string to uint32_t RGBA
	std::istringstream converter(sanitizedHex);
	uint32_t rgba;
	converter >> std::hex >> rgba;

	//Set RGBA
	set(rgba);
}

/**
   Given 3 values H:[0,360); S:[0,100]; V:[0:100]: Set this color from that HSV triple.
   Transparency is preserved.
*/
void Color::setFromHSV(double h, double s, double v)
{
	
    if(h>360.001 || h<0) { std::printf("Hue should be within [0, 360] (currently %f)\n", h); }
    if(s>100.001 || s<0.) { std::printf("Saturation should be within [0, 100] (currently %f)\n", s); }
    if(v>100.001 || v<0) { std::printf( "Value should be within [0, 100] (currently %f)\n", v); }

	if(h==360) h = 0;

    s = s/100.0;
    v = v/100.0;
    float c = s*v;
    float x = c*(1-std::abs(std::fmod(h/60.0, 2)-1));
    float m = v-c;
    float sr, sg, sb;

    if(h>=0 && h<60)            { sr = c; sg = x; sb = 0; }
    else if(h>=60 && h<120)     { sr = x; sg = c; sb = 0; }
    else if(h>=120 && h<180)    { sr = 0; sg = c; sb = x; }
    else if(h>=180 && h<240)    { sr = 0; sg = x; sb = c; }
    else if(h>=240 && h<300)    { sr = x; sg = 0; sb = c; }
    else                        { sr = c; sg = 0; sb = x; }

    uint8_t r = std::round((sr+m)*255.0);
    uint8_t g = std::round((sg+m)*255.0);
    uint8_t b = std::round((sb+m)*255.0);


    set(r, g, b, a);
}

void Color::setBrightness(int val)
{
	double curr = getHSV()[2];

	if(val<0) val = 0;
	if(val>100) val = 100;
	brighten(val-curr);
}

Color& Color::operator=(const Color& other)
{
    set(other.r, other.g, other.b, other.a);
    return *this;
}