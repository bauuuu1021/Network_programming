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
#include <queue>

#define MAX_SERVER 5

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;
string QUERY_STRING;
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

  string init() {
    string s = 
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
            "err {"
              "color: #fc0000;"
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
      s = s + "<th scope=\"col\"><host>" + q.hostname + ":" + q.port + "</host></th>";
    } 
    s = s +  "</tr>"
            "</thead>"
            "<tbody>"
              "<tr>";
    for (auto q : query_list) {
      s = s + "<td><pre id=" + to_string(q.session_id) + " class=\"mb-0\"></pre></td>";
    }                   
    s = s +  "</tr>"
            "</tbody>"
          "</table>"
        "</body>"
      "</html>";

    return s;
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

  string output_shell(string s) {
    escape(s);
    return "<script>document.getElementById(\'" + to_string(this->session) + "\').innerHTML += \'" + s +  "\';</script>\n";
  }

  string output_command(string s) {
    escape(s);
    return "<script>document.getElementById(\'" + to_string(this->session) + "\').innerHTML += \'<cmd><b>" + s + "</b></cmd>\';</script>\n";
  }

  string output_err(string s) {
    escape(s);
    return "<script>document.getElementById(\'" + to_string(this->session) + "\').innerHTML += \'<err><b>" + s + "</b></err>\';</script>\n";  
  }

private:
  int session;
};

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

class NP_client 
{
public:
  NP_client(boost::asio::io_context& io_context, query_info query)
    : socket_(io_context)
  {
    web.setSessionID(query.session_id);
    load_cmd("test_case/" + query.testcase);
    do_connection(resolveDNS(query.hostname, (unsigned short)stoul(query.port, NULL, 0)));
  }

private:

