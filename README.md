# BATSim - Behavioural Agent-based Traffic Simulation

A dynamic behavioural traffic assignment model with strategic agents.

This is an alpha version!

The simulator can read inputs in MATSim and TRANSIMS formats.

## Installation

In order to compile and run the simulator, you need a patched version of Repast HPC 2.2:

1. download Repast HPC 2.2;
2. replace the sources files with the ones in the directory patch_repast;
3. install Repast;
4. compile TrafficSim with make.

## Running the simulation

1. Set up the parameters in the file bin/model.props.
2. Run the script run.sh X where X is the number of cores to be used.

## Creating the documentation

1. Navigate to the doc directory.
2. Type doxygen trafficsim.
