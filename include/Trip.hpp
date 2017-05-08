/*
 * Trip.hpp
 *
 *  Created on: 05/06/2014
 *      Author: johan
 */

#ifndef TRIP_HPP_
#define TRIP_HPP_

#include <boost/serialization/access.hpp>
#include <string>

class Trip {

	friend class boost::serialization::access;

private:

  template<class Archive>
  void serialize(Archive & ar, const unsigned int version ) {
	  ar & _id_origin;
	  ar & _id_destination;
	  ar & _starting_time;
  }

  std::string  _id_origin;
  std::string  _id_destination;
  float _starting_time;

public:

  Trip();
  Trip(std::string idOrigin, std::string idDestination, float startingTime);
  ~Trip() {};

  std::string getIdDestination() const {
	  return _id_destination;
  }

  void setIdDestination(std::string idDestination) {
	  _id_destination = idDestination;
  }

  float getStartingTime() const {
	  return _starting_time;
  }

  void setStartingTime(float startingTime) {
	  _starting_time = startingTime;
  }

  std::string getIdOrigin() const {
	  return _id_origin;
  }

  void setIdOrigin(std::string idOrigin) {
	  _id_origin = idOrigin;
  }

};


#endif /* TRIP_HPP_ */