  tcp::endpoint resolveDNS(string hostname, unsigned short port) {

    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(hostname, to_string(port));
    tcp::resolver::iterator iter = resolver.resolve(query);
    tcp::endpoint endpoint = iter->endpoint();

    return endpoint;
  }

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
          cout << web.output_shell(string(r_buf));
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
    cout << web.output_command(string(cmd));
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
        web.output_err("Connection failed\n");
      }
    });
  }

  Web_session web;
  tcp::socket socket_;
  enum { max_length = 1024 };
  char r_buf[max_length], w_buf[max_length];
  queue<string> cmd_Q;
};

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
      response = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
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

  string parseRequest(string request) {
    sscanf(r_buf, "%s %s %s %s %s", REQUEST_METHOD, REQUEST_URI, SERVER_PROTOCOL, ignore, HTTP_HOST);
    
    string cgi_name(REQUEST_URI);
    cgi_name = cgi_name.substr(1);
    size_t pos;
    if ((pos = cgi_name.find("?")) != string::npos) {
      QUERY_STRING = cgi_name.substr(pos + 1);
      cgi_name = cgi_name.substr(0, pos);
    }

    if (cgi_name.compare(cgi_name.size()-4, 4, ".cgi")) {
      response(false);
      return "";
    }
    response(true);
      
    return cgi_name;
  }

  string panel_content() {
      const char* s = R"(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <title>NP Project 3 Panel</title>
            <link
            rel="stylesheet"
            href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
            integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
            crossorigin="anonymous"
            />
            <link
            href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
            rel="stylesheet"
            />
            <link
            rel="icon"
            type="image/png"
            href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"
            />
            <style>
            * {
                font-family: 'Source Code Pro', monospace;
            }
            </style>
        </head>
        <body class="bg-secondary pt-5">
            <form action="console.cgi" method="GET">
            <table class="table mx-auto bg-light" style="width: inherit">
                <thead class="thead-dark">
                <tr>
                    <th scope="col">#</th>
                    <th scope="col">Host</th>
                    <th scope="col">Port</th>
                    <th scope="col">Input File</th>
                </tr>
                </thead>
                <tbody>
                <tr>
                    <th scope="row" class="align-middle">Session 1</th>
                    <td>
                    <div class="input-group">
                        <select name="h0" class="custom-select">
                        <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                        </select>
                        <div class="input-group-append">
                        <span class="input-group-text">.cs.nctu.edu.tw</span>
                        </div>
                    </div>
                    </td>
                    <td>
                    <input name="p0" type="text" class="form-control" size="5" />
                    </td>
                    <td>
                    <select name="f0" class="custom-select">
                        <option></option>
                        <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                    </select>
                    </td>
                </tr>
                <tr>
                    <th scope="row" class="align-middle">Session 2</th>
                    <td>
                    <div class="input-group">
                        <select name="h1" class="custom-select">
                        <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                        </select>
                        <div class="input-group-append">
                        <span class="input-group-text">.cs.nctu.edu.tw</span>
                        </div>
                    </div>
                    </td>
                    <td>
                    <input name="p1" type="text" class="form-control" size="5" />
                    </td>
                    <td>
                    <select name="f1" class="custom-select">
                        <option></option>
                        <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                    </select>
                    </td>
                </tr>
                <tr>
                    <th scope="row" class="align-middle">Session 3</th>
                    <td>
                    <div class="input-group">
                        <select name="h2" class="custom-select">
                        <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                        </select>
                        <div class="input-group-append">
                        <span class="input-group-text">.cs.nctu.edu.tw</span>
                        </div>
                    </div>
                    </td>
                    <td>
                    <input name="p2" type="text" class="form-control" size="5" />
                    </td>
                    <td>
                    <select name="f2" class="custom-select">
                        <option></option>
                        <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                    </select>
                    </td>
                </tr>
                <tr>
                    <th scope="row" class="align-middle">Session 4</th>
                    <td>
                    <div class="input-group">
                        <select name="h3" class="custom-select">
                        <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                        </select>
                        <div class="input-group-append">
                        <span class="input-group-text">.cs.nctu.edu.tw</span>
                        </div>
                    </div>
                    </td>
                    <td>
                    <input name="p3" type="text" class="form-control" size="5" />
                    </td>
                    <td>
                    <select name="f3" class="custom-select">
                        <option></option>
                        <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                    </select>
                    </td>
                </tr>
                <tr>
                    <th scope="row" class="align-middle">Session 5</th>
                    <td>
                    <div class="input-group">
                        <select name="h4" class="custom-select">
                        <option></option><option value="nplinux1.cs.nctu.edu.tw">nplinux1</option><option value="nplinux2.cs.nctu.edu.tw">nplinux2</option><option value="nplinux3.cs.nctu.edu.tw">nplinux3</option><option value="nplinux4.cs.nctu.edu.tw">nplinux4</option><option value="nplinux5.cs.nctu.edu.tw">nplinux5</option><option value="nplinux6.cs.nctu.edu.tw">nplinux6</option><option value="nplinux7.cs.nctu.edu.tw">nplinux7</option><option value="nplinux8.cs.nctu.edu.tw">nplinux8</option><option value="nplinux9.cs.nctu.edu.tw">nplinux9</option><option value="nplinux10.cs.nctu.edu.tw">nplinux10</option><option value="nplinux11.cs.nctu.edu.tw">nplinux11</option><option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                        </select>
                        <div class="input-group-append">
                        <span class="input-group-text">.cs.nctu.edu.tw</span>
                        </div>
                    </div>
                    </td>
                    <td>
                    <input name="p4" type="text" class="form-control" size="5" />
                    </td>
                    <td>
                    <select name="f4" class="custom-select">
                        <option></option>
                        <option value="t1.txt">t1.txt</option><option value="t2.txt">t2.txt</option><option value="t3.txt">t3.txt</option><option value="t4.txt">t4.txt</option><option value="t5.txt">t5.txt</option>
                    </select>
                    </td>
                </tr>
                <tr>
                    <td colspan="3"></td>
                    <td>
                    <button type="submit" class="btn btn-info btn-block">Run</button>
                    </td>
                </tr>
                </tbody>
            </table>
            </form>
        </body>
        </html>)";
  
    strcpy(w_buf, s);
    return string(s);
  }

  string parseQueryString() {
    Web_session web;
    string query_string = QUERY_STRING;

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

      if (!query.hostname.empty() && !query.port.empty() && !query.testcase.empty()) {
        query.session_id = i;
        query_list.push_back(query);
      }
    }

    return web.init();
    
  }

  void cgi_handler(string cgi_name) {
    
    if (!cgi_name.compare(0, cgi_name.size(), "panel.cgi")) {
      do_write(panel_content().size());
    }
    else if (!cgi_name.compare(0, cgi_name.size(), "console.cgi")) {
      query_list.clear();
      auto web_init = parseQueryString();
      strcpy(w_buf, web_init.c_str());  
      do_write(web_init.size());

      for (auto q : query_list) {
        boost::asio::io_context io_context;
        NP_client client(io_context, q);
        io_context.run();
      }
    }
  }  

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(r_buf, max_length),
      [this, self](boost::system::error_code ec, size_t length) {
        if (!ec) {
          string cgi_name = parseRequest(r_buf);
          if (!cgi_name.empty()) {
            cgi_handler(cgi_name);
          }
        }
    });
  }

  void do_write(size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(w_buf, length),
        [this, self](boost::system::error_code ec, size_t /*length*/)
        {});
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char r_buf[max_length*50], w_buf[max_length*50];
  char REQUEST_METHOD[max_length], REQUEST_URI[max_length],
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
            make_shared<session>(move(socket))->start();
            do_accept();
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
