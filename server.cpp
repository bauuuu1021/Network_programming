#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <string>
#include <string.h>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

void printenv() {
  cout << "REQUEST_METHOD: " << getenv("REQUEST_METHOD") << endl;
  cout << "REQUEST_URI: " << getenv("REQUEST_URI") << endl;
  cout << "QUERY_STRING: " << getenv("QUERY_STRING") << endl;
  cout << "SERVER_PROTOCOL: " << getenv("SERVER_PROTOCOL") << endl;
  cout << "HTTP_HOST: " << getenv("HTTP_HOST") << endl;
  cout << "SERVER_ADDR: " << getenv("SERVER_ADDR") << endl;
  cout << "SERVER_PORT: " << getenv("SERVER_PORT") << endl;
  cout << "REMOTE_ADDR: " << getenv("REMOTE_ADDR") << endl;
  cout << "REMOTE_PORT: " << getenv("REMOTE_PORT") << endl;
  cout << "------------------------------\n";
}

class session
  : public enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:

  void response(bool isSuccessed) {
    auto self(shared_from_this());
    
    string response;
    if (isSuccessed) {
      response = "HTTP/1.1 200 OK\r\n";
    }
    else {
      response = "HTTP/1.1 404 Not Found\r\n";
    }

    boost::asio::async_write(socket_, boost::asio::buffer(response, response.length()), 
      [this, self](boost::system::error_code ec, size_t /*length*/) {
        if (ec) {
          cerr << "reply error" << endl;
        }
    });
  }

  void setupEnv() {
    strcpy(SERVER_ADDR, socket_.local_endpoint().address().to_string().c_str());
    sprintf(SERVER_PORT, "%u", socket_.local_endpoint().port());
    strcpy(REMOTE_ADDR, socket_.remote_endpoint().address().to_string().c_str());
    sprintf(REMOTE_PORT, "%u", socket_.remote_endpoint().port());

    setenv("REQUEST_METHOD", REQUEST_METHOD, !0);
    setenv("REQUEST_URI", REQUEST_URI, !0);
    setenv("QUERY_STRING", QUERY_STRING, !0);
    setenv("SERVER_PROTOCOL", SERVER_PROTOCOL, !0);
    setenv("HTTP_HOST", HTTP_HOST, !0);
    setenv("SERVER_ADDR", SERVER_ADDR, !0);
    setenv("SERVER_PORT", SERVER_PORT, !0);
    setenv("REMOTE_ADDR", REMOTE_ADDR, !0);
    setenv("REMOTE_PORT", REMOTE_PORT, !0);
  }

  string parseRequest(string request) {
    sscanf(data_, "%s %s %s %s %s", REQUEST_METHOD, REQUEST_URI, SERVER_PROTOCOL, ignore, HTTP_HOST);
    
    string cgi_name(REQUEST_URI);
    cgi_name = cgi_name.substr(1);
    if (cgi_name.compare(cgi_name.size()-4, 4, ".cgi")) {
      response(false);
      return "";
    }
    
    response(true);
    setupEnv();
      
    return cgi_name;
  }

  void cgi_handler(string cgi_name) {
    string cgi = "./" + cgi_name;

    dup2(socket_.native_handle(), STDIN_FILENO);
    dup2(socket_.native_handle(), STDOUT_FILENO);
    dup2(socket_.native_handle(), STDERR_FILENO);
    socket_.close();
    if (execlp(cgi.c_str(), cgi.c_str(), NULL) < 0) {
      cerr << strerror(errno) << endl;
    }
  }  

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
      [this, self](boost::system::error_code ec, size_t length) {
        if (!ec) {
          string cgi_name = parseRequest(data_);
          if (!cgi_name.empty()) {
            printenv();
            cgi_handler(cgi_name);
          }
        }
    });
  }

  void do_write(size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, size_t /*length*/)
        {
          if (!ec)
          {
            do_read();
          }
        });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
  char REQUEST_METHOD[max_length], REQUEST_URI[max_length], QUERY_STRING[max_length],
    SERVER_PROTOCOL[max_length], HTTP_HOST[max_length], SERVER_ADDR[max_length],
    SERVER_PORT[max_length], REMOTE_ADDR[max_length], REMOTE_PORT[max_length], ignore[max_length];
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    cout << "------------------------------\n";
    cout << "Welcome to Remote Batch System" << endl;
    cout << "------------------------------\n";
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec) {
            pid_t pid = fork();
            if (pid < 0)  {
              cerr << "fork failed" << endl;
              do_accept();
            }
            else if (pid == 0) {
              io_context.notify_fork(boost::asio::io_context::fork_child);
              make_shared<session>(move(socket))->start();
            }
            else {
              io_context.notify_fork(boost::asio::io_context::fork_parent);
              do_accept();
            }
          }
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    server s(io_context, atoi(argv[1]));

    io_context.run();
  }
  catch (exception& e)
  {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
