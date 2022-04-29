#include <server/server.h>
#include <iostream>

void session::start() {
    do_read();
}

void session::do_read() {
//    boost::asio::async_read(
//            socket_,boost::asio::buffer(data_),
//            [self = shared_from_this(), this](boost::system::error_code in_err,
//                                              std::size_t in_len) {
//                if (!in_err) {
//                    std::cout << "read " << data_.substr(0, in_len) << std::endl;
//                    this->do_write(in_len);
//                } else {
//                    std::cout << "read err " << in_err.message() << std::endl;
//                    socket_.close();
//                }
//            }
//    );
    socket_.async_read_some(
            boost::asio::buffer(data_),
            [self = shared_from_this(), this](boost::system::error_code in_err,
                                              std::size_t in_len) {
                if (!in_err) {
                    std::cout << "read " << data_.substr(0, in_len) << std::endl;
                    this->do_write(in_len);
                } else {
                    std::cout << "read err " << in_err.message() << std::endl;
                    socket_.close();
                }
            });

}

void session::do_write(std::size_t in_len) {
    msg_ = data_.substr(0, in_len);
    boost::asio::async_write(
            socket_,
            boost::asio::buffer(msg_),
            [self = shared_from_this(), this, in_len](boost::system::error_code in_err, std::size_t in_len_) {

                if (!in_err) {
                    std::cout << "write " << data_.substr(0, in_len) << std::endl;
                    this->do_read();
                } else {
                    std::cout << "write err " << in_err.message() << std::endl;
                }
                socket_.close();
            });
//    socket_.async_write_some(
//            boost::asio::buffer(msg_),
//            [self = shared_from_this(), this, in_len](boost::system::error_code in_err, std::size_t in_len_) {
//
//                if (!in_err) {
//                    std::cout << "write " << data_.substr(0, in_len) << std::endl;
//                    this->do_read();
//                } else {
//                    std::cout << "write err " << in_err.message() << std::endl;
//                }
//                socket_.close();
//            });
}
void session::stop() {
  socket_.close();
}

void server::do_accept() {
    acceptor_.async_accept(
            [this](boost::system::error_code in_err, boost::asio::ip::tcp::socket &in_socket) {
                if (!in_err) {
                    std::make_shared<session>(std::move(in_socket))->start();
                }
                this->do_accept();
            });
}
