/****************************************************************
 * MODEL.CPP
 *
 * This file contains all the definitions of the methods of
 * Model.hpp (see this file for methods' documentation)
 *
 * Authors: J. Barthelemy
 * Date   : 11 june 2014
 ****************************************************************/

#include "../include/Model.hpp"

using namespace repast;
using namespace std;
using namespace tinyxml2;


Model::Model( boost::mpi::communicator* world, Properties & props ) : _props(props), _time(0.0f) {

	// Reading properties, rank of the process and input filenames ----

	_proc  = RepastProcess::instance()->rank();
	agents = new SharedContext<Individual>(world);
	_time_tolerance = boost::lexical_cast<float>(_props.getProperty("par.time_tolerance"));

	// Model space initialization -------------------------------------

	_network = Data::getInstance()->getNetwork();
	//_network.shuffleNodesCoordinates();

	//Point<double> origin(_network.getMinX() - 1.0, _network.getMinY() - 1.0);
	//Point<double> extent(_network.getMaxX() - _network.getMinX() + 1.0, _network.getMaxY() - _network.getMinY() + 1.0);


	Point<double> origin(0.0, 0.0 );
	Point<double> extent(world->size(), 1.0);
	
	GridDimensions grid_dim(origin, extent);

	vector<int> processDims;
	processDims.push_back( strToInt( _props.getProperty("par.proc_x") ) );
	processDims.push_back( strToInt( _props.getProperty("par.proc_y") ) );

	int buffer_size = 0;

	// The following block can be used to generate a buffer area between the process
	// Note that it may slow down significantly the simulation
	#ifdef BUFFERREPAST
        	for (auto itr_link : net.getLinks() ) if( buffer_size < (int)(itr_link.second.getLength() + 2.5)) buffer_size = (int)(itr_link.second.getLength() / 2 + 2.5);
	#endif

	continuous_space = new SharedContinuousSpace<Individual, WrapAroundBorders, SimpleAdder<Individual>>("space", grid_dim, processDims, buffer_size, world);
	agents->addProjection(continuous_space);
	
	cout << "INFO: Proc " << _proc << ": Dimensions: " << continuous_space->dimensions() << endl;
	
	// Model agents initialization ------------------------------------
	
	unsigned int n_trips = 0;
	if (this->_props.getProperty("par.network_format").compare("matsim") == 0 ) {
	  cout << "INFO: Proc " << _proc << " starts init trips (MATSIM format)" << endl;
		n_trips = init_matsim();
	}
	else {
	  cout << "INFO: Proc " << _proc << " start init trips (TRANSIMS format)" << endl;
		n_trips = init_transims();
	}
	cout << "INFO: Proc " << _proc << " done init trips" << endl;

	cout << "INFO: Proc " << this->_proc << " has " << this->agents->size() << " agents" << endl;
	int n_agents_total = 0;;
	boost::mpi::all_reduce(*RepastProcess::instance()->getCommunicator(), this->agents->size(), n_agents_total, std::plus<int>());
	_props.putProperty("number.agents",n_agents_total);

	// Agents initial paths and strategies ------------------------

	compute_initial_paths();
	init_agents_strategies();

	// Determining the recording time intervals -----------------------

	_time_interval_records = boost::lexical_cast<unsigned int>(_props.getProperty("par.record_interval_aggregate"));
	unsigned int n_records = 1440 / _time_interval_records;

	_time_interval_records_snapshots = boost::lexical_cast<unsigned int>(_props.getProperty("par.record_interval_snapshot"));
	unsigned int n_records_snapshot = 1440 / _time_interval_records_snapshots;

	if( _proc == 0 ) cout << "Number of records : aggregate : " << n_records << " - snapshots : " << n_records_snapshot << endl;

	// Trips starting time recording ----------------------------------

	this->_trips_starting_time.reserve(n_trips);

	// Link state recording initialization ------------------------

	for( auto lnk : _network.getLinks() ) {

		Node lnk_orig_node = _network.getNodes().at(lnk.second.getStartNodeId());
		if( isInLocalBounds( lnk_orig_node.getX(), lnk_orig_node.getY() ) == true ) {
			_links_load_over_time[lnk.first] = vector<int>(n_records);
			_links_state_snapshot[lnk.first] = vector<int>(n_records_snapshot);
		}

	}

	cout << "Proc " << _proc << " has " << _links_load_over_time.size() << " links to watch!" << endl;

	// Process nodes recording ----------------------------------------

	for( auto nde : _network.getNodes() ) {

		if( isInLocalBounds( nde.second.getX(), nde.second.getY() ) == true ) {
			_map_node_process[nde.second.getId()] = _proc;
		}

	}

	cout << "Proc " << _proc << " has " << _map_node_process.size() << " nodes to watch!" << endl;

	constructMapNodeProcess();

	// Aggregate data output ------------------------------------------

	string fileOutputName("../output/sim_out.csv");
	SVDataSetBuilder builder( fileOutputName.c_str(), ";", RepastProcess::instance()->getScheduleRunner().schedule() );
	builder.addDataSource(repast::createSVDataSource("total_agents", &this->_total_agents, std::plus<int>()));
	builder.addDataSource(repast::createSVDataSource("total_moving_agents", &this->_total_moving_agents, std::plus<int>()));
	builder.addDataSource(repast::createSVDataSource("total_trips_performed", &this->_total_trips_performed, std::plus<int>()));
	builder.addDataSource(repast::createSVDataSource("total_reroutings", &this->_total_rerouting, std::plus<int>()));
	this->_data_collection = builder.createDataSet();

	if ( _proc == 0 ) cout << "... end of model initialization!" << endl;

}


