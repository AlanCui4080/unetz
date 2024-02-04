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

#include "unetz.hpp"

#include <bit>
unetz::ip::address::address(const std::string& str)
{
    inet_pton(AF_INET6, str.c_str(), static_cast<in6_addr*>(this));
}
unetz::ip::port::port(unsigned short val)
    : data(std::endian::native != std::endian::big ? std::byteswap(val) : val)
{
}
unetz::http_server::http_server()
{
    acceptor_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (acceptor_socket < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "failed to socket()");
    }
    auto result = bind(acceptor_socket,
                       reinterpret_cast<sockaddr*>(&listen_address),
                       sizeof(listen_address));
    if (result < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "failed to bind()");
    }
    result = listen(acceptor_socket, queued_connection_max);
    if (result < 0) {
        throw std::system_error(errno, std::generic_category(),
                                "failed to listen()");
    }
    poller_thread = std::thread([&]() {
        while (true) {
            sockaddr_in6 peer_address;
            socklen_t    peer_address_length;
            auto         peer_fd = accept(acceptor_socket,
                                          reinterpret_cast<sockaddr*>(&peer_address),
                                          &peer_address_length);
            if (peer_fd < 0) {
                throw std::system_error(errno, std::generic_category(),
                                        "failed to accept()");
            }
            std::lock_guard lg(mutex_dispatcher_thread_list);
            dispatcher_thread_list.push_back(std::thread([&]() {
                // do sth
                std::string              path;
                std::vector<std::string> argument_list;
                auto unique_stream = std::make_unique<basic_netstream<char>>(
                    basic_netbuf<char>(peer_fd, peer_address));
                auto            route_result = std::async(std::launch::async,
                                                          route_list[path],
                                                          std::move(unique_stream));
                std::lock_guard lg(mutex_dispatcher_thread_list);
                produce_result_list.emplace_back(std::move(route_result));
            }));
        }
    });
}
