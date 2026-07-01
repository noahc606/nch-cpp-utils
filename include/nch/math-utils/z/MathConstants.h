#pragma once

namespace nch {
static constexpr double G = 6.6743e-11; //Gravitational constant. SI: m³/(kg·s²)
static constexpr double e = 2.71828182845904523536;
static constexpr double pi = 3.14159265358979323846;

static constexpr double deg_rad = 180.0/pi; //Degrees per radian (180/pi)
static constexpr double rad_deg = pi/180.0; //Radians per degree (pi/180)
static constexpr double m_Gm = 1e9; //Meters per gigameter.
static constexpr double m_km = 1e3; //Meters per kilometer.
static constexpr double Gm_m = 1e-9; //Gigameters per meter.
static constexpr double Gm_km = 1e-6; //Gigameters per kilometer.
static constexpr double Gm_ly = 9460730472.5808; //Gigameters per light year. Divide a Gm quantity by this to get light years.
static constexpr double G_for_Gm = G/(m_Gm*m_Gm*m_Gm); //Gravitational constant scaled for Gm: Gm³/(kg·s²). 1 Gm³ = 1e27 m³.
}