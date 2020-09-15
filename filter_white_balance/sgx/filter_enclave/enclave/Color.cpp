﻿#include "ImageProcessing.h"
#include "TestEnclave.h"

using namespace std;

float Color::min(float a, float b) { return (b < a) ? b : a; }
float Color::max(float a, float b) { return (b > a) ? b : a; }

#pragma region Constructors
Color::Color(unsigned char a, unsigned char b, unsigned char c)
{
	this->data[0] = a;
	this->data[1] = b;
	this->data[2] = c;
}

Color::Color() 
{
	memset(&this->data, 0, sizeof(this->data));
}

#pragma endregion


#pragma region Conversions

/*Convert from RGB(red,green,blue) to HSV(hue,saturation,value)*/
float* Color::RGBtoHSV() {

	// float HSV[3];
	float *HSV = new float[3]{ 0,0,0 };

	float fRed = ((float)(this->data[0])) / 255.0;
	float fGreen = ((float)(this->data[1])) / 255.0;
	float fBlue = ((float)(this->data[2])) / 255.0;

	float fHue = 0;
	float fSaturation = 0;
	float fValue = 0;

	float fCMax = max(max(fRed, fGreen), fBlue);
	float fCMin = min(min(fRed, fGreen), fBlue);
	float fDelta = fCMax - fCMin;

	if (fDelta > 0) {
		if (fCMax == fRed) {
			fHue = 60.0 * (fmod(((fGreen - fBlue) / fDelta), 6.0));
		}
		else if (fCMax == fGreen) {
			fHue = 60.0 * (((fBlue - fRed) / fDelta) + 2.0);
		}
		else if (fCMax == fBlue) {
			fHue = 60.0 * (((fRed - fGreen) / fDelta) + 4.0);
		}

		if (fCMax > 0) {
			fSaturation = fDelta / fCMax;
		}
		else {
			fSaturation = 0;
		}

		fValue = fCMax;
	}
	else {
		fHue = 0;
		fSaturation = 0;
		fValue = fCMax;
	}

	if (fHue < 0) {
		fHue = 360.0 + fHue;
	}

	HSV[0] = fHue;
	HSV[1] = fSaturation;
	HSV[2] = fValue;

	// printf("The HSV we get are: %f, %f, %f\n", HSV[0], HSV[1], HSV[2]);
	
	return HSV;
}

/*Convert from HSV(hue,saturation,value) to RGB(red,green,blue)*/
float* Color::HSVtoRGB(float* HSV) {

	//float RGB[3];
	float *RGB = new float[3]{ 0,0,0 };

	float fRed = 0;
	float fGreen = 0;
	float fBlue = 0;
	float fHue = HSV[0];
	float fSaturation = HSV[1];
	float fValue = HSV[2];

	float fC = fValue * fSaturation;
	float fHPrime = fmod(fHue / 60.0, 6);
	float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
	float fM = fValue - fC;

	if (0 <= fHPrime && fHPrime < 1) {
		fRed = fC;
		fGreen = fX;
		fBlue = 0;
	}
	else if (1 <= fHPrime && fHPrime < 2) {
		fRed = fX;
		fGreen = fC;
		fBlue = 0;
	}
	else if (2 <= fHPrime && fHPrime < 3) {
		fRed = 0;
		fGreen = fC;
		fBlue = fX;
	}
	else if (3 <= fHPrime && fHPrime < 4) {
		fRed = 0;
		fGreen = fX;
		fBlue = fC;
	}
	else if (4 <= fHPrime && fHPrime < 5) {
		fRed = fX;
		fGreen = 0;
		fBlue = fC;
	}
	else if (5 <= fHPrime && fHPrime < 6) {
		fRed = fC;
		fGreen = 0;
		fBlue = fX;
	}
	else {
		fRed = 0;
		fGreen = 0;
		fBlue = 0;
	}

	fRed += fM;
	fGreen += fM;
	fBlue += fM;

	RGB[0] = fRed * 255.0;
	RGB[1] = fGreen * 255.0;
	RGB[2] = fBlue * 255.0;
	
	// printf("The RGB wer get is: %f, %f, %f\n", RGB[0], RGB[1], RGB[2]);

	return RGB;
}

