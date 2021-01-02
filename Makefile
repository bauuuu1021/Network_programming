CXX = g++
CXXFLAGS = -std=c++14 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS = /usr/local/include
CXX_INCLUDE_PARAMS = $(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS = /usr/local/lib
CXX_LIB_PARAMS = $(addprefix -L , $(CXX_LIB_DIRS))

EXE = http_server console.cgi
CMD_SRC = commands
CMD = noop number removetag removetag0
BIN_DIR = bin

all: $(EXE)

build_cmd: unix_cmd $(CMD)

unix_cmd: 
	mkdir -p $(BIN_DIR)
	cp /bin/ls $(BIN_DIR)
	cp /bin/cat $(BIN_DIR)

$(CMD): %: $(CMD_SRC)/%.cpp
	$(CXX) -o $(BIN_DIR)/$@ $^

http_server: server.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

console.cgi: console.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

.PHONY: clean
clean:
	rm -rf $(EXE) $(BIN_DIR)
