#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

class session
    : public std::enable_shared_from_this<session> {
public:
  session(tcp::socket socket)
      : socket_(std::move(socket)) {
  }

  void start() {
    do_receive();
  }

private:
  void print_info(int connect_type, uint16_t dst_port, string dst_ip) {

    cout << "<S_IP>: " << socket_.remote_endpoint().address() << endl;
    cout << "<S_PORT>: " << socket_.remote_endpoint().port() << endl;
    cout << "<D_IP>: " << dst_ip << endl;
    cout << "<D_PORT>: " << dst_port << endl;
    cout << "<Command>: " << ((connect_type == 1) ? "CONNECT" : "BIND") << endl;
    cout << "<Reply>: " << endl;
    cout << "----------------------------\n";
  }

  string resolve_DNS(char *data_, uint16_t port, size_t length) {
    int begin = -1, end = -1;
    string hostname;
    stringstream ss;

    for (size_t i = 8; i < length; i++) {
      if (data_[i] == '\0' && begin == -1)
	      begin = i;
      else if (data_[i] == '\0' && begin != -1)
	      end = i;
    }

    for (auto i = begin + 1; i < end; i++) {
      ss << data_[i];
    }
    ss >> hostname;

    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(hostname, to_string(port));
    tcp::resolver::iterator iter = resolver.resolve(query);
    tcp::endpoint endpoint = iter->endpoint();

    return endpoint.address().to_string();
  }

  void do_receive() {
    auto self(shared_from_this());
    socket_.async_receive(boost::asio::buffer(data_, max_length),
			  [this, self](boost::system::error_code ec, std::size_t length) {
			    if (!ec) {
			      int conn_type = (uint8_t)data_[1];
			      uint16_t port = ((data_[2] & 0xff) << 8) | (data_[3] & 0xff);
			      string ip = to_string((data_[4]) & 0xff) + "." +
					  to_string((data_[5]) & 0xff) + "." +
					  to_string((data_[6]) & 0xff);

			      if (!(ip.compare("0.0.0")) && ((data_[7]) & 0xff)) { // resolve DNS
				      ip = resolve_DNS(data_, port, length);
			      } else { // normal request
				      ip = ip + "." + to_string((data_[7]) & 0xff);
			      }

			      print_info(conn_type, port, ip);
			    }
			  });
  }

  void do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
			    [this, self](boost::system::error_code ec, std::size_t length) {
			      if (!ec) {
				      //do_write(length);
			      }
			    });
  }

  void do_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
			     [this, self](boost::system::error_code ec, std::size_t /*length*/) {
			       if (!ec) {
				      do_read();
			       }
			     });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {
public:
  server(boost::asio::io_context &io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    do_accept();
  }

private:
  void do_accept() {
    acceptor_.async_accept(
	[this](boost::system::error_code ec, tcp::socket socket) {
	  if (!ec) {
	    std::make_shared<session>(std::move(socket))->start();
	  }

	  do_accept();
	});
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " " << "<port>\n";
      return 1;
    }

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
