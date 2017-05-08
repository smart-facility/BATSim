/****************************************************************
 * INDIVIDUAL.CPP
 * 
 * This file contains all the definitions of the methods of
 * individual.hpp (see this file for methods' documentation)
 * 
 * Authors: J. Barthelemy
 * Date   : 9 July 2014
 ****************************************************************/

#include "../include/Individual.hpp"

using namespace std;
using namespace repast;


IndividualPackage::IndividualPackage() :
		id(),
		init_proc(),
		agent_type(),
		cur_proc(),
		trips(),
		x(),
		y(),
		remaining_time(),
		strategy(),
		path(),
		en_route(),
		at_node(),
		cur_link(),
		size(),
		cur_trip_duration_theo(),
		n_path_performed(),
		n_link_in_path() {
}

IndividualPackage::IndividualPackage(int aId, int aInitProc, int aAgentType, int aCurProc, std::vector<Trip> aTrips, float aX, float aY, float aRemainingTime,
									 Strategy aStrategy, std::vector<std::string> aPath, bool aEnRoute, bool aAtNode, std::string aCurLink, int aSize, float aCurTripDurationTheo,
									 int aNPathPerformed, int aLinkInPath) :
		id(aId),
		init_proc(aInitProc),
		agent_type(aAgentType),
		cur_proc(aCurProc),
		trips(aTrips),
		x(aX),
		y(aY),
		remaining_time(aRemainingTime),
		strategy(aStrategy),
		path(aPath),
		en_route(aEnRoute),
		at_node(aAtNode),
		cur_link(aCurLink),
		size(aSize),
		cur_trip_duration_theo(aCurTripDurationTheo),
		n_path_performed(aNPathPerformed),
		n_link_in_path(aLinkInPath) {
}

Individual::Individual(repast::AgentId id, std::vector<Trip> trips, float x, float y, float remaining_time,
			           Strategy strategy, std::vector<std::string> path, bool en_route, bool at_node, std::string cur_link,
			           int size, float cur_trip_duration_norm, int n_path_performed, int n_link_in_path) :
		_id(id),
		_trips(trips),
		_x(x),
		_y(y),
		_remaining_time(remaining_time),
		_strategy(strategy),
		_path(path),
		_en_route(en_route),
		_at_node(at_node),
		_cur_link(cur_link),
		_size(size),
		_cur_trip_duration_theo(cur_trip_duration_norm),
		_n_path_performed(n_path_performed),
		_n_link_in_path(n_link_in_path){
}

Individual::Individual(repast::AgentId id, std::vector<Trip> trips, int size) :
		_id(id),
		_trips(trips),
		_x(0.0f),
		_y(0.0f),
		_strategy(),
		_path(),
		_en_route(false),
		_at_node(true),
		_cur_link(),
		_size(size),
		_cur_trip_duration_theo(0.0f),
		_n_path_performed(1),
		_n_link_in_path(0) {

	if( this->_trips.size() > 0 ) {
		this->_remaining_time = this->_trips[0].getStartingTime();
	}
	else {
		this->_remaining_time = 0.0f;
	}

}

Individual::~Individual() {
}

void Individual::print() {

	ostringstream screen_output;
	screen_output << setprecision(15);

	screen_output << "Individual " << this->_id << endl;
	if( this->_trips.size() > 0 ){
		screen_output << "    Trips:" << endl;
		for( auto &t : this->_trips ) {
			screen_output << "       from " << t.getIdOrigin() << " to " << t.getIdDestination() << " at " << t.getStartingTime() << endl;
		}
	}
	screen_output << "    Remaining time: " << this->_remaining_time << endl;
	screen_output << "    Strategy parameters: " << this->_strategy << endl;
	screen_output << "    Localization: " << this->_x << ", " << this->_y << endl;
	if( this->_path.size() > 0 ) {
		screen_output << "    Path: ";
		for( auto &l : this->_path ) {
			screen_output << " " << l;
		}
		screen_output << endl;
	}
	screen_output << "    En route (1 = yes, 0 = no): " << this->_en_route << endl;
	screen_output << "    Current normalized time spent en-route: " << this->_cur_trip_duration_theo << endl;
	screen_output << endl;

	cout << screen_output.str();

}


string Individual::getNextLink() {
	return _path.back();
}


string Individual::getNextLinkAndRemove() {

	// Getting the next link id
	string link = _path.back();

	// Removing it from the path
	_path.pop_back();

	// incrementing the number of link traveled on the path
	_n_link_in_path++;

	// Returning the result
	return link;

}


bool Individual::isRerouting( Network & network, float simulation_time ) {

	// Computing inputs of the strategy
	float x1 = 0.0f;
	if( _cur_trip_duration_theo > 0.0 )
		x1 = ( simulation_time - _trips.front().getStartingTime() ) / _cur_trip_duration_theo;

	float x2 = network.getLinks().at(_cur_link.c_str()).getNAgents() / network.getLinks().at(_cur_link.c_str()).getCapacity();

	// Applying the strategy: true -> re-reroute, false -> keep current path

	// ... checking if at least one car on the next link before computing the strategy
	if( x2 > 0.0f ) return _strategy.computeStrategy(x1, x2);

	// ... otherwise no rerouting
	return false;

}


void Individual::setNextTrip( Network& network, float time ) {

	// Removing previous trip
	this->_trips.erase( this->_trips.begin() );

	// Characterizing new trip
	std::string origin_node_id      = this->_trips.front().getIdOrigin();
	std::string destination_node_id = this->_trips.front().getIdDestination();
	this->_path = network.computePath(origin_node_id, destination_node_id);

	// Updating agent position
	this->_x = network.getNodes().at(origin_node_id).getX();
	this->_y = network.getNodes().at(origin_node_id).getY();

	// Agent stopped at a node and waiting there until departure time
	this->_en_route = false;
	this->_at_node  = true;

	// Reset of theoretical trip duration time
	this->_cur_trip_duration_theo = 0.0f;

	// Remaining time before next event = next trip starting time - current time.
	// If remaining time < 0 then agent is late and remaining time is set to 0.
	this->_remaining_time = max<float>(this->_trips.front().getStartingTime() - time, 0.0);

	// incrementing the path id and resetting the number of links traveled in the current path
	_n_path_performed++;
	_n_link_in_path = 0;

}


void Individual::decreaseRemainingTime(float time) {
	_remaining_time = max<float>( _remaining_time - time, 0.0f );
}

void Individual::increaseTripDurationTheo(float time) {
	_cur_trip_duration_theo = _cur_trip_duration_theo + time;
}



