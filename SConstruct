# -*- Mode: Python -*-

import os

CacheDir(".cache")

AddOption("--coverage",
          dest="enable_coverage",
          action="store_true", default=False)

env = Environment(ENV = os.environ,
                  CXX = os.environ.get("CXX", "g++"),
                  CXXFLAGS = "-Wall -std=c++17 -g -Wall",
                  LINKFLAGS = "-g")

if GetOption("enable_coverage"):
    env.Append(CXXFLAGS = " -Og --coverage ",
               LINKFLAGS = " --coverage ")
else:
    env.Append(CXXFLAGS = " -O2 ")

env.Program("ptwalker", [Glob("*.cpp")])
