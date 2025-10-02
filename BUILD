load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "concurrency",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob(["include/*.hpp"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    copts = ["-std=c++17", "-pthread", "-g", "-O0"],  # C++17, threading, debug info
    linkopts = ["-pthread"],
)
