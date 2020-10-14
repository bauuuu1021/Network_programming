#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

using namespace std;

extern long cmd_count;

void np_fork(string cmd) {

    cout << cmd_count << ":" << cmd << endl;
}

void execute_cmd(string cmd) {

    string pipe_delimiter = "|", space_delimiter = " ";
    size_t new_end = 0;

    while ((new_end = cmd.find(pipe_delimiter)) != string::npos) {
        string cur_cmd = cmd.substr(0, new_end);
        np_fork(cur_cmd);

        // Skip leading space(s) of cmd
        cmd = cmd.substr(new_end+1);
        while (cmd.length()!=0 && cmd.at(0)==' ') {
            try {
                cmd = cmd.substr(cmd.find(space_delimiter)+1);
            }
            catch (const std::exception& e) {
                cerr << "[execute_cmd] skip leading 0:\n" << e.what() << "\n";
            }
        } 
    }
    np_fork(cmd);
}