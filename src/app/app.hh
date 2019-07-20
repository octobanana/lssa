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

Licensed under the MIT License

Copyright (c) 2019 Brett Robinson <https://octobanana.com/>

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

#ifndef APP_HH
#define APP_HH

#include "ob/text.hh"
#include "ob/term.hh"
#include "ob/belle.hh"
#include "ob/string.hh"

#include <boost/asio.hpp>

#include <cstddef>

#include <set>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <utility>
#include <sstream>
#include <iostream>

namespace Belle = OB::Belle;
namespace String = OB::String;
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

class App final
{
public:

  App() {}
  App(App&&) = delete;
  App(App const&) = delete;
  ~App() = default;

  App& operator=(App&&) = delete;
  App& operator=(App const&) = delete;

  void do_timer();
  void on_timer(Belle::error_code const& ec_);
  void update_progress();

  void artists(std::vector<std::string> const& str_);
  void count(std::size_t const val_);
  void headers(std::vector<std::string> const& val_);
  void progress(bool const val_);

  void run();
  void print_artist();
  void print_results();

private:

  // HTTP connect port
  unsigned short const _port {443};

  // HTTP connect address
  std::string const _address {"www.last.fm"};

  // artist regex
  std::string const _rx {"href=\"/music/([^/.]+?)\""};

  // total number of redirects to follow per artist
  std::size_t const _redirect_total {3};

  // update / timer interval
  std::chrono::milliseconds const _interval {100};

  // progress output char values
  std::string const _progress_str {"-\\|/"};

  // HTTP response status code
  int _status;

  // HTTP error response message
  std::string _reason;

  // artist result
  struct Result
  {
    // the artist
    std::string artist;

    // the artist formatted for use in url
    std::string artist_url;

    // current page number
    std::size_t page_count {1};

    // number of redirects followed
    std::size_t redirect_count {0};

    // similar artist matches
    std::set<std::string> match;

    // insert ordered iterators to similar artist matches
    std::vector<decltype(match)::const_iterator> index;
  }; // struct Result
  using Results = std::vector<Result>;

  // the io context
  Belle::io _io {1};

  // capture and handle signals
  Belle::Signal _sig {_io};

  // the results
  Results _results;

  // current index position in the results
  std::size_t _result_index {0};

  // total number of matches to find per artist
  std::size_t _match_total {0};

  // buffer for HTTP response body
  std::string _page;

  // regex iterator
  OB::Text::Regex _it;

  // the HTTP request, reused for each subsequent request
  Belle::Request _req;

  // the HTTP client
  Belle::Client::Http _http {_io, _address, _port, true};

  // default HTTP request headers to use
  std::vector<std::pair<std::string, std::string>> _headers {
    {"host", _address},
    {"dnt", "1"},
    {"pragma", "no-cache"},
    {"cache-control", "no-cache"},
    {"upgrade-insecure-requests", "1"},
    {"accept-encoding", "gzip,deflate"},
    {"accept-language", "en-US,en;q=0.9"},
    {"accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
    {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.142 Safari/537.36"},
  };

  // when true, progress is output to stderr
  bool _progress {false};

  // current index position in the progress string
  std::size_t _progress_index {0};

  // timer for progress output
  Belle::net::high_resolution_timer _timer {_io, _interval};
}; // class App

#endif // APP_APP_HH