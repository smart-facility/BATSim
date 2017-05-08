/****************************************************************
 * RANDOM.HPP
 * 
 * This file contains random number generators related functions.
 * 
 * Authors: J. Barthelemy
 * Date   : 17 july 2012
 ****************************************************************/

/*! \file Random.hpp
 *  \brief Random number generators and related tools.
 */

#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include <iostream>
#include <limits>
#include <cstdlib>
#include <vector>
#include <math.h>
#include <stdexcept>


//! A simple structure for returning a 2d draw.
typedef struct draw_2d draw_2d;
struct draw_2d {

	float x1;  //!< first component
	float x2;  //!< second component

	//! Constructor.
	draw_2d(float aX1, float aX2) : x1(aX1), x2(aX2) {};

};


//! \brief A structure for storing the parameters of an univariate distribution.
/*!
 This structure saves the location, scale parameters and
 upper bound of an univariate distribution.
 */
typedef struct dist_param dist_param;
struct dist_param {
	float mu;    //!< mean of the distribution
	float sigma; //!< standard error of the distribution
	float max;   //!< upper bound of the distribution
};


//! \brief A structure for storing the parameters of a mixture of univariate distributions.
/*!
 This structure saves the the location and scale parameters of the components
 and the upper bound of an univariate distribution.
 */
typedef struct dist_param_mixture dist_param_mixture;
struct dist_param_mixture {
	std::vector<float> mu;    //!< vector storing the mean of each component
	std::vector<float> sigma; //!< vector storing the standard error of each component
	std::vector<float> p;     //!< proportions of each components
	float              max;   //!< upper bound of the distribution
};


//! \brief A structure for storing the parameters of a bivariate distribution (X_1, X_2).
/*!
 This structure saves the location vector, the (Lower triangular matrix of Choleski decomposition
 of the) covariance matrix and the upper bounds vector of a bivariate distribution.
 */
typedef struct dist_param_2d dist_param_2d;
struct dist_param_2d {
	float mu[2];    //!< mean of the distribution
	float sigma[3]; //!< covariance matrix or its choleski decomposition
};


//! \brief A structure for storing the parameters of a mixture of bivariate distributions.
/*!
 This structure saves the location vectors, the Choleski decomposition
 of the covariance matrices and the upper bounds vectors of the components of
 bivariate mixture distribution.
 */
typedef struct dist_param_mixture_2d dist_param_mixture_2d;
struct dist_param_mixture_2d {
	std::vector<dist_param_2d > components;  //!< vector storing the mixture's components
	std::vector<float> p;                    //!< mixing proportions of each components
	float max[2];                            //!< vector of upper bounds
};

//! Simplest and fastest random number generator recommended by Numerical Recipes.
/*!
  Implements the Ranq1 algorithm.
 */
struct Ranq1 {

	unsigned long long v;  //!< state of the random number generator

	//! Constructor j is the seed.
	/*!
      \param j the seed.
	 */
	Ranq1( unsigned long long j) : v(4101842887655102017LL) {
		v ^= j;
		v = int64();
	}

	//! Generates an unsigned 64 bits integer
	/*!
      \return a random number
	 */
	inline unsigned long long int64() {
		v ^= v >> 21;
		v ^= v << 35;
		v ^= 4;
		return v * 2685821657736338717LL;
	}

	//! Generates a double
	/*!
      \return a random number
	 */
	inline double doub() {
		return 5.42101086242752217E-20 * int64();
	}

	//! Generates a float
	/*!
      \return a random number
	 */
	inline float fl() {
		return (float)doub();
	}

	//! Generates an unsigned 32 bits integer
	/*!
      \return a random number
	 */
	inline unsigned int int32() {
		return (unsigned int)int64();
	}

	//! Generates an unsigned 32 bits random number in the interval [0,limit].
	/*!
	  \return a random number in [0,limit]
	 */

	inline unsigned int int32(unsigned int limit) {

		unsigned int divisor = std::numeric_limits<unsigned int>::max()/(limit+1);
		unsigned int result;

		do {
			result = int32() / divisor;
		} while (result > limit);

		return result;

	}

};


//! Fast and good random number generator (see Numerical Recipes).
/*!
  Implements Knuth's substractive generator using only floating
  operations.
 */
struct Ranfib {

	double dtab[55];      //!< part of the generator state
	double dd;            //!< part of the generator state
	int inext;            //!< part of the generator state
	int inextp;           //!< part of the generator state

	//! Constructor.
	/*!
    \param j any integer seed.
	 */
	Ranfib( unsigned long j ) : inext(0), inextp(31) {

		dd = 0.0;

		// Using Ranq1 for initialization.
		Ranq1 init(j);

		for( int k = 0; k < 55; k++) {
			dtab[k] = init.doub();
		}

	}

	//! Returns random double-precision floating point value in [0,1].
	/*!
    \return a random number
	 */
	inline double doub() {

		if( ++inext  == 55 ) inext  = 0;
		if( ++inextp == 55 ) inextp = 0;
		dd = dtab[inext] - dtab[inextp];
		if( dd < 0 ) dd += 1.0;
		return ( dtab[inext] = dd );

	}

	//! Returns random simple-precision floating point value in [0,1].
	/*!
    \return a random number
	 */
	inline float fl() {
		return (float)doub();
	}

};


//! Fast Random number generator for normal distribution (Numerical Recipes).
struct Normaldev : Ranfib {

	//! Constructor.
	/*!
    \param i seed of the generator
	 */
	Normaldev(unsigned long long i) : Ranfib(i) {};

	//! Returns a normal random draw with mean mu and standart deviation sigma.
	/*!
    \param mu mean of the distribution
    \param sigma standart deviation of the distribution

    \return a random number
	 */
	inline float dev(double mu, double sigma) {

		float u, v, x, y, q;

		do {
			u = fl();
			v = 1.7156 * ( fl() - 0.5 );
			x = u - 0.449871;
			y = abs(v) + 0.386595;
			q = x * x + y * ( 0.19600 * y - 0.25472 * x );
		} while ( q > 0.27597 && ( q > 0.27846 || v * v > -4.0 * log(u) * u * u ) );

		return mu + sigma * v / u;

	}

	//! Returns a upper bounded normal random draw.
	/*!
    \param mu mean of the distribution
    \param sigma standard deviation of the distribution
    \param max upper bound of the distribution

    \return a random number
	 */
	inline float dev(double mu, double sigma, float max) {

		float result;

		do {
			result = dev(mu, sigma);
		} while ( result > max );
		return result;

	}

	//! Returns a bounded normal random draw.
	/*!
    \param mu mean of the distribution
    \param sigma standard deviation of the distribution
    \param min lower bound of the distribution
    \param max upper bound of the distribution

    \return a random number
	 */
	inline float dev(double mu, double sigma, float min, float max) {

		float result;

		do {
			result = dev(mu, sigma);
		} while ( result > max || result < min );
		return result;

	}


};


//! Fast Random number generator for log-normal distribution (Numerical Recipes).
struct LogNormaldev : Ranfib {

	//! Constructor.
	/*!
    \param i seed of the generator
	 */
	LogNormaldev(unsigned long long i) : Ranfib(i) {};

	//! Returns a Log-normal random draw.
	/*!
    \param mu mean of the distribution
    \param sigma standart deviation of the distribution

    \return a random number
	 */
	inline float dev(double mu, double sigma) {

		float u, v, x, y, q;

		// normal draw
		do {
			u = fl();
			v = 1.7156 * ( fl() - 0.5 );
			x = u - 0.449871;
			y = abs(v) + 0.386595;
			q = x * x + y * ( 0.19600 * y - 0.25472 * x );
		} while ( q > 0.27597 && ( q > 0.27846 || v * v > -4.0 * log(u) * u * u ) );

		return exp(mu + sigma * v / u);

	}

	//! Returns a bounded Log-normal random draw.
	/*!
    \param mu mean of the distribution
    \param sigma standart deviation of the distribution
    \param max upper bound of the distribution

    \return a random number
	 */
	inline float dev(double mu, double sigma, float max) {

		float result;

		do {
			result = dev(mu, sigma);
		} while ( result > max );
		return result;

	}

	//! Returns a bounded Log-normal random draw.
	/*!
    \param mu mean of the distribution
    \param sigma standard deviation of the distribution
    \param min lower bound of the distribution
    \param max upper bound of the distribution

    \return a random number
	 */
	inline float dev(double mu, double sigma, float min, float max) {

		float result;

		do {
			result = dev(mu, sigma);
		} while ( result > max || result < min );
		return result;

	}


};


//! Fast Random number generator for mixture of normal distribution (Numerical Recipes).
struct MixtureNormal : Normaldev {

	//! Constructor.
	MixtureNormal(unsigned long long i) : Normaldev(i) {};

	//! Returns a draw from the mixture distribution.
	/*!
    \param mu vector of means
    \param sigma vector of standart deviations
    \param p vector of mixing proportions

    \return a random number
	 */
	inline float dev(std::vector<float> mu, std::vector<float> sigma, std::vector<float> p) {

		float  a_p, prop_cum;
		int    comp;
		float u, v, x, y, q;

		// looking for the right component of the mixture
		a_p      = fl();
		comp     = 0;
		prop_cum = p[comp];
		while( prop_cum < a_p ) {
			comp++;                               // moving to the next component
			prop_cum = prop_cum + p[comp];        // computation of the cumulative proportion
		}

		// normal draw
		do {
			u = fl();
			v = 1.7156 * ( fl() - 0.5 );
			x = u - 0.449871;
			y = abs(v) + 0.386595;
			q = x * x + y * ( 0.19600 * y - 0.25472 * x );
		} while ( q > 0.27597 && ( q > 0.27846 || v * v > -4.0 * log(u) * u * u ) );

		return (mu[comp] + sigma[comp] * v / u);

	}

