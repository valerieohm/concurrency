load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_library(
    name = "libnae",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob(["include/*.hpp"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    copts = ["-g", "-O0"],  # Add debug info and disable optimization
    linkstatic = 1,
)

cc_binary(
    name = "test_positions",
    srcs = ["src/test_positions.cpp"],
    deps = [":libnae"],
)