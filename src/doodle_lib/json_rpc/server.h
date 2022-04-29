#pragma once

#include <boost/asio.hpp>

class session : public std::enable_shared_from_this<session> {
    boost::asio::ip::tcp::socket socket_;

public:
    explicit session(boost::asio::ip::tcp::socket in_socket)
            : socket_(std::move(in_socket)),
              data_(std::size_t(1024), '\0') {

    };

    void start();
    void stop();
private:
    void do_read();

    void do_write(std::size_t in_len);

    std::string data_{};
    std::string msg_{};
};

class server {
    boost::asio::ip::tcp::acceptor acceptor_;

public:
    explicit server(boost::asio::io_context &in_io_context,
                    std::uint16_t in_port)
            : acceptor_(in_io_context, boost::asio::ip::tcp::endpoint{
            boost::asio::ip::address::from_string("127.0.0.1"), in_port}) {
        do_accept();
    };

private:
    void do_accept();
};
