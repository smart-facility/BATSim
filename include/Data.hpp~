/****************************************************************
 * DATA.HPP
 * 
 * This file contains all the data related class and methods.
 * 
 * Authors: J. Barthelemy and L. Hollaert
 * Date   : 17 july 2012
 ****************************************************************/

/*! \file Data.hpp
 *  \brief Data related class and methods.
 */

#ifndef DATA_HPP_
#define DATA_HPP_

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include <repast_hpc/Properties.h>
#include <repast_hpc/RepastProcess.h>
#include <repast_hpc/TDataSource.h>
#include "repast_hpc/SVDataSet.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Network.hpp"
#include "Random.hpp"
#include "Strategy.hpp"
#include "tinyxml2.hpp"

enum class Mode_transims : int { WALK = 1, CAR_DRIVER = 2, BUS = 3, LIGHT_RAIL = 4, BICYCLE = 7,
								 OTHER = 8, CAR_PASSENGER = 11, TAXI = 12 };

//! \brief Singleton class for the Data class.
template <typename T>
class Singleton {

protected:

	//! Constructor.
	Singleton () {}

	//! Destructor.
	~Singleton () {}

public:

	//! Generates a singleton instance of a T object.
	/*!
    \param aProps model properties of VirtualBelgium
	 */
	static void makeInstance ( repast::Properties aProps ) {
		if (NULL == _singleton)  {
			_singleton = new T(aProps);
		}
	}

	//! Return the generated, unique, instance of a T object (if already instanciated).
	/*!
    \return the unique instance of a T object
	 */
	static T *getInstance () {
		if (NULL == _singleton)    {
			std::cerr << "No instance already created!" << std::endl;
		}
		return (static_cast<T*> (_singleton));
	}

	//! Free the memory.
	static void kill () {
		if (NULL != _singleton) {
			delete _singleton;
			_singleton = NULL;
		}
	}

private:

	static T *_singleton;    //!< unique instance of the Data class

};

//! Initialize the singleton to NULL.
template <typename T> T *Singleton<T>::_singleton = NULL;

//! \brief A data class.
/*!
 A data class reading and producing all the inputs required by
 TrafficSim. This class is implemented using a singleton design pattern.
 */
class Data : public Singleton<Data> {

	friend class Singleton<Data>;

private:

	Network               _network;           //!< road network.
	repast::Properties    _props;             //!< properties of simulation.
	std::map<std::string, std::string>  _map_act_loc_nodes; //!< map linking the activities id to the road network node (transim input).
	std::map<std::string, std::string>  _map_2way_links;    //!< map in which each key is an id of a link A->B and the value is the id of link B->A (transim input).
	std::vector<Strategy> _strategies;        //!< vector containing the possible strategies

public:

	//! Constructor.
	/*!
      The constructor initialize all the private members of a Data object.

      \param aProps the properties of TrafficSim
	 */
	Data( repast::Properties aProps ) {

		if (repast::RepastProcess::instance()->rank() == 0) std::cout << "Data reading" << std::endl;

		// Models properties

		this->_props = aProps;

		// Network data

		if (this->_props.getProperty("par.network_format").compare("matsim") == 0 ) {
			read_network_matsim();
		}
		else {
			read_network_transims();
		}

		if (repast::RepastProcess::instance()->rank() == 0) {
			cout << "       network bounding box: x min " << _network.getMinX() << ", x max " << _network.getMaxX();
			cout << ", y min " << _network.getMinY() << ", y max " << _network.getMaxY() << endl;
			cout << "       network contains " << _network.getLinks().size() << " links and " << _network.getNodes().size() << " nodes" << endl;
		}

		// Agents strategies

		read_strategies();

	}

	//! Destructor.
	~Data() {};

	//! Read the road network (MATSim format).
	void read_network_matsim();

	//! Read the road network (transims format).
	void read_network_transims();

	//! Read the agents strategies.
	void read_strategies();

	//! Return the road network.
	/*!
      \return a road network
	 */
	const Network & getNetwork() const {
		return _network;
	}

	const std::map<std::string, std::string> & getMap2wayLinks() const {
		return _map_2way_links;
	}

	const std::map<std::string, std::string> & getMapActLocNodes() const {
		return _map_act_loc_nodes;
	}

	const std::vector<Strategy>& getStrategies() const {
		return _strategies;
	}

	//! Return one randomly selected strategy.
	const Strategy& getOneStrategy() const;

};


//! \brief Aggregate output data class.
/*!
  This class is responsible for gathering aggregate data from the simulation.
 */
class AggregateSum : public repast::TDataSource<int> {

private :
	int _sum;   //!< the aggregate sum computed over all processes used to run the simulation

public :

	//! Constructor.
	AggregateSum();

	//! Destructor.
	~AggregateSum() {};

	//! Reset sum to 0.
	void setData(int val);

	//! Increment the data by 1.
	void incrementData();

	//! Decrement the data by 1.
	void decrementData();

	//! Return the current state of sum.
	/*!
    \return the value of sum
	 */
	int getData();

};

/////////////////////////////
//  Some useful routines.  //
/////////////////////////////

//! Convert a number of seconds to the format hour:min:sec.
/*!
 \param n_sec the number of seconds to convert

 \return a string in the hour:min:sec format
 */
std::string secToTime( float n_sec );

//! Convert a given time formatted as hh:mm:ss to the number of seconds since midnight.
/*!
  \param aTime a string that represents an hour in the hh:mm:ss format

  \return the number of seconds elapsed since midnight
 */
long timeToSec( const std::string &aTime );

//! Convert a number of seconds to hours.
/*!
 \param n_sec the number of seconds to convert

 \return the hours
 */
unsigned int secToHour( float n_sec );

//! Compute the number of lines in a file.
/*!
 \param filename the path to the file to analyze

 \return the number of lines in the file
 */
long int linesCount(std::string filename);

//! Decompose a string according to a separator into a vector of type T.
/*! 
 \param msg the string to decompose
 \param separators separators used for the decomposition

 \return a vector of type T
 */
template<typename T>
std::vector<T> split(const std::string & msg, const std::string & separators);

#endif /* DATA_HPP_ */
