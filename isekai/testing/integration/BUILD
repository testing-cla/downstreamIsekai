licenses(["notice"])

# Exports the OMNest configuration files like *.ned and *.ini
# so that the files can be packaged and used by flume.
exports_files(glob(["*.ned"]) + glob(["*.ini"]))

filegroup(
    name = "integration_config",
    data = [
        "dummy_network.ned",
        "dummy_omnetpp.ini",
    ],
    visibility = ["//isekai:__subpackages__"],
)

cc_library(
    name = "omnest_embedded_kernel",
    srcs = ["omnest_embedded_kernel.cc"],
    hdrs = ["omnest_embedded_kernel.h"],
    copts = ["-fexceptions"],
    features = ["-use_header_modules"],
    visibility = ["//isekai/host/rnic:__pkg__"],
    deps = [
        "//isekai/common:config_cc_proto",
        "//isekai/common:file_util",
        "//isekai/host/rnic:omnest_environment",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_glog//:glog",
        "@omnetpp",
    ],
)

cc_binary(
    name = "isekai_simulation_main",
    deps = [
        ":omnest_falcon_host",
        ":omnest_roce_host",
        "//isekai/fabric:network_router",
        "//isekai/omnetpp:main",
        "@inet//:omnetpp_inet",
        "@omnetpp",
    ],
)

cc_library(
    name = "omnest_falcon_host",
    srcs = ["omnest_falcon_host.cc"],
    hdrs = ["omnest_falcon_host.h"],
    deps = [
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:file_util",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon/gen2:falcon_model",
        "//isekai/host/falcon/gen3:falcon_model",
        "//isekai/host/rdma:rdma_falcon_model",
        "//isekai/host/rnic:connection_manager",
        "//isekai/host/rnic:omnest_host",
        "//isekai/host/rnic:traffic_shaper_model",
        "//isekai/host/traffic:traffic_generator",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
        "@inet//:omnetpp_inet",
        "@omnetpp",
    ],
    alwayslink = 1,
)

cc_test(
    name = "omnest_embedded_kernel_integration_test",
    # Sets a long timeout so that the testing will not timeout.
    timeout = "long",
    srcs = ["omnest_embedded_kernel_integration_test.cc"],
    data = [
        "falcon_host.ned",
        "host.ned",
        "omnetpp.ini",
        "omnetpp_latency_eval.ini",
        "test_network.ned",
        "//isekai/test_data:test_configurations",
        "@inet//:inet_src",
    ],
    deps = [
        ":omnest_embedded_kernel",
        ":omnest_falcon_host",
        "//isekai/common:model_interfaces",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@omnetpp",
    ],
)

cc_test(
    name = "omnest_falcon_network_model_integration_test",
    # Sets a long timeout so that the testing will not timeout.
    timeout = "long",
    srcs = ["omnest_falcon_network_model_integration_test.cc"],
    data = [
        "falcon_host.ned",
        "host.ned",
        "omnetpp_bifurcated_hosts.ini",
        "omnetpp_falcon_gen2_inter_rack_static_route_simulation.ini",
        "omnetpp_falcon_gen2_network_integration_bidirection.ini",
        "omnetpp_falcon_gen2_per_flow_delay.ini",
        "omnetpp_falcon_gen3_inter_rack_static_route_simulation.ini",
        "omnetpp_falcon_gen3_network_integration_bidirection_multipath_disabled.ini",
        "omnetpp_falcon_gen3_network_integration_bidirection_multipath_enabled.ini",
        "omnetpp_falcon_inter_rack_simulation.ini",
        "omnetpp_falcon_network_integration_bidirection.ini",
        "omnetpp_falcon_network_one_side_op_integration.ini",
        "omnetpp_falcon_network_solicitation_write_bidirectional.ini",
        "omnetpp_falcon_network_two_side_op_integration.ini",
        "omnetpp_two_falcon_host_network.ini",
        "test_falcon_network_model_integration.ned",
        "test_falcon_network_model_inter_rack.ned",
        "test_falcon_network_model_ndv6_flows.ned",
        "//isekai/fabric:network_model_src",
        "//isekai/test_data:test_configurations",
        "@inet//:inet_src",
    ],
    tags = ["external"],
    deps = [
        ":omnest_embedded_kernel",
        ":omnest_falcon_host",
        "//isekai/fabric:network_router",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@omnetpp",
    ],
)

cc_library(
    name = "omnest_roce_host",
    srcs = ["omnest_roce_host.cc"],
    hdrs = ["omnest_roce_host.h"],
    deps = [
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:file_util",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/rdma:rdma_configuration",
        "//isekai/host/rdma:rdma_roce_cc_dcqcn",
        "//isekai/host/rdma:rdma_roce_model",
        "//isekai/host/rnic",
        "//isekai/host/rnic:connection_manager",
        "//isekai/host/rnic:omnest_host",
        "//isekai/host/traffic:traffic_generator",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
        "@inet//:omnetpp_inet",
        "@omnetpp",
    ],
    alwayslink = 1,
)

cc_test(
    name = "omnest_roce_network_model_integration_test",
    # Sets a long timeout so that the testing will not timeout.
    timeout = "long",
    srcs = ["omnest_roce_network_model_integration_test.cc"],
    data = [
        "host.ned",
        "omnetpp_roce_inter_rack_simulation.ini",
        "omnetpp_roce_network_integration.ini",
        "roce_host.ned",
        "test_roce_network_model_integration.ned",
        "test_roce_network_model_inter_rack.ned",
        "//isekai/fabric:network_model_src",
        "//isekai/test_data:test_configurations",
        "@inet//:inet_src",
    ],
    deps = [
        ":omnest_embedded_kernel",
        ":omnest_roce_host",
        "//isekai/fabric:network_router",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@omnetpp",
    ],
)

cc_test(
    name = "omnest_roce_pfc_test",
    # Sets a long timeout so that the testing will not timeout.
    timeout = "long",
    srcs = ["omnest_roce_pfc_test.cc"],
    data = [
        "host.ned",
        "omnetpp_roce_pfc.ini",
        "roce_host.ned",
        "test_roce_pfc.ned",
        "//isekai/fabric:network_model_src",
        "//isekai/test_data:test_configurations",
        "@inet//:inet_src",
    ],
    # Disable test caching
    tags = ["external"],
    deps = [
        ":omnest_embedded_kernel",
        ":omnest_roce_host",
        "//isekai/common:model_interfaces",
        "//isekai/fabric:network_router",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@omnetpp",
    ],
)
