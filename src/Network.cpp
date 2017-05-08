/****************************************************************
 * NETWORK.CPP
 *
 * This file contains all the definitions of the methods of
 * network.hpp (see this file for methods' documentation)
 *
 * Authors: J. Barthelemy and L. Hollaert
 * Date   : 26 jun 2013
 ****************************************************************/

#include "../include/Network.hpp"
#include "repast_hpc/RepastProcess.h"

using namespace std;
using namespace repast;

// Insert a node in the network
void Network::addNode(Node aNode) {

  this->_Nodes.insert(make_pair(aNode.getId(), aNode));

  auto x  = aNode.getX();
  auto y  = aNode.getY();

  if( x < this->min_x ) this->min_x = x;
  if( x > this->max_x ) this->max_x = x;
  if( y < this->min_y ) this->min_y = y;
  if( y > this->max_y ) this->max_y = y;

}

// Insert a link in the network
void Network::addLink(Link aLink) {

  this->_Links.insert(make_pair(aLink.getId(), aLink));

}

vector<std::string> Network::computePath(std::string source_id, std::string dest_id, bool fastest) {

	vector<std::string> result;                                             // shortest path between source_id and dest_id given link by link
	map<std::string, std::string> prec;                                            // map giving the precedent node on the shortest path
	std::string curr_node = source_id;                                      // current node found by Dijkstra algorithm

	// Initialization

	FibonacciHeap<std::string,float> Q;                                     // Fibonacci heap
	std::map<std::string, FibonacciHeapNode<std::string,float>* > Q_nodes;  // array of pointer to the nodes of the F-heap
	std::set<std::string> Q_marked;                                         // set of marked nodes

	// ... every nodes' key are set to a large number
	map<std::string, Node>::iterator itr;
	for (itr = this->_Nodes.begin(); itr != this->_Nodes.end(); itr++) {
		Q_nodes.insert( std::pair< std::string, FibonacciHeapNode<std::string,float>* >( itr->first, Q.insert(itr->first,std::numeric_limits<float>::max()) ) );
	}

	// ... except root node key set to 0
	Q_nodes[curr_node] = Q.insert(curr_node, 0.0f);
	Q_marked.insert(curr_node);

	// Dijkstra main loop
	while( curr_node != dest_id ) {

		// ... extracting the node with minimum key (i.e. distance from source)

		curr_node   = Q.minimum()->data();         // id
		float d     = Q.minimum()->key();          // distance
		Node * i    = &this->_Nodes.at(curr_node); // get a pointer to the minimum node
		Q_marked.insert(curr_node);                // mark the node

		// ... updating the distances between starting node and node i's sink nodes if necessary

		for (unsigned int j = 0; j < i->getLinksOutId().size(); j++) {

			std::string id = this->_Links[i->getLinksOutId().at(j)].getEndNodeId();

			// ... if node not already marked
			if ( Q_marked.find(id) == Q_marked.end() ) {

				float key_j = Q_nodes[id]->key();                                           // current weight
				float w_ij;                                                                 // new weight
				if ( fastest == true ) w_ij  = _Links[i->getLinksOutId().at(j)].getFreeFlowTime() + d;
				else                   w_ij  = _Links[i->getLinksOutId().at(j)].getLength() + d;

				// ... update the weight if necessary
				if ( w_ij < key_j ) {
					Q.decreaseKey(Q_nodes[id],w_ij);
					prec[id] = i->getLinksOutId().at(j);
				}

			}

		}

		// ... removing the min node from the heap
		Q.deletemin();

	}

	// reconstructing minimal path

	do {

		result.push_back(prec.at(curr_node));
		curr_node = this->_Links.at(prec.at(curr_node)).getStartNodeId();

	} while( curr_node != source_id );

	return result;

}


vector<std::string> Network::computePath(std::string source_id, std::string dest_id, std::string link_id_to_avoid, bool fastest) {

	// Setting the link cost to a large number (in order for the agent to try to avoid then) and saving the original value

	float init_data_link;
	if( fastest == true ) {
		init_data_link = _Links.at(link_id_to_avoid).getFreeFlowTime();
		_Links.at(link_id_to_avoid).setFreeFlowTime(std::numeric_limits<float>::max() * 0.5f );
	}
	else {
		init_data_link = _Links.at(link_id_to_avoid).getLength();
		_Links.at(link_id_to_avoid).setLength(std::numeric_limits<float>::max() * 0.5f );
	}

	// Compute new path, trying to avoid the link

	vector<std::string> result = computePathAStar(source_id, dest_id, fastest);

	// Setting the cost to the original values
	
	if( fastest == true ) {
	  _Links.at(link_id_to_avoid).setFreeFlowTime(init_data_link);
	}
	else {
	  _Links.at(link_id_to_avoid).setLength(init_data_link);
	}

	return result;

}