Model::~Model() {
	delete agents;
}

unsigned int Model::init_transims() {

	if( this->_proc == 0 ) cout << "... initialization agents (from transims input format) !" << endl;

	unsigned int n_trips = 0;
	int cur_agent_id = 1;


	// Network configuration

	Network net = Data::getInstance()->getNetwork();
	/*
	double proc_min_x = continuous_space->bounds().origin().getX();
	double proc_max_x = proc_min_x + continuous_space->bounds().extents().getX();
	double proc_min_y = continuous_space->bounds().origin().getY();
	double proc_max_y = proc_min_y + continuous_space->bounds().extents().getY();
	*/
	
	double proc_min_x = continuous_space->dimensions().origin().getX();
        double proc_max_x = proc_min_x + continuous_space->dimensions().extents().getX();
	double proc_min_y = continuous_space->dimensions().origin().getY();
        double proc_max_y = proc_min_y + continuous_space->dimensions().extents().getY();

	cout << "bounding box for process " << this->_proc << ": [" << proc_min_x << "," << proc_min_y << "] x [" << proc_max_x << "," << proc_max_y << "]" << endl;

	// Reading data file

	string filename = this->_props.getProperty("file.trips_transims");      // path to the file
	ifstream file(filename.c_str(), ios::in);                               // opening data file
	string a_line;                                                          // a line of data

	if (file) {
		int prev_agent_id = -42;                                            // initial agent id, yes -42!
		int prev_agent_hh_id = -1;                                          // initial agent household it
		float end_time_previous_trip = 0.0;                                 // end time of previous trip
		vector<Trip> trips;                                                 // vector containing trips
		getline(file, a_line);                                              // skipping header line
		while (getline(file, a_line)) {

			// extracting data
			auto data            = split<string>(a_line,",");
			auto mode_trip       = boost::lexical_cast<int>(data[4]);
			auto start_time_trip = boost::lexical_cast<float>(data[6]);
			auto orig_trip       = boost::lexical_cast<std::string>(data[7]);
			auto end_time_trip   = boost::lexical_cast<float>(data[8]);
			auto dest_trip       = boost::lexical_cast<std::string>(data[9]);
			auto agent_hh_id     = boost::lexical_cast<int>(data[0]);
			auto agent_id        = boost::lexical_cast<int>(data[1]);

			cur_agent_id = agent_hh_id * 100 + agent_id;

			// initial agent id
			if ( prev_agent_id == -42 ) {
				prev_agent_id = agent_id;
				prev_agent_hh_id = agent_hh_id;
			}

			// finding origin and destination node
			orig_trip = Data::getInstance()->getMapActLocNodes().at(orig_trip);
			dest_trip = Data::getInstance()->getMapActLocNodes().at(dest_trip);

			// current trip generation
			Trip curTrip(orig_trip, dest_trip, start_time_trip);

			// adding current trip to the other ones if not reading a new agent
			if( agent_id == prev_agent_id && agent_hh_id == prev_agent_hh_id ) {

				// ... correcting starting time if required
				if (this->_props.getProperty("par.correct_start_time").compare("y") == 0 )  {
					if( start_time_trip < end_time_previous_trip ) curTrip.setStartingTime(end_time_previous_trip);
				}

				// ... checking if agent actually moves and its mode is car or taxi
				if( orig_trip != dest_trip && ( mode_trip == static_cast<int>(Mode_transims::CAR_DRIVER) || mode_trip == static_cast<int>(Mode_transims::TAXI) ) ) {
					trips.push_back(curTrip);
					++n_trips;
				}

				// ... updating previous trip end time
				end_time_previous_trip = end_time_trip;

			}
			// ... else adding the previous agent to the context and starting the creating of a new one
			else {

				// checking if agent is actually traveling
				if( trips.size() > 0 ) {

					// checking if agent belongs to current process
					if( isInLocalBounds( net.getNodes().at(trips[0].getIdOrigin()).getX(), net.getNodes().at(trips[0].getIdOrigin()).getY()) ) {

						// previous agent generation
						//cur_agent_id++;
						AgentId agent_id_repast(cur_agent_id, this->_proc, MODEL_AGENT_IND_TYPE, this->_proc);
						Individual * newAgent = new Individual(agent_id_repast, trips);
						agents->addAgent(newAgent);

					}

				}

				// new agent's trips initialization
				trips.clear();                                                               // reseting trips
				prev_agent_id    = agent_id;                                                 // new agent id
				prev_agent_hh_id = agent_hh_id;                                              // new agent household id
				if( orig_trip != dest_trip
						&& ( mode_trip == static_cast<int>(Mode_transims::CAR_DRIVER)
								|| mode_trip == static_cast<int>(Mode_transims::TAXI) ) ) {

					trips.push_back(curTrip);                                                // first trip of the new agent
					++n_trips;

				}

				// reseting end time of previous trip if correcting starting time is required
				if (this->_props.getProperty("par.correct_start_time").compare("y") == 0 ) {
					end_time_previous_trip = end_time_trip;
				}

			}

		}

		// Adding the last agent to the context of the right process
		if( trips.size() > 0 ) {
			if( isInLocalBounds(net.getNodes().at(trips[0].getIdOrigin()).getX(), net.getNodes().at(trips[0].getIdOrigin()).getY() ) == true ) {

				//cur_agent_id++;
				AgentId agent_id_repast(cur_agent_id, this->_proc, MODEL_AGENT_IND_TYPE, this->_proc);                // adding last agent
				Individual * newAgent = new Individual(agent_id_repast, trips);
				agents->addAgent(newAgent);

			}
		}

		file.close();

	} else {
		cerr << "Could not open " << filename << endl;
		throw "Error opening transims input file";
	}

	return n_trips;

}


