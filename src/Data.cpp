/****************************************************************
 * DATA.CPP
 * 
 * This file contains all the definitions of the methods of
 * Data.hpp (see this file for methods' documentation)
 * 
 * Authors: J. Barthélemy
 * Date   : 3 June 2014
 ****************************************************************/

#include "../include/Data.hpp"

using namespace std;
using namespace repast;
using namespace tinyxml2;

////////////////
// Data class //
////////////////

void Data::read_network_matsim() {

	if (RepastProcess::instance()->rank() == 0) cout << "... reading network" << endl;

	// Loading XML file containing the network data.
	string filename = this->_props.getProperty("file.network_matsim");
	XMLDocument doc(filename.c_str());
	doc.loadFile(filename.c_str());

	// Parsing the node data ////////////////////////////////////////////////////////////////

	XMLElement * ele = doc.FirstChildElement("network")->FirstChildElement("nodes")->FirstChildElement("node");
	const XMLAttribute * attr;

	while (ele) {

		// reading id
		attr = ele->FirstAttribute();
		string id = attr->StringValue();

		// reading x coordinate
		attr = attr->Next();
		double x = attr->FloatValue();

		// reading y coordinate
		attr = attr->Next();
		double y = attr->FloatValue();

		// adding the node to the network
		Node currNode(id, x, y);
		this->_network.addNode(currNode);

		ele = ele->NextSiblingElement("node");

	}

	this->_network.shuffleNodesCoordinates();

	// Parsing the link data ////////////////////////////////////////////////////////////////

	ele = doc.FirstChildElement("network")->FirstChildElement("links")->FirstChildElement("link");

	while (ele) {

		// reading id
		attr = ele->FirstAttribute();
		string id = attr->StringValue();

		// reading starting node id
		attr = attr->Next();
		string start_node = attr->StringValue();

		// reading ending node id
		attr = attr->Next();
		string end_node = attr->StringValue();

		// ... and adding it to the list of end node of start node
		this->_network.addLinkOutToNode(start_node, id);

		// reading length
		attr = attr->Next();
		float length = attr->FloatValue();

		// reading free-flow speed

		attr = attr->Next();
		float ff_speed = attr->FloatValue();

		// reading capacity
		attr = attr->Next();
		float capacity = attr->FloatValue();

		// determining link starting node coordinates
		double link_x = this->_network.getNodes().at(start_node).getX();
		double link_y = this->_network.getNodes().at(end_node).getY();

		// adding the link to the network
		Link currLink(id, start_node, end_node, length, ff_speed, capacity, link_x, link_y);
		this->_network.addLink(currLink);

		// moving to next link
		ele = ele->NextSiblingElement("link");

	}

	


}

void Data::read_network_transims() {

	if (RepastProcess::instance()->rank() == 0) cout << "... reading network (transims format)" << endl;

	string filename;                                                  // string containing the path to an input file
	ifstream file;                                                    // an input stream extracting from a file
	string a_line;                                                    // a line of data

	// Nodes ///////////////////////////////////////////////////////////////////////////////

	if (RepastProcess::instance()->rank() == 0) cout << "       parsing nodes" << endl;

	filename = this->_props.getProperty("file.nodes_transims");       // path to data
	file.open(filename.c_str(), ios::in);                             // opening data file

	// file reading
	if(file) {
		getline(file, a_line);
		while (getline(file, a_line)) {
			auto data = split<string>(a_line,"\t");
			auto id = boost::lexical_cast<string>(data[0]);
			auto x  = boost::lexical_cast<double>(data[1]);
			auto y  = boost::lexical_cast<double>(data[2]);

			Node cur_node(id, x, y);
			this->_network.addNode(cur_node);

		}
		file.close();
		this->_network.shuffleNodesCoordinates();               // shuffling coordinates
	} else {
		cerr << "Could not open " << filename << endl;
	}

	// Nodes - Activities location matching ////////////////////////////////////////////////

	if (RepastProcess::instance()->rank() == 0) cout << "       parsing activities locations (matching locations and nodes)" << endl;

	filename = this->_props.getProperty("file.activities_transims");
	file.open(filename.c_str(), ios::in);                                  // opening data file

	if(file) {
		getline(file, a_line);                                             // dropping the first line
		while (getline(file, a_line)) {
			auto data = split<string>(a_line,"\t");                        // extracting data
			auto id_loc      = boost::lexical_cast<string>(data[0]);
			auto id_node_net = boost::lexical_cast<string>(data[2]);

			this->_map_act_loc_nodes[id_loc] = id_node_net;

		}
		file.close();
	} else {
		cerr << "Could not open " << filename << endl;
	}

	// Links ///////////////////////////////////////////////////////////////////////////////

	if (RepastProcess::instance()->rank() == 0) cout << "       parsing links" << endl;

	filename = this->_props.getProperty("file.links_transims");
	file.open(filename.c_str(), ios::in);                             // opening data file

	// file reading
	if (file) {
		getline(file, a_line);                                          // dropping the first line
		while (getline(file, a_line)) {

			// extracting data
			auto data     = split<string>(a_line, "\t");
			string type   = data[21];

			// removing walking paths
			if( type.compare("WALK") != 0 ) {

				auto id_link  = boost::lexical_cast<string>(data[0]);
				auto id_orig  = boost::lexical_cast<string>(data[2]);
				auto id_dest  = boost::lexical_cast<string>(data[3]);
				auto length   = boost::lexical_cast<float>(data[4]);
				auto ff_speed = boost::lexical_cast<float>(data[15]);
				auto capacity = boost::lexical_cast<float>(data[16]);

				auto link_x = this->_network.getNodes().at(id_orig).getX();
				auto link_y = this->_network.getNodes().at(id_orig).getY();


				// adding the link to the list of outgoing links of node id_orig
				this->_network.addLinkOutToNode(id_orig, id_link);

				// adding the link to the network
				Link currLink(id_link, id_orig, id_dest, length, ff_speed, capacity, link_x, link_y);
				this->_network.addLink(currLink);

#ifdef DEBUGDATA
				if (RepastProcess::instance()->rank() == 0) {
					cout << "Adding link " << id_link << " with chars : " << id_orig << " " << id_dest << " " << length << " " << capacity << " ";
					cout << ff_speed << " " << link_x << " " << link_y << endl;
				}
#endif

				// checking if current link is 2-way
				auto return_link_lanes = boost::lexical_cast<unsigned int>(data[17]);
				if ( return_link_lanes > 0 ) {
					auto id_return_link = "-"+id_link;
					auto id_return_orig = id_dest;
					auto id_return_dest = id_orig;
					auto ff_speed_return = boost::lexical_cast<float>(data[19]);
					auto capacity_return = boost::lexical_cast<float>(data[20]);
					auto link_x_return = this->_network.getNodes().at(id_return_orig).getX();
					auto link_y_return = this->_network.getNodes().at(id_return_orig).getY();

					// adding the links to the map tracking 2-ways links
					this->_map_2way_links[id_link] = id_return_link;

					// adding the link to the list of outgoing links of node id_orig
					this->_network.addLinkOutToNode(id_return_orig, id_return_link);

					// adding the link to the network
					Link currLinkReturn(id_return_link, id_return_orig, id_return_dest, length, ff_speed_return, capacity_return, link_x_return, link_y_return);
					this->_network.addLink(currLinkReturn);

#ifdef DEBUGDATA
					if (RepastProcess::instance()->rank() == 0) {
						cout << "Adding return link " << id_return_link << "with chars : " << id_return_orig << " " << id_return_dest << " " << length << " " << capacity_return << " ";
						cout << ff_speed_return << " " << link_x_return << " " << link_y_return << endl;
					}
#endif

				}

			}

		}
		file.close();
	} else {
		cerr << "Could not open " << filename << endl;
	}

}

