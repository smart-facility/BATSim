/*
 * Trip.cpp
 *
 *  Created on: 05/06/2014
 *      Author: johan
 */

#include "../include/Trip.hpp"


Trip::Trip() : _id_origin("0"), _id_destination("0"), _starting_time(0.0f) {

}

Trip::Trip( std::string idOrigin, std::string idDestination, float startingTime ) : _id_origin(idOrigin), _id_destination(idDestination), _starting_time(startingTime) {

}
