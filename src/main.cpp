/****************************************************************
 * MAIN.CPP
 * 
 * This file contains the functions starting the Virtual Belgium
 * simulation.
 * 
 * Authors: J. Barthelemy
 * Date   : 2 June 2014
 ****************************************************************/

/*!\mainpage TrafficSim Documentation
 *
 * Documentation!
 */

/*! \file main.cpp
 *  \brief Main method and functions handling the launching of the simulation.
 */

#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/io.h"
#include "repast_hpc/logger.h"
#include "repast_hpc/Utilities.h"
#include "repast_hpc/initialize_random.h"
#include <boost/mpi.hpp>
#include <boost/serialization/export.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <exception>
#include <time.h>
#include <iomanip>
#include "../include/Model.hpp"
#include "../include/Data.hpp"
#include "../include/Random.hpp"
#include "../include/Network.hpp"

using namespace std;
using namespace repast;
using namespace boost;

/*!
 * Print to the standard error output the required arguments to
 * execute TrafficSim.
 */
void usage() {
  cerr << "usage: trafficsim  string string" << endl;
  cerr << "  first string: string is the path to the Repast HPC \n\tconfiguration properties file" << endl;
  cerr << "  second string: string is the path to the model properties file" << endl;
}

//! Initialize and launch the simulation.
/*!
 * \param propsFile the file containing VirtualBelgium properties.
 * \param argc the total number of arguments passed to the main function.
 * \param argv the arguments passed to the main function.
 */
void runSimulation(std::string propsFile, int argc, char ** argv) {

  // Initialization of mpi's world.
  mpi::communicator world;

  // Random draws generator initialization.
  RandomGenerators::makeInstance( (unsigned long long)world.rank() );

  // Reading model's properties.
  Properties props(propsFile, argc, argv, &world);

  // Timer.
  Timer timer;
  string time;
  timestamp(time);
  props.putProperty("date_time.run", time);
  props.putProperty("process.count", world.size());
  timer.start();

  // Create and initialize the inputs and the model.
  Data::makeInstance(props);
  props.putProperty("data_creation.time", timer.stop());
  props.putProperty("number.nodes",Data::getInstance()->getNetwork().getNodes().size());
  props.putProperty("number.links",Data::getInstance()->getNetwork().getLinks().size());

  Model model(&world, props);
  props.putProperty("model_init.time", timer.stop());
  model.initSchedule();

  // Get the schedule runner and run it, starting the simulation.
  if (world.rank() == 0) cout << "Starting simulation... " << endl;
  ScheduleRunner & runner = RepastProcess::instance()->getScheduleRunner();
  runner.run();
  props.putProperty("run.time", timer.stop());

  // Writing the log file (only for the root process).
  if (world.rank() == 0) {
    vector<string> keysToWrite;
    keysToWrite.push_back("date_time.run");              // starting time of the simulation
    keysToWrite.push_back("process.count");              // number of process
    keysToWrite.push_back("data_creation.time");         // time required to read the data
    keysToWrite.push_back("model_init.time");            // time required to initialize the agents
    keysToWrite.push_back("run.time");                   // run time of the simulation
    keysToWrite.push_back("number.nodes");               // number of nodes in the road network
    keysToWrite.push_back("number.links");               // number of links in the road network
    keysToWrite.push_back("number.agents");              // total number of agents
    keysToWrite.push_back("number.strat_agents");        // total number of strategic agents
    props.log("root");
    props.writeToSVFile("../logs/log_simulation.csv", keysToWrite);
  }

  // Free the memory.
  Data::getInstance()->kill();
  RandomGenerators::getInstance()->kill();

}


//! Main function.
/*!
 * \return EXIT_SUCCESS if the simulation's launching is successful, EXIT_FAILURE otherwise.
 */
int main(int argc, char ** argv) {

  // MPI and simulation variable
  mpi::environment env(argc, argv);   // MPI environment
  mpi::communicator world;            // MPI communicator
  string config, props;               // configuration and properties files

  // Setting the output precision
  cout << setprecision(15);

  // Reading the repast configuration and model properties file paths.
  if (argc >= 3) {
    config = argv[1];
    props = argv[2];
  }
  else {
    if (world.rank() == 0) usage();
    return EXIT_FAILURE;
  }

  // Starting the simulation
  if (config.size() > 0 && props.size() > 0) {
    RepastProcess::init(config, &world);
    runSimulation(props, argc, argv);
  }
  else {
    if (world.rank() == 0) usage();
    return EXIT_FAILURE;
  }

  // Ending the simulation
  if( world.rank() == 0 )  cout << "End of simulation!\n";

  // Freeing memory
  RepastProcess::instance()->done();

  return EXIT_SUCCESS;

}
