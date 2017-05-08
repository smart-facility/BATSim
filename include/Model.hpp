/****************************************************************
 * MODEL.HPP
 *
 * This file contains all the VirtualBelgium's model and
 * scheduler related classes and functions.
 *
 * Authors: J. Barthelemy and L. Hollaert
 * Date   : 17 july 2012
 ****************************************************************/

/*! \file Model.hpp
 *  \brief VirtualBelgium's models and schedulers declarations.
 */

#ifndef MODEL_HPP_
#define MODEL_HPP_

#include "Individual.hpp"
#include "Data.hpp"
#include "tinyxml2.hpp"
#include "FiboHeap.hpp"

#include "repast_hpc/SharedContext.h"
#include "repast_hpc/Schedule.h"
#include "repast_hpc/Random.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/Utilities.h"
#include "repast_hpc/io.h"
#include "repast_hpc/logger.h"
#include "repast_hpc/initialize_random.h"
#include "repast_hpc/SVDataSetBuilder.h"
#include "repast_hpc/SharedContinuousSpace.h"

#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <vector>
#include <iomanip>
#include <boost/serialization/access.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpi.hpp>
#include <boost/mpi/collectives.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/unordered_set.hpp>
#include <boost/math/special_functions/pow.hpp>
#include <boost/range/algorithm.hpp>
#include <functional>

const int MODEL_AGENT_IND_TYPE = 0;     //!< constant for the individual agent type

//! Model class.
/*!
  This class contains the scheduler and is responsible for data aggregation.
  It is the core of TrafficSim.
 */
class Model {

 private :

  friend class ProviderReceiver;

  int                       _proc;                            //!< rank of the model's process
  repast::Properties&       _props;                           //!< properties of the model
  repast::SVDataSet*        _data_collection;                 //!< aggregated output data set
  Network                   _network;                         //!< road network
  float                     _time;                            //!< simulation time
  float                     _time_tolerance;                  //!< minimum numbers of seconds between 2 events
  unsigned int              _time_interval_records;           //!< time interval between for link flows/saturation recording
  unsigned int              _time_interval_records_snapshots; //!< time interval between for link flows/saturation recording
  AggregateSum              _total_agents;                    //!< number of agents remaining in the simulation
  AggregateSum              _total_moving_agents;             //!< number of agents moving
  AggregateSum              _total_trips_performed;           //!< cumulative total number of trips performed
  AggregateSum              _total_rerouting;                 //!< number of rerouted agents
  map<std::string, vector<int> >   _links_load_over_time;            //!< number of agents on each link per unit of time (defined by user)
  map<std::string, vector<int> >   _links_state_snapshot;            //!< number of agents on each link at a given point in time
  vector<float>             _trips_starting_time;             //!< trips starting time
  map<std::string, int>     _map_node_process;                //!< map containing identifying the process of every node
  map<repast::AgentId, int> _map_agents_to_move_process;      //!< map containing the agents id to be moved and their destination process
  map<int, float>           _map_agent_fitness;                //!< map containing the final fitness of the agent after the trip is over

  std::map<std::string, std::map<std::string, std::vector<std::string>>> _look_up_paths; //!< Look up table for path

 public :

  repast::SharedContext<Individual>* agents;               //!< Shared context containing the individual agents of the simulation
  repast::SharedContinuousSpace<Individual,repast::WrapAroundBorders,repast::SimpleAdder<Individual>>* continuous_space; //!< Shared continuous space

  //! Constructor.
  /*
   /param comm the mpi communicator
   /param props the model properties
   */
  Model( boost::mpi::communicator* comm, repast::Properties & props );

  //! Destructor.
  ~Model();
  
  //! Model agents initialization (transims input format).
  unsigned int init_transims();

  //! Model agents initialization (MATSim input format).
  unsigned int init_matsim();

  //! Model agents strategies initialization.
  void init_agents_strategies();

  //! Agents initial path computation.
  void compute_initial_paths();

  //! Model agents localization initialization.
  void synch_agents();

  //! Initialization of the simulation's schedule.
  void initSchedule();

  //! Implements one step of the simulation.
  void step();

  //! Used by Repast HPC to exchange Individual agents between process.
  /*!
    \param agent the agent to exchange
    \param out the package containing the agent to exchange
   */
  void providePackage(Individual * agent , std::vector<IndividualPackage>& out);

  //! Used by Repast HPC to create an Individual Agent from an IndividualPackage.
  /*!
    \param package a package containing an individual agent
    
    \return an individual agent
   */
  Individual * createAgent(IndividualPackage package);

  //! Used by Repast HPC to packaged requested agents.
  /*!
    \param req the requested agents
    \param out the vector containing the packaged agents
   */
  void provideContent(repast::AgentRequest req, std::vector<IndividualPackage>& out);

  //! Update an individual given the information provided in the package.
  /*!
    \param package the package containing the new attributes values
   */
  void updateAgent(IndividualPackage package);

  //! Getting the simulation time.
  /*!
    \return the simulation time
   */
  float getTime() const {
	  return _time;
  }

  //! Setting the simulation time.
  /*!
    \param time the new simulation time
   */
  void setTime(float time) {
	  _time = time;
  }

  //! Increasing the simulation time by a given value.
  /*!
    \param time the value by which the simulation time is increased
   */
  void increaseTime(float time) {
	  _time = _time + time;
  }

  //! Return the models properties read from properties file.
  const repast::Properties& getProps() const {
  	return _props;
  }

  //! Check if the simulation should continue or stop.
  void checkStop();

  //! Writing the link states (snapshot and aggregate) in a file.
  void writeLinksState();

  //! Writing the trips starting times in a file.
  void writeTripsStartingTimes();

  //! Writing final agents fitness
  void writeAgentFitness();

  //! Writing the outputs for Moves
  void writeOutputsMoves(int id, std::string link_id, float time_entering_link, float time_on_link, int path_id, int link_id_on_path);

  //! Constructing the map of processes' nodes
  void constructMapNodeProcess();

  //! Check if a (x,y) coordinates belongs to the local continuous space.
  /*!
    \param x a x coordinate
    \param y a y coordinate

    \return true if (x,y) in local continuous space, false otherwise
   */
  bool isInLocalBounds(double x, double y);



};

#endif /* MODEL_HPP_ */
