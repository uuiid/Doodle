//
// Created by TD on 24-11-15.
//

#pragma once
#include <boost/beast.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
// #include <boost/iostreams/>

namespace doodle::http {
struct zlib_deflate_file_body {
  // using value_type = boost::beast::http::file_body::value_type;

  class value_type {
   public:
    std::shared_ptr<boost::iostreams::file_source> source_;
    value_type() {}
    void open(const std::filesystem::path& path, std::ios::openmode mode, boost::system::error_code& ec) {
      source_ = std::make_shared<boost::iostreams::file_source>(path.generic_string(), mode);
      if (!source_->is_open()) {
        ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
      }
    }
  };

  class writer;

  class writer {
    boost::iostreams::filtering_stream<boost::iostreams::input> stream_;
    char buf_[BOOST_BEAST_FILE_BUFFER_SIZE];
    value_type& body_;

   public:
    using const_buffers_type = boost::asio::const_buffer;
    template <bool isRequest, class Fields>
    explicit writer(boost::beast::http::header<isRequest, Fields> const& in_h, value_type& b) : body_(b) {
      BOOST_ASSERT(body_.source_->is_open());
      stream_.push(boost::iostreams::zlib_compressor{});
      stream_.push(*b.source_);
    }

    void init(boost::system::error_code& ec) {
      BOOST_ASSERT(body_.source_->is_open());
      ec.clear();
    }

    boost::optional<std::pair<const_buffers_type, bool>> get(boost::system::error_code& ec) {
      if (!stream_) return boost::none;
      std::size_t const n = stream_.read(buf_, BOOST_BEAST_FILE_BUFFER_SIZE).gcount();
      if (n == 0) {
        ec = {};
        return boost::none;
      }
      BOOST_ASSERT(n != 0);
      ec = {};
      return {{boost::asio::const_buffer(buf_, n), !!stream_}};
    }
  };

  class reader {
    value_type& body_;
    boost::iostreams::filtering_stream<boost::iostreams::output> stream_;

   public:
    template <bool isRequest, class Fields>
    explicit reader(boost::beast::http::header<isRequest, Fields>&, value_type& b) : body_(b) {
      stream_.push(*body_.source_);
    }

    void init(boost::optional<std::uint64_t> const& content_length, boost::system::error_code& ec) {
      // VFALCO We could reserve space in the file
      boost::ignore_unused(content_length);
      BOOST_ASSERT(body_.source_->is_open());
      ec = {};
    }

    template <class ConstBufferSequence>
    std::size_t put(ConstBufferSequence const& buffers, boost::system::error_code& ec) {
      std::size_t nwritten = 0;
      for (auto buffer : boost::beast::buffers_range_ref(buffers)) {
        if (!stream_.write(buffer.data(), buffer.size())) {
          ec = boost::system::errc::make_error_code(boost::beast::errc::bad_file_descriptor);
          return nwritten;
        }
        nwritten += buffer.size();
      }
      ec = {};
      return buffers.size();
    }
    void finish(boost::system::error_code& ec) {
      stream_.flush();
      ec = {};
    }
  };
};
}  // namespace doodle::http