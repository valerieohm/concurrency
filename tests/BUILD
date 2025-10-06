load("@rules_cc//cc:cc_test.bzl", "cc_test")

# Fast tests - data structures
cc_test(
    name = "test_data_structures",
    srcs = [
        "test_main.cpp",
        "test_thread_safe_queue.cpp",
        "test_thread_safe_cache.cpp",
    ],
    deps = [
        "//:concurrency",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-g", "-O0"],
)

# Producer-Consumer tests only
cc_test(
    name = "test_producer_consumer",
    srcs = [
        "test_main.cpp",
        "test_producer_consumer.cpp",
    ],
    deps = [
        "//:concurrency",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-g", "-O0"],
)

# Resource Pool with Semaphores
cc_test(
    name = "test_resource_pool",
    srcs = [
        "test_main.cpp",
        "test_resource_pool.cpp",
    ],
    deps = [
        "//:concurrency",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-g", "-O0", "-std=c++20"],  # C++20 for semaphore support
)

# Slow tests - dining philosophers
cc_test(
    name = "test_dining_philosophers",
    srcs = [
        "test_main.cpp",
        "test_dining_philosophers.cpp",
    ],
    deps = [
        "//:concurrency",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-g", "-O0"],
    timeout = "long",  # Mark as slow test
)

# All tests combined (for CI/full validation)
cc_test(
    name = "test_main",
    srcs = [
        "test_main.cpp",
        "test_thread_safe_queue.cpp",
        "test_thread_safe_cache.cpp",
        "test_dining_philosophers.cpp",
        "test_producer_consumer.cpp",
    ],
    deps = [
        "//:concurrency",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-g", "-O0"],
    timeout = "long",
)
