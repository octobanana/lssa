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

#include "app/app.hh"

void App::do_timer()
{
  _timer.expires_at(_timer.expires_at() + _interval);

  _timer.async_wait([&](auto ec)
  {
    on_timer(ec);
  });
}

void App::on_timer(Belle::error_code const& ec_)
{
  if (ec_ || _http.status() == 1)
  {
    return;
  }

  std::cerr
  << aec::cr
  << _progress_str.at(_progress_index);

  ++_progress_index;

  if (_progress_index >= _progress_str.size())
  {
    _progress_index = 0;
  }

  do_timer();
}

void App::update_progress()
{
  if (_progress)
  {
    std::cerr
    << aec::cr
    << (_results.at(_result_index).match.size() < _match_total ? _results.at(_result_index).match.size() : _match_total)
    << "/" << _match_total;

    std::this_thread::sleep_for(std::chrono::milliseconds(_interval));

    if (_results.at(_result_index).match.size() >= _match_total)
    {
      std::cerr
      << aec::cr
      << aec::erase_line;
    }
  }
  else
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(_interval));
  }
}

void App::artists(std::vector<std::string> const& val_)
{
  for (auto const& e : val_)
  {
    auto& val = _results.emplace_back(Result());
    val.artist = String::titlecase(e);
    val.artist_url = Belle::Util::url_encode(val.artist);
  }
}

void App::count(std::size_t const val_)
{
  _match_total = val_;
}

void App::headers(std::vector<std::string> const& val_)
{
  for (auto const& kv : val_)
  {
    auto const k_v = String::split_view(kv, ":", 1);

    if (k_v.size() == 2)
    {
      _headers.emplace_back(String::trim(std::string(k_v.at(0))), String::trim(std::string(k_v.at(1))));
    }
  }
}

void App::progress(bool const val_)
{
  _progress = val_;
}

void App::run()
{
  _sig.on_signal({SIGINT, SIGTERM}, [&](auto const& /*ec*/, auto /*sig*/) {
    // std::cerr << "\nEvent: " << Belle::Signal::str(sig) << "\n";
    _io.stop();
  });
  _sig.wait();

  _http.on_error([&](auto& ctx)
  {
    _reason = ctx.ec.message();
  });

  _http.on_open([&](auto& /*ctx*/)
  {
    for (auto const& [k, v] : _headers)
    {
      _req.set(k, v);
    }

    _req.keep_alive(true);
    _req.method(Belle::Method::get);
    _req.params().emplace("page", std::to_string(_results.at(_result_index).page_count));
    _req.target("/music/" + _results.at(_result_index).artist_url + "/+similar");

    _http.write(_req);
  });

  _http.on_read([&](auto& ctx)
  {
    // std::cerr << "Response Head:\n" << ctx.res.base() << "\n";
    // std::cerr << "Response Body:\n" << ctx.res.body() << "\n";

    _status = static_cast<int>(ctx.res.result_int());

    if (_status != 200)
    {
      if (_status != 301 && _status != 302 &&
        ctx.res["location"].empty())
      {
        _reason = std::string(ctx.res.reason());
        _http.close();

        return;
      }

      if (_results.at(_result_index).redirect_count >= _redirect_total)
      {
        _reason = "redirect limit reached (" + std::to_string(_redirect_total) + ")";
        _http.close();

        return;
      }

      ++_results.at(_result_index).redirect_count;
      auto const redirect = std::string(ctx.res["location"]);

      auto const begin = redirect.find("/music/") + 7;
      if (begin == std::string::npos)
      {
        _reason = "invalid redirect";
        _http.close();

        return;
      }

      auto const end = redirect.find("/+similar", begin);
      if (end == std::string::npos)
      {
        _reason = "invalid redirect";
        _http.close();

        return;
      }

      _results.at(_result_index).artist_url = redirect.substr(begin, end - begin);
      _results.at(_result_index).artist = OB::String::replace(Belle::Util::url_decode(_results.at(_result_index).artist_url), "&amp;", "&");

      _req.params().clear();
      _req.params().emplace("page", std::to_string(_results.at(_result_index).page_count));
      _req.target("/music/" + _results.at(_result_index).artist_url + "/+similar");

      _http.write(_req);

      return;
    }

    // handle results
    {
      _page = std::move(ctx.res.body());

      if (_page.empty())
      {
        _reason = "received empty response";
        _http.close();

        return;
      }

      _it.match(_rx, _page);

      if (_it.empty())
      {
        _reason = "no matches found";
        _http.close();

        return;
      }

      if (_results.at(_result_index).page_count == 1)
      {
        print_artist();
      }

      for (auto const& e : _it)
      {
        std::string val {OB::String::replace(Belle::Util::url_decode(std::string(e.group.at(0))), "&amp;", "&")};

        if (val.front() == ' ' || val.find(_results.at(_result_index).artist) != std::string::npos)
        {
          continue;
        }

        if (auto const it = _results.at(_result_index).match.insert(val); it.second)
        {
          _results.at(_result_index).index.emplace_back(it.first);

          update_progress();

          if (_results.at(_result_index).match.size() >= _match_total)
          {
            break;
          }
        }
      }
    }

    ++_results.at(_result_index).page_count;

    if (_results.at(_result_index).match.size() >= _match_total)
    {
      print_results();

      ++_result_index;

      if (_result_index >= _results.size())
      {
        _reason.clear();
        _http.close();

        return;
      }
    }

    _req.params().clear();
    _req.params().emplace("page", std::to_string(_results.at(_result_index).page_count));
    _req.target("/music/" + _results.at(_result_index).artist_url + "/+similar");

    _http.write(_req);
  });

  _http.on_write([&](auto& /*ctx*/)
  {
    // std::cerr << "Request Head:\n" << ctx.req.base() << "\n";
    _http.read();
  });

  _http.on_close([&](auto& /*ctx*/)
  {
    _io.stop();
  });

  if (_progress)
  {
    do_timer();
  }

  _http.run();
  _io.run();

  if (_reason.size())
  {
    throw std::runtime_error(_reason.c_str());
  }
}

void App::print_artist()
{
  if (_progress)
  {
    std::cerr
    << aec::cr
    << aec::erase_line;
  }

  auto const& result = _results.at(_result_index);

  std::cout
  << result.artist << "\n"
  << std::flush;
}

void App::print_results()
{
  auto const& result = _results.at(_result_index);

  if (result.match.size())
  {
    for (auto const& artist : result.index)
    {
      std::cout
      << *artist << "\n";
    }

    if (_result_index + 1 < _results.size())
    {
      std::cout << "\n";
    }

    std::cout << std::flush;
  }
}
