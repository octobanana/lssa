# lssa
List similar artists.

[lssa](https://raw.githubusercontent.com/octobanana/lssa/master/assets/lssa.png)

## Contents
* [About](#about)
* [Usage](#usage)
* [Pre-Build](#pre-build)
  * [Environments](#environments)
  * [Compilers](#compilers)
  * [Dependencies](#dependencies)
  * [Linked Libraries](#linked-libraries)
  * [Included Libraries](#included-libraries)
  * [macOS](#macos)
* [Build](#build)
* [Install](#install)
* [License](#license)

## About
__lssa__ is a CLI program for the terminal that accepts one or more music artists as arguments and returns a list of similar artists.

## Usage
View the usage and help output with the `--help` or `-h` flag,
or `./help.txt` to view the help output as a plain text file.

## Pre-Build
This section describes what environments this program may run on,
any prior requirements or dependencies needed,
and any third party libraries used.

> #### Important
> Any shell commands using relative paths are expected to be executed in the
> root directory of this repository.

### Environments
* __Linux__ (supported)
* __BSD__ (supported)
* __macOS__ (supported)

### Compilers
* __GCC__ >= 8.0.0 (supported)
* __Clang__ >= 7.0.0 (supported)
* __Apple Clang__ >= 11.0.0 (untested)

### Dependencies
* __ZLib__
* __PThread__
* __OpenSSL__ >= 1.1.0
* __CMake__ >= 3.8
* __ICU__ >= 62.1
* __Boost__ >= 1.68.0

### Linked Libraries
* __z__ (libz) ZLib library for gzip and deflate compression
* __ssl__ (libssl) part of the OpenSSL library
* __crypto__ (libcrypto) part of the OpenSSL library
* __pthread__ (libpthread) POSIX threads library
* __icuuc__ (libicuuc) part of the ICU library
* __icui18n__ (libicui18n) part of the ICU library
* __boost_system__ (libboost_system) part of the Boost library
* __boost_iostreams__ (libboost_iostreams) part of the Boost library

### Included Libraries
* [__Parg__](https://github.com/octobanana/parg):
  for parsing CLI args, modified and included as `./src/ob/parg.hh`
* [__Belle__](https://github.com/octobanana/belle):
  HTTP client, modified and included as `./src/ob/belle.hh`

### macOS
Using a new version of __GCC__ or __Clang__ is __required__, as the default
__Apple Clang compiler__ does __not__ support C++17 Standard Library features such as `std::filesystem`.

A new compiler can be installed through a third-party package manager such as __Brew__.
Assuming you have __Brew__ already installed, the following commands should install
the latest __GCC__.

```
brew install gcc
brew link gcc
```

The following CMake argument will then need to be appended to the end of the line when running the build script.
Remember to replace the placeholder `<path-to-g++>` with the canonical path to the new __g++__ compiler binary.

```sh
./build.sh -DCMAKE_CXX_COMPILER='<path-to-g++>'
```

## Build
The following shell command will build the project in release mode:
```sh
./build.sh
```
To build in debug mode, run the script with the `--debug` flag.

## Install
The following shell command will install the project in release mode:
```sh
./install.sh
```
To install in debug mode, run the script with the `--debug` flag.

## License
This project is licensed under the MIT License.

Copyright (c) 2019 [Brett Robinson](https://octobanana.com/)

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
