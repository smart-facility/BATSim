/****************************************************************
 * NETWORK.HPP
 *
 * This file contains all the data related class and methods.
 *
 * Authors: J. Barthelemy
 * Date   : 08 april 2013
 ****************************************************************/

/*! \file Network.hpp
    \brief Road network related class and methods (Heap, Node, Link and Network classes).
 */

#ifndef NETWORK_HPP_
#define NETWORK_HPP_

#include <map>
#include <vector>
#include <iostream>
#include <functional>
#include <limits>
#include <utility>
#include <sstream>
#include <set>
#include <algorithm>
#include <string>
#include <math.h>
#include "Random.hpp"
#include "FiboHeap.hpp"
#include <boost/math/special_functions/pow.hpp>

//! A node class.
/*!
 This class intends to represents the nodes of a road network.
 Additionaly to the basic attributes (id, coordinates, outgoing links),
 some members of the node are related to the municipality to which it belongs
 (ins, indicators) and (key, index) and Dijkstra shortest path algorithm 
 (key, index).
 */
class Node {

private:

  std::string               _id;            //!< id of the node
  double                    _x;             //!< x coordinate
  double                    _y;             //!< y coordinate
  std::vector<std::string>  _links_out_id;  //!< set of outgoing links.
  std::map<std::string,int> _indicators;    //!< a map of indicators related to the node.
  double                    _x_data;
  double                    _y_data; 

public:

  //! Default Constructor.
  Node() : _id("0"), _x(0.0f), _y(0.0f), _links_out_id(), _indicators(), _x_data(0.0f), _y_data(0.0f) {};

  //! Constructor.
  /*!
    \param id a node id
    \param x x coordinate
    \param y y coordinate
   */
  Node(std::string id, double x, double y);

  //! Destructor.
  ~Node() {};

  //! Return the id of the node.
  /*!
    \return the node id
   */
  std::string getId() const {
    return _id;
  }

  //! Set the id of the node.
  /*!
    \param id a new node id
   */
  void setId(std::string id) {
    _id = id;
  }

  //! Return the set of outgoing links of the node.
  /*!
    \return a vector of link id
   */
  const std::vector<std::string>& getLinksOutId() const {
    return _links_out_id;
  }

  //! Setter for the set of outgoing links of the node.
  /*!
    \param linksOutId a new set of outgoing links id
   */
  void setLinksOutId(const std::vector<std::string>& linksOutId) {
    _links_out_id = linksOutId;
  }

  //! Add an link to the set of outgoing links.
  /*!
    \param linkId a link id
   */
  void addLinkOutId( std::string linkId ) {
    _links_out_id.push_back(linkId);
  }

  //! Return the x coordinate.
  /*!
    \return x coordinate
   */
  double getX() const {
    return _x;
  }

  //! Set the x coordinate.
  /*!
    \param x a new node's x coordinate
   */
  void setX(double x) {
    _x = x;
  }

  //! Return the y coordinate.
  /*!
    \return y coordinate
   */
  double getY() const {
    return _y;
  }

  //! Set the y coordinate.
  /*!
    \param y a new node's y coordinate
   */
  void setY(double y) {
    _y = y;
  }


  //! Return the x coordinate shuffled.
  /*!
    \return x coordinate
   */
  double getXData() const {
    return _x_data;
  }

  //! Set the x coordinate.
  /*!
    \param x a new node's x coordinate
   */
  void setXData(double x) {
    _x_data = x;
  }

  //! Return the y coordinate.
  /*!
    \return y coordinate
   */
  double getYData() const {
    return _y_data;
  }

  //! Set the y coordinate.
  /*!
    \param y a new node's y coordinate
   */
  void setYData(double y) {
    _y_data = y;
  }
  
  //! Return the indicators of the node.
  /*!
    \return the map of node's service indicators used to geo-localize activities
   */
  const std::map<std::string, int>& getIndicators() const {
    return _indicators;
  }

  //! Set the indicators of the node.
  /*!
    \param indicators a new map of services indicators
   */
  void setIndicators(const std::map<std::string, int>& indicators) {
    _indicators = indicators;
  }

  //! Add an indicator and its value to the set of node's indicator.
  /*!
    \param aIndicator an indicator id
    \param aIndicatorValue the value associated with the indicator
   */
  void addIndicator(std::string aIndicator, int aIndicatorValue );

};


//! A link class.
/*!
 This class intends to represents the links of a road network.
 A link is represented by an id, a source node, a sink node and
 a length (in meters).
 */
class Link {

private:

