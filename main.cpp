#include "unetz.hpp"
using namespace unetz;
int main() {
  // well example.com
  basic_netstream<char> ms(ip::address("2606:2800:220:1:248:1893:25c8:1946"),
                           ip::port(80));
  ms << "GET / HTTP/1.0\r\n"
     << "Host: example.com\r\n"
     << "User-Agent: Mozilla/5.0\r\n"
     << "Accept: text/html,application/xhtml+xml,application/xml;\r\n"
     << "Accept-Language: en-US,en;q=0.5\r\n"
        "\r\n\r\n";
  while (ms.good()) {
    char w[128];
    ms.getline(w, 128, '\n');
    std::cout << w << std::endl;
  }
}
