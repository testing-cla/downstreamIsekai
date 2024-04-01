package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rue",
    hdrs = [
        "bits.h",
        "constants.h",
        "fixed.h",
        "format.h",
        "format_bna.h",
        "format_dna_a.h",
        "format_dna_b.h",
        "format_dna_c.h",
        "util.h",
    ],
    deps = [
        "//isekai/host/falcon",
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "latency_generator",
    hdrs = ["latency_generator.h"],
    deps = [
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/random:bit_gen_ref",
        "@com_google_absl//absl/random:distributions",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "bits_test",
    srcs = [
        "bits_test.cc",
    ],
    deps = [
        ":rue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fixed_test",
    srcs = [
        "fixed_test.cc",
    ],
    deps = [
        ":rue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "format_test",
    srcs = [
        "format_test.cc",
    ],
    deps = [
        ":rue",
        "//isekai/host/falcon",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "util_test",
    srcs = [
        "util_test.cc",
    ],
    deps = [
        ":rue",
        "//isekai/host/falcon",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "latency_generator_test",
    srcs = ["latency_generator_test.cc"],
    deps = [
        ":latency_generator",
        "@com_google_absl//absl/random:mock_distributions",
        "@com_google_absl//absl/random:mocking_bit_gen",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)