unsigned int Model::init_matsim() {

	std::hash<std::string> hasher;

	if( this->_proc == 0 ) cout << "... initialization agents (from MATSim input format) !" << endl;

	unsigned n_trips = 0;

	// Loading XML file containing the network data.
	string filename = this->_props.getProperty("file.trips_matsim");
	XMLDocument doc(filename.c_str());
	doc.loadFile(filename.c_str());

	// Loop on the individuals
	XMLElement * ele = doc.FirstChildElement("plans")->FirstChildElement("person");
	while (ele) {

		// Agent id
		bool add_agent = true;
		const XMLAttribute * attr = ele->FirstAttribute();
		std::string id_str = attr->StringValue();
		int id = (int)hasher(id_str);

		vector<Trip> trips;

		// First activity: leaving home ---------------------------------------

		XMLElement * ele_act = ele->FirstChildElement("plan")->FirstChildElement("act");

		// reading data from XML input file
		//double  house_x          = ele_act->DoubleAttribute("x");
		//double  house_y          = ele_act->DoubleAttribute("y");
		string act_end_time_str  = ele_act->StringAttribute("end_time");
		string act_node_id_start = ele_act->StringAttribute("node_id");
		float  act_end_time_prev = timeToSec(act_end_time_str);
		string house_node_id     = act_node_id_start;

		double house_x = _network.getNodes().at(house_node_id).getX();
		double house_y = _network.getNodes().at(house_node_id).getY();

		//cout << "House id " << house_node_id << " at coordinates (" << house_x << "," << house_y << ")" << endl;
		//cout << " => original coordinates house id " << house_node_id << " at coordinates (" << _network.getNodes().at(house_node_id).getXData() << "," << _network.getNodes().at(house_node_id).getYData() << ")" << endl;

		if( isInLocalBounds(house_x, house_y) == true ) {

			// Loop on the current individual remaining activities-----------------
			ele_act = ele_act->NextSiblingElement("act");
			while ( ele_act->NextSiblingElement("act") ) {

				// reading data from XML
				string act_end_time_str = ele_act->StringAttribute("end_time");
				string act_node_id_dest = ele_act->StringAttribute("node_id");

				// construct trip and pushing it to the set of trip performed by the individual
				if( act_node_id_start != act_node_id_dest ) {
					Trip cur_trip(act_node_id_start, act_node_id_dest, act_end_time_prev);
					trips.push_back(cur_trip);
					++n_trips;
				}
				else {
					add_agent = false;
					//cerr << "Agent id " << id_str << " trips data error!" << endl;
					//cerr << "act_node_id_start = " << act_node_id_start << " - act_node_id_dest =  " << act_node_id_dest << endl;
				}

				// next starting time = end time of current activity
				act_end_time_prev = timeToSec(act_end_time_str);

				// next starting position = current activity location
				act_node_id_start = act_node_id_dest;

				// next activity
				ele_act = ele_act->NextSiblingElement("act");

			}

			// Last trip: return to home ------------------------------------------

			Trip trip_to_home(act_node_id_start,house_node_id,act_end_time_prev);
			if( act_node_id_start != house_node_id ) {
				trips.push_back(trip_to_home);
				++n_trips;
			}
			else {

				add_agent = false;
				//cerr << "Agent id " << id_str << " trips data error!" << endl;
			}

			// Agent generation ---------------------------------------------------
			if(add_agent) {
				AgentId agent_id_repast(id, this->_proc, MODEL_AGENT_IND_TYPE, this->_proc);
				Individual * newAgent = new Individual(agent_id_repast, trips);
				agents->addAgent(newAgent);
			}

		}

		ele = ele->NextSiblingElement("person");

	}

	return n_trips;

}


