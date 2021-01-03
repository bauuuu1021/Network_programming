#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <boost/asio.hpp>

#define MAX_SERVER 5

using namespace std;
using boost::asio::ip::tcp;

typedef struct query_info_ {
  string hostname;
  string port;
  string testcase;
  int session_id;
} query_info;

vector<query_info> query_list;

class Web_session
{
public:

  Web_session() {}

  void init() {
    cout << "Content-type: text/html\r\n\r\n";
    cout <<  
      "<!DOCTYPE html>"
      "<html lang=\"en\">"
        "<head>"
          "<meta charset=\"UTF-8\" />"
          "<title>NP Project 3 Console</title>"
          "<link"
            "rel=\"stylesheet\""
            "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\""
            "integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\""
            "crossorigin=\"anonymous\""
          "/>"
          "<link"
            "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\""
            "rel=\"stylesheet\""
          "/>"
          "<link"
            "rel=\"icon\""
            "type=\"image/png\""
            "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\""
          "/>"
          "<style>"
            "* {"
              "font-family: 'Source Code Pro', monospace;"
              "font-size: 1rem !important;"
            "}"
            "body {"
              "background-color: #212529;"
            "}"
            "pre {"
              "color: #cccccc;"
            "}"
            "cmd {"
              "color: #01b468;"
            "}"
            "host {"
              "color: #f7ff0a;"
            "}"
          "</style>"
        "</head>"
        "<body>"
          "<table class=\"table table-dark table-bordered\">"
            "<thead>"
              "<tr>";
    for (auto q : query_list) {
      cout << "<th scope=\"col\"><host>" << q.hostname << ":" << q.port << "</host></th>";
    } 
    cout <<   "</tr>"
            "</thead>"
            "<tbody>"
              "<tr>";
    for (auto q : query_list) {
      cout << "<td><pre id=" << q.session_id << " class=\"mb-0\"></pre></td>";
    }                   
    cout <<   "</tr>"
            "</tbody>"
          "</table>"
        "</body>"
      "</html>";
  }

  void setSessionID(int id) {
    this->session = id;
  }

  void escape(std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            case '\n': buffer.append("&NewLine;");   break;
            case '\r': buffer.append("");            break;
            case '\t': buffer.append("&emsp");       break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    data.swap(buffer);
  }

  void output_shell(string s) {
    escape(s);
    cout << "<script>document.getElementById(\'" << this->session << "\').innerHTML += \'" << s << "\';</script>" << endl;
  }

  void output_command(string s) {
    escape(s);
    cout << "<script>document.getElementById(\'" << this->session << "\').innerHTML += \'<cmd>" << s << "</cmd>\';</script>" << endl;
  }

private:
  int session;
};

class NP_client 
{
public:
  NP_client(boost::asio::io_context& io_context, tcp::endpoint end, string file, int session_id)
    : socket_(io_context)
  {
    web.setSessionID(session_id);
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
          web.output_shell(string(r_buf));
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

    string cmd = cmd_Q.front() + "\n";
    web.output_command(string(cmd));
    cmd_Q.pop();

    strcpy(w_buf, cmd.c_str());
    size_t length = cmd.size();

    boost::asio::async_write(socket_, boost::asio::buffer(w_buf, length),
      [this](boost::system::error_code ec, size_t /*length*/){});
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

  Web_session web;
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

void parseQueryString() {
  Web_session web;
  string query_string = string(getenv("QUERY_STRING"));

  for (auto i = 0; i < MAX_SERVER; i++) {
    query_info query;

    string host_start = "h" + to_string(i);
    string port_start = "p" + to_string(i);
    string file_start = "f" + to_string(i);

    unsigned pos;
    pos = query_string.find("&");
    query.hostname = query_string.substr(3, pos - 3);
    query_string = query_string.substr(pos + 1);

    pos = query_string.find("&");
    query.port = query_string.substr(3, pos - 3);
    query_string = query_string.substr(pos + 1);

    pos = query_string.find("&");
    if (pos != string::npos) {
      query.testcase = query_string.substr(3, pos - 3);
      query_string = query_string.substr(pos + 1);
    }
    else
      query.testcase = query_string.substr(3);

    if (!query.hostname.empty()) {
      query.session_id = i;
      query_list.push_back(query);
    }
  }

  web.init();
  
}

int main(int argc, char* argv[])
{
  try
  {
    parseQueryString();
    for (auto q : query_list) {
      boost::asio::io_context io_context;
      tcp::endpoint end = resolveDNS(q.hostname, (unsigned short)stoul(q.port, NULL, 0));
      string filename("test_case/" + q.testcase);
      NP_client client(io_context, end, filename, q.session_id);
      io_context.run();
    }
  }
  catch (std::invalid_argument& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}