	//! Returns a draw from the mixture distribution.
	/*!
    \param mu vector of means
    \param sigma vector of standart deviations
    \param p vector of mixing proportions
    \param max upper bound of the distribution

    \return a random number
	 */
	inline float dev( std::vector<float> mu, std::vector<float> sigma, std::vector<float> p, float max) {

		float result;

		do {
			result = dev(mu, sigma, p);
		} while ( result > max );
		return result;

	}

	//! Returns a draw from the mixture distribution.
	/*!
    \param mu vector of means
    \param sigma vector of standart deviations
    \param p vector of mixing proportions
    \param max upper bound of the distribution
    \param min lower bound of the distribution

    \return a random number
	 */
	inline float dev( std::vector<float> mu, std::vector<float> sigma, std::vector<float> p, float min, float max) {

		float result;

		do {
			result = dev(mu, sigma, p);
		} while ( result > max || result < min );
		return result;

	}

};


//! Fast Random number generator for mixture of log-normal distribution (Numerical Recipes).
struct MixtureLogNormal : LogNormaldev {

	//! Constructor.
	MixtureLogNormal(unsigned long long i) : LogNormaldev(i) {};

	//! Returns a draw from the mixture distribution.
	/*!
    \param mu vector of means
    \param sigma vector of standart deviations
    \param p vector of mixing proportions

    \return a random number
	 */
	inline float dev(std::vector<float> mu, std::vector<float> sigma, std::vector<float> p) {

		float  a_p, prop_cum;
		int    comp;
		float u, v, x, y, q;

		// looking for the right component of the mixture
		a_p      = fl();
		comp     = 0;
		prop_cum = p[comp];
		while( prop_cum < a_p ) {
			comp++;                               // moving to the next component
			prop_cum = prop_cum + p[comp];        // computation of the cumulative proportion
		}

		// log-normal draw
		do {
			u = doub();
			v = 1.7156 * ( fl() - 0.5 );
			x = u - 0.449871;
			y = abs(v) + 0.386595;
			q = x * x + y * ( 0.19600 * y - 0.25472 * x );
		} while ( q > 0.27597 && ( q > 0.27846 || v * v > -4.0 * log(u) * u * u ) );


		return exp(mu[comp] + sigma[comp] * v / u);

	}

	//! Returns a draw from the mixture distribution.
	/*!
    \param mu vector of means
    \param sigma vector of standart deviations
    \param p vector of mixing proportions
    \param max upper bound of the distribution

    \return a random number
	 */
	inline float dev( std::vector<float> mu, std::vector<float> sigma, std::vector<float> p, float max) {

		float result;

		do {
			result = dev(mu, sigma, p);
		} while ( result > max );
		return result;

	}

	//! Returns a draw from the mixture distribution.
	/*!
    \param mu vector of means
    \param sigma vector of standart deviations
    \param p vector of mixing proportions
    \param max upper bound of the distribution
    \param min lower bound of the distribution

    \return a random number
	 */
	inline float dev( std::vector<float> mu, std::vector<float> sigma, std::vector<float> p, float min, float max) {

		float result;

		do {
			result = dev(mu, sigma, p);
		} while ( result > max || result < min );
		return result;

	}

};


//! Fast Random number generator for mixture of bivariate log-normal distribution (Numerical Recipes).
struct MixtureLogNormal2D : Normaldev {

	//! Constructor.
	MixtureLogNormal2D(unsigned long long i) : Normaldev(i) {};

	//! Returns 2 draws from a bounded mixture distribution.
	inline draw_2d dev(dist_param_mixture_2d distrib) {

		float   a_p, prop_cum;
		int     comp;
		draw_2d result(0.0,0.0);

		// looking for the right component of the mixture
		a_p      = fl();
		comp     = 0;
		prop_cum = distrib.p[comp];
		while( prop_cum < a_p ) {
			comp++;                                       // moving to the next component
			prop_cum = prop_cum + distrib.p[comp];        // computation of the cumulative proportion
		}

		do {

			result.x1 = Normaldev::dev(0,1);
			result.x2 = Normaldev::dev(0,1);

			// generating the draws
			result.x1 = exp(distrib.components[comp].mu[0] + distrib.components[comp].sigma[0] * result.x1 + distrib.components[comp].sigma[1] * result.x2);
			result.x2 = exp(distrib.components[comp].mu[1] + distrib.components[comp].sigma[2] * result.x2);

		} while (result.x1 > distrib.max[0] || result.x2 > distrib.max[1] );

		return result;

	}

};


