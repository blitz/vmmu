![GitHub release](https://img.shields.io/github/v/release/blitz/vmmu)
[![stability-experimental](https://img.shields.io/badge/stability-experimental-orange.svg)](https://github.com/emersion/stability-badges#experimental)
![GitHub](https://img.shields.io/github/license/blitz/vmmu.svg)
![GitHub commit activity](https://img.shields.io/github/commit-activity/m/blitz/vmmu)
![Codecov](https://img.shields.io/codecov/c/github/blitz/vmmu)

# vMMU

Tihs repository contains an implementation of the x86
[MMU](https://en.wikipedia.org/wiki/Memory_management_unit) in modern C++. It supports the following
paging modes:

- 64-bit 4-level paging,
- 32-bit PAE paging,
- 32-bit 2-level paging,
- disabled paging.

This library has no dependencies beyond a C++ 17 toolchain.

# Building

The library builds using [CMake](https://cmake.org/). I recommend using
[Ninja](https://ninja-build.org/) as generator, but Make works as well. To install the library into
`/usr/local`, you could do:

```sh
% mkdir build
% cd build
% cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/usr/local
% ninja
% ninja install
```

# Usage

The library is a bit light on documentation for now, but to get started checkout out the `translate`
function in [vmmu/vmmu.hpp](libvmmu/include/vmmu/vmmu.hpp). If you need caching support there is
also a simple TLB class.