/*Convert from RGB to XYZ Color Space*/
/*
void Color::RGBtoXYZ()
{
	double red = this->data[0] / 255.0;
	double green = this->data[1] / 255.0;
	double blue = this->data[2] / 255.0;

	red = ((red > 0.04045) ? pow((red + 0.055) / 1.055, 2.4) : (red / 12.92)) * 100.0;
	green = ((green > 0.04045) ? pow((green + 0.055) / 1.055, 2.4) : (green / 12.92)) * 100.0;
	blue = ((blue > 0.04045) ? pow((blue + 0.055) / 1.055, 2.4) : (blue / 12.92)) * 100.0;

	this->data[0] = red * 0.4124564 + green * 0.3575761 + blue * 0.1804375;
	this->data[1] = red * 0.2126729 + green * 0.7151522 + blue * 0.0721750;
	this->data[2] = red * 0.0193339 + green * 0.1191920 + blue * 0.9503041;
}
*/

/*Convert from XYZ to RGB Color Space*/
/*
void Color::XYZtoRGB()
{
	double x = this->data[0] / 100.0;
	double y = this->data[1] / 100.0;
	double z = this->data[2] / 100.0;

	double red = x * 3.2404542 + y * -1.5371385 + z * -0.4985314;
	double green = x * -0.9692660 + y * 1.8760108 + z * 0.0415560;
	double blue = x * 0.0556434 + y * -0.2040259 + z * 1.0572252;

	red = ((red > 0.0031308) ? (1.055*pow(red, 1 / 2.4) - 0.055) : (12.92*red)) * 255.0;
	green = ((green > 0.0031308) ? (1.055*pow(green, 1 / 2.4) - 0.055) : (12.92*green)) * 255.0;
	blue = ((blue > 0.0031308) ? (1.055*pow(blue,1 / 2.4) - 0.055) : (12.92*blue)) * 255.0;

	this->data[0] = red;
	this->data[1] = green;
	this->data[2] = blue;
}
*/
/*Outputs color temperature*/
/*
float Color::getTemperature()
{
	
	if (this->ColorSpace != ColorType::XYZ)
		this->SwitchTo(ColorType::XYZ);

	float x = this->data[0] / (this->data[0] + this->data[1] + this->data[2]);
	float y = this->data[1] / (this->data[0] + this->data[1] + this->data[2]);


	float xe = 0.3366;
	float ye = 0.1735;
	float a0 = -949.86315;
	float a1 = 6253.80338;
	float t1 = 0.92159;
	float a2 = 28.70599;
	float t2 = 0.20039;
	float a3 = 0.00004;
	float t3 = 0.07125;
	float n = (x - xe) / (y - ye);
	
	float temperature_CCT = a0 + (a1*_CMATH_::exp(-n / t1)) + (a2*_CMATH_::exp(-n / t2)) + (a3*_CMATH_::exp(-n / t2));

	return temperature_CCT;
}
*/
/*Easly switch between color spaces*/
/*
void Color::SwitchTo(ColorType targetColorSpace)
{
	if (this->ColorSpace == ColorType::RGB && targetColorSpace == ColorType::XYZ)
	{
		this->RGBtoXYZ();
		this->ColorSpace = ColorType::XYZ;
	}
	else if (this->ColorSpace == ColorType::RGB && targetColorSpace == ColorType::HSV)
	{
		this->RGBtoHSV();
		this->ColorSpace = ColorType::HSV;
	}
	else if (this->ColorSpace == ColorType::XYZ && targetColorSpace == ColorType::RGB)
	{
		this->XYZtoRGB();
		this->ColorSpace = ColorType::RGB;
	}
	else if (this->ColorSpace == ColorType::XYZ && targetColorSpace == ColorType::HSV)
	{
		this->XYZtoRGB();
		this->RGBtoHSV();
		this->ColorSpace = ColorType::HSV;
	}
	else if (this->ColorSpace == ColorType::HSV && targetColorSpace == ColorType::RGB)
	{
		this->HSVtoRGB();
		this->ColorSpace = ColorType::RGB;
	}
	else if (this->ColorSpace == ColorType::HSV && targetColorSpace == ColorType::XYZ)
	{
		this->HSVtoRGB();
		this->RGBtoXYZ();
		this->ColorSpace = ColorType::XYZ;
	}
}*/
#pragma endregion

