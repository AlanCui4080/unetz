#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include <arpa/inet.h>
#include <sys/socket.h>
namespace unetz {
namespace ip {
class address : public in6_addr {
public:
  address(const std::string &str);
};
class port {
public:
  std::uint16_t data;

public:
  port(unsigned short val);
};
} // namespace ip

template <typename _CharT, typename _Traits = std::char_traits<_CharT>>
class basicnet_buf : public std::basic_streambuf<_CharT, _Traits> {
public:
  using file_descriptor_type = int;
  typedef _CharT char_type;
  typedef _Traits traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;

private:
  file_descriptor_type socket_fd;
  sockaddr_in6 socket_address;

public:
  basicnet_buf(const ip::address &addr, const ip::port &port) {
    socket_address.sin6_family = AF_INET6;
    socket_address.sin6_addr = addr;
    socket_address.sin6_port = port.data;
    socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "failed to socket()");
    }
    auto result =
        connect(socket_fd, reinterpret_cast<sockaddr *>(&socket_address),
                sizeof(socket_address));
    if (result < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "failed to connect()");
    }
  }

protected:
  virtual std::streamsize xsputn(const char_type *sbuf,
                                 std::streamsize count) override {
    auto result = send(socket_fd, sbuf, count, 0);
    if (result < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "failed to send()");
    }
    return result;
  }
  virtual std::streamsize xsgetn(char_type *sbuf,
                                 std::streamsize count) override {
    auto result = recv(socket_fd, sbuf, count, 0);
    if (result < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "failed to recv()");
    }
    return result;
  }
  virtual int_type underflow() override {
    char_type c;
    auto result = recv(socket_fd, &c, 1, MSG_PEEK);
    if (result < 0) {
      throw std::system_error(errno, std::generic_category(),
                              "failed to recv()");
    }
    if (result) {
      return traits_type::to_int_type(c);
    } else {
      return traits_type::eof();
    }
  }
  virtual int_type uflow() override {
    char_type c;
    if (xsgetn(&c, 1)) {
      return traits_type::to_int_type(c);
    } else {
      return traits_type::eof();
    }
  }
  virtual int_type overflow(int_type c = traits_type::eof()) override {
    char_type ch = traits_type::to_char_type(c);
    if (xsputn(&ch, 1)) {
      return traits_type::to_int_type(c);
    } else {
      return traits_type::eof();
    }
  }
};

template <typename _CharT, typename _Traits = std::char_traits<_CharT>>
class basic_netstream : public std::basic_iostream<_CharT, _Traits> {
public:
  typedef _CharT char_type;
  typedef _Traits traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;
  basic_netstream(const ip::address &addr, const ip::port &port)
      : std::basic_iostream<char_type>(
            new basicnet_buf<char_type, traits_type>(addr, port)) {}
};
} // namespace unetz
