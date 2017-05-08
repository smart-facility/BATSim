# -------------------------------------
# Makefile for building trafficsim
#-O0 -ggdb -pg
# Authors: J.Barthelemy
# Date   : 29 July 2014  
# -------------------------------------

export CXX                = mpicxx 
export CXXFLAGS           = -Wall -Ofast -DNDEBUG -march='native' -mtune='native' -std=c++14  -flto
export CXXFLAGSDEBUG      = -Wall -O0 -ggdb -std=c++14 -D DEBUGDATA -D DEBUGSIM -Wall
export CXXFLAGS_PROF_GEN  = -Wall -Ofast -DNDEBUG -march='native' -mtune='native' -std=c++14   -flto -fprofile-generate
export CXXFLAGS_PROF_USE  = -Wall -Ofast -DNDEBUG -march='native' -mtune='native' -std=c++14   -flto -fprofile-use
export EXEC_NAME          = trafficsim

SRC_DIR   = ./src/
BIN_DIR   = ./bin/

all :
	@(cd $(SRC_DIR) && $(MAKE))

all_profile_use : CXXFLAGS = $(CXXFLAGS_PROF_USE)
all_profile_use :
	@(cd $(SRC_DIR) && $(MAKE))

all_profile_generate : CXXFLAGS = $(CXXFLAGS_PROF_GEN)
all_profile_generate :
	@(cd $(SRC_DIR) && $(MAKE))

debug : CXXFLAGS = $(CXXFLAGSDEBUG)
debug :
	@(cd $(SRC_DIR) && $(MAKE))

clean :
	@rm $(SRC_DIR)*.o $(BIN_DIR)$(EXEC_NAME)
