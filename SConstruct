# -*- Mode: Python -*-

import os

CacheDir(".cache")

AddOption("--lto", dest="enable_lto", action="store_true", default=False)
AddOption("--coverage", dest="enable_coverage", action="store_true", default=False)
AddOption("--asan", dest="enable_asan", action="store_true", default=False)
AddOption("--ubsan", dest="enable_ubsan", action="store_true", default=False)

env = Environment(ENV = os.environ,
                  CXX = os.environ.get("CXX", "g++"),
                  CXXFLAGS = "-Wall -std=c++17 -g -Wall",
                  CPPPATH = ["#src"],
                  LINKFLAGS = "-g ")

if GetOption("enable_coverage"):
    env.Append(CXXFLAGS = " -Og --coverage ",
               LINKFLAGS = " --coverage ")
else:
    env.Append(CXXFLAGS = " -O2 ")

if GetOption("enable_asan"):
    env.Append(CXXFLAGS = " -fsanitize=address ",
               LINKFLAGS = " -fsanitize=address ")

if GetOption("enable_ubsan"):
    env.Append(CXXFLAGS = " -fsanitize=undefined ",
               LINKFLAGS = " -fsanitize=undefined ")

if GetOption("enable_lto"):
    env.Append(CXXFLAGS = " -flto ",
               LINKFLAGS = " -flto ")

env.Program("ptwalker", [Glob("src/*.cpp") + Glob("test/*.cpp")])
