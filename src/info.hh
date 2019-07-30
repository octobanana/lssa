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

#ifndef INFO_HH
#define INFO_HH

#include "ob/parg.hh"
#include "ob/term.hh"

#include <cstddef>

#include <string>
#include <string_view>
#include <iostream>

inline int program_info(OB::Parg& pg);
inline bool program_color(std::string_view color);
inline void program_init(OB::Parg& pg);

inline void program_init(OB::Parg& pg)
{
  pg.name("lssa").version("0.3.0 (29.07.2019)");
  pg.description("List similar artists.");

  pg.usage("[--colour=<on|off|auto>] [-c|--count=<1-100>] [-H|--header=<key:value>]... [-i|--ignore-case] <artist>...");
  pg.usage("[--colour=<on|off|auto>] -h|--help");
  pg.usage("[--colour=<on|off|auto>] -v|--version");
  pg.usage("[--colour=<on|off|auto>] --license");

  pg.info({"Examples", {
    {"lssa <artist>",
      "list 10 similar artists to <artist>"},
    {"lssa <artist-1> <artist-2>",
      "list 10 similar artists for each <artist-n>"},
    {"lssa -c 20 <artist>",
      "list 20 similar artists to <artist>"},
    {"lssa -H 'user-agent:custom user agent string' -H 'dnt:1' <artist>",
      "list 10 similar artists to <artist> and pass custom HTTP request headers"},
    {"lssa --help --colour=off",
      "print the help output, without colour"},
    {"lssa --help",
      "print the help output"},
    {"lssa --version",
      "print the program version"},
    {"lssa --license",
      "print the program license"},
  }});

  pg.info({"Exit Codes", {
    {"0", "normal"},
    {"1", "error"},
  }});

  pg.info({"Remotes", {
    {"", "https://www.last.fm/music/<artist>/+similar"},
  }});

  pg.info({"Repository", {
    {"", "https://github.com/octobanana/lssa.git"},
  }});

  pg.info({"Homepage", {
    {"", "https://octobanana.com/software/lssa"},
  }});

  pg.info({"Meta", {
    {"", "The version format is 'major.minor.patch (day.month.year)'."},
  }});

  pg.author("Brett Robinson (octobanana) <octobanana.dev@gmail.com>");

  // general flags
  pg.set("help,h", "Print the help output.");
  pg.set("version,v", "Print the program version.");
  pg.set("license", "Print the program license.");

  // options
  pg.set("colour", "auto", "on|off|auto", "Print the program output with colour either on, off, or auto based on if stdout is a tty, the default value is 'auto'.");
  pg.set("count,c", "10", "1-100", "The maximum number of matches to find for each artist, the default value is '10'.");
  pg.set("header,H", {}, "key:value", "Pass a custom HTTP request header, this option can be used multiple times.", true);
  pg.set("ignore-case,i", "Ignore artist case and use titlecase.");

  // allow and capture positional arguments
  pg.set_pos();
}

inline bool program_color(std::string_view color)
{
  if (color == "on")
  {
    // color on
    return true;
  }

  if (color == "off")
  {
    // color off
    return false;
  }

  // color auto
  return OB::Term::is_term(STDOUT_FILENO);
}

inline int program_info(OB::Parg& pg)
{
  // init info/options
  program_init(pg);

  // parse options
  auto const status {pg.parse()};

  // set output color choice
  pg.color(program_color(pg.get<std::string>("colour")));

  if (status < 0)
  {
    // an error occurred
    std::cerr
    << pg.usage()
    << "\n"
    << pg.error();

    return -1;
  }

  if (pg.get<bool>("help"))
  {
    // show help output
    std::cout << pg.help();

    return 1;
  }

  if (pg.get<bool>("version"))
  {
    // show version output
    std::cout << pg.version();

    return 1;
  }

  if (pg.get<bool>("license"))
  {
    // show license output
    std::cout << pg.license();

    return 1;
  }

  if (auto const artist_count = pg.get_pos_vec().size(); artist_count == 0 || artist_count > 100)
  {
    if (artist_count == 0)
    {
      pg.error("missing required argument <artist>, expected at least one artist");
    }
    else
    {
      pg.error("too many arguments for <artist>, the number of artists must be between 1-100");
    }

    // an error occurred
    std::cerr
    << pg.usage()
    << "\n"
    << pg.error();

    return -1;
  }

  if (pg.get<std::size_t>("count") < 1 || pg.get<std::size_t>("count") > 100)
  {
    pg.error("count is out of range, value must be between 1-100");

    // an error occurred
    std::cerr
    << pg.usage()
    << "\n"
    << pg.error();

    return -1;
  }

  // success
  return 0;
}

#endif // INFO_HH
