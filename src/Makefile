SOURCES   = $(wildcard *.cpp)
OBJECTS   = $(SOURCES:.cpp=.o)
BIN_DIR   = ../bin/

all : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -lboost_system -lboost_mpi -lboost_serialization -lboost_filesystem -lrepast_hpc-2.2 -lnetcdf_c++ -o $(BIN_DIR)$(EXEC_NAME)

%.o : %.cpp ../include/%.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

main.o : main.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

Network.o : Network.cpp ../include/Network.hpp ../include/FiboHeap.hpp 	
	$(CXX) $(CXXFLAGS) -o $@ -c $<