void Model::compute_initial_paths() {

	// Loop over every local agent belonging to the SharedContext

	auto it_cur = (*agents).localBegin();
	while ( it_cur != (*agents).localEnd() ) {

		// generation of the agent location in the continuous space
		(*it_cur)->setX( _network.getNodes().at( (*it_cur)->getTrips()[0].getIdOrigin()).getX() );
		(*it_cur)->setY( _network.getNodes().at( (*it_cur)->getTrips()[0].getIdOrigin()).getY() );
		repast::Point<double> initialLocation( (*it_cur)->getX(), (*it_cur)->getY() );

		// moving it to the location
		this->continuous_space->moveTo( (*it_cur)->getId(), initialLocation );

		// computation of the initial shortest path
		string id_origin = (*it_cur)->getTrips()[0].getIdOrigin();
		string id_destin = (*it_cur)->getTrips()[0].getIdDestination();

		//cout << "compute initial path for individual " << (*it_cur)->getId().id() << endl;

		vector<std::string> path;
		if( _look_up_paths.count(id_origin) == 1 && _look_up_paths[id_origin].count(id_destin) == 1 ) {
			path = _look_up_paths[id_origin][id_destin];
		}
		else {
			path = _network.computePathAStar( id_origin, id_destin );
			_look_up_paths[id_origin][id_destin] = path;
		}
		(*it_cur)->setPath( path );

		// moving to next agent
		it_cur++;

	}

	cout << "End computation initial trips by proc " << _proc << "(" << agents->size() << " agents)" << endl;

}


void Model::init_agents_strategies() {

	float prop_strat_agents = boost::lexical_cast<float>( this->_props.getProperty("par.prop_strategic_agents") );
	unsigned int n_strat_agents_local = 0;
	unsigned int n_strat_agents_total = 0;

	auto it_cur = (*agents).localBegin();          // initial individual agent
	while ( it_cur != (*agents).localEnd() ) {

		// testing whether this agent should have a strategy

		float rnd_draw = RandomGenerators::getInstance()->fast_unif.fl();
		if( rnd_draw < prop_strat_agents ) {

			// and randomly draw one from the set of possible strategies
			(*it_cur)->setStrategy(Data::getInstance()->getOneStrategy());
			++n_strat_agents_local;

		}

		// moving to next agent
		it_cur++;

	}

	// Computing total number of agents across every process
	boost::mpi::all_reduce(*RepastProcess::instance()->communicator(), n_strat_agents_local, n_strat_agents_total, std::plus<unsigned int>());
	if( this->_proc == 0 ) {
		cout << "Strategic agents in the simulation: " << n_strat_agents_total << endl;
	}
	_props.putProperty("number.strat_agents",n_strat_agents_total);

}


void Model::synch_agents() {

	this->continuous_space->balance(_map_agents_to_move_process);
	repast::RepastProcess::instance()->synchronizeAgentStatus<Individual,IndividualPackage,Model,Model,Model>(*this->agents, *this, *this, *this);
	//repast::RepastProcess::instance()->synchronizeProjectionInfo<Individual,IndividualPackage,Model,Model,Model>(*this->agents, *this, *this, *this); 

}


void Model::initSchedule() {

	// Initialize the scheduler

	ScheduleRunner & runner = RepastProcess::instance()->getScheduleRunner();

	// Call the step method on the Model every tick

	runner.scheduleEvent(1,     1, Schedule::FunctorPtr(new MethodFunctor<Model>(this, &Model::step)));
	runner.scheduleEvent(1.1, 100, Schedule::FunctorPtr(new MethodFunctor<Model>(this, &Model::checkStop)));

	// Schedule the data recording and writing

	runner.scheduleEndEvent(Schedule::FunctorPtr(new MethodFunctor<DataSet>(_data_collection, &DataSet::write)));
	runner.scheduleEndEvent(Schedule::FunctorPtr(new MethodFunctor<Model>(this, &Model::writeLinksState)));
	runner.scheduleEndEvent(Schedule::FunctorPtr(new MethodFunctor<Model>(this, &Model::writeTripsStartingTimes)));
	runner.scheduleEndEvent(Schedule::FunctorPtr(new MethodFunctor<Model>(this, &Model::writeAgentFitness)));

}


void Model::providePackage(Individual * agent, std::vector<IndividualPackage>& out) {

	AgentId id = agent->getId();
	IndividualPackage package = { id.id(), id.startingRank(), id.agentType(), id.currentRank(),
			agent->getTrips(), agent->getX(), agent->getY(), agent->getRemainingTime(),
			agent->getStrategy(), agent->getPath(), agent->isEnRoute(), agent->isAtNode(),
			agent->getCurLink(), agent->getSize(), agent->getCurTripDurationTheo(),
			agent->getNPathPerformed(), agent->getNLinkInPath()};
	out.push_back(package);

}