void Data::read_strategies() {

	if (RepastProcess::instance()->rank() == 0) cout << "... reading strategies" << endl;

	string filename = this->_props.getProperty("file.strategies");       // path to data
	ifstream file(filename.c_str(), ios::in);                             // opening data file
	string a_line;                                                    // a line of data

	// file reading
	if(file) {

		while (getline(file, a_line)) {

			auto data  = split<float>(a_line, ";");
			auto alpha = data[0];
			auto beta  = data[1];

			Strategy cur_strat(alpha,beta);
			this->_strategies.emplace_back(cur_strat);

		}

		file.close();

	} else {
		cerr << "Could not open " << filename << endl;
	}

#ifdef DEBUGDATA
	if( RepastProcess::instance()->rank() == 0 ) {
		for( unsigned int s = 0; s < this->_strategies.size(); s++ ) {
			cout << this->_strategies.at(s) << endl;
		}
	}
#endif

}


const Strategy& Data::getOneStrategy() const {

	unsigned int strat_index = RandomGenerators::getInstance()->unif.int32( _strategies.size() - 1 );
	return _strategies.at(strat_index);

}

/////////////////////////////////
// Aggregate output data class //
/////////////////////////////////

AggregateSum::AggregateSum() {
	_sum = 0;
}

void AggregateSum::setData(int val) {
	_sum = val;
}

void AggregateSum::incrementData() {
	++_sum;
}

void AggregateSum::decrementData() {
	--_sum;
}

int AggregateSum::getData() {
	return _sum;
}


///////////////////////
// Some useful tools //
///////////////////////


long int linesCount(string filename) {

	ifstream file(filename.c_str(), ios::in);                    // opening the file
	int lines = 0;                                               // number of lines

	// file reading
	if (file) {
		while (file.ignore(numeric_limits<int>::max(), '\n')) {
			lines++;                                                 // counting lines
		}
		file.close();
	} else {
		cerr << "Could not open " << filename << endl;
	}

	return lines;

}


template<typename T>
std::vector<T> split(const std::string & msg, const std::string & separators) {

	std::vector<T> result;                                         // resulting vector
	T              token;                                          // one token of the string
	boost::char_separator<char> sep(separators.c_str());           // separator
	boost::tokenizer<boost::char_separator<char> > tok(msg, sep);  // token's generation

	// string decomposition into token
	for (boost::tokenizer<boost::char_separator<char> >::const_iterator i = tok.begin(); i != tok.end(); i++) {

		std::stringstream s(*i);
		string s_string = s.str();                                   // getting the string
	    boost::algorithm::trim(s_string);                            // removing trailing blanks
	    token = boost::lexical_cast<T>( s_string );                  // casting operator

		result.push_back(token);
	}

	// returning the resulting decomposition
	return result;
}


unsigned int secToHour( float n_sec ) {
	return (unsigned long)floor(n_sec / 3600.0f);
}


std::string secToTime( float n_sec ) {

	unsigned long n_sec_int = (unsigned long) floor(n_sec);

	std::ostringstream result;

	unsigned int hour = n_sec_int / 3600;
	unsigned int min  = (n_sec_int / 60) % 60;
	unsigned int sec  = n_sec_int % 60;

	result << hour << ":" << min << ":" << sec;

	return result.str();

}


long timeToSec( const std::string &aTime ) {

	std::vector<long> time = split<long>(aTime, ":");
	return time[0] * 3600 + time[1] * 60 + time[2];

}