//! \brief SingletonRnd class for the RandomGenerators class.
template <typename T>
class SingletonRnd {

protected:

	//! Constructor.
	SingletonRnd () {}

	//! Destructor.
	~SingletonRnd () {}

public:

	//! Create a singleton instance of a T object
	/*!
    /param i a seed for the T constructor
	 */
	static void makeInstance ( unsigned long long i ) {
		if (nullptr == _singleton)  {
			_singleton = new T(i);
		}
	}

	//! Return the instance of SingletonRnd if already generated
	/*!
    /return the instance of SingletonRnd if generated, an exception otherwise
	 */
	static T *getInstance () {
		if (nullptr == _singleton) {
			std::cerr << "No instance already created!" << std::endl;
		}
		return (static_cast<T*> (_singleton));
	}

	//! Killing the instance and freeing memory
	static void kill () {
		if (nullptr != _singleton) {
			delete _singleton;
			_singleton = nullptr;
		}
	}

private:

	static T *_singleton;    //!< unique instance of the RandomGenerators class

};


//! Initialize the singleton to NULL
template <typename T>
T *SingletonRnd<T>::_singleton = nullptr;


//! Random generators
/*!
  This class 
 */
class RandomGenerators : public SingletonRnd<RandomGenerators> {

	friend class SingletonRnd<RandomGenerators>;

public:

	Ranq1              unif;                //!< uniform random draws
	Ranfib             fast_unif;           //!< fast uniform random draws
	Normaldev          norm_dev;            //!< normal random draws
	LogNormaldev       lognorm_dev;         //!< log-normal random draws
	MixtureNormal      mixt_norm_dev;       //!< mixture of univariate normal random draws
	MixtureLogNormal   mixt_lognorm_dev;    //!< mixture of univariate log-normal random draws
	MixtureLogNormal2D mixt_lognorm_dev_2d; //!< mixture of bivariate log-normal random draws

	//! Constructor, initialize every random number generators
	/*!
      \param i the seed
	 */
	RandomGenerators(unsigned long long i) : unif(i + 10000), fast_unif(i+10), norm_dev(i + 100), lognorm_dev(i + 1000),
			mixt_norm_dev(i + 10000), mixt_lognorm_dev(i + 100000), mixt_lognorm_dev_2d(i + 1000000) {};

	//! Destructor.
	virtual ~RandomGenerators() {};

};


//! Randomly draws a class identifier within an interval of an empirical density function.
/*!
  If no lower and upper bounds are given, then the random draw is done considering
  every class identifier. Otherwise only a subset of the class identifiers is
  considered. Note that the class identifiers are integers.

  \param freq the empirical density function in wich the random draw is done.
  \param lo_bound the lower bound index of freq to consider.
  \param up_bound the upper bound index of freq to consider.

  \return the class identifier of freq randomly draw.
 */
template <typename T> int draw_discrete( std::vector<T> freq, int lo_bound = 0, int up_bound = 0) {

	int   N = freq.size();;                                         // compute the size of the interval
	float cumfreq [N];                              // cumulative distribution
	float draw;                                     // random draw
	float icum = 0.0;
	float low;

	// Build the cumulative distribution.

	if( lo_bound == up_bound ) {

		for( int i = 0; i < N; i++ ) {
			icum=icum+freq[i];
			cumfreq[i]=icum;
		}

	}
	else {

		N = ( up_bound - lo_bound ) + 1;
		int j = 0;
		for( int i = lo_bound; i <= up_bound; i++ ) {
			icum=icum+freq[i];
			cumfreq[j]=icum;
			j++;
		}

	}

	// ... distribution is non-trivial
	if(icum>0) {
		for(int i = 0; i < N; i++ ) {
			cumfreq[i]=cumfreq[i]/icum;
		}
	}
	// ... distribution is trivial
	else {
		std::cout << "Error: trivial distribution" << std::endl;
		for(int i = 0; i < N; i++ ) {
			cumfreq[i]=(i+1)/(N+1);
		}
	}

	//  Draw a random number between 0 and 1.

	draw = RandomGenerators::getInstance()->fast_unif.fl();

	//  Find the interval in which this number falls
	//  and return the associated class identifier.

	low  = 0.0;
	for (int  i = 0; i < N; i++ ) {
		if ( (low <= draw) && (draw <= cumfreq[i]) ) {
			return i + lo_bound;
		}
		low = cumfreq[i];
	}

	std::cerr << "Could not find a class identifier!" << std::endl;
	return -1;

}

#endif /* RANDOM_HPP_ */