Individual * Model::createAgent(IndividualPackage package) {

	repast::AgentId id(package.id, package.init_proc, MODEL_AGENT_IND_TYPE, package.cur_proc);
	return new Individual(id, package.trips, package.x, package.y, package.remaining_time,
			package.strategy, package.path, package.en_route, package.at_node,
			package.cur_link, package.size, package.cur_trip_duration_theo,
			package.n_path_performed, package.n_link_in_path);

}


void Model::provideContent(repast::AgentRequest req, std::vector<IndividualPackage>& out) {

	std::vector<AgentId> ids = req.requestedAgents();
	for( unsigned int i = 0; i < ids.size(); i++ ) {
		providePackage(this->agents->getAgent(ids[i]),out);
	}

}


void Model::updateAgent(IndividualPackage package) {

	repast::AgentId id(package.id, package.init_proc, MODEL_AGENT_IND_TYPE);
	Individual * agent = this->agents->getAgent(id);

	agent->getId().currentRank(package.cur_proc);
	agent->setTrips(package.trips);
	agent->setX(package.x);
	agent->setY(package.y);
	agent->setRemainingTime(package.remaining_time);
	agent->setStrategy(package.strategy);
	agent->setPath(package.path);
	agent->setEnRoute(package.en_route);
	agent->setAtNode(package.at_node);
	agent->setCurLink(package.cur_link);
	agent->setSize(package.size);
	agent->setCurTripDurationTheo(package.cur_trip_duration_theo);
	agent->setNPathPerformed(package.n_path_performed);
	agent->setNLinkInPath(package.n_link_in_path);

}


