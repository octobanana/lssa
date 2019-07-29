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

#include "info.hh"

#include "app/app.hh"

#include "ob/parg.hh"
#include "ob/term.hh"

#include <cstddef>

#include <string>
#include <sstream>
#include <iostream>

using Parg = OB::Parg;

namespace Term = OB::Term;
namespace iom = OB::Term::iomanip;
namespace aec = OB::Term::ANSI_Escape_Codes;

int main(int argc, char** argv)
{
  std::ios_base::sync_with_stdio(false);

  Parg pg {argc, argv};
  auto const pg_status {program_info(pg)};
  if (pg_status > 0) return 0;
  if (pg_status < 0) return 1;

  try
  {
    App app;

    app.artists(pg.get_pos_vec(), pg.get<bool>("ignore-case"));
    app.count(pg.get<std::size_t>("count"));
    app.headers(pg.get_all<std::string>("header"));
    app.progress(Term::is_term(STDERR_FILENO));

    auto const color = pg.get<std::string>("colour");
    app.color(color == "auto" ? Term::is_term(STDOUT_FILENO) : color == "on");

    app.run();
  }
  catch(std::exception const& e)
  {
    std::cerr << "\nError: " << e.what() << "\n";

    return 1;
  }
  catch(...)
  {
    std::cerr << "\nError: an unexpected error occurred\n";

    return 1;
  }

  return 0;
}
