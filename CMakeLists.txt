cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

set (OB_TARGET "lssa")
set (OB_VERSION "0.1.0 (19.07.2019)")
project (${OB_TARGET})

message (">> Building ${OB_TARGET} ${OB_VERSION} in ${CMAKE_BUILD_TYPE} mode")
message (">> System is ${CMAKE_SYSTEM}")
message (">> C++ compiler is ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

find_program (OB_CCACHE_FOUND ccache)
if (OB_CCACHE_FOUND)
  message (">> Found ccache")
  set_property (GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property (GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif ()

set (OB_BOOST_VERSION "1.68.0")
set (OB_BOOST_LIBRARIES system iostreams)
find_package (Boost ${OB_BOOST_VERSION} COMPONENTS ${OB_BOOST_LIBRARIES} REQUIRED)
if (Boost_FOUND)
  message (">> Found Boost ${Boost_VERSION}")
else ()
  message (FATAL_ERROR ">> The Boost library could not be found")
endif ()

# set (CMAKE_CXX_COMPILER "g++")
# set (CMAKE_CXX_COMPILER "clang++")

set (OB_COMPILER_VERSION_GNU "8.0.0")
set (OB_COMPILER_VERSION_CLANG "7.0.0")
set (OB_COMPILER_VERSION_APPLECLANG "11.0.0")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # check if supported version
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OB_COMPILER_VERSION_GNU}")
    message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler version ${CMAKE_CXX_COMPILER_VERSION} is unsupported, requires at least version ${OB_COMPILER_VERSION_GNU}")
  endif ()

  set (OB_FLAGS_GENERAL "-std=c++17 -fdiagnostics-color=auto -fmax-errors=8")

  set (OB_FLAGS_DEBUG "-g -Og -rdynamic -Wpedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused")
  set (DEBUG_LINK_FLAGS "-fprofile-arcs -ftest-coverage")

  set (OB_FLAGS_RELEASE "-O3 -flto")
  set (OB_LINKER_FLAGS_RELEASE "-O3 -flto -s")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  # check if supported version
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OB_COMPILER_VERSION_CLANG}")
      message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler version ${CMAKE_CXX_COMPILER_VERSION} is unsupported, requires at least version ${OB_COMPILER_VERSION_CLANG}")
    endif ()
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    # unsupported compiler
    message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler is unsupported")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OB_COMPILER_VERSION_APPLECLANG}")
      message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler version ${CMAKE_CXX_COMPILER_VERSION} is unsupported, requires at least version ${OB_COMPILER_VERSION_APPLECLANG}")
    endif ()
  else ()
    # unsupported compiler
    message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler is unsupported")
  endif ()

  set (OB_FLAGS_GENERAL "-std=c++17 -fdiagnostics-color=auto -ferror-limit=8")

  set (OB_FLAGS_DEBUG "-g -Og -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-disabled-macro-expansion -Wno-exit-time-destructors -Wno-global-constructors -Wno-weak-vtables")
  set (OB_LINKER_FLAGS_DEBUG "-fprofile-arcs -ftest-coverage")

  set (OB_FLAGS_RELEASE "-O3 -flto")
  set (OB_LINKER_FLAGS_RELEASE "-O3 -flto -s")
else ()
  # unsupported compiler
  message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler is unsupported")
endif ()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OB_FLAGS_GENERAL}")

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OB_FLAGS_DEBUG}")
set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${OB_LINKER_FLAGS_DEBUG}")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OB_FLAGS_RELEASE}")
set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${OB_LINKER_FLAGS_RELEASE}")

set (OB_SOURCES
  src/main.cc
  src/app/app.cc
  src/ob/string.cc
)

add_executable (
  ${OB_TARGET}
  ${OB_SOURCES}
)

target_include_directories (
  ${OB_TARGET}
  PRIVATE
  ./src
)

target_link_libraries (
  ${OB_TARGET}
  ${Boost_LIBRARIES}
  pthread
  icui18n
  icuuc
  z
  ssl
  crypto
)

install (TARGETS ${OB_TARGET} DESTINATION bin)