void Model::step() {

  //boost::mpi::communicator* comm = RepastProcess::instance()->getCommunicator();
	_map_agents_to_move_process.clear();

	// Determining next time step
	/*
	static float global_remaining_time = -1.0f;

	if( global_remaining_time < 0.0f ) {

		// Finding the agent with local lowest remaining time before next event
		float local_min_remaining_time = std::numeric_limits<float>::max();
		repast::SharedContext<Individual>::const_local_iterator it = (*agents).localBegin();
		while ( it != (*agents).localEnd() ) {
			if( (*it)->getRemainingTime() < local_min_remaining_time ) {
				local_min_remaining_time = (*it)->getRemainingTime();
			}
			it++;
		}

		// Determining the global minimum remaining time across every process
		boost::mpi::all_reduce(*comm, local_min_remaining_time, global_remaining_time, boost::mpi::minimum<float>());

	}
	else {
		global_remaining_time = 1.0f;
	}

	// Updating simulation time accordingly

#ifdef DEBUGSIM
	if ( global_remaining_time < 0.0f ) cerr << "ERROR: GLOBAL_REMAINING_TIME < 0!" << endl;
#endif
	
	this->increaseTime( global_remaining_time );
	*/
	float global_remaining_time = 1.0f;
	this->increaseTime(1.0f);

	// Determining time interval for aggregate data recording

	int cur_time_interval = (int)floorf( this->_time / ( 60.0f * (float)_time_interval_records ) );

	// consolidation of current time interval if simulation time > 24h
	if( this->_time > 86400.0f ) {
		const static int n_int = 1440 / _time_interval_records;
		cur_time_interval = cur_time_interval % n_int;
	}

	// Main loop: traffic dynamic

	auto it_cur = (*agents).localBegin();
	while( it_cur != (*agents).localEnd() ) {

		bool remove_agent = false;

		//	(*it_cur)->print();

		// Decreasing agent remaining time till next event

		(*it_cur)->decreaseRemainingTime( global_remaining_time );

		// Moving active agent to its next event

		if( (*it_cur)->getRemainingTime() <= this->_time_tolerance ) {

			// Agent is at a node -> preparing to move to the next one
			if( (*it_cur)->isAtNode() == true) {

				// agent is starting a new trip
				if( (*it_cur)->isEnRoute() == false ) {
					(*it_cur)->setEnRoute(true);
					this->_total_moving_agents.incrementData();
					this->_trips_starting_time.push_back(this->_time);

				}

				// Setting the agent to move, determining its next planned link and moving to it
				(*it_cur)->setAtNode(false);
				std::string id_next_link = (*it_cur)->getNextLinkAndRemove();
				(*it_cur)->setCurLink(id_next_link);

				//cout << "DEBUG: Agent at node " <<  _network.getLinks().at( (*it_cur)->getCurLink() ).getStartNodeId() << endl;
				//cout << "DEBUG:    next link " << id_next_link << endl;

				// Determining and applying strategy: agent stays on the link or decide to take an other one
				if  ( (*it_cur)->getStrategy().isOptimized()    == true &&
					  (*it_cur)->isRerouting( _network, _time ) == true ) {

					// Incrementing the number of rerouting
					this->_total_rerouting.incrementData();

					// Determining new path by avoiding the next link initially planned
					std::string cur_node_id  = _network.getLinks().at( (*it_cur)->getCurLink() ).getStartNodeId();
					if( _network.getNodes().at(cur_node_id).getLinksOutId().size() > 1 ) {

						std::string dest_node_id = (*it_cur)->getTrips().front().getIdDestination();
						vector<std::string> new_path = _network.computePath(cur_node_id, dest_node_id, id_next_link);
						(*it_cur)->setPath( new_path );
						id_next_link = (*it_cur)->getNextLinkAndRemove();
						(*it_cur)->setCurLink(id_next_link);
						//cout << "DEBUG:    NEW next link " << id_next_link << endl;
					}

				}

				// Updating agent theoretical travel time
				(*it_cur)->increaseTripDurationTheo( _network.getLinks().at(id_next_link).getFreeFlowTime() );

				// Adding the agent to the next link it takes and computing the time required to travel
				(*it_cur)->setRemainingTime( this->_network.getLinks().at(id_next_link).timeOnLink() );
				_network.incrementAgentOnLink(id_next_link);

				// Link densities recording
				this->_links_load_over_time[id_next_link][cur_time_interval]++;

				// Recording the data for Moves:
				// agent_id | link id | time entering the link | time on link | path id | link on path
				this->writeOutputsMoves((*it_cur)->getId().id(), id_next_link, this->_time,
						                (*it_cur)->getRemainingTime(), (*it_cur)->getNPathPerformed(), (*it_cur)->getNLinkInPath());


			}
			// Agent reaches the next node on its way
			else {

				// Moving to next node if not the final one of current trip
				if( (*it_cur)->getPath().size() > 0 ) {

					// Decrement number of agent on previous link
					std::string id_prev_link = (*it_cur)->getCurLink();
					_network.decrementAgentOnLink(id_prev_link);

					// Moving to new node, i.e. destination of previous link
					std::string id_new_node = _network.getLinks().at(id_prev_link).getEndNodeId();
					Node     newNode     = _network.getNodes().at(id_new_node);
					(*it_cur)->setX(newNode.getX());
					(*it_cur)->setY(newNode.getY());

					// Stopping at the new node
					(*it_cur)->setAtNode(true);

					// Moving agent in the continuous space
					repast::Point<double> loc( (*it_cur)->getX(), (*it_cur)->getY() );
					this->continuous_space->moveTo( (*it_cur)->getId(), loc );

					if( isInLocalBounds((*it_cur)->getX(), (*it_cur)->getY()) == false ) {
						_map_agents_to_move_process[(*it_cur)->getId()] = _map_node_process[id_new_node];
					}


					

				}

				// Agent reached end of current trip
				else {

					// Computing its fitness
					float start_time_trip   = (*it_cur)->getTrips().front().getStartingTime();
					float trip_duration_teo = (*it_cur)->getCurTripDurationTheo();
					float trip_duration_sim = this->_time - start_time_trip;

					// update fitness

					float fitness  = trip_duration_teo / trip_duration_sim;
					if ( this->_map_agent_fitness.find( (*it_cur)->getId().id() ) == this->_map_agent_fitness.end() ) {
						_map_agent_fitness[(*it_cur)->getId().id()] = fitness;

					} else {
						_map_agent_fitness[(*it_cur)->getId().id()] = (_map_agent_fitness[(*it_cur)->getId().id()] + fitness) * 0.5;
					}

					// Incrementing the number of trips performed
					this->_total_trips_performed.incrementData();

					// Decrementing the number of moving agent
					this->_total_moving_agents.decrementData();

					// Decrementing the number of agent on previous link
					_network.decrementAgentOnLink( (*it_cur)->getCurLink() );

					//Agent arrived at destination -> preparing next trip if any
					if( (*it_cur)->getTrips().size() > 1 ) {

						// Setting next trip
						(*it_cur)->setNextTrip(_network, this->_time);

						// Moving agent in the continuous space
						repast::Point<double> loc( (*it_cur)->getX(), (*it_cur)->getY() );
						this->continuous_space->moveTo( (*it_cur)->getId(), loc );


						if( isInLocalBounds( (*it_cur)->getX(), (*it_cur)->getY()) == false ) {
						//if(continuous_space->bounds().contains(loc) == false ) {
							_map_agents_to_move_process[(*it_cur)->getId()] = _map_node_process[ (*it_cur)->getTrips().front().getIdOrigin() ];
						}

					}

					//Agent arrived at final destination -> mark it for removing it from the simulation
					else {
					  //cout << "REMOVING AGENT!" << endl;
						remove_agent = true;


					}

				}

			}

		}

		// Moving to next agent

		Individual* agt = (*it_cur).get();
		AgentId agt_id = (*it_cur)->getId();

		//if (remove_agent == true) {
		//	cout << "ID TO REMOVE: " << agt_id.id() << endl;
		//}

		it_cur++;

		if (remove_agent == true) {
		//		cout << "ID BEING REMOVED: " << agt_id.id() << endl;
			this->continuous_space->removeAgent( agt );
			this->agents->removeAgent( agt_id );
		}



	}

	// Snapshot of the links state

	if( (int)floorf(_time) % (_time_interval_records_snapshots * 60 ) == 0.0 ) {

		unsigned int interval = (int)floorf(_time) / (_time_interval_records_snapshots * 60 );

		if( _time > 86400.0f) {
			const static unsigned int n_int_snapshot = 1440 / _time_interval_records_snapshots;
			interval = interval % n_int_snapshot;
		}

		auto it = (*agents).localBegin();
		while( it != (*agents).localEnd() ) {
			if( (*it)->isEnRoute() == true ) _links_state_snapshot[(*it)->getCurLink()][interval]++;
			it++;
		}

	}

	// Recording aggregate data

	this->_total_agents.setData(this->agents->size());
	this->_data_collection->record();

	// Synchronizing agents states (eventually moving them to a new process)

	this->synch_agents();

}


