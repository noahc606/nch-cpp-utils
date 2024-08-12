#include "Color.h"
#include <iostream>
#include <math.h>

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { set(r, g, b, a); }
Color::Color(uint8_t r, uint8_t g, uint8_t b) { set(r, g, b); }
Color::Color(uint32_t rgba) { set(rgba); }
Color::Color(): Color(0, 0, 0){}
Color::~Color(){}
/**
 * H = [0-360)
 * S = [0-100]
 * V = [0-100]
 */
std::vector<double> Color::getHSV()
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

/*
    Brighten the current color by 'val' (added to 'value' in HSV).
	When HSV's value exceeds 100, HSV's saturation is decreased by the remaining amount to increase brightness even more.
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

void Color::transpare(int dA)
{
	int ra = a;
	ra += (-dA);
	if(ra<0) ra = 0;
	a = ra;
}

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