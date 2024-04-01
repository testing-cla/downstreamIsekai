package(default_visibility = ["//visibility:public"])

proto_library(
    name = "config_proto",
    srcs = [
        "algorithm.proto",
        "bypass.proto",
        "swift.proto",
    ],
)

cc_proto_library(
    name = "config_cc_proto",
    deps = [":config_proto"],
)

cc_library(
    name = "algorithm",
    srcs = [
        "bypass.cc",
        "swift.cc",
    ],
    hdrs = [
        "bypass.h",
        "swift.h",
    ],
    deps = [
        ":config_cc_proto",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "dram_state_manager",
    hdrs = [
        "dram_state_manager.h",
    ],
    deps = [
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "dram_state_manager_test",
    srcs = [
        "dram_state_manager_test.cc",
    ],
    deps = [
        ":dram_state_manager",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/log:check",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stateful_algorithm",
    hdrs = [
        "stateful_algorithm.h",
    ],
    deps = [
        ":algorithm",
        "//isekai/host/falcon/rue",
    ],
)

cc_test(
    name = "stateful_algorithm_test",
    srcs = [
        "stateful_algorithm_test.cc",
    ],
    deps = [
        ":algorithm",
        ":config_cc_proto",
        ":dram_state_manager",
        ":stateful_algorithm",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "bypass_test",
    srcs = [
        "bypass_test.cc",
    ],
    deps = [
        ":algorithm",
        ":config_cc_proto",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "swift_test",
    srcs = [
        "swift_test.cc",
    ],
    deps = [
        ":algorithm",
        ":config_cc_proto",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "swift_bna_test",
    srcs = ["swift_bna_test.cc"],
    deps = [
        ":algorithm",
        ":config_cc_proto",
        ":dram_state_manager",
        ":stateful_algorithm",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)
