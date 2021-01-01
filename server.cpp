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

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:

  void replyOK() {
    auto self(shared_from_this());
    string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
    boost::asio::async_write(socket_, boost::asio::buffer(response, response.length()), 
      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
          cerr << "reply error" << endl;
        }
    });
  }

  void cgi_handler(string request) {
    string cgi("panel.cgi");
    
    replyOK();

    dup2(socket_.native_handle(), 0);
    dup2(socket_.native_handle(), 1);
    dup2(socket_.native_handle(), 2);
    socket_.close();
    if (execlp(cgi.c_str(), cgi.c_str(), NULL) < 0) {
      cerr << strerror(errno) << endl;
    }
  }  

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
      [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          cout << data_ << endl;
          string request(data_);
          cgi_handler(request);
        }
    });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
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
              std::make_shared<session>(std::move(socket))->start();
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
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
