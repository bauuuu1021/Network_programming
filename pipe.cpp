#include <stdlib.h>
#include <string>

using namespace std;

void execute_cmd(string cmd) {
    system(cmd.c_str());
}