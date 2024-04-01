package(default_visibility = ["//isekai:__subpackages__"])

licenses(["unencumbered"])

cc_library(
    name = "connection_state_manager",
    srcs = ["connection_state_manager.cc"],
    hdrs = ["connection_state_manager.h"],
    deps = [
        ":falcon_utils",
        "//isekai/common:status_util",
        "//isekai/host/falcon/gen2:connection_state_manager",
    ],
)

cc_test(
    name = "connection_state_manager_test",
    srcs = ["connection_state_manager_test.cc"],
    deps = [
        ":connection_state_manager",
        ":falcon_model",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "resource_manager",
    srcs = ["resource_manager.cc"],
    hdrs = ["resource_manager.h"],
    deps = [
        ":falcon_utils",
        "//isekai/host/falcon/gen2:resource_manager",
    ],
)

cc_test(
    name = "resource_manager_test",
    srcs = ["resource_manager_test.cc"],
    deps = [
        ":falcon_utils",
        ":resource_manager",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/fabric:constants",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_resource_credits",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon/rue",
        "//isekai/host/rdma:rdma_falcon_model",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "inter_host_rx_scheduler",
    srcs = ["inter_host_rx_scheduler.cc"],
    hdrs = ["inter_host_rx_scheduler.h"],
    deps = ["//isekai/host/falcon/gen2:inter_host_rx_scheduler"],
)

cc_library(
    name = "connection_scheduler",
    srcs = ["connection_scheduler.cc"],
    hdrs = ["connection_scheduler.h"],
    deps = [
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_protocol_connection_scheduler_types",
        "//isekai/host/falcon/gen2:connection_scheduler",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "connection_scheduler_test",
    srcs = ["connection_scheduler_test.cc"],
    deps = [
        ":connection_scheduler",
        ":falcon_model",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon:falcon_types",
        "//isekai/host/falcon/rue",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "retransmission_scheduler",
    srcs = ["retransmission_scheduler.cc"],
    hdrs = ["retransmission_scheduler.h"],
    deps = ["//isekai/host/falcon/gen2:retransmission_scheduler"],
)

cc_library(
    name = "ack_nack_scheduler",
    srcs = ["ack_nack_scheduler.cc"],
    hdrs = ["ack_nack_scheduler.h"],
    deps = ["//isekai/host/falcon/gen2:ack_nack_scheduler"],
)

cc_library(
    name = "arbiter",
    srcs = ["arbiter.cc"],
    hdrs = ["arbiter.h"],
    deps = ["//isekai/host/falcon/gen2:arbiter"],
)

cc_library(
    name = "admission_control_manager",
    srcs = ["admission_control_manager.cc"],
    hdrs = ["admission_control_manager.h"],
    deps = [
        ":falcon_utils",
        "//isekai/host/falcon/gen2:admission_control_manager",
    ],
)

cc_test(
    name = "admission_control_manager_test",
    srcs = ["admission_control_manager_test.cc"],
    deps = [
        ":admission_control_manager",
        ":falcon_model",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "reliability_manager",
    srcs = ["reliability_manager.cc"],
    hdrs = ["reliability_manager.h"],
    deps = ["//isekai/host/falcon/gen2:reliability_manager"],
)

cc_library(
    name = "rate_update_engine",
    srcs = ["rate_update_engine.cc"],
    hdrs = ["rate_update_engine.h"],
    deps = ["//isekai/host/falcon/gen2:rate_update_engine"],
)

cc_library(
    name = "reorder_engine",
    srcs = ["reorder_engine.cc"],
    hdrs = ["reorder_engine.h"],
    deps = [
        ":falcon_utils",
        "//isekai/host/falcon/gen2:reorder_engine",
    ],
)

cc_test(
    name = "reorder_engine_test",
    srcs = ["reorder_engine_test.cc"],
    deps = [
        ":falcon_model",
        ":reorder_engine",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_counters",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_gen3_component_test",
    srcs = ["falcon_gen3_component_test.cc"],
    deps = [
        ":falcon_utils",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_test_infrastructure",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_counters",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_protocol_resource_manager",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "rate_update_engine_adapter_test",
    srcs = ["rate_update_engine_adapter_test.cc"],
    deps = [
        ":falcon_model",
        ":falcon_utils",
        ":rate_update_engine",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/log:check",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stats_manager",
    srcs = ["stats_manager.cc"],
    hdrs = ["stats_manager.h"],
    deps = [
        ":falcon_utils",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon/gen2:stats_manager",
    ],
)

cc_library(
    name = "falcon_model",
    srcs = ["falcon_model.cc"],
    hdrs = ["falcon_model.h"],
    deps = [
        ":falcon_utils",
        "//isekai/common:packet",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon/gen2:falcon_model",
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "ack_coalescing_engine",
    srcs = ["ack_coalescing_engine.cc"],
    hdrs = ["ack_coalescing_engine.h"],
    deps = [
        "//isekai/host/falcon/gen2:ack_coalescing_engine",
    ],
)

cc_library(
    name = "base_connection_scheduler",
    srcs = ["base_connection_scheduler.cc"],
    hdrs = ["base_connection_scheduler.h"],
    deps = ["//isekai/host/falcon/gen2:base_connection_scheduler"],
)

cc_library(
    name = "falcon_utils",
    srcs = ["falcon_utils.cc"],
    hdrs = ["falcon_utils.h"],
    deps = [
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_protocol_connection_scheduler_types",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "packet_metadata_transformer",
    srcs = ["packet_metadata_transformer.cc"],
    hdrs = ["packet_metadata_transformer.h"],
    deps = [
        ":falcon_utils",
        "//isekai/common:packet",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon/gen2:packet_metadata_transformer",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "packet_metadata_transformer_test",
    srcs = ["packet_metadata_transformer_test.cc"],
    deps = [
        ":falcon_utils",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)
