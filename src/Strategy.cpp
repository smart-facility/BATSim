/*
 * Strategy.cpp
 *
 *  Created on: 07/07/2014
 *      Author: johan
 */

#include "../include/Strategy.hpp"


Strategy::Strategy() {

	_sin_alpha = 0.0f;
	_cos_alpha = 0.0f;
	_theta     = 0.0f;
	_optimized = false;

}


Strategy::Strategy(float alpha, float theta) : _theta(theta), _optimized(true) {

	_sin_alpha = (float)sin(alpha);
	_cos_alpha = (float)cos(alpha);

}


bool Strategy::computeStrategy(float x1, float x2) {

	if(  (x1 * _cos_alpha + x2 * _sin_alpha) - _theta > 0.0f ) return true;
	else return false;

}


std::ostream& operator<<(std::ostream& os, const Strategy& obj) {

	os << "cos_alpha = " << obj._cos_alpha << ", sin_alpha = " << obj._sin_alpha << ", theta = " << obj._theta;

	if( obj._optimized == true ) os << ", optimized = TRUE";
	else                         os << ", optimized = FALSE";

	return os;

}
