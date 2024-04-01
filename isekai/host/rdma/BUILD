package(default_visibility = ["//isekai:__subpackages__"])

licenses(["notice"])

cc_library(
    name = "rdma_base_model",
    srcs = ["rdma_base_model.cc"],
    hdrs = ["rdma_base_model.h"],
    deps = [
        ":rdma_component_interfaces",
        ":rdma_free_list_manager",
        ":rdma_latency_histograms_cc_proto",
        ":rdma_qp_manager",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:net_address",
        "//isekai/common:status_util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "rdma_component_interfaces",
    hdrs = ["rdma_component_interfaces.h"],
    deps = [
        "//isekai/common:model_interfaces",
        "//isekai/common:net_address",
        "//isekai/common:packet",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "rdma_configuration",
    hdrs = ["rdma_configuration.h"],
)

cc_library(
    name = "rdma_roce_cc_dcqcn",
    srcs = ["rdma_roce_cc_dcqcn.cc"],
    hdrs = ["rdma_roce_cc_dcqcn.h"],
    deps = [
        ":rdma_component_interfaces",
        ":rdma_configuration",
        "//isekai/common:common_util",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "rdma_roce_model",
    srcs = ["rdma_roce_model.cc"],
    hdrs = ["rdma_roce_model.h"],
    deps = [
        ":rdma_base_model",
        ":rdma_component_interfaces",
        ":rdma_configuration",
        ":rdma_qp_manager",
        ":rdma_roce_cc_dcqcn",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/meta:type_traits",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_prod",
    ],
)

cc_test(
    name = "rdma_roce_model_test",
    srcs = ["rdma_roce_model_test.cc"],
    deps = [
        ":rdma_component_interfaces",
        ":rdma_configuration",
        ":rdma_roce_cc_dcqcn",
        ":rdma_roce_model",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/fabric:constants",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@omnetpp//:omnetpp_headers",
    ],
)

cc_test(
    name = "rdma_roce_cc_dcqcn_test",
    srcs = ["rdma_roce_cc_dcqcn_test.cc"],
    deps = [
        ":rdma_base_model",
        ":rdma_component_interfaces",
        ":rdma_configuration",
        ":rdma_qp_manager",
        ":rdma_roce_cc_dcqcn",
        ":rdma_roce_model",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@omnetpp//:omnetpp_headers",
    ],
)

cc_library(
    name = "rdma_free_list_manager",
    srcs = ["rdma_free_list_manager.cc"],
    hdrs = ["rdma_free_list_manager.h"],
    deps = [
        ":rdma_component_interfaces",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "rdma_free_list_manager_test",
    srcs = ["rdma_free_list_manager_test.cc"],
    deps = [
        ":rdma_free_list_manager",
        "//isekai/common:config_cc_proto",
        "@com_google_absl//absl/status",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "rdma_falcon_model",
    srcs = [
        "rdma_falcon_model.cc",
        "rdma_falcon_work_scheduler.cc",
        "rdma_falcon_work_scheduler.h",
    ],
    hdrs = ["rdma_falcon_model.h"],
    deps = [
        ":rdma_base_model",
        ":rdma_component_interfaces",
        ":rdma_latency_histograms_cc_proto",
        ":rdma_per_host_rx_buffers",
        ":rdma_qp_manager",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:tdigest",
        "//isekai/common:token_bucket",
        "//isekai/host/falcon:falcon_utils",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "rdma_falcon_model_test",
    srcs = ["rdma_falcon_model_test.cc"],
    deps = [
        ":rdma_base_model",
        ":rdma_component_interfaces",
        ":rdma_falcon_model",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/fabric:constants",
        "//isekai/host/rnic:memory_interface",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "rdma_latency_histograms_proto",
    srcs = ["rdma_latency_histograms.proto"],
    deps = ["//isekai/common:tdigest_proto"],
)

cc_proto_library(
    name = "rdma_latency_histograms_cc_proto",
    deps = [":rdma_latency_histograms_proto"],
)

cc_library(
    name = "rdma_qp_manager",
    srcs = ["rdma_qp_manager.cc"],
    hdrs = ["rdma_qp_manager.h"],
    deps = [
        ":rdma_component_interfaces",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "rdma_per_host_rx_buffers",
    srcs = ["rdma_per_host_rx_buffers.cc"],
    hdrs = ["rdma_per_host_rx_buffers.h"],
    deps = [
        ":rdma_base_model",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/rnic:memory_interface",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "rdma_per_host_rx_buffers_test",
    srcs = ["rdma_per_host_rx_buffers_test.cc"],
    deps = [
        ":rdma_per_host_rx_buffers",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/rnic",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)
