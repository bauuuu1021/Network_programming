#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

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
    do_receive();
  }

private:

  void print_info(int connect_type, uint16_t dst_port, string dst_ip) {

    cout << "<S_IP>: " << socket_.remote_endpoint().address() << endl;
	  cout << "<S_PORT>: " << socket_.remote_endpoint().port() << endl;
    cout << "<D_IP>: " << dst_ip << endl;
    cout << "<D_PORT>: " << dst_port << endl;
    cout << "<Command>: " << ((connect_type==1)?"CONNECT":"BIND") << endl;
    cout << "<Reply>: " << endl;
	  cout << "----------------------------\n";
  }

  void do_receive()
  {
    auto self(shared_from_this());
    socket_.async_receive(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            int conn_type = (uint8_t)data_[1];
            uint16_t port = *(uint16_t*)&data_[2];
            
            string ip = to_string((data_[4])&0xff) + "." + to_string((data_[5])&0xff) 
                        + "." + to_string((data_[6])&0xff);
            if (!(ip.compare("0.0.0")) && ((data_[7])&0xff)) {
              // resolve domaine name
            }
            else {
              ip = ip + "." + to_string((data_[7])&0xff);
            }

            print_info(conn_type, port, ip);
          }
        });
  }

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            //do_write(length);
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
    do_accept();
  }


private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
	          std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
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
      std::cerr << "Usage: " << argv[0] << " " << "<port>\n";
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
