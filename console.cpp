#include <iostream>
#include <cstdlib>
#include <cstring>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class NP_client 
{
public:
  NP_client(boost::asio::io_context& io_context, tcp::endpoint end) : socket_(io_context)
  {
    do_connection(end);
  }

private:

  void do_read() {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
      [this](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          cout << data_ << endl;
        }
    });
  }

  void do_connection(tcp::endpoint end) {
    socket_.async_connect(end, [this](boost::system::error_code ec){
      if (!ec) {
        do_read();
      }
    });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
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
    NP_client client(io_context, end);
    io_context.run();
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}