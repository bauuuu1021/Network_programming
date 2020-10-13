#include <iostream>
#include <stdio.h>
#include <string>

using namespace std;

extern void execute_cmd(string cmd);

void printenv(string cmd) {
    
    char *ret;

    try {
        string env_name = cmd.substr(9, string::npos);
        ret = getenv(env_name.c_str());
    }
    catch (const std::exception& e) {
        cerr << "[printenv] error:\n" << e.what() << "\n";
        return;
    }

    if (ret)
        printf("%s\n", ret);
}

void setenv(string cmd) {

    try {
        string delimiter = " ";
        cmd = cmd.substr(cmd.find(delimiter)+1);
        string env_name = cmd.substr(0, cmd.find(delimiter));
        string env_value = cmd.substr(cmd.find(delimiter)+1, string::npos);
        setenv(env_name.c_str(), env_value.c_str(), !0);
    }
    catch (const std::exception& e) {
        cerr << "[setenv] error:\n" << e.what() << "\n";
        return;
    }
}

int main () {

    setenv("PATH", "bin:.", !0);

    string cmd, delimiter = " ";
    cout << "% ";
    while (getline(cin, cmd) && cmd.compare("exit")) {

        // Skip leading space(s) of cmd
        while (cmd.length()!=0 && cmd.at(0)==' ') {
            try {
                cmd = cmd.substr(cmd.find(delimiter)+1);
            }
            catch (const std::exception& e) {
                cerr << "[rm leading space] error:\n" << e.what() << "\n";
            }
        }         
        
        if (!cmd.compare(0, 8, "printenv")) {
            printenv(cmd);
        }
        else if (!cmd.compare(0, 6, "setenv")) {
            setenv(cmd);
        }
        else if (cmd.length()==0) {     // empty command
            ;
        }
        else {
            execute_cmd(cmd.c_str());
        }

        cout << "% ";
    }
}
