CXX = g++
CXXFLAGS = -std=c++14 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS = /usr/local/include
CXX_INCLUDE_PARAMS = $(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS = /usr/local/lib
CXX_LIB_PARAMS = $(addprefix -L , $(CXX_LIB_DIRS))

EXE = http_server console.cgi cgi_server.exe
CMD_SRC = command
CMD = delayedremovetag noop number removetag removetag0
BIN_DIR = bin

all:
	@echo 'The project consists of two part'
	@echo 'For linux environment, please type `make part1`'
	@echo 'For windows environment, please type `make part2`'
	@echo 'For both environment, please type `make build_cmd` to make commands available' 
build_cmd: unix_cmd $(CMD)

unix_cmd: 
	mkdir -p $(BIN_DIR)
	cp /bin/ls $(BIN_DIR)
	cp /bin/cat $(BIN_DIR)

$(CMD): %: $(CMD_SRC)/%.cpp
	$(CXX) -o $(BIN_DIR)/$@ $^

part1: http_server console.cgi

http_server: server.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

console.cgi: console.cpp
	$(CXX) -o $@ $^ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

part2: cgi_server.exe

cgi_server.exe: cgi_server.cpp
	$(CXX) -o $@ $^ -lws2_32 -lwsock32 -std=c++14

.PHONY: clean
clean:
	rm -rf $(EXE) $(BIN_DIR)
