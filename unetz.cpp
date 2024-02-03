#include "unetz.hpp"
#include <bit>
unetz::ip::address::address(const std::string &str) {
  inet_pton(AF_INET6, str.c_str(), static_cast<in6_addr *>(this));
}
unetz::ip::port::port(unsigned short val)
    : data(std::endian::native != std::endian::big ? std::byteswap(val) : val) {
}