void Model::writeLinksState() {

	boost::mpi::communicator* comm = RepastProcess::instance()->getCommunicator();
	unsigned int n_time_intervals = 1440 / this->_time_interval_records;                          // 1440 minutes in a day
	unsigned int n_time_intervals_snapshot = 1440 / this->_time_interval_records_snapshots;

	// Opening output files

	ofstream file_output_flows;
	ofstream file_output_props;
	ofstream file_output_flows_snapshot;
	ofstream file_output_props_snapshot;


	if( this->_proc == 0 ) {

		file_output_flows.open("../output/links_flows.csv", ios::out);
		file_output_props.open("../output/links_saturation.csv", ios::out);
		file_output_flows_snapshot.open("../output/links_flows_snapshot.csv", ios::out);
		file_output_props_snapshot.open("../output/links_saturation_snapshot.csv", ios::out);

		// Writing header

		file_output_flows << "LINK";
		file_output_props << "LINK";
		file_output_flows_snapshot << "LINK";
		file_output_props_snapshot << "LINK";

		// ... aggregates records
		for( unsigned int i = 0; i < n_time_intervals; ++i ) {
			file_output_flows << ";t_"<< i;
			file_output_props << ";t_"<< i;
		}

		// ... snapshots records
		for( unsigned int i = 0; i < n_time_intervals_snapshot; ++i ) {
			file_output_flows_snapshot << ";t" << i;
			file_output_props_snapshot << ";t" << i;
		}

		file_output_flows << endl;
		file_output_props << endl;
		file_output_flows_snapshot << endl;
		file_output_props_snapshot << endl;

		// Closing the file

		file_output_flows.close();
		file_output_props.close();
		file_output_flows_snapshot.close();
		file_output_props_snapshot.close();

	}

	// Loop over the process

	for( int p = 0; p < comm->size(); p++ ) {

		// only one process at a time can write in the output files
		comm->barrier();
		if( comm->rank() == p ) {

			file_output_flows.open("../output/links_flows.csv",ios::app);
			file_output_props.open("../output/links_saturation.csv", ios::app);

			for( auto lnk : this->_links_load_over_time ) {

				file_output_flows << lnk.first;
				file_output_props << lnk.first;

				for( unsigned int t = 0; t < lnk.second.capacity(); ++t ) {
					file_output_flows << ";" << lnk.second.at(t);
					file_output_props << ";" << (float)lnk.second.at(t) / this->_network.getLinks().at(lnk.first).getCapacity();
				}

				file_output_flows << endl;
				file_output_props << endl;

			}

			file_output_flows_snapshot.open("../output/links_flows_snapshot.csv", ios::app);
			file_output_props_snapshot.open("../output/links_saturation_snapshot.csv", ios::app);

			for( auto lnk : this->_links_state_snapshot ) {

				file_output_flows_snapshot << lnk.first;
				file_output_props_snapshot << lnk.first;

				for( unsigned int t = 0; t < lnk.second.capacity(); ++t ) {
					file_output_flows_snapshot << ";" << lnk.second.at(t);
					file_output_props_snapshot << ";" << (float)lnk.second.at(t) / this->_network.getLinks().at(lnk.first).getCapacity();
				}

				file_output_flows_snapshot << endl;
				file_output_props_snapshot << endl;

			}

			file_output_flows_snapshot.close();
			file_output_props_snapshot.close();

		}

	}

}


