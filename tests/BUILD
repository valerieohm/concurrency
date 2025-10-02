load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "test_main",
    srcs = [
        "test_main.cpp",
        "test_thread_safe_queue.cpp",
        # "test_thread_safe_cache.cpp",
        # "test_dining_philosophers.cpp",
        # "test_producer_consumer.cpp",
        # "test_read_write_lock.cpp",
    ],
    deps = [
        "//:concurrency",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
    copts = ["-g", "-O0"],
)