  std::string   _id;                 //!< id of the link.
  std::string   _start_node_id;      //!< source node's id.
  std::string   _end_node_id;        //!< sink node's id.
  float         _length;             //!< length of the link (unit: meters).
  unsigned int  _n_agents;           //!< number of agents on the link.
  float         _free_flow_time;    //!< free flow speed on the link (unit: km per hour).
  float         _capacity;           //!< link capacity (unit: vehicle per hour per km)
  double         _x;                  //!< x coordinate of source node
  double         _y;                  //!< y coordinate of source node

public:

  //! Default constructor.
  Link() : _id("0"), _start_node_id("0"), _end_node_id("0"), _length(0.0), _n_agents(0), _free_flow_time(0.0f), _capacity(0.0f), _x(0), _y(0) {};

  //! Constructor.
  /*!
    \param id link's id
    \param start_node link's source node
    \param end_node link's sink node
    \param length links length
   */
  Link(std::string id, std::string start_node, std::string end_node, float length);

  //! Constructor.
  /*!
    \param id link's id
    \param start_node link's source node
    \param end_node link's sink node
    \param length links length (m)
    \param free_flow_speed link's free flow speed (km/h)
    \param capacity capacity of the link (veh per hour per km)
    \param x x coordinate of the link's origin node
    \param y y coordinate of the link's origin node
   */
  Link(std::string id, std::string start_node, std::string end_node, float length,
  	   float free_flow_speed, float capacity, double x, double y);

  //! Destructor.
  ~Link() {};

  //! Return the sink node's id.
  /*!
    \return a node id
   */
  std::string getEndNodeId() const {
    return _end_node_id;
  }

  //! Set the sink node's id.
  /*!
    \param endNodeId a node id
   */
  void setEndNodeId(std::string endNodeId) {
    _end_node_id = endNodeId;
  }

  //! Return the link's id.
  /*!
    \return link's id
   */
  std::string getId() const {
    return _id;
  }

  //! Set the link's id.
  /*!
    \param id the link id
   */
  void setId(std::string id) {
    _id = id;
  }

  //! Get the length (in meters) of the link.
  /*!
    \return link's length
   */
  float getLength() const {
    return _length;
  }

  //! Set the length of the link.
  /*!
    \param length a link length
   */
  void setLength(float length) {
    _length = length;
  }

  //! Return the source node's id.
  /*!
    /return a node id
   */
  std::string getStartNodeId() const {
    return _start_node_id;
  }

  //! Set the source node's id.
  /*!
    \param startNodeId a node id
   */
  void setStartNodeId(std::string startNodeId) {
    _start_node_id = startNodeId;
  }

  //! Return the link capacity.
  /*!
   /return a capacity
   */
  float getCapacity() const {
    return _capacity;
  }

  //! Set the link capacity.
  /*!
    \param capacity a capacity
   */
  void setCapacity(float capacity) {
    _capacity = capacity;
  }

  //! Return the free flow speed on the link.
  /*!
    /return a speed
   */
  float getFreeFlowTime() const {
    return _free_flow_time;
  }

  //! Set the link free flow speed.
  /*!
    \param freeFlowSpeed a free flow speed
   */
  void setFreeFlowTime(float freeFlowTime) {
    _free_flow_time = freeFlowTime;
  }

  //! Return the number of agents currently using the link.
    /*!
      /return a number of agents
     */
  unsigned int getNAgents() const {
    return _n_agents;
  }

  //! Set the number of agents using link.
  /*!
    \param nAgents the number of agents
   */
  void setNAgents(unsigned int nAgents) {
    _n_agents = nAgents;
  }

  //! Return the link origin x coordinate.
  /*!
    /return a x coordinate
   */
  double getX() const {
    return _x;
  }

  //! Set the link origin x coordinate.
  /*!
    \param x a coordinate
   */
  void setX(double x) {
    _x = x;
  }

  //! Return the link origin y coordinate.
  /*!
    /return a y coordinate
   */
  double getY() const {
    return _y;
  }

  //! Set the link origin y coordinate.
  /*!
    \param y a coordinate
   */
  void setY(double y) {
    _y = y;
  }

  //! Increase the number of agents on the link.
  void increaseAgents() {
	  _n_agents++;
  }

  //! Decrease the number of agents on the link.
  void decrementAgents() {
 	  _n_agents--;
   }

  //! Compute the required time for an agent to go trough the link.
  float timeOnLink() const;

};

//! A Network class.
/*!
  This class implements a network consisting of a set of nodes and links.
  Refers to the Node and Link class for more informations.
 */
class Network {

private:

  std::map<std::string, Node> _Nodes;                            //!< Nodes of the network (see Node class)
  std::map<std::string, Link> _Links;                            //!< Links of the network (see Link class)

  double min_x;                                                   //!< Minimum x coordinate
  double max_x;                                                   //!< Maximum x coordinate
  double min_y;                                                   //!< Minimum y coordinate
  double max_y;                                                   //!< Maximum y coordinate

