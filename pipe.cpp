#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <string.h>

#define ARG_MAX 1024

using namespace std;

extern long cmd_count;
vector<pid_t> child_list;

string skip_lead_space(string str) {

    string delimiter = " ";
    while (str.length()!=0 && str.at(0)==' ') {
        try {
            str = str.substr(str.find(delimiter)+1);
        }
        catch (const std::exception& e) {
            cerr << "[np_exec] skip leading 0:\n" << e.what() << "\n";
        }
    }

    return str;
}

void np_exec(string cmd) {
    char *arg[ARG_MAX];
    int arg_cnt = 0, arg_end = 0;
    string arg_delimiter = " ";

    if (isdigit(cmd.at(0))) {   // numbered pipe
        cout << "number pipe\n";
    }
    else {  // normal commands

        string parse_arg(cmd), arg_delimiter = " ";
        while ((arg_end = parse_arg.find(arg_delimiter)) != string::npos) {
            string cur_arg = parse_arg.substr(0, arg_end);
            if (cur_arg.length())
                arg[arg_cnt++] = strdup(cur_arg.c_str());

            // Skip leading space(s) of cmd
            parse_arg = parse_arg.substr(arg_end+1);
            parse_arg = skip_lead_space(parse_arg); 
        }
        if (parse_arg.length())
            arg[arg_cnt++] = strdup(parse_arg.c_str());
        arg[arg_cnt] = NULL;

        if (execvp(arg[0], arg) == -1) {
            cerr << "Unknown command: [" << cmd << "].\n";
        }
    }
}

void np_fork(string cmd) {

    pid_t child_pid;

    while ((child_pid = fork())<0) {
        // handle fork error
    }

    if (child_pid == 0) {   // child process
        np_exec(cmd);
    }
    else {  // parent process
        child_list.push_back(child_pid);
    }

}

void execute_cmd(string cmd) {

    string pipe_delimiter = "|", space_delimiter = " ";
    size_t new_end = 0;
    child_list.clear();

    while ((new_end = cmd.find(pipe_delimiter)) != string::npos) {
        string cur_cmd = cmd.substr(0, new_end);
        np_fork(cur_cmd);

        // Skip leading space(s) of cmd
        cmd = cmd.substr(new_end+1);
        cmd = skip_lead_space(cmd);
    }
    np_fork(cmd);

    for(const auto& pid: child_list) {
        int status;
        waitpid(pid, &status, 0);
    }
}
