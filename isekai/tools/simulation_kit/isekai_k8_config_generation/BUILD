licenses(["notice"])

package(default_visibility = ["//isekai/tools/simulation_kit:__subpackages__"])

proto_library(
    name = "isekai_paths_storage_proto",
    srcs = ["isekai_paths_storage.proto"],
)

cc_proto_library(
    name = "isekai_paths_storage_cc_proto",
    deps = [":isekai_paths_storage_proto"],
)

cc_library(
    name = "isekai_k8_config_generation",
    srcs = ["isekai_k8_config_generation.cc"],
    hdrs = [
        "isekai_k8_config_generation.h",
    ],
    deps = [
        ":isekai_paths_storage_cc_proto",
        "//isekai/common:file_util",
        "//isekai/tools/simulation_kit/common:config_generation_util",
        "//isekai/tools/simulation_kit/common:simulation_kit_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_prod",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "isekai_k8_config_generation_test",
    srcs = ["isekai_k8_config_generation_test.cc"],
    deps = [
        ":isekai_k8_config_generation",
        ":isekai_paths_storage_cc_proto",
        "//isekai/common:file_util",
        "//isekai/common:status_util",
        "//isekai/tools/simulation_kit/common:config_generation_util",
        "//isekai/tools/simulation_kit/common:simulation_kit_cc_proto",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_binary(
    name = "isekai_k8_config_generation_binary",
    srcs = ["isekai_k8_config_generation_binary.cc"],
    deps = [
        ":isekai_k8_config_generation",
        "//isekai/common:status_util",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
    ],
)
