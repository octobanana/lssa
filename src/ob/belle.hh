/*
                                    88888888
                                  888888888888
                                 88888888888888
                                8888888888888888
                               888888888888888888
                              888888  8888  888888
                              88888    88    88888
                              888888  8888  888888
                              88888888888888888888
                              88888888888888888888
                             8888888888888888888888
                          8888888888888888888888888888
                        88888888888888888888888888888888
                              88888888888888888888
                            888888888888888888888888
                           888888  8888888888  888888
                           888     8888  8888     888
                                   888    888

                                   OCTOBANANA

Belle
0.6.0-develop-stripped

An HTTP / Websocket library in C++17 using Boost.Beast and Boost.ASIO.
https://octobanana.com/software/belle

Licensed under the MIT License

Copyright (c) 2018-2019 Brett Robinson <https://octobanana.com/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef OB_BELLE_HH
#define OB_BELLE_HH

#define OB_BELLE_VERSION_MAJOR 0
#define OB_BELLE_VERSION_MINOR 6
#define OB_BELLE_VERSION_PATCH 0

// Config Begin

// compile with -DOB_BELLE_CONFIG_<OPT> or
// comment out defines to alter the library

// zlib support
#ifndef OB_BELLE_CONFIG_ZLIB_OFF
#define OB_BELLE_CONFIG_ZLIB_ON
#endif // OB_BELLE_CONFIG_ZLIB_OFF

// signal support
#ifndef OB_BELLE_CONFIG_SIGNAL_OFF
#define OB_BELLE_CONFIG_SIGNAL_ON
#endif // OB_BELLE_CONFIG_SIGNAL_OFF

// ssl support
#ifndef OB_BELLE_CONFIG_SSL_OFF
#define OB_BELLE_CONFIG_SSL_ON
#endif // OB_BELLE_CONFIG_SSL_OFF

// client support
#ifndef OB_BELLE_CONFIG_CLIENT_OFF
#define OB_BELLE_CONFIG_CLIENT_ON
#endif // OB_BELLE_CONFIG_CLIENT_OFF

// Config End

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>

#ifdef OB_BELLE_CONFIG_ZLIB_ON
#include <zlib.h>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#endif // OB_BELLE_CONFIG_ZLIB_ON

#ifdef OB_BELLE_CONFIG_CLIENT_ON
#include <boost/asio/connect.hpp>
#include <boost/asio/read_until.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#endif // OB_BELLE_CONFIG_CLIENT_ON

#ifdef OB_BELLE_CONFIG_SSL_ON
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#endif // OB_BELLE_CONFIG_SSL_ON

#include <boost/config.hpp>

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <csignal>

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <array>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <algorithm>
#include <functional>
#include <regex>
#include <memory>
#include <chrono>
#include <utility>
#include <initializer_list>
#include <optional>
#include <limits>
#include <type_traits>
#include <thread>
#include <fstream>
#include <filesystem>

namespace OB::Belle
{

// aliases
namespace fs = std::filesystem;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;

#ifdef OB_BELLE_CONFIG_SSL_ON
namespace ssl = boost::asio::ssl;
#endif // OB_BELLE_CONFIG_SSL_ON

using io = boost::asio::io_context;
using tcp = boost::asio::ip::tcp;
using error_code = boost::system::error_code;
using Method = boost::beast::http::verb;
using Status = boost::beast::http::status;
using Header = boost::beast::http::field;
using Headers = boost::beast::http::fields;

namespace Detail
{

// prototypes
inline std::string lowercase(std::string str);
inline std::vector<std::string> split(std::string const& str, std::string const& delim,
  std::size_t size = std::numeric_limits<std::size_t>::max());
inline std::string hex_encode(char const c);
inline char hex_decode(std::string const& s);
inline std::string base64_encode(std::string const& val);
inline std::string base64_decode(std::string const& val);

// string to lowercase
inline std::string lowercase(std::string str)
{
  auto const to_lower = [](char& c)
  {
    if (c >= 'A' && c <= 'Z')
    {
      c += 'a' - 'A';
    }

    return c;
  };

  for (char& c : str)
  {
    c = to_lower(c);
  }

  return str;
}

// split a string by a delimiter 'n' times
inline std::vector<std::string> split(std::string const& str,
  std::string const& delim, std::size_t size)
{
  std::vector<std::string> vtok;
  std::size_t start {0};
  auto end = str.find(delim);

  while ((size-- > 0) && (end != std::string::npos))
  {
    vtok.emplace_back(str.substr(start, end - start));
    start = end + delim.length();
    end = str.find(delim, start);
  }

  vtok.emplace_back(str.substr(start, end));

  return vtok;
}

// convert object into a string
template<typename T>
inline std::string to_string(T const& t)
{
  std::stringstream ss;
  ss << t;

  return ss.str();
}

inline std::string hex_encode(char const c)
{
  char s[3];

  if (c & 0x80)
  {
    std::snprintf(&s[0], 3, "%02X",
      static_cast<unsigned int>(c & 0xff)
    );
  }
  else
  {
    std::snprintf(&s[0], 3, "%02X",
      static_cast<unsigned int>(c)
    );
  }

  return std::string(s);
}

inline char hex_decode(std::string const& s)
{
  unsigned int n;

  std::sscanf(s.data(), "%x", &n);

  return static_cast<char>(n);
}

inline std::string base64_encode(std::string const& val)
{
  using it = boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<std::string::const_iterator, 6, 8>>;

  return std::string(it(val.begin()), it(val.end())).append((3 - val.size() % 3) % 3, '=');
}

inline std::string base64_decode(std::string const& val)
{
  using it = boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6>;

  return boost::algorithm::trim_right_copy_if(std::string(it(val.begin()), it(val.end())),
    [](char const c)
    {
      return c == '\0';
    });
}

#ifdef OB_BELLE_CONFIG_SSL_ON
// TODO switch to boost::beast::ssl_stream when it moves out of experimental
template<typename Next_Layer>
class ssl_stream final : public ssl::stream_base
{
// This class (ssl_stream) is a derivative work based on Boost.Beast,
// orignal copyright below:
/*
  Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)

  Boost Software License - Version 1.0 - August 17th, 2003

  Permission is hereby granted, free of charge, to any person or organization
  obtaining a copy of the software and accompanying documentation covered by
  this license (the "Software") to use, reproduce, display, distribute,
  execute, and transmit the Software, and to prepare derivative works of the
  Software, and to permit third-parties to whom the Software is furnished to
  do so, all subject to the following:

  The copyright notices in the Software and this entire statement, including
  the above license grant, this restriction and the following disclaimer,
  must be included in all copies of the Software, in whole or in part, and
  all derivative works of the Software, unless such copies or derivative
  works are solely in the form of machine-executable object code generated by
  a source language processor.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
  SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
  FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

  using stream_type = ssl::stream<Next_Layer>;

public:

  using native_handle_type = typename stream_type::native_handle_type;
  using impl_struct = typename stream_type::impl_struct;
  using next_layer_type = typename stream_type::next_layer_type;
  using lowest_layer_type = typename stream_type::lowest_layer_type;
  using executor_type = typename stream_type::executor_type;

  ssl_stream(Next_Layer&& arg, ssl::context& ctx) :
    _ptr {std::make_unique<stream_type>(std::move(arg), ctx)}
  {
  }

  executor_type get_executor() noexcept
  {
    return _ptr->get_executor();
  }

  native_handle_type native_handle()
  {
    return _ptr->native_handle();
  }

  next_layer_type const& next_layer() const
  {
    return _ptr->next_layer();
  }

  next_layer_type& next_layer()
  {
    return _ptr->next_layer();
  }

  lowest_layer_type& lowest_layer()
  {
    return _ptr->lowest_layer();
  }

  lowest_layer_type const& lowest_layer() const
  {
    return _ptr->lowest_layer();
  }

  void set_verify_mode(ssl::verify_mode v)
  {
    _ptr->set_verify_mode(v);
  }

  void set_verify_mode(ssl::verify_mode v, error_code& ec)
  {
    _ptr->set_verify_mode(v, ec);
  }

  void set_verify_depth(int depth)
  {
    _ptr->set_verify_depth(depth);
  }

  void set_verify_depth(int depth, error_code& ec)
  {
    _ptr->set_verify_depth(depth, ec);
  }

  template<typename VerifyCallback>
  void set_verify_callback(VerifyCallback callback)
  {
    _ptr->set_verify_callback(callback);
  }

  template<typename VerifyCallback>
  void set_verify_callback(VerifyCallback callback, error_code& ec)
  {
    _ptr->set_verify_callback(callback, ec);
  }

  void handshake(handshake_type type)
  {
    _ptr->handshake(type);
  }

  void handshake(handshake_type type, error_code& ec)
  {
    _ptr->handshake(type, ec);
  }

  template<typename ConstBufferSequence>
  void handshake(handshake_type type, ConstBufferSequence const& buffers)
  {
    _ptr->handshake(type, buffers);
  }

  template<typename ConstBufferSequence>
  void handshake(handshake_type type, ConstBufferSequence const& buffers, error_code& ec)
  {
    _ptr->handshake(type, buffers, ec);
  }

  template<typename HandshakeHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(HandshakeHandler, void(error_code))
  async_handshake(handshake_type type, BOOST_ASIO_MOVE_ARG(HandshakeHandler) handler)
  {
    return _ptr->async_handshake(type, BOOST_ASIO_MOVE_CAST(HandshakeHandler)(handler));
  }

  template<typename ConstBufferSequence, typename BufferedHandshakeHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(BufferedHandshakeHandler, void (error_code, std::size_t))
  async_handshake(handshake_type type, ConstBufferSequence const& buffers,
    BOOST_ASIO_MOVE_ARG(BufferedHandshakeHandler) handler)
  {
    return _ptr->async_handshake(type, buffers, BOOST_ASIO_MOVE_CAST(BufferedHandshakeHandler)(handler));
  }

  void shutdown()
  {
    _ptr->shutdown();
  }

  void shutdown(error_code& ec)
  {
    _ptr->shutdown(ec);
  }

  template<typename ShutdownHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ShutdownHandler, void (error_code))
  async_shutdown(BOOST_ASIO_MOVE_ARG(ShutdownHandler) handler)
  {
    return _ptr->async_shutdown(BOOST_ASIO_MOVE_CAST(ShutdownHandler)(handler));
  }

  template<typename ConstBufferSequence>
  std::size_t write_some(ConstBufferSequence const& buffers)
  {
    return _ptr->write_some(buffers);
  }

  template<typename ConstBufferSequence>
  std::size_t write_some(ConstBufferSequence const& buffers, error_code& ec)
  {
    return _ptr->write_some(buffers, ec);
  }

  template<typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void (error_code, std::size_t))
  async_write_some(ConstBufferSequence const& buffers,
    BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
  {
    return _ptr->async_write_some(buffers, BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));
  }

  template<typename MutableBufferSequence>
  std::size_t read_some(MutableBufferSequence const& buffers)
  {
    return _ptr->read_some(buffers);
  }

  template<typename MutableBufferSequence>
  std::size_t read_some(MutableBufferSequence const& buffers, error_code& ec)
  {
    return _ptr->read_some(buffers, ec);
  }

  template<typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(error_code, std::size_t))
  async_read_some(MutableBufferSequence const& buffers,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
  {
    return _ptr->async_read_some(buffers, BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));
  }

private:

  std::unique_ptr<stream_type> _ptr;
}; // class ssl_stream
#endif // OB_BELLE_CONFIG_SSL_ON

} // namespace Detail

namespace Util
{

// Ordered_Map: an insert ordered map
// enables random lookup and insert ordered iterators
// unordered map stores key value pairs
// queue holds insert ordered iterators to each key in the unordered map
template<typename K, typename V>
class Ordered_Map final
{
public:

  // map iterators
  using m_iterator = typename std::unordered_map<K, V>::iterator;
  using m_const_iterator = typename std::unordered_map<K, V>::const_iterator;

  // index iterators
  using i_iterator = typename std::deque<m_iterator>::iterator;
  using i_const_iterator = typename std::deque<m_const_iterator>::const_iterator;


  Ordered_Map()
  {
  }

  Ordered_Map(std::initializer_list<std::pair<K, V>> const& lst)
  {
    for (auto const& [key, val] : lst)
    {
      _it.emplace_back(_map.insert({key, val}).first);
    }
  }

  ~Ordered_Map()
  {
  }

  Ordered_Map& operator()(K const& k, V const& v)
  {
    auto it = _map.insert_or_assign(k, v);

    if (it.second)
    {
      _it.emplace_back(it.first);
    }

    return *this;
  }

  i_iterator operator[](std::size_t index)
  {
    return _it[index];
  }

  i_const_iterator const operator[](std::size_t index) const
  {
    return _it[index];
  }

  V& at(K const& k)
  {
    return _map.at(k);
  }

  V const& at(K const& k) const
  {
    return _map.at(k);
  }

  m_iterator find(K const& k)
  {
    return _map.find(k);
  }

  m_const_iterator find(K const& k) const
  {
    return _map.find(k);
  }

  std::size_t size() const
  {
    return _it.size();
  }

  bool empty() const
  {
    return _it.empty();
  }

  Ordered_Map& clear()
  {
    _it.clear();
    _map.clear();

    return *this;
  }

  Ordered_Map& erase(K const& k)
  {
    auto it = _map.find(k);

    if (it != _map.end())
    {
      for (auto e = _it.begin(); e < _it.end(); ++e)
      {
        if ((*e) == it)
        {
          _it.erase(e);
          break;
        }
      }

      _map.erase(it);
    }

    return *this;
  }

  i_iterator begin()
  {
    return _it.begin();
  }

  i_const_iterator begin() const
  {
    return _it.begin();
  }

  i_const_iterator cbegin() const
  {
    return _it.cbegin();
  }

  i_iterator end()
  {
    return _it.end();
  }

  i_const_iterator end() const
  {
    return _it.end();
  }

  i_const_iterator cend() const
  {
    return _it.cend();
  }

  m_iterator map_begin()
  {
    return _map.begin();
  }

  m_const_iterator map_begin() const
  {
    return _map.begin();
  }

  m_const_iterator map_cbegin() const
  {
    return _map.cbegin();
  }

  m_iterator map_end()
  {
    return _map.end();
  }

  m_const_iterator map_end() const
  {
    return _map.end();
  }

  m_const_iterator map_cend() const
  {
    return _map.cend();
  }

private:

  std::unordered_map<K, V> _map;
  std::deque<m_iterator> _it;
}; // class Ordered_Map

std::unordered_map<std::string, std::string> const mime_types
{
  {"html", "text/html"},
  {"htm", "text/html"},
  {"shtml", "text/html"},
  {"css", "text/css"},
  {"xml", "text/xml"},
  {"gif", "image/gif"},
  {"jpg", "image/jpg"},
  {"jpeg", "image/jpg"},
  {"js", "application/javascript"},
  {"atom", "application/atom+xml"},
  {"rss", "application/rss+xml"},
  {"mml", "text/mathml"},
  {"txt", "text/plain"},
  {"jad", "text/vnd.sun.j2me.app-descriptor"},
  {"wml", "text/vnd.wap.wml"},
  {"htc", "text/x-component"},
  {"png", "image/png"},
  {"tif", "image/tiff"},
  {"tiff", "image/tiff"},
  {"wbmp", "image/vnd.wap.wbmp"},
  {"ico", "image/x-icon"},
  {"jng", "image/x-jng"},
  {"bmp", "image/x-ms-bmp"},
  {"svg", "image/svg+xml"},
  {"svgz", "image/svg+xml"},
  {"webp", "image/webp"},
  {"woff", "application/font-woff"},
  {"jar", "application/java-archive"},
  {"war", "application/java-archive"},
  {"ear", "application/java-archive"},
  {"json", "application/json"},
  {"hqx", "application/mac-binhex40"},
  {"doc", "application/msword"},
  {"pdf", "application/pdf"},
  {"ps", "application/postscript"},
  {"eps", "application/postscript"},
  {"ai", "application/postscript"},
  {"rtf", "application/rtf"},
  {"m3u8", "application/vnd.apple.mpegurl"},
  {"xls", "application/vnd.ms-excel"},
  {"eot", "application/vnd.ms-fontobject"},
  {"ppt", "application/vnd.ms-powerpoint"},
  {"wmlc", "application/vnd.wap.wmlc"},
  {"kml", "application/vnd.google-earth.kml+xml"},
  {"kmz", "application/vnd.google-earth.kmz"},
  {"7z", "application/x-7z-compressed"},
  {"cco", "application/x-cocoa"},
  {"jardiff", "application/x-java-archive-diff"},
  {"jnlp", "application/x-java-jnlp-file"},
  {"run", "application/x-makeself"},
  {"pm", "application/x-perl"},
  {"pl", "application/x-perl"},
  {"pdb", "application/x-pilot"},
  {"prc", "application/x-pilot"},
  {"rar", "application/x-rar-compressed"},
  {"rpm", "application/x-redhat-package-manager"},
  {"sea", "application/x-sea"},
  {"swf", "application/x-shockwave-flash"},
  {"sit", "application/x-stuffit"},
  {"tk", "application/x-tcl"},
  {"tcl", "application/x-tcl"},
  {"crt", "application/x-x509-ca-cert"},
  {"pem", "application/x-x509-ca-cert"},
  {"der", "application/x-x509-ca-cert"},
  {"xpi", "application/x-xpinstall"},
  {"xhtml", "application/xhtml+xml"},
  {"xspf", "application/xspf+xml"},
  {"zip", "application/zip"},
  {"dll", "application/octet-stream"},
  {"exe", "application/octet-stream"},
  {"bin", "application/octet-stream"},
  {"deb", "application/octet-stream"},
  {"dmg", "application/octet-stream"},
  {"img", "application/octet-stream"},
  {"iso", "application/octet-stream"},
  {"msm", "application/octet-stream"},
  {"msp", "application/octet-stream"},
  {"msi", "application/octet-stream"},
  {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
  {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
  {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
  {"kar", "audio/midi"},
  {"midi", "audio/midi"},
  {"mid", "audio/midi"},
  {"mp3", "audio/mpeg"},
  {"ogg", "audio/ogg"},
  {"m4a", "audio/x-m4a"},
  {"ra", "audio/x-realaudio"},
  {"3gp", "video/3gpp"},
  {"3gpp", "video/3gpp"},
  {"ts", "video/mp2t"},
  {"mp4", "video/mp4"},
  {"mpg", "video/mpeg"},
  {"mpeg", "video/mpeg"},
  {"mov", "video/quicktime"},
  {"webm", "video/webm"},
  {"flv", "video/x-flv"},
  {"m4v", "video/x-m4v"},
  {"mng", "video/x-mng"},
  {"asf", "video/x-ms-asf"},
  {"asx", "video/x-ms-asf"},
  {"wmv", "video/x-ms-wmv"},
  {"avi", "video/x-msvideo"},
};

// prototypes
inline std::string mime_type(fs::path const& path);
inline std::string url_encode(std::string const& str);
inline std::string url_decode(std::string const& str);
#ifdef OB_BELLE_CONFIG_ZLIB_ON
inline std::string zlib_encode(std::string const& val);
inline std::string zlib_decode(std::string const& val);
inline std::string gzip_encode(std::string const& val);
inline std::string gzip_decode(std::string const& val);
#endif // OB_BELLE_CONFIG_ZLIB_ON

// find the mime type of a string path
inline std::string mime_type(fs::path const& path)
{
  if (std::string ext = path.extension(); ext.size())
  {
    auto const str = Detail::lowercase(ext);

    if (mime_types.find(str) != mime_types.end())
    {
      return mime_types.at(str);
    }
  }

  return "application/octet-stream";
}

inline std::string url_encode(std::string const& str)
{
  std::string res;
  res.reserve(str.size());

  for (auto const& e : str)
  {
    if (e == ' ')
    {
      res += "+";
    }
    else if (std::isalnum(static_cast<unsigned char>(e)) ||
      e == '-' || e == '_' || e == '.' || e == '~')
    {
      res += e;
    }
    else
    {
      res += "%" + Detail::hex_encode(e);
    }
  }

  return res;
}

inline std::string url_decode(std::string const& str)
{
  std::string res;
  res.reserve(str.size());

  for (std::size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '+')
    {
      res += " ";
    }
    else if (str[i] == '%' && i + 2 < str.size() &&
      std::isxdigit(static_cast<unsigned char>(str[i + 1])) &&
      std::isxdigit(static_cast<unsigned char>(str[i + 2])))
    {
      res += Detail::hex_decode(str.substr(i + 1, 2));
      i += 2;
    }
    else
    {
      res += str[i];
    }
  }

  return res;
}

#ifdef OB_BELLE_CONFIG_ZLIB_ON
inline std::string zlib_encode(std::string const& str)
{
  boost::iostreams::array_source src {str.data(), str.size()};
  boost::iostreams::filtering_istream is;

  is.push(boost::iostreams::zlib_compressor(MAX_WBITS));
  is.push(src);

  std::stringstream body;
  boost::iostreams::copy(is, body);

  return body.str();
}

inline std::string zlib_decode(std::string const& str)
{
  boost::iostreams::array_source src {str.data(), str.size()};
  boost::iostreams::filtering_istream is;

  is.push(boost::iostreams::zlib_decompressor(MAX_WBITS));
  is.push(src);

  std::stringstream body;
  boost::iostreams::copy(is, body);

  return body.str();
}

inline std::string gzip_encode(std::string const& str)
{
  boost::iostreams::array_source src {str.data(), str.size()};
  boost::iostreams::filtering_istream is;

  is.push(boost::iostreams::gzip_compressor());
  is.push(src);

  std::stringstream body;
  boost::iostreams::copy(is, body);

  return body.str();
}

inline std::string gzip_decode(std::string const& str)
{
  boost::iostreams::array_source src {str.data(), str.size()};
  boost::iostreams::filtering_istream is;

  is.push(boost::iostreams::gzip_decompressor());
  is.push(src);

  std::stringstream body;
  boost::iostreams::copy(is, body);

  return body.str();
}
#endif // OB_BELLE_CONFIG_ZLIB_ON

} // namespace Util

class Request final : public http::request<http::string_body>
{
  using Base = http::request<http::string_body>;

public:

  using Path = std::vector<std::string>;
  using Params = std::unordered_multimap<std::string, std::string>;

  // inherit base constructors
  using http::request<http::string_body>::message;

  // default constructor
  Request() = default;

  // copy constructor
  Request(Request const&) = default;

  // move constructor
  Request(Request&&) = default;

  // copy assignment
  Request& operator=(Request const&) = default;

  // move assignment
  Request& operator=(Request&&) = default;

  // default deconstructor
  ~Request() = default;

  Request&& move() noexcept
  {
    return std::move(*this);
  }

  // get the path
  Path& path()
  {
    return _path;
  }

  // get the query parameters
  Params& params()
  {
    return _params;
  }

  // serialize path and query parameters to the target
  void params_serialize()
  {
    std::string path {target().to_string()};

    _path.clear();
    _path.emplace_back(path);

    if (! _params.empty())
    {
      path += "?";
      auto it = _params.begin();
      for (; it != _params.end(); ++it)
      {
        path += Util::url_encode(it->first) + "=" + Util::url_encode(it->second) + "&";
      }
      path.pop_back();
    }

    target(path);
  }

  // parse the query parameters from the target
  void params_parse()
  {
    std::string path {target().to_string()};

    // separate the query params
    auto params = Detail::split(path, "?", 1);

    // set params
    if (params.size() == 2)
    {
      auto kv = Detail::split(params.at(1), "&");

      for (auto const& e : kv)
      {
        if (e.empty())
        {
          continue;
        }

        auto k_v = Detail::split(e, "=", 1);

        if (k_v.size() == 1)
        {
          _params.emplace(Util::url_decode(e), "");
        }

        else if (k_v.size() == 2)
        {
          _params.emplace(Util::url_decode(k_v.at(0)), Util::url_decode(k_v.at(1)));
        }

        continue;
      }
    }
  }

private:

  Path _path {};
  Params _params {};
}; // Request

#ifdef OB_BELLE_CONFIG_CLIENT_ON
namespace Client
{

class Http final
{
public:

  struct Status
  {
    enum
    {
      closed = 0,
      closing,
      resolving,
      connecting,
      handshaking,
      error,
      open,
      reading,
      writing,
    };
  };

  struct Session_Ctx
  {
    // http request
    Request req {};

    // http response
    http::response<http::string_body> res {};
  }; // struct Session_Ctx

  struct Error_Ctx
  {
    // error code
    error_code const& ec;
  }; // struct Error_Ctx

  // callbacks
  using fn_on_open = std::function<void(Session_Ctx&)>;
  using fn_on_read = std::function<void(Session_Ctx&)>;
  using fn_on_write = std::function<void(Session_Ctx&)>;
  using fn_on_close = std::function<void(Session_Ctx&)>;
  using fn_on_error = std::function<void(Error_Ctx&)>;

  struct Attr
  {
#ifdef OB_BELLE_CONFIG_SSL_ON
    // use ssl
    bool ssl {false};

    // ssl context
    ssl::context ssl_context {ssl::context::tlsv12_client};
#endif // OB_BELLE_CONFIG_SSL_ON

    // socket status
    int status {Status::closed};

    // socket timeout
    std::chrono::seconds timeout {10};

    // address to connect to
    std::string address {"127.0.0.1"};

    // port to connect to
    unsigned short port {8080};

    // callbacks
    fn_on_open on_open {};
    fn_on_read on_read {};
    fn_on_write on_write {};
    fn_on_close on_close {};
    fn_on_error on_error {};
  }; // struct Attr

  struct Session_Type
  {
    virtual ~Session_Type() = default;
    virtual void read() = 0;
    virtual void write(Request&&) = 0;
    virtual void close() = 0;
    virtual void error(error_code const&) = 0;
  }; // struct Session_Type

  template<typename Derived>
  class Session_Base : public Session_Type
  {
    Derived& derived()
    {
      return static_cast<Derived&>(*this);
    }

  public:

    Session_Base(net::io_context& io_, std::shared_ptr<Attr> const attr_) :
      _resolver {io_},
      _strand {io_.get_executor()},
      _timer {io_, (std::chrono::steady_clock::time_point::max)()},
      _attr {attr_}
    {
    }

    ~Session_Base()
    {
      _attr->status = Status::closed;
    }

    void cancel_timer()
    {
      // set the timer to expire immediately
      _timer.expires_at((std::chrono::steady_clock::time_point::min)());
    }

    void do_timer()
    {
      _timer.cancel();
      _timer.expires_after(_attr->timeout);

      // wait on the timer
      _timer.async_wait(
        net::bind_executor(_strand,
          [self = derived().shared_from_this()](error_code ec)
          {
            self->on_timer(ec);
          }
        )
      );
    }

    void on_timer(error_code ec_)
    {
      if (ec_ && ec_ != net::error::operation_aborted)
      {
        on_error(ec_);

        return;
      }

      // check if socket has been closed
      if (_timer.expires_at() == (std::chrono::steady_clock::time_point::min)())
      {
        return;
      }

      // check expiry
      if (_timer.expiry() <= std::chrono::steady_clock::now())
      {
        derived().do_close();

        return;
      }

      if (_close)
      {
        return;
      }
    }

    void do_resolve()
    {
      _attr->status = Status::resolving;

      // domain name server lookup
      _resolver.async_resolve(_attr->address,
        Detail::to_string(_attr->port),
        net::bind_executor(_strand,
          [self = derived().shared_from_this()]
          (error_code ec, tcp::resolver::results_type results)
          {
            self->on_resolve(ec, results);
          }
        )
      );
    }

    void on_resolve(error_code ec_, tcp::resolver::results_type results_)
    {
      if (ec_)
      {
        on_error(ec_);

        return;
      }

      _attr->status = Status::connecting;

      // connect to the endpoint
      net::async_connect(derived().socket().lowest_layer(),
        results_.begin(), results_.end(),
        net::bind_executor(_strand,
          [self = derived().shared_from_this()](error_code ec, auto)
          {
            self->derived().on_connect(ec);
          }
        )
      );
    }

    void prepare_req()
    {
      // serialize target and params
      _ctx.req.params_serialize();

      // set default user-agent header value if not present
      if (_ctx.req.find(Header::user_agent) == _ctx.req.end())
      {
        _ctx.req.set(Header::user_agent, "Belle");
      }

      // set default host header value if not present
      if (_ctx.req.find(Header::host) == _ctx.req.end())
      {
        _ctx.req.set(Header::host, _attr->address);
      }

#ifdef OB_BELLE_CONFIG_ZLIB_ON
      // set default accept-encoding header value if not present
      if (_ctx.req.find(Header::accept_encoding) == _ctx.req.end())
      {
        _ctx.req.set(Header::accept_encoding, "gzip, deflate");
      }
#endif // OB_BELLE_CONFIG_ZLIB_ON

      // prepare the payload
      _ctx.req.prepare_payload();
    }

    void do_write()
    {
      prepare_req();
      do_timer();

      _attr->status = Status::writing;

      // Send the HTTP request
      http::async_write(derived().socket(), _ctx.req,
        net::bind_executor(_strand,
          [self = derived().shared_from_this()](error_code ec, std::size_t bytes)
          {
            self->on_write(ec, bytes);
          }
        )
      );
    }

    void on_write(error_code ec_, std::size_t bytes_)
    {
      boost::ignore_unused(bytes_);

      _attr->status = Status::open;

      if (ec_)
      {
        on_error(ec_);

        return;
      }

      if (_attr->on_write)
      {
        try
        {
          // run user function
          _attr->on_write(_ctx);
        }
        catch (...)
        {
          on_error();
        }
      }
    }

    void do_read()
    {
      // clear the HTTP response
      _ctx.res = {};
      do_timer();

      _attr->status = Status::reading;

      // Receive the HTTP response
      http::async_read(derived().socket(), _buf, _ctx.res,
        net::bind_executor(_strand,
          [self = derived().shared_from_this()](error_code ec, std::size_t bytes)
          {
            self->on_read(ec, bytes);
          }
        )
      );
    }

    void on_read(error_code ec_, std::size_t bytes_)
    {
      boost::ignore_unused(bytes_);

      _attr->status = Status::open;

      if (ec_)
      {
        on_error(ec_);

        return;
      }

      if (_attr->on_read)
      {
        try
        {
#ifdef OB_BELLE_CONFIG_ZLIB_ON
          if (_ctx.res.body().size() && _ctx.res["content-encoding"] != "" &&
            _ctx.res["content-encoding"] != "identity")
          {
            if (_ctx.res["content-encoding"] == "gzip")
            {
              _ctx.res.body() = OB::Belle::Util::gzip_decode(_ctx.res.body());
            }
            else if (_ctx.res["content-encoding"] == "deflate")
            {
              _ctx.res.body() = OB::Belle::Util::zlib_decode(_ctx.res.body());
            }
          }
#endif // OB_BELLE_CONFIG_ZLIB_ON

          // run user function
          _attr->on_read(_ctx);
        }
        catch (...)
        {
          on_error();
        }
      }
    }

    void on_error(error_code const& ec_ = {})
    {
      _attr->status = Status::error;

      if (_attr->on_error)
      {
        try
        {
          // run user function
          Error_Ctx err {ec_};
          _attr->on_error(err);
        }
        catch (...)
        {
        }
      }
    }

    void read()
    {
      do_read();
    }

    void write(Request&& req_)
    {
      _ctx.req = std::move(req_);
      do_write();
    }

    void close()
    {
      derived().do_close();
    }

    void error(error_code const& ec_)
    {
      on_error(ec_);
    }

    tcp::resolver _resolver;
    net::strand<net::io_context::executor_type> _strand;
    net::steady_timer _timer;
    std::shared_ptr<Attr> const _attr;
    Session_Ctx _ctx {};
    beast::flat_buffer _buf {};
    bool _close {false};
  }; // class Session_Base

  class Session final :
    public Session_Base<Session>,
    public std::enable_shared_from_this<Session>
  {
  public:
    Session(net::io_context& io_, std::shared_ptr<Attr> attr_) :
      Session_Base<Session>(io_, attr_),
      _socket {io_}
    {
    }

    ~Session()
    {
    }

    tcp::socket& socket()
    {
      return _socket;
    }

    tcp::socket&& socket_move()
    {
      return std::move(_socket);
    }

    void run()
    {
      do_timer();
      do_resolve();
    }

    void on_connect(error_code ec_)
    {
      if (ec_)
      {
        on_error(ec_);

        return;
      }

      _attr->status = Status::open;

      if (_attr->on_open)
      {
        try
        {
          // run user function
          _attr->on_open(_ctx);
        }
        catch (...)
        {
          on_error();
        }
      }
    }

    void do_close()
    {
      _attr->status = Status::closing;

      cancel_timer();

      error_code ec;

      // shutdown the socket
      _socket.shutdown(tcp::socket::shutdown_both, ec);
      _socket.close(ec);

      _attr->status = Status::closed;

      // ignore not_connected error
      if (ec && ec != boost::system::errc::not_connected)
      {
        on_error(ec);

        return;
      }

      if (_attr->on_close)
      {
        try
        {
          // run user function
          _attr->on_close(_ctx);
        }
        catch (...)
        {
          on_error();
        }
      }

      // the connection is now closed
    }

  private:

    tcp::socket _socket;
  }; // class Session

#ifdef OB_BELLE_CONFIG_SSL_ON
  class Session_Secure final :
    public Session_Base<Session_Secure>,
    public std::enable_shared_from_this<Session_Secure>
  {
  public:
    Session_Secure(net::io_context& io_, std::shared_ptr<Attr> attr_) :
      Session_Base<Session_Secure>(io_, attr_),
      _socket {tcp::socket(io_), attr_->ssl_context}
    {
      _close = true;
    }

    ~Session_Secure()
    {
    }

    Detail::ssl_stream<tcp::socket>& socket()
    {
      return _socket;
    }

    Detail::ssl_stream<tcp::socket>&& socket_move()
    {
      return std::move(_socket);
    }

    void run()
    {
      // start the timer
      do_timer();

      // set server name indication
      // use SSL_ctrl instead of SSL_set_tlsext_host_name macro
      // to avoid old style C cast to char*
      // if (! SSL_set_tlsext_host_name(_socket.native_handle(), _attr->address.data()))
      if (! SSL_ctrl(_socket.native_handle(), SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, _attr->address.data()))
      {
        error_code ec
        {
          static_cast<int>(ERR_get_error()),
          net::error::get_ssl_category()
        };

        on_error(ec);

        return;
      }

      do_resolve();
    }

    void on_connect(error_code ec_)
    {
      if (ec_)
      {
        on_error(ec_);

        return;
      }

      do_handshake();
    }

    void do_handshake()
    {
      _attr->status = Status::handshaking;

      // perform the ssl handshake
      _socket.async_handshake(ssl::stream_base::client,
        net::bind_executor(_strand,
          [self = shared_from_this()](error_code ec)
          {
            self->on_handshake(ec);
          }
        )
      );
    }

    void on_handshake(error_code ec_)
    {
      if (ec_)
      {
        on_error(ec_);

        return;
      }

      _close = false;

      _attr->status = Status::open;

      if (_attr->on_open)
      {
        try
        {
          // run user function
          _attr->on_open(_ctx);
        }
        catch (...)
        {
          on_error();
        }
      }
    }

    void do_close()
    {
      if (_close)
      {
        return;
      }

      _attr->status = Status::closing;

      _close = true;

      // shutdown the socket
      _socket.async_shutdown(
        net::bind_executor(_strand,
          [self = shared_from_this()](error_code ec)
          {
            self->on_shutdown(ec);
          }
        )
      );
    }

    void on_shutdown(error_code ec_)
    {
      cancel_timer();

      // ignore errors
      ec_.assign(0, ec_.category());

      // close the socket
      _socket.next_layer().close(ec_);
      _attr->status = Status::closed;

      if (_attr->on_close)
      {
        try
        {
          // run user function
          _attr->on_close(_ctx);
        }
        catch (...)
        {
          on_error();
        }
      }

      // the connection is now closed
    }

  private:

    Detail::ssl_stream<tcp::socket> _socket;
  }; // class Session_Secure
#endif // OB_BELLE_CONFIG_SSL_ON

  // default constructor
  Http(net::io_context& io_) :
    _io {io_}
  {
  }

  Http(net::io_context& io_, std::string const& url_) :
    _io {io_}
  {
    url(url_);
  }

  // constructor with address and port
  Http(net::io_context& io_, std::string address_, unsigned short port_) :
    _io {io_}
  {
    _attr->address = address_;
    _attr->port = port_;
  }

#ifdef OB_BELLE_CONFIG_SSL_ON
  // constructor with address, port, and ssl
  Http(net::io_context& io_, std::string address_, unsigned short port_, bool ssl_) :
    _io {io_}
  {
    _attr->address = address_;
    _attr->port = port_;
    _attr->ssl = ssl_;
  }
#endif // OB_BELLE_CONFIG_SSL_ON

  // destructor
  ~Http()
  {
  }

  Http& url(std::string const& url_)
  {
    std::smatch rx_match;
    auto const rx_opts {std::regex::ECMAScript | std::regex::icase};

    std::regex const rx_str {"^(http|https)://(.+?)(?::(\\d*)|)$", rx_opts};
    auto const rx_flgs {std::regex_constants::match_not_null};

    if (std::regex_match(url_, rx_match, rx_str, rx_flgs))
    {
      _attr->address = rx_match[2].str();
      _attr->port = static_cast<unsigned short>(rx_match[3].str().size() ? std::stoi(rx_match[3].str()) : (rx_match[1].str().size() == 5 ? 443 : 80));
#ifdef OB_BELLE_CONFIG_SSL_ON
      _attr->ssl = rx_match[1].str().size() == 5 ? true : false;
#endif // OB_BELLE_CONFIG_SSL_ON
    }

    return *this;
  }

  // set the address to connect to
  Http& address(std::string address_)
  {
    _attr->address = address_;

    return *this;
  }

  // get the address to connect to
  std::string address()
  {
    return _attr->address;
  }

  // set the port to connect to
  Http& port(unsigned short port_)
  {
    _attr->port = port_;

    return *this;
  }

  // get the port to connect to
  unsigned short port()
  {
    return _attr->port;
  }

  // set the socket timeout
  Http& timeout(std::chrono::seconds timeout_)
  {
    _attr->timeout = timeout_;

    return *this;
  }

  // get the socket timeout
  std::chrono::seconds timeout()
  {
    return _attr->timeout;
  }

  // get the io_context
  net::io_context& io()
  {
    return _io;
  }

#ifdef OB_BELLE_CONFIG_SSL_ON
  // set ssl
  Http& ssl(bool ssl_)
  {
    _attr->ssl = ssl_;

    return *this;
  }

  // get ssl
  bool ssl()
  {
    return _attr->ssl;
  }

  // get the ssl context
  ssl::context& ssl_context()
  {
    return _attr->ssl_context;
  }

  // set the ssl context
  Http& ssl_context(ssl::context&& ctx_)
  {
    _attr->ssl_context = std::move(ctx_);

    return *this;
  }
#endif // OB_BELLE_CONFIG_SSL_ON

  Http& on_open(fn_on_open on_open_)
  {
    _attr->on_open = on_open_;

    return *this;
  }

  Http& on_read(fn_on_read on_read_)
  {
    _attr->on_read = on_read_;

    return *this;
  }

  Http& on_write(fn_on_write on_write_)
  {
    _attr->on_write = on_write_;

    return *this;
  }

  Http& on_close(fn_on_close on_close_)
  {
    _attr->on_close = on_close_;

    return *this;
  }

  Http& on_error(fn_on_error on_error_)
  {
    _attr->on_error = on_error_;

    return *this;
  }

  Http& run()
  {
#ifdef OB_BELLE_CONFIG_SSL_ON
    if (_attr->ssl)
    {
      // use secure
      auto tmp = std::make_shared<Session_Secure>(_io, _attr);
      tmp->run();
      _session = tmp.get();
    }
    else
#endif // OB_BELLE_CONFIG_SSL_ON
    {
      // use plain
      auto tmp = std::make_shared<Session>(_io, _attr);
      tmp->run();
      _session = tmp.get();
    }

    return *this;
  }

  int status()
  {
    return _attr->status;
  }

  std::string const& status_string()
  {
    return _status_string.at(static_cast<std::size_t>(_attr->status));
  }

  bool read()
  {
    if (_session)
    {
      _session->read();
    }

    return _session;
  }

  bool write(Request req_)
  {
    if (_session)
    {
      _session->write(std::move(req_));
    }

    return _session;
  }

  bool close()
  {
    if (_session)
    {
      _session->close();
    }

    return _session;
  }

  bool error(error_code const& ec_ = {})
  {
    if (_session)
    {
      _session->error(ec_);
    }

    return _session;
  }

private:

  std::vector<std::string> const _status_string {
    "closed",
    "closing",
    "resolving",
    "connecting",
    "handshaking",
    "error",
    "open",
    "reading",
    "writing",
  };

  // the io context
  net::io_context& _io;

  // hold the client attributes
  std::shared_ptr<Attr> const _attr {std::make_shared<Attr>()};

  // the session
  Session_Type* _session {nullptr};
}; // class Http

} // namespace Client
#endif // OB_BELLE_CONFIG_CLIENT_ON

#ifdef OB_BELLE_CONFIG_SIGNAL_ON
class Signal final
{
public:

  using fn_on_signal = std::function<void(error_code const&, int)>;

  Signal(net::io_context& io_) noexcept :
    _io {io_}
  {
  }

  void wait()
  {
    _set.async_wait(_callback);
  }

  Signal& on_signal(int const sig_, fn_on_signal const fn_)
  {
    _set.add(sig_);
    _event[sig_] = fn_;

    return *this;
  }

  Signal& on_signal(std::vector<int> const sig_, fn_on_signal const fn_)
  {
    for (auto const& e : sig_)
    {
      _set.add(e);
      _event[e] = fn_;
    }

    return *this;
  }

  static std::string str(int sig_)
  {
    std::string res;

    if (_str.find(sig_) != _str.end())
    {
      res = _str.at(sig_);
    }

    return res;
  }

private:

  std::function<void(Belle::error_code, int)> const _callback = [&](auto ec_, auto sig_) {
    if (_event.find(sig_) != _event.end())
    {
      _event.at(sig_)(ec_, sig_);
    }
  };

  net::io_context& _io;

  Belle::net::signal_set _set {_io};

  std::unordered_map<int, fn_on_signal> _event;

  static inline std::unordered_map<int, std::string> const _str {
    {SIGHUP, "SIGHUP"},
    {SIGINT, "SIGINT"},
    {SIGQUIT, "SIGQUIT"},
    {SIGILL, "SIGILL"},
    {SIGTRAP, "SIGTRAP"},
    {SIGABRT, "SIGABRT"},
    {SIGBUS, "SIGBUS"},
    {SIGFPE, "SIGFPE"},
    {SIGKILL, "SIGKILL"},
    {SIGUSR1, "SIGUSR1"},
    {SIGSEGV, "SIGSEGV"},
    {SIGUSR2, "SIGUSR2"},
    {SIGPIPE, "SIGPIPE"},
    {SIGALRM, "SIGALRM"},
    {SIGTERM, "SIGTERM"},
    {SIGSTKFLT, "SIGSTKFLT"},
    {SIGCHLD, "SIGCHLD"},
    {SIGCONT, "SIGCONT"},
    {SIGSTOP, "SIGSTOP"},
    {SIGTSTP, "SIGTSTP"},
    {SIGTTIN, "SIGTTIN"},
    {SIGTTOU, "SIGTTOU"},
    {SIGURG, "SIGURG"},
    {SIGXCPU, "SIGXCPU"},
    {SIGXFSZ, "SIGXFSZ"},
    {SIGVTALRM, "SIGVTALRM"},
    {SIGPROF, "SIGPROF"},
    {SIGWINCH, "SIGWINCH"},
    {SIGPOLL, "SIGPOLL"},
    {SIGPWR, "SIGPWR"},
    {SIGSYS, "SIGSYS"},
  };
}; // class Signal
#endif // OB_BELLE_CONFIG_SIGNAL_ON

} // namespace OB::Belle

#endif // OB_BELLE_HH
