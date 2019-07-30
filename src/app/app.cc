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
  if (_progress)
  {
    _timer.expires_at(_timer.expires_at() + _interval);

    _timer.async_wait([&](auto ec)
    {
      on_timer(ec);
    });
  }
}

void App::on_timer(Belle::error_code const& ec_)
{
  if (ec_)
  {
    return;
  }

  if (_color)
  {
    std::cerr
    << aec::cr
    << aec::erase_line
    << aec::bold
    << aec::fg_green
    << _progress_str.at(_progress_index)
    << aec::fg_white
    << "["
    << aec::fg_green
    << _http.status_string()
    << aec::fg_white
    << "]"
    << aec::clear;

    if (_results.at(_result_index).match.size())
    {
      std::cerr
      << aec::fg_green
      << _results.at(_result_index).match.size()
      << aec::fg_white
      << "/"
      << aec::fg_green
      << _match_total
      << aec::clear;
    }
  }
  else
  {
    std::cerr
    << aec::cr
    << aec::erase_line
    << _progress_str.at(_progress_index)
    << "["
    << _http.status_string()
    << "]";

    if (_results.at(_result_index).match.size())
    {
      std::cerr
      << _results.at(_result_index).match.size()
      << "/"
      << _match_total;
    }
  }

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
    if (_color)
    {
      std::cerr
      << aec::cr
      << aec::erase_line
      << aec::bold
      << aec::fg_green
      << _progress_str.at(_progress_index)
      << aec::fg_white
      << "["
      << aec::fg_green
      << _http.status_string()
      << aec::fg_white
      << "]"
      << aec::fg_green
      << _results.at(_result_index).match.size()
      << aec::fg_white
      << "/"
      << aec::fg_green
      << _match_total
      << aec::clear;
    }
    else
    {
      std::cerr
      << aec::cr
      << aec::erase_line
      << _progress_str.at(_progress_index)
      << "["
      << _http.status_string()
      << "]"
      << _results.at(_result_index).match.size()
      << "/"
      << _match_total;
    }
  }

  update_wait();
}

void App::update_wait()
{
  if (_result_index + 1 == _results.size() && _results.at(_result_index).match.size() >= _match_total)
  {
    return;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(_interval));
}

