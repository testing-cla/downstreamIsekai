package(default_visibility = ["//isekai:__subpackages__"])

licenses(["notice"])

cc_library(
    name = "traffic_generator",
    srcs = ["traffic_generator.cc"],
    hdrs = ["traffic_generator.h"],
    deps = [
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_absl//absl/types:variant",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_prod",
    ],
)

cc_test(
    name = "traffic_generator_test",
    srcs = ["traffic_generator_test.cc"],
    data = [
        "//isekai/test_data:test_configurations",
    ],
    deps = [
        ":traffic_generator",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:file_util",
        "//isekai/common:model_interfaces",
        "//isekai/common:simple_environment",
        "//isekai/fabric:constants",
        "//isekai/host/rdma:rdma_falcon_model",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)
