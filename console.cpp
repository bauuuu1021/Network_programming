#include <iostream>
#include <fstream>
#include <queue>
#include <cstdlib>
#include <cstring>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class NP_client 
{
public:
  NP_client(boost::asio::io_context& io_context, tcp::endpoint end, string file)
    : socket_(io_context)
  {
    load_cmd(file);
    do_connection(end);
  }

private:

  void load_cmd(string filename) {
    ifstream ifs(filename);
    string line;

    while (getline(ifs, line)) {
      cmd_Q.push(line);
    }

    ifs.close();
  }

  void recv() {
    memset(r_buf, 0, max_length);
    socket_.async_read_some(boost::asio::buffer(r_buf, max_length),
      [this](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          cout << r_buf;
          string tmp(r_buf);
          if (tmp.find("% ") != string::npos) {
            send();
          }
          
          recv();
        }
    });
  }

  void send() {
    if (cmd_Q.empty())  return;

    string cmd = cmd_Q.front();
    cout << cmd << endl;
    cmd_Q.pop();

    strcpy(w_buf, (cmd + "\n").c_str());
    size_t length = cmd.size() + 1;

    boost::asio::async_write(socket_, boost::asio::buffer(w_buf, length),
      [this](boost::system::error_code ec, size_t /*length*/) {
        if (!ec) {
          recv();
        }
    });
  }

  void do_connection(tcp::endpoint end) {
    socket_.async_connect(end, [this](boost::system::error_code ec){
      if (!ec) {
        recv();
      }
      else {
        cerr << "Connection failed" << endl;
      }
    });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char r_buf[max_length], w_buf[max_length];
  queue<string> cmd_Q;
};

tcp::endpoint resolveDNS(string hostname, unsigned short port) {

  boost::asio::io_service io_service;
  tcp::resolver resolver(io_service);
  tcp::resolver::query query(hostname, to_string(port));
  tcp::resolver::iterator iter = resolver.resolve(query);
  tcp::endpoint endpoint = iter->endpoint();

  return endpoint;
}

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_context io_context;
    tcp::endpoint end = resolveDNS("nplinux1.cs.nctu.edu.tw", 12345U);
    string filename("test_case/t1.txt");
    NP_client client(io_context, end, filename);
    io_context.run();
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}