void App::artists(std::vector<std::string> const& val_, bool const ignore_case_)
{
  for (auto const& e : val_)
  {
    auto& res = _results.emplace_back(Result());
    res.artist = ignore_case_ ? String::titlecase(e) : e;
    res.artist_url = Belle::Util::url_encode(res.artist);
    res.artist_lowercase = String::lowercase(e);
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

void App::color(bool const val_)
{
  _color = val_;
}

void App::run()
{
  _sig.on_signal({SIGINT, SIGTERM}, [&](auto const& /*ec*/, auto sig) {
    _io.stop();
    _reconnect = false;
    _http_reason = Belle::Signal::str(sig);
  });
  _sig.wait();

  // init request
  {
    for (auto const& [k, v] : _headers)
    {
      _req.set(k, v);
    }

    _req.keep_alive(true);
    _req.method(Belle::Method::get);
    _req.params().emplace("page", std::to_string(_results.at(_result_index).page_count));
    _req.target("/music/" + _results.at(_result_index).artist_url + "/+similar");
  }

  _http.on_error([&](auto& ctx)
  {
    _http_reason = ctx.ec.message();
    _http.close();
    _timer.cancel();
  });

  _http.on_open([&](auto& /*ctx*/)
  {
    _http.write(_req);
  });

  _http.on_read([&](auto& ctx)
  {
    _reconnect = false;
    _http_status = static_cast<int>(ctx.res.result_int());

    if (_http_status != 200)
    {
      if (_http_status != 301 && _http_status != 302 && ctx.res["location"].empty())
      {
        _http_reason = std::string(ctx.res.reason());
        _http.close();

        return;
      }

      if (_results.at(_result_index).redirect_count >= _redirect_total)
      {
        _http_reason = "redirect limit reached (" + std::to_string(_redirect_total) + ")";
        _http.close();

        return;
      }

      auto const redirect = String::match(std::string(ctx.res["location"]), std::regex("^https://www.last.fm/music/(.+?)/\\+similar.*$", std::regex::icase));

      if (! redirect)
      {
        _http_reason = "invalid redirect URL (" + std::string(ctx.res["location"]) + ")";
        _http.close();
      }

      ++_results.at(_result_index).redirect_count;

      _results.at(_result_index).artist_url = redirect->at(1);
      _results.at(_result_index).artist = String::replace(Belle::Util::url_decode(_results.at(_result_index).artist_url), {{"&amp;", "&"}, {"%2B", "+"}});
      _results.at(_result_index).artist_lowercase = String::lowercase(_results.at(_result_index).artist);

      _req.params().clear();
      _req.params().emplace("page", std::to_string(_results.at(_result_index).page_count));
      _req.target("/music/" + _results.at(_result_index).artist_url + "/+similar");

      _reconnect = true;

      _http.write(_req);

      return;
    }

    // handle results
    {
      _page = std::move(ctx.res.body());

      if (_page.empty())
      {
        _http_reason = "received empty response";
        _http.close();

        return;
      }

      _it.match(_rx, _page);

      if (_it.empty())
      {
        _http_reason = "no matches found";
        _http.close();

        return;
      }

      _timer.cancel();

      auto& result = _results.at(_result_index);

      if (result.page_count == 1)
      {
        print_artist();
      }

      std::string artist;
      std::unordered_map<std::string, std::string> cache;

      for (auto const& match : _it)
      {
        artist = match.group.at(0);

        if (auto const cached = cache.find(artist);
          cached != cache.end())
        {
          artist = cached->second;
        }
        else
        {
          artist = String::replace(Belle::Util::url_decode(artist), {{"&amp;", "&"}, {"%2B", "+"}});
        }

        if (artist.at(0) != ' ' &&
          String::lowercase(artist) == result.artist_lowercase)
        {
          continue;
        }

        if (auto const it = result.match.insert(artist); it.second)
        {
          result.index.emplace_back(it.first);

          update_progress();

          if (result.match.size() >= _match_total)
          {
            break;
          }
        }
      }
    }

    ++_results.at(_result_index).page_count;

    if (_results.at(_result_index).match.size() >= _match_total ||
      _results.at(_result_index).page_count > _page_total)
    {
      print_results();

      ++_result_index;

      if (_result_index >= _results.size())
      {
        _http_reason.clear();
        _http.close();

        return;
      }

      do_timer();
    }

    _req.params().clear();
    _req.params().emplace("page", std::to_string(_results.at(_result_index).page_count));
    _req.target("/music/" + _results.at(_result_index).artist_url + "/+similar");

    _http.write(_req);
  });

  _http.on_write([&](auto& /*ctx*/)
  {
    _http.read();
  });

  _http.on_close([&](auto& /*ctx*/)
  {
    _timer.cancel();
    _io.stop();
  });

  do
  {
    do_timer();
    _http.run();
    _io.run();
    _io.restart();
  }
  while (_reconnect);

  if (_http_reason.size())
  {
    throw std::runtime_error(_http_reason.c_str());
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

  if (_color)
  {
    std::cout
    << aec::bold
    << aec::fg_magenta
    << result.artist
    << aec::clear
    << aec::nl
    << std::flush;
  }
  else
  {
    std::cout
    << result.artist
    << aec::nl
    << std::flush;
  }
}

void App::print_results()
{
  if (_progress)
  {
    std::cerr
    << aec::cr
    << aec::erase_line;
  }

  auto const& result = _results.at(_result_index);

  if (result.match.size())
  {
    for (auto const& artist : result.index)
    {
      if (_color)
      {
        std::cout
        << aec::bold
        << aec::fg_white
        << *artist
        << aec::clear
        << aec::nl;
      }
      else
      {
        std::cout
        << *artist
        << aec::nl;
      }
    }

    if (_result_index + 1 < _results.size())
    {
      std::cout
      << aec::nl;
    }

    std::cout
    << std::flush;
  }
}
