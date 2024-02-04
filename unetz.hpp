// This file is a part of unetz
// Copyright (C) 2024 AlanCui4080

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <mutex>
namespace unetz
{
    namespace ip
    {
        class address : public in6_addr {
        public:
            address(const std::string& str);
        };
        class port {
        public:
            std::uint16_t data;

        public:
            port(unsigned short val);
        };
    } // namespace ip

    template <typename _CharT, typename _Traits = std::char_traits<_CharT>>
    class basic_netbuf : public std::basic_streambuf<_CharT, _Traits> {
    public:
        using file_descriptor_type = int;
        typedef _CharT                         char_type;
        typedef _Traits                        traits_type;
        typedef typename traits_type::int_type int_type;
        typedef typename traits_type::pos_type pos_type;
        typedef typename traits_type::off_type off_type;

    private:
        file_descriptor_type socket_fd;
        sockaddr_in6         socket_address;

    public:
        basic_netbuf()                          = delete;
        basic_netbuf(const basic_netbuf<char>&) = delete;
        basic_netbuf(basic_netbuf<char>&& rhs)
        {
            socket_fd      = rhs.socket_fd;
            socket_address = rhs.socket_address;
        }
        basic_netbuf(const ip::address& addr, const ip::port& port)
        {
            socket_address.sin6_family = AF_INET6;
            socket_address.sin6_addr   = addr;
            socket_address.sin6_port   = port.data;
            socket_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
            if (socket_fd < 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "failed to socket()");
            }
            auto result = connect(socket_fd,
                                  reinterpret_cast<sockaddr*>(&socket_address),
                                  sizeof(socket_address));
            if (result < 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "failed to connect()");
            }
        }
        basic_netbuf(file_descriptor_type fd, sockaddr_in6 addr)
            : socket_fd(fd)
            , socket_address(addr)
        {
        }
        ~basic_netbuf()
        {
            shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
        }

    protected:
        virtual std::streamsize xsputn(const char_type* sbuf,
                                       std::streamsize  count) override
        {
            auto result = send(socket_fd, sbuf, count, 0);
            if (result < 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "failed to send()");
            }
            return result;
        }
        virtual std::streamsize xsgetn(char_type*      sbuf,
                                       std::streamsize count) override
        {
            auto result = recv(socket_fd, sbuf, count, 0);
            if (result < 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "failed to recv()");
            }
            return result;
        }
        virtual int_type underflow() override
        {
            char_type c;
            auto      result = recv(socket_fd, &c, 1, MSG_PEEK);
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
        virtual int_type uflow() override
        {
            char_type c;
            if (xsgetn(&c, 1)) {
                return traits_type::to_int_type(c);
            } else {
                return traits_type::eof();
            }
        }
        virtual int_type overflow(int_type c = traits_type::eof()) override
        {
            char_type ch = traits_type::to_char_type(c);
            if (xsputn(&ch, 1)) {
                return traits_type::to_int_type(c);
            } else {
                return traits_type::eof();
            }
        }
    };
    // NOT THREAD-SAFE
    template <typename _CharT, typename _Traits = std::char_traits<_CharT>>
    class basic_netstream : public std::basic_iostream<_CharT, _Traits> {
    public:
        typedef _CharT                         char_type;
        typedef _Traits                        traits_type;
        typedef typename traits_type::int_type int_type;
        typedef typename traits_type::pos_type pos_type;
        typedef typename traits_type::off_type off_type;

    private:
        basic_netbuf<char_type, traits_type> buffer;

    public:
        basic_netstream(basic_netbuf<char_type, traits_type>&& buf)
            : buffer(std::move(buf))
            , std::basic_iostream<char_type, traits_type>(nullptr)
        {
            std::basic_ios<char_type, traits_type>::rdbuf(&buffer);
        }
        ~basic_netstream()
        {
        }
    };
    class http_server {
    public:
        using file_descriptor_type = basic_netbuf<char>::file_descriptor_type;
        using route_produce_type =
            std::function<void(std::unique_ptr<basic_netstream<char>>)>;

    private:
        static constexpr auto queued_connection_max = 16;
        file_descriptor_type  acceptor_socket;
        sockaddr_in6          listen_address;
        std::thread           poller_thread;

        std::mutex               mutex_dispatcher_thread_list;
        std::vector<std::thread> dispatcher_thread_list;

        std::mutex                     mutex_produce_result_list;
        std::vector<std::future<void>> produce_result_list;

        std::mutex                                mutex_route_list;
        std::map<std::string, route_produce_type> route_list;

    public:
        http_server();

    public:
        void register_route(const std::string& path, route_produce_type produce)
        {
            std::lock_guard lg(mutex_route_list);
            route_list.insert_or_assign(path, produce);
        }
    };

} // namespace unetz
