#!/bin/bash

cd ./bin/
mpirun -np $1 ./trafficsim config.props model.props
cd ..
