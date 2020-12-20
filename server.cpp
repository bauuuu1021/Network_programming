#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <fstream>

#define CONN 1
#define BIND 2
#define GRANTED 90
#define REJECT  91

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

class session
    : public std::enable_shared_from_this<session> {
public:
  session(tcp::socket socket)
      : client_socket_(std::move(socket)), server_socket_(io_context) {
  }

  void start() {
    recv_sock4();
  }

private:

  string firewallStatus(int connect_type, string dst_ip) {
    auto self(shared_from_this());

    ifstream conf("socks.conf");
    string rule;

    while (getline(conf, rule)) {
      if ((connect_type == CONN && rule.at(7) == 'b') || (connect_type == BIND && rule.at(7) == 'c'))
        continue;
      else {
        rule = rule.substr(rule.find(" ")+1);
        rule = rule.substr(rule.find(" ")+1);
        
        // check if they're match
        string dst(dst_ip);
        for (auto i = 0; i < 4; i++) {          

          string r = rule.substr(0, rule.find("."));
          string d = dst.substr(0, dst.find("."));

          if (!r.compare("*") || !r.compare(d)) {
            rule = rule.substr(rule.find(".") + 1);
            dst = dst.substr(dst.find(".") + 1);
          
            if (i == 3)
              return "ACCEPT";
          }
          else
            break;
        }
      }
    }

    char reply[8] = {0};
    reply[1] = REJECT;
    boost::asio::async_write(client_socket_, boost::asio::buffer(reply, sizeof(reply)),
      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
			  if (ec) {
          cerr << "reply error" << endl;
        }
		});
    
    conf.close();
    client_socket_.close();

    return "REJECT";
  }

  void connection_info(int connect_type, uint16_t dst_port, string dst_ip) {

    cout << "<S_IP>: " << client_socket_.remote_endpoint().address() << endl;
    cout << "<S_PORT>: " << client_socket_.remote_endpoint().port() << endl;
    cout << "<D_IP>: " << dst_ip << endl;
    cout << "<D_PORT>: " << dst_port << endl;
    cout << "<Command>: " << ((connect_type == 1) ? "CONNECT" : "BIND") << endl;
    cout << "<Reply>: " << firewallStatus(connect_type, dst_ip) << endl;
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

  void recv_sock4() {
    auto self(shared_from_this());
    client_socket_.async_receive(boost::asio::buffer(data_, max_length),
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

			      connection_info(conn_type, port, ip);

            server_socket_.async_connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port), [this, self](boost::system::error_code ec){
              if (!ec) {
                char reply[8] = {0};
                reply[1] = GRANTED;

                boost::asio::async_write(client_socket_, boost::asio::buffer(reply, sizeof(reply)),
                      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
			                  if (!ec) {
                          client_read();
                          server_read();
                        }
			          }); 
              }
            });
			    }
			  });
  }

  void client_read() {
    auto self(shared_from_this());
    client_socket_.async_read_some(boost::asio::buffer(data_, max_length),
			    [this, self](boost::system::error_code ec, std::size_t length) {
			      if (!ec) {
				      server_write(length);
			      }
			    });
  }

  void client_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(client_socket_, boost::asio::buffer(data_, length),
			     [this, self](boost::system::error_code ec, std::size_t /*length*/) {
			       if (!ec) {
				      server_read();
			       }
			     });
  }

  void server_read() {
    auto self(shared_from_this());
    server_socket_.async_read_some(boost::asio::buffer(data_, max_length),
			    [this, self](boost::system::error_code ec, std::size_t length) {
			      if (!ec) {
				      client_write(length);
			      }
			    });
  }

  void server_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(server_socket_, boost::asio::buffer(data_, length),
			     [this, self](boost::system::error_code ec, std::size_t /*length*/) {
			       if (!ec) {
				      client_read();
			       }
			     });
  }

  tcp::socket client_socket_;
  tcp::socket server_socket_;
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
