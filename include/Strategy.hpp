/*
 * Strategy.hpp
 *
 *  Created on: 07/07/2014
 *      Author: johan
 */

#ifndef STRATEGY_HPP_
#define STRATEGY_HPP_

#include <boost/serialization/access.hpp>
#include <iostream>
#include <math.h>

class Strategy {

	friend class boost::serialization::access;

private:

	float _sin_alpha;
	float _cos_alpha;
	float _theta;
	bool  _optimized;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version ) {
		ar & _sin_alpha;
		ar & _cos_alpha;
		ar & _theta;
		ar & _optimized;
	}


public:

	//! Default constructor.
	Strategy();

	//! Constructor.
	Strategy(float alpha, float theta);

	//! Destructor.
	~Strategy() {};

	float getSinAlpha() const {
		return _sin_alpha;
	}

	void setSinAlpha(float sin_alpha) {
		_sin_alpha = sin_alpha;
	}

	float getCosAlpha() const {
		return _cos_alpha;
	}

	void setCosAlpha(float cos_alpha) {
		_cos_alpha = cos_alpha;
	}

	float getTheta() const {
		return _theta;
	}

	void setTheta(float beta) {
		_theta = beta;
	}

	bool isOptimized() const {
		return _optimized;
	}

	void setOptimized(bool optimized) {
		_optimized = optimized;
	}

	bool computeStrategy(float x1, float x2);

	friend std::ostream& operator<<(std::ostream& os, const Strategy& obj);

};



#endif /* STRATEGY_HPP_ */
