/*
ÆµÆ×²ÎÊý¼ÆËã
*/
#pragma once
#include <cmath>

class Spectrum
{
public:
	Spectrum(double windSpeed);
	
	double maxZeta() const;

	double minZeta() const;

	double operator()(double zeta)const;


private:

	double m_windSpeed = 1;

};
