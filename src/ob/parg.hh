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

Parg
0.1.0-develop

A header only c++ library for parsing command line arguments and generating usage/help output.
https://octobanana.com/software/parg

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

#ifndef OB_PARG_HH
#define OB_PARG_HH

#include "ob/algorithm.hh"
#include "ob/string.hh"
#include "ob/term.hh"

#include <unistd.h>

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace OB
{

namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

class Parg final
{
public:

  struct Style
  {
    std::string h1 {aec::fg_magenta + aec::underline};
    std::string h2 {aec::fg_green};
    std::string p {aec::fg_white_bright};
    std::string pa {aec::fg_white};
    std::string opt {aec::fg_green};
    std::string arg {aec::fg_yellow};
    std::string val {aec::fg_cyan};
    std::string success {aec::fg_green};
    std::string error {aec::fg_red};
  } style;

  struct Section
  {
    std::string head;
    std::vector<std::pair<std::string, std::string>> body;
  };

  struct Section_Command
  {
    std::string head;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> body;
  };

  using Output = std::function<void(OB::Term::ostream&, Style const&)>;

  Parg()
  {
  }

  Parg(int const argc_, char** argv_)
  {
    argvf(argc_, argv_);
  }

  std::string path() const
  {
    return _path;
  }

  bool color() const
  {
    return _color;
  }

  Parg& color(bool const val_)
  {
    _color = val_;

    return *this;
  }

  Parg& name(std::string const& name_)
  {
    _name = name_;

    return *this;
  }

  std::string name() const
  {
    return _name;
  }

  Parg& version(std::string const& version_)
  {
    _version = version_;

    return *this;
  }

  std::string version() const
  {
    if (_version.empty())
    {
      return {};
    }

    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os
    << aec::wrap(_name, style.h2)
    << " "
    << aec::wrap(_version, style.p)
#ifdef DEBUG
    << " "
    << aec::wrap("DEBUG", style.error)
#endif
    << "\n";

    return out.str();
  }

  Parg& usage(std::string const& usage_)
  {
    _usage.emplace_back(_name + " " + usage_);

    return *this;
  }

  std::string usage() const
  {
    if (_usage.empty())
    {
      return {};
    }

    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os << aec::wrap("Usage", style.h1) << iom::push();

    bool alt {false};

    for (auto const& e : _usage)
    {
      os << aec::wrap(e, (alt ? style.pa : style.p)) << "\n";

      alt = ! alt;
    }

    os << iom::pop();

    return out.str();
  }

  Parg& description(std::string const& description_)
  {
    _description = description_;

    return *this;
  }

  std::string description() const
  {
    return _description;
  }

  Parg& info(Output const& out)
  {
    _info.emplace_back(out);

    return *this;
  }

  Parg& info(Section const& sect_)
  {
    info([sect_](auto& os, auto const& st)
    {
      os << aec::wrap(sect_.head, st.h1) << iom::push();

      bool alt {false};

      for (auto const& [k, v] : sect_.body)
      {
        if (k.empty())
        {
          os << aec::wrap(v, (alt ? st.pa : st.p)) << "\n";
        }
        else
        {
          OB::Algorithm::for_each(OB::String::split_view(k, ", "),
          [&os, &st](auto const& obj)
          {
            os << st.h2 << obj << st.p << ", ";
          },
          [&os, &st](auto const& obj)
          {
            os << st.h2 << obj;
          });

          os
          << aec::clear << iom::push()
          << aec::wrap(v, (alt ? st.pa : st.p)) << iom::pop();
        }

        alt = ! alt;
      }

      os << "\n" << iom::pop();
    });

    return *this;
  }

  Parg& info(Section_Command const& sect_)
  {
    info([sect_](auto& os, auto const& st)
    {
      os << aec::wrap(sect_.head, st.h1) << iom::push();

      bool alt {false};

      for (auto const& [k, v] : sect_.body)
      {
        auto const cmd = OB::String::split_view(k, " ", 2);

        os
        << iom::word_break(false)
        << aec::wrap(cmd.at(0), st.h2);

        if (cmd.size() == 2)
        {
          os << " " << aec::wrap(cmd.at(1), (v.at(0).first.empty() ? st.arg : st.val));
        }
        else if (cmd.size() >= 3)
        {
          os
          << " " << aec::wrap(cmd.at(1), st.val)
          << " " << aec::wrap(cmd.at(2), st.arg);
        }

        os
        << iom::word_break(true)
        << iom::push();

        for (auto const& [k1, v1] : v)
        {
          if (k1.empty())
          {
            os << aec::wrap(v1, (alt ? st.pa : st.p)) << iom::pop();
          }
          else
          {
            os
            << aec::wrap(k1, st.val) << iom::push()
            << aec::wrap(v1, (alt ? st.pa : st.p)) << iom::pop();
          }

          alt = ! alt;
        }

        if (! v.at(0).first.empty())
        {
          os << iom::pop();
        }
      }

      os << "\n" << iom::pop();
    });

    return *this;
  }

  Parg& author(std::string const& author_)
  {
    _author = author_;

    return *this;
  }

  std::string author() const
  {
    return _author;
  }

  std::string license() const
  {
    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os
    << aec::wrap("MIT License", style.h1)
    << "\n\n"
    << aec::wrap("Copyright (c) 2019 Brett Robinson", style.h2)
    << "\n\n"
    << iom::first_wrap(true)
    << style.p
    << R"(Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.)"
    << "\n";

    return out.str();
  }

  void ostream_init(OB::Term::ostream& os_) const
  {
    if (OB::Term::is_term(STDOUT_FILENO))
    {
      std::size_t width {0};
      OB::Term::width(width, STDOUT_FILENO);
      os_.width(width);
      os_.line_wrap(true);
    }
    else
    {
      os_.line_wrap(false);
    }

    // debug
    // for printing './help.txt' output
    // os_.width(80);
    // os_.line_wrap(true);

    os_.escape_codes(_color);
    os_.indent(2);
    os_.first_wrap(false);
    os_.white_space(false);
  }

  std::string help() const
  {
    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    if (! _description.empty())
    {
      os
      << aec::wrap(_name, style.h1) << iom::push()
      << aec::wrap(_description, style.p) << iom::pop();
    }

    if (! _usage.empty())
    {
      os
      << "\n"
      << aec::wrap("Usage", style.h1) << iom::push();

      bool alt {false};

      for (auto const& e : _usage)
      {
        os << aec::wrap(e, (alt ? style.pa : style.p)) << "\n";

        alt = ! alt;
      }

      os << "\n" << iom::pop();
    }

    // options
    if (! _data.empty())
    {
      os << aec::wrap("Options", style.h1) << iom::push();

      bool alt {false};

      for (auto const& v : _data)
      {
        auto const& e = v.second;

        if (! e.short_.empty())
        {
          os << aec::wrap("-" + e.short_, style.opt);
        }

        if (! e.short_.empty() && ! e.long_.empty())
        {
          os << aec::wrap(", ", style.p);
        }

        if (! e.long_.empty())
        {
          os << aec::wrap("--" + e.long_, style.opt);
        }

        if (! e.mode_)
        {
          os
          << aec::wrap("=", style.p)
          << aec::wrap("<" + e.arg_ + ">", style.arg);
        }

        os
        << iom::push()
        << aec::wrap(e.info_, (alt ? style.pa : style.p))
        << iom::pop();

        alt = ! alt;
      }

      os << "\n" << iom::pop();
    }

    if (! _info.empty())
    {
      for (auto const& e : _info)
      {
        e(os, style);
      }
    }

    if (! _author.empty())
    {
      os
      << aec::wrap("Author", style.h1) << iom::push()
      << aec::wrap(_author, style.p) << iom::pop();
    }

    return out.str();
  }

  int parse()
  {
    if (_is_stdin)
    {
      pipe_stdin();
    }

    _status = parse_args();

    return _status;
  }

  int parse(int const argc_, char** argv_)
  {
    if (_is_stdin)
    {
      pipe_stdin();
    }

    argvf(argc_, argv_);
    _status = parse_args();

    return _status;
  }

  int parse(std::string const& str_)
  {
    _args = str_to_args(str_);
    _status = parse_args();

    return _status;
  }

  std::vector<std::string> str_to_args(std::string const& str_)
  {
    std::vector<std::string> args;
    std::string const backslash {"\\"};

    // parse str into arg vector as if it was parsed by the shell
    for (std::size_t i = 0; i < str_.size(); ++i)
    {
      std::string e {str_.at(i)};

      // default
      if (e.find_first_not_of(" \n\t\"'") != std::string::npos)
      {
        bool escaped {false};
        args.emplace_back("");

        for (;i < str_.size(); ++i)
        {
          e = str_.at(i);

          if (! escaped && e.find_first_of(" \n\t") != std::string::npos)
          {
            // put back unmatched char
            --i;

            break;
          }
          else if (e == backslash)
          {
            escaped = true;
          }
          else if (escaped)
          {
            args.back() += e;
            escaped = false;
          }
          else
          {
            args.back() += e;
          }
        }

        continue;
      }

      // whitespace
      else if (e.find_first_of(" \n\t") != std::string::npos)
      {
        for (;i < str_.size(); ++i)
        {
          e = str_.at(i);

          if (e.find_first_not_of(" \n\t") != std::string::npos)
          {
            // put back unmatched char
            --i;

            break;
          }
        }

        continue;
      }

      // string
      else if (e.find_first_of("\"'") != std::string::npos)
      {
        std::string quote {e};
        bool escaped {false};
        args.emplace_back("");

        // skip start quote
        ++i;

        for (;i < str_.size(); ++i)
        {
          e = str_.at(i);

          if (! escaped && e == quote)
          {
            // skip end quote

            break;
          }
          else if (e == backslash)
          {
            escaped = true;
          }
          else if (escaped)
          {
            args.back() += e;
            escaped = false;
          }
          else
          {
            args.back() += e;
          }
        }
      }
    }

    return args;
  }

  void set(std::string const& name_, std::string const& info_)
  {
    // set a flag
    set_impl(true, name_, {"0"}, {}, info_, false);
  }

  void set(std::string const& name_, std::string const& default_,
    std::string const& arg_, std::string const& info_, bool multi_ = false)
  {
    // set an option
    set_impl(false, name_, std::vector<std::string>{default_}, arg_, info_, multi_);
  }

  template<typename T,
    std::enable_if_t<
      std::is_same_v<T, std::string> ||
      std::is_same_v<T, std::filesystem::path>,
      int> = 0>
  T get(std::string const& key_)
  {
    if (_data.find(key_) == _data.end())
    {
      throw std::logic_error("parg get '" + key_ + "' is not defined");
    }

    if (_data[key_].value_.empty())
    {
      return _data[key_].default_.at(0);
    }

    return _data[key_].value_.at(0);
  }

  template<typename T,
    std::enable_if_t<
      std::is_integral_v<T>,
      int> = 0>
  T get(std::string const& key_)
  {
    if (_data.find(key_) == _data.end())
    {
      throw std::logic_error("parg get '" + key_ + "' is not defined");
    }

    if (_data[key_].value_.empty())
    {
      std::stringstream ss;
      ss << _data[key_].default_.at(0);
      T val;
      ss >> val;

      return {val};
    }

    std::stringstream ss;
    ss << _data[key_].value_.at(0);
    T val;
    ss >> val;

    return val;
  }

  template<typename T,
    std::enable_if_t<
      std::is_same_v<T, std::string> ||
      std::is_same_v<T, std::filesystem::path>,
      int> = 0>
  std::vector<T> get_all(std::string const& key_)
  {
    if (_data.find(key_) == _data.end())
    {
      throw std::logic_error("parg get '" + key_ + "' is not defined");
    }


    std::vector<T> res;

    if (_data[key_].value_.empty())
    {
      for (auto const& e : _data[key_].default_)
      {
        res.emplace_back(e);
      }
    }
    else
    {
      for (auto const& e : _data[key_].value_)
      {
        res.emplace_back(e);
      }
    }

    return res;
  }

  template<typename T,
    std::enable_if_t<
      std::is_integral_v<T>,
      int> = 0>
  std::vector<T> get_all(std::string const& key_)
  {
    if (_data.find(key_) == _data.end())
    {
      throw std::logic_error("parg get '" + key_ + "' is not defined");
    }

    std::vector<T> res;

    if (_data[key_].value_.empty())
    {
      for (auto const& e : _data[key_].default_)
      {
        std::stringstream ss;
        ss << e;
        T val;
        ss >> val;
        res.emplace_back(val);
      }
    }
    else
    {
      for (auto const& e : _data[key_].value_)
      {
        std::stringstream ss;
        ss << e;
        T val;
        ss >> val;
        res.emplace_back(val);
      }
    }

    return res;
  }

  bool find(std::string const& key_) const
  {
    // key must exist
    if (_data.find(key_) == _data.end())
    {
      return false;
    }

    return _data.at(key_).seen_;
  }

  Parg& set_pos(bool const positional_ = true)
  {
    _is_positional = positional_;

    return *this;
  }

  std::string get_pos() const
  {
    std::string str;

    if (_positional_vec.empty())
    {
      return str;
    }

    for (auto const& e : _positional_vec)
    {
      str += e + " ";
    }

    str.pop_back();

    return str;
  }

  std::vector<std::string> const& get_pos_vec() const
  {
    return _positional_vec;
  }

  Parg& set_stdin(bool const stdin_ = true)
  {
    _is_stdin = stdin_;

    return *this;
  }

  std::string const& get_stdin() const
  {
    return _stdin;
  }

  int status() const
  {
    return _status;
  }

  Parg& error(std::string const& val_)
  {
    _error = val_;

    return *this;
  }

  std::string error() const
  {
    if (_error.empty())
    {
      return {};
    }

    std::ostringstream out;
    OB::Term::ostream os {out};
    ostream_init(os);

    os
    << iom::first_wrap(false)
    << aec::wrap("Error: ", style.error)
    << _error << "\n"
    << iom::first_wrap(true);

    auto const similar_names = similar();

    if (similar_names.size() > 0)
    {
      os
      << "\nDid you mean:"
      << style.opt << iom::push();

      OB::Algorithm::for_each(similar_names,
      [&os](auto const& e)
      {
        os << "--" << e << "\n";
      },
      [&os](auto const& e)
      {
        os << "--" << e;
      });

      os << aec::clear << iom::pop();
    }

    return out.str();
  }

  std::vector<std::string> const& similar() const
  {
    return _similar;
  }

  std::size_t flags_found() const
  {
    std::size_t count {0};

    for (auto const& e : _data)
    {
      if (e.second.mode_ && e.second.seen_)
      {
        ++count;
      }
    }

    return count;
  }

  std::size_t options_found() const
  {
    std::size_t count {0};

    for (auto const& e : _data)
    {
      if (! e.second.mode_ && e.second.seen_)
      {
        ++count;
      }
    }

    return count;
  }

  struct Option
  {
    bool mode_ {false};
    bool multi_ {false};
    std::string arg_;
    std::string info_;
    std::string short_;
    std::string long_;
    std::vector<std::string> default_;

    bool seen_ {false};
    std::vector<std::string> value_;
  };

private:

  std::vector<std::string> _args;
  std::string _path;
  bool _color {true};
  std::string _name;
  std::string _version;
  std::vector<std::string> _usage;
  std::string _description;
  std::vector<Output> _info;
  std::string _author;
  std::map<std::string, Option> _data;
  std::map<std::string, std::string> _flags;
  bool _is_positional {false};
  std::string _positional;
  std::vector<std::string> _positional_vec;
  std::string _stdin;
  bool _is_stdin {false};
  int _status {0};
  std::string _error;
  std::vector<std::string> _similar;

  void set_impl(bool const mode_, std::string const& name_, std::vector<std::string> const& default_,
    std::string const& arg_, std::string const& info_, bool multi_ = false)
  {
    std::string name_long;
    std::string delim {","};

    if (name_.find(delim) != std::string::npos)
    {
      std::string name_short;

      auto const names = OB::String::split_view(name_, delim, 1);
      assert(names.size() >= 1 && names.size() <= 2);
      name_long = names.at(0);

      if (names.size() == 2)
      {
        // short name must be one char
        assert(names.at(1).size() == 1);
        name_short = names.at(1);
      }

      _flags[name_short] = name_long;
      _data[name_long].short_ = name_short;
    }
    else
    {
      name_long = name_;

      if (name_long.size() == 1)
      {
        _flags[name_long] = name_long;
        _data[name_long].short_ = name_long;
      }
    }

    _data[name_long].long_ = name_long;
    _data[name_long].mode_ = mode_;
    _data[name_long].multi_ = multi_;
    _data[name_long].default_ = default_;
    _data[name_long].arg_ = arg_;
    _data[name_long].info_ = info_;
  }

  void argvf(int const argc_, char** argv_)
  {
    if (argc_ < 1)
    {
      return;
    }

    _path = argv_[0];

    for (int i = 1; i < argc_; ++i)
    {
      _args.emplace_back(argv_[i]);
    }
  }

  int pipe_stdin()
  {
    if (isatty(STDIN_FILENO))
    {
      _stdin.clear();

      return -1;
    }

    _stdin.assign((std::istreambuf_iterator<char>(std::cin)),
      (std::istreambuf_iterator<char>()));

    return 0;
  }

  void find_similar(std::string const& name_)
  {
    std::size_t const weight_max {8};
    std::vector<std::pair<std::size_t, std::string>> dist;

    for (auto const& [key, val] : _data)
    {
      std::size_t weight {0};

      if (! OB::String::starts_with(val.long_, name_))
      {
        weight = OB::String::damerau_levenshtein(name_, val.long_, 1, 2, 3, 0);
      }

      if (weight < weight_max)
      {
        dist.emplace_back(weight, val.long_);
      }
    }

    std::sort(dist.begin(), dist.end(),
    [](auto const& lhs, auto const& rhs)
    {
      return (lhs.first == rhs.first) ?
        (lhs.second.size() < rhs.second.size()) :
        (lhs.first < rhs.first);
    });

    for (auto const& [key, val] : dist)
    {
      _similar.emplace_back(val);
    }

    std::size_t const similar_max {3};
    if (_similar.size() > similar_max)
    {
      _similar.erase(_similar.begin() + similar_max, _similar.end());
    }
  }

  int parse_args()
  {
    if (_args.size() < 1)
    {
      return 1;
    }

    bool dashdash {false};

    for (std::size_t i = 0; i < _args.size(); ++i)
    {
      std::string const& tmp {_args.at(static_cast<std::size_t>(i))};
      // std::cerr << "ARG: " << i << " -> " << tmp << std::endl;

      if (dashdash)
      {
        _positional_vec.emplace_back(tmp);

        continue;
      }

      if (tmp.size() > 1 && tmp.at(0) == '-' && tmp.at(1) != '-')
      {
        // short
        // std::cerr << "SHORT: " << tmp << std::endl;

        std::string c {tmp.at(1)};
        if (_flags.find(c) != _flags.end() && !(_data.at(_flags.at(c)).mode_))
        {
          // short arg
          // std::cerr << "SHORT: arg -> " << c << std::endl;

          if (_data.at(_flags.at(c)).seen_ && ! _data.at(_flags.at(c)).multi_)
          {
            // error
            _error = "flag '-" + c + "' has already been seen";

            return -1;
          }

          if (tmp.size() > 2 && tmp.at(2) != '=')
          {
            _data.at(_flags.at(c)).value_.emplace_back(tmp.substr(2, tmp.size() - 1));
            _data.at(_flags.at(c)).seen_ = true;
          }
          else if (tmp.size() > 3 && tmp.at(2) == '=')
          {
            _data.at(_flags.at(c)).value_.emplace_back(tmp.substr(3, tmp.size() - 1));
            _data.at(_flags.at(c)).seen_ = true;
          }
          else if (i + 1 < _args.size())
          {
            _data.at(_flags.at(c)).value_.emplace_back(_args.at(static_cast<std::size_t>(i + 1)));
            _data.at(_flags.at(c)).seen_ = true;
            ++i;
          }
          else
          {
            // error
            _error = "flag '-" + c + "' requires an arg";

            return -1;
          }
        }
        else
        {
          // short mode
          for (std::size_t j = 1; j < tmp.size(); ++j)
          {
            std::string s {tmp.at(j)};

            if (_flags.find(s) != _flags.end() && _data.at(_flags.at(s)).mode_)
            {
              // std::cerr << "SHORT: mode -> " << s << std::endl;

              if (_data.at(_flags.at(s)).seen_ && ! _data.at(_flags.at(s)).multi_)
              {
                // error
                _error = "flag '-" + s + "' has already been seen";

                return -1;
              }

              _data.at(_flags.at(s)).value_.emplace_back("1");
              _data.at(_flags.at(s)).seen_ = true;
            }
            else
            {
              // error
              _error = "invalid flag '" + tmp + "'";
              // find_similar(s);

              return -1;
            }
          }
        }
      }
      else if (tmp.size() > 2 && tmp.at(0) == '-' && tmp.at(1) == '-')
      {
        // long || --
        // std::cerr << "LONG: " << tmp << std::endl;
        std::string c {tmp.substr(2, tmp.size() - 1)};
        std::string a;

        auto const delim = c.find("=");
        if (delim != std::string::npos)
        {
          c = tmp.substr(2, delim);
          a = tmp.substr(3 + delim, tmp.size() - 1);
        }

        if (_data.find(c) != _data.end())
        {
          if (_data.at(c).seen_ && ! _data.at(c).multi_)
          {
            // error
            _error = "option '--" + c + "' has already been seen";

            return -1;
          }

          if (_data.at(c).mode_ && a.size() == 0)
          {
            // std::cerr << "LONG: mode -> " << c << std::endl;
            _data.at(c).value_.emplace_back("1");
            _data.at(c).seen_ = true;
          }
          else
          {
            // std::cerr << "LONG: arg -> " << c << std::endl;
            if (a.size() > 0)
            {
              _data.at(c).value_.emplace_back(a);
              _data.at(c).seen_ = true;
            }
            else if (i + 1 < _args.size())
            {
              _data.at(c).value_.emplace_back(_args.at(static_cast<std::size_t>(i + 1)));
              _data.at(c).seen_ = true;
              ++i;
            }
            else
            {
              // error
              _error = "option '--" + c + "' requires an arg";

              return -1;
            }
          }
        }
        else
        {
          // error
          _error = "invalid option '" + tmp + "'";
          find_similar(c);

          return -1;
        }
      }
      else if (tmp.size() > 0 && _is_positional)
      {
        // positional
        // std::cerr << "POS: " << tmp << std::endl;
        if (tmp == "--")
        {
          dashdash = true;
        }
        else
        {
          _positional_vec.emplace_back(tmp);
        }
      }
      else
      {
        // error
        _error = "no match for '" + tmp + "'";
        find_similar(tmp);

        return -1;
      }
    }

    return 0;
  }
}; // class Parg

} // namespace OB

#endif // OB_PARG_HH
