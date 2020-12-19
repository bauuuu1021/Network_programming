CXX = g++
CXXFLAGS = -std=c++14 -Wall -pedantic -pthread -lboost_system -g
CXX_INCLUDE_DIRS = /usr/local/include
CXX_INCLUDE_PARAMS = $(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS = /usr/local/lib
CXX_LIB_PARAMS = $(addprefix -L , $(CXX_LIB_DIRS))

EXE = sock_server

all: $(EXE)

sock_server: server.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f $(EXE)