  std::map<long, std::map<long, std::vector<long>>> _look_up_paths; //!< Look up table for path

public:

  //! Constructor.
  Network() {

    min_x = std::numeric_limits<double>::max();
    min_y = std::numeric_limits<double>::max();
    max_x = std::numeric_limits<double>::min();
    max_y = std::numeric_limits<double>::min();

  };

  //! Destructor.
  ~Network() {};

  //! Returns the network's links.
  /*!
    \return the network's map <links id, links>
   */
  const std::map<std::string, Link>& getLinks() const {
    return _Links;
  }

  //! Setting the network's links.
  /*!
    \param links a map of <links id, links>
   */
  void setLinks(const std::map<std::string, Link>& links) {
    _Links = links;
  }

  //! Return the network's nodes.
  /*!
    \return the networks's map <nodes id, nodes>
   */
  const std::map<std::string, Node>& getNodes() const {
    return _Nodes;
  }

  //! Setting the network's nodes.
  /*!
    \param nodes a map of <nodes id, nodes>
   */
  void setNodes(const std::map<std::string, Node>& nodes) {
    _Nodes = nodes;
  }

  //! Add a link to the set of outgoing link of a node.
  /*
   /param nodeId the source node's id
   /param linkId the id of link to be added
   */
  void addLinkOutToNode(std::string nodeId, std::string linkId) {
    _Nodes[nodeId].addLinkOutId(linkId);
  }

  //! Add a node to the network.
  /*!
   /param aNode the node to add
   */
  void addNode(Node aNode);

  //! Add a link to the network.
  /*!
    /param aLink the node to add
   */
  void addLink(Link aLink);

  //! Compute the shortest path between two nodes in the network.
  /*!
    This method computes the shortest path between two given node
    in the network. This algorithm relies on a Fibonacci heap data
    structure.

    Note that the resulting path is given in 'reverse' order, i.e.
    the first link to take between is the last one in the resulting
    vector, while the final link reaching the destination node is
    the front link in the vector.

    Depending on the 'fastest' parameter, this method compute the
    shortest or fastest (at free flow speed).

    \param source_id source node
    \param dest_id destination node
    \param fastest if flag set to true then compute the fastest path, otherwise the shortest one
   */
  std::vector<std::string> computePath(std::string source_id, std::string dest_id, bool fastest = true);

  std::vector<std::string> computePath(std::string source_id, std::string dest_id, std::string link_id_to_avoid, bool fastest = true);

  std::vector<std::string> computePathAStar(std::string source_id, std::string dest_id, bool fastest = true);

  //! Compute the Euclidean distance between two nodes.
  /*
    \param source_id the first node id
    \param dest_id the second node id
    \return the distance between the nodes
   */
  float euclidian_distance(std::string source_id, std::string dest_id);

  //! Return the maximum x coordinate.
  /*!
    \return maximum x coordinate
   */
  double getMaxX() const {
    return max_x;
  }

  //! Set maximum x coordinate.
  /*!
    \param maxX maximum x coordinate
   */
  void setMaxX(double maxX) {
    max_x = maxX;
  }

  //! Return the maximum y coordinate.
  /*!
    \return maximum y coordinate
   */
  double getMaxY() const {
    return max_y;
  }

  //! Set maximum x coordinate.
  /*!
    \param maxY maximum y coordinate
   */
  void setMaxY(double maxY) {
    max_y = maxY;
  }

  //! Return the minimum x coordinate.
  /*!
    \return minimum x coordinate
   */
  double getMinX() const {
    return min_x;
  }

  //! Set minimum x coordinate.
  /*!
    \param minX minimum x coordinate
   */
  void setMinX(double minX) {
    min_x = minX;
  }

  //! Return the minimum y coordinate.
  /*!
    \return minimum y coordinate
   */
  double getMinY() const {
    return min_y;
  }

  //! Set minimum y coordinate.
  /*!
    \param minY minimum y coordinate
   */
  void setMinY(double minY) {
    min_y = minY;
  }

  //! Shuffle the nodes coordinates
  /*!
    The new nodes coordinates {x,y}_shuffle are randomly set in [min_{x,y},max_{x,y}]
  */
  void shuffleNodesCoordinates();  
  
  //! Increment the number of agent on a given link.
  void incrementAgentOnLink(std::string aLinkId){
	  _Links.at(aLinkId).increaseAgents();
  }

  //! Decrement the number of agent on a given link.
  void decrementAgentOnLink(std::string aLinkId){
	  _Links.at(aLinkId).decrementAgents();
  }

};

#endif /* NETWORK_HPP_ */