vector<std::string> Network::computePathAStar(std::string source_id, std::string dest_id, bool fastest) {

	vector<std::string> result;                                       // shortest path between source_id and dest_id (links)
	map<std::string, std::string> prec;                                      // map giving the precedent node on the shortest path
	std::string curr_node = source_id;                                // current node found by Dijkstra algorithm


	if( source_id == dest_id ) {
		return result;
	}

	// Initialization

	FibonacciHeap<std::string,float> Q_open;                          // Fibonacci heap for the open set of tentative nodes
	                                                           //  ... key is f_score = true dist + euclidian distance
	std::map<std::string, FibonacciHeapNode<std::string,float>* > Q_nodes;   // array of pointer to the nodes of the F-heap
	std::set<std::string> Q_closed;                                   // closed set containing nodes already evaluated
	std::map<std::string,float> g_score;                              // true cost between source_id and the other nodes

	// ... except root node key set to 0 and mark it as a possible node
	Q_nodes[curr_node] = Q_open.insert(curr_node, euclidian_distance(source_id,dest_id) );
	g_score[curr_node] = 0.0f;

	// Dijkstra main loop
	while( curr_node != dest_id ) {

		// ... extracting the node with minimum key (i.e. distance from source) in the open set

		curr_node   = Q_open.minimum()->data();         // id
		Node * i    = &this->_Nodes.at(curr_node);      // get a pointer to the minimum node
		Q_closed.insert(curr_node);                     // mark the node, i.e. include it in the closed set
		float d = g_score[curr_node];

		// ... removing the min node from the heap
		Q_open.deletemin();

		// ... updating the distances between starting node and node i's sink nodes if necessary

		for (unsigned int j = 0; j < i->getLinksOutId().size(); j++) {

			std::string id = this->_Links[i->getLinksOutId().at(j)].getEndNodeId();

			// ... if node not already marked, i.e not in closed set
			if ( Q_closed.find(id) == Q_closed.end() ) {

				float w_ij;                                                                 // new possible weight
				if ( fastest == true ) w_ij  = _Links[i->getLinksOutId().at(j)].getFreeFlowTime() + d;
				else                   w_ij  = _Links[i->getLinksOutId().at(j)].getLength()       + d;

				if(Q_nodes.count(id) == 0 || w_ij < g_score[id] ) {
					prec[id] = i->getLinksOutId().at(j);
					g_score[id] = w_ij;
					float f_score = w_ij + euclidian_distance(id,dest_id);
					if( Q_nodes.count(id) > 0 ) Q_open.decreaseKey(Q_nodes[id],f_score);
					else                        Q_nodes[id] = Q_open.insert(id, f_score);
				}

			}

		}


	}

	// reconstructing minimal path

	do {

		result.push_back(prec.at(curr_node));
		curr_node = this->_Links.at(prec.at(curr_node)).getStartNodeId();

	} while( curr_node != source_id );

	return result;

}

float Network::euclidian_distance(std::string source_id, std::string dest_id) {

	/*
	 return sqrt( boost::math::pow<2,float>( _Nodes.at(dest_id).getX() - _Nodes.at(source_id).getX() ) +
			      boost::math::pow<2,float>( _Nodes.at(dest_id).getY() - _Nodes.at(source_id).getY() ) );
	 */

	return abs( _Nodes.at(dest_id).getXData() - _Nodes.at(source_id).getXData() ) +
		   abs( _Nodes.at(dest_id).getYData() - _Nodes.at(source_id).getYData() ) ;

}


void Network::shuffleNodesCoordinates() {

  // setting the seed to be sure that every proc do the same thing!
  Ranq1 rnd(0);

  //int n_nodes = _Nodes.size();
  int cur_node = 0;
  int n_proc = RepastProcess::instance()->worldSize();
  
  for( auto& n : _Nodes ) {
    
    // saving original data
    n.second.setXData(n.second.getX());
    n.second.setYData(n.second.getY());

    // computing the new coordinates
    
    //double x = this->min_x + rnd.doub() * (this->max_x - this->min_x);
    //double y = this->min_y + rnd.doub() * (this->max_x - this->min_y);

    int node_proc = cur_node % n_proc;

    double x = node_proc + 0.5;
    double y = 0.5;
    

    cur_node++;

    
    //cout << "Node " << n.first << " at (" << n.second.getX() << "," << n.second.getY() << ")" << endl;    
    
    // setting the new coordinates
    n.second.setX(x);
    n.second.setY(y);

    //cout << "=> new coordinates Node " << n.first << " at (" << n.second.getX() << "," << n.second.getY() << ")" << endl;

  }
  
}

// Constructor
Node::Node(std::string id, double x, double y) :
  _id(id), _x(x), _y(y), _indicators(), _x_data(x), _y_data(y) {
}


// Adding an indicator to a node
void Node::addIndicator(string aIndicator, int aIndicatorValue) {
  this->_indicators.insert(make_pair(aIndicator,aIndicatorValue));
}


// Constructor
Link::Link(std::string id, std::string start_node, std::string end_node, float length) :
    _id(id), _start_node_id(start_node), _end_node_id(end_node), _length(length),
    _n_agents(0), _free_flow_time(0), _capacity(0), _x(0), _y(0) {
}


// Constructor
Link::Link(std::string id, std::string start_node, std::string end_node, float length,
		   float free_flow_speed, float capacity, double x, double y) :
    _id(id), _start_node_id(start_node), _end_node_id(end_node), _length(length), _n_agents(0),
   _capacity(capacity), _x(x), _y(y) {

	_free_flow_time = length / free_flow_speed;
}

// Time required to go through the link
float Link::timeOnLink() const {
	return  _free_flow_time * ( 1.0f + 0.15f * boost::math::pow<4,float>( (float)( _n_agents / _capacity ) ) );
}