void Model::writeTripsStartingTimes() {

	boost::mpi::communicator* comm = RepastProcess::instance()->getCommunicator();

	// Proces 0 (root) responsible for gathering data from every other process

	if(this->_proc == 0 ) {

		vector<vector<float> > result_gather;
		vector<float> result_concatenate;

		// root process gathering data
		boost::mpi::gather(*comm, _trips_starting_time, result_gather, 0);

		// concatening results
		for( unsigned int i = 0; i < result_gather.size(); i++ ) {
			result_concatenate.insert( result_concatenate.end(), result_gather[i].begin(), result_gather[i].end() );
		}

		// sorting results
		boost::sort(result_concatenate);

		ofstream file_output("../output/starting_times.csv", ios::out);
		file_output << "STARTING_TIME" << endl;

		for( unsigned int i = 0; i < result_concatenate.size(); i++ ) {
			file_output << result_concatenate[i] << endl;
		}

		file_output.close();

	}

	// Processes sending their data to root process

	else {

		boost::mpi::gather(*comm, _trips_starting_time, 0);

	}

}


void Model::checkStop() {

	boost::mpi::communicator* comm = RepastProcess::instance()->getCommunicator();
	int remaining_agents_local = this->agents->size();
	int remaining_agents_total = 0;

	boost::mpi::all_reduce(*comm, remaining_agents_local, remaining_agents_total, std::plus<int>() );

#ifdef DEBUGSIM
	int total_moving_agents_local = this->_total_moving_agents.getData();
	int total_moving_agents_total = 0;
	boost::mpi::all_reduce(*comm, total_moving_agents_local, total_moving_agents_total, std::plus<int>() );
	int total_trips_done_local = this->_total_trips_performed.getData();
	int total_trips_donc_total = 0;
	boost::mpi::all_reduce(*comm, total_trips_done_local, total_trips_donc_total, std::plus<int>() );
#endif

	if ( this->_proc == 0 ) {
		cout << "Remaining agents = " << remaining_agents_total;	
		cout << "; time = " << this->_time;
#ifdef DEBUGSIM
		cout << "; agents moving = " << total_moving_agents_total;
		cout << "; trips done = " << total_trips_donc_total;
#endif
		cout << endl;
	}

	if ( remaining_agents_total == 0 ) RepastProcess::instance()->getScheduleRunner().stop();

}


void Model::constructMapNodeProcess() {

	boost::mpi::communicator* comm = RepastProcess::instance()->getCommunicator();

	vector<map<std::string, int>> result_gather;

	// gathering data from every process
	boost::mpi::all_gather(*comm, _map_node_process, result_gather);

	// updating map with gathered data
	for( unsigned int i = 0; i < result_gather.size(); i++ ) {
		_map_node_process.insert( result_gather[i].begin(), result_gather[i].end() );
	}

#ifdef DEBUGSIM
	if( _proc == 0 ) {

	  ofstream file_output_map_node_process;
	  file_output_map_node_process.open("../logs/dump_map_node_process.csv", ios::out);
	  file_output_map_node_process << "NODE;PROC" << endl;
	  
	  for( auto nde : _map_node_process ) {
	    cout << "-> NODE " << nde.first << " BELONGS TO PROCESS " << nde.second << endl;
	    file_output_map_node_process << nde.first << ";" << nde.second << endl;
	  }

	  file_output_map_node_process.close();
		
	}
#endif

}


void Model::writeAgentFitness() {

	boost::mpi::communicator* comm = RepastProcess::instance()->getCommunicator();

	ofstream file_output_fitness;
	string file_out = "../output/agents_fitness.csv";

	// Opening output files

	if( this->_proc == 0 ) {

		file_output_fitness.open(file_out.c_str(), ios::out);

		// Writing header

		file_output_fitness << "AGENT ID;FITNESS" << endl;

		// Closing the file

		file_output_fitness.close();

	}

	// Loop over the process

	for( int p = 0; p < comm->size(); p++ ) {

		// only one process at a time can write in the output files
		comm->barrier();
		if( comm->rank() == p ) {

			file_output_fitness.open(file_out.c_str(),ios::app);

			for( const auto &agt : this->_map_agent_fitness ) {

				file_output_fitness << agt.first << ";" << agt.second << endl;

			}

			file_output_fitness.close();

		}

	}


}

bool Model::isInLocalBounds(double x, double y) {

        double proc_min_x = continuous_space->dimensions().origin().getX();
        double proc_max_x = proc_min_x + continuous_space->dimensions().extents().getX();
	double proc_min_y = continuous_space->dimensions().origin().getY();
        double proc_max_y = proc_min_y + continuous_space->dimensions().extents().getY();
		
	if( proc_min_x <= x && proc_max_x >= x && proc_min_y <= y && proc_max_y >= y ) return true;

	return false;

}

void Model::writeOutputsMoves(int id, std::string link_id, float time_entering_link, float time_on_link, int path_id, int link_id_on_path) {
	// todo: replace that by a database!

	// opening output file
	ofstream file_output_moves;
	string file_out = "../output/moves_proc_" + to_string(_proc) + ".csv";
	file_output_moves.open(file_out.c_str(), ios::app);

	// writing the data
	file_output_moves << id << ";" << link_id << ";" << time_entering_link << ";" << time_on_link << ";" <<  path_id << ";" << link_id_on_path << endl;

	// closing the file
	file_output_moves.close();

}
