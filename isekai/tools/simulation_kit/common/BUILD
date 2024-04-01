licenses(["notice"])

package(default_visibility = ["//isekai/tools/simulation_kit:__subpackages__"])

cc_library(
    name = "config_generation_util",
    srcs = ["config_generation_util.cc"],
    hdrs = [
        "config_generation_util.h",
    ],
    deps = [
        ":simulation_kit_cc_proto",
        "//isekai/common:config_cc_proto",
        "//isekai/common:file_util",
        "//isekai/common:status_util",
        "//isekai/tools/simulation_kit/isekai_k8_config_generation:isekai_paths_storage_cc_proto",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "config_generation_util_test",
    srcs = ["config_generation_util_test.cc"],
    deps = [
        ":config_generation_util",
        "//isekai/common:status_util",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

proto_library(
    name = "simulation_kit_proto",
    srcs = ["simulation_kit.proto"],
)

cc_proto_library(
    name = "simulation_kit_cc_proto",
    deps = [":simulation_kit_proto"],
)
