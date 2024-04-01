package(default_visibility = ["//isekai:__subpackages__"])

licenses(["unencumbered"])

cc_library(
    name = "connection_state_manager",
    srcs = ["connection_state_manager.cc"],
    hdrs = ["connection_state_manager.h"],
    deps = [
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:status_util",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_protocol_connection_state_manager",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status",
    ],
)

cc_test(
    name = "connection_state_manager_test",
    srcs = ["connection_state_manager_test.cc"],
    deps = [
        ":connection_state_manager",
        ":falcon_model",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/status",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "resource_manager",
    srcs = ["resource_manager.cc"],
    hdrs = ["resource_manager.h"],
    deps = [
        ":falcon_component_interfaces",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_protocol_resource_manager",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "inter_host_rx_scheduler",
    srcs = ["inter_host_rx_scheduler.cc"],
    hdrs = ["inter_host_rx_scheduler.h"],
    deps = [
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_inter_host_rx_scheduler",
    ],
)

cc_library(
    name = "connection_scheduler",
    srcs = ["connection_scheduler.cc"],
    hdrs = ["connection_scheduler.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_packet_type_based_connection_scheduler"],
)

cc_library(
    name = "retransmission_scheduler",
    srcs = ["retransmission_scheduler.cc"],
    hdrs = ["retransmission_scheduler.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_retransmission_scheduler"],
)

cc_library(
    name = "ack_nack_scheduler",
    srcs = ["ack_nack_scheduler.cc"],
    hdrs = ["ack_nack_scheduler.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_ack_nack_scheduler"],
)

cc_library(
    name = "arbiter",
    srcs = ["arbiter.cc"],
    hdrs = ["arbiter.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_round_robin_arbiter"],
)

cc_library(
    name = "admission_control_manager",
    srcs = ["admission_control_manager.cc"],
    hdrs = ["admission_control_manager.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_admission_control_manager"],
)

cc_library(
    name = "reliability_manager",
    srcs = ["reliability_manager.cc"],
    hdrs = ["reliability_manager.h"],
    deps = [
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:constants",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_protocol_connection_scheduler_types",
        "//isekai/host/falcon:falcon_protocol_packet_reliability_manager",
        "//isekai/host/falcon:falcon_types",
        "//isekai/host/falcon:falcon_utils",
        "//isekai/host/falcon:weighted_round_robin_policy",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "reliability_manager_test",
    srcs = ["reliability_manager_test.cc"],
    deps = [
        ":falcon_utils",
        ":reliability_manager",
        "//isekai/common:config_cc_proto",
        "//isekai/common:constants",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_protocol_rate_update_engine",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "rate_update_engine",
    srcs = ["rate_update_engine.cc"],
    hdrs = ["rate_update_engine.h"],
    deps = [
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_protocol_rate_update_engine",
        "//isekai/host/falcon:falcon_types",
        "//isekai/host/falcon/rue",
        "//isekai/host/falcon/rue/algorithm",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/numeric:bits",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "rate_update_engine_test",
    srcs = ["rate_update_engine_test.cc"],
    deps = [
        ":falcon_utils",
        ":rate_update_engine",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_counters",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_protocol_rate_update_engine",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon/rue",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "reorder_engine",
    srcs = ["reorder_engine.cc"],
    hdrs = ["reorder_engine.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_buffer_reorder_engine"],
)

cc_library(
    name = "stats_manager",
    srcs = ["stats_manager.cc"],
    hdrs = ["stats_manager.h"],
    deps = [
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:stats_manager",
    ],
)

cc_library(
    name = "falcon_model",
    srcs = ["falcon_model.cc"],
    hdrs = ["falcon_model.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_utils",
        ":rx_packet_buffer_fetch_arbiter",
        ":sram_dram_reorder_engine",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/falcon:falcon_component_factories",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/rnic:memory_interface",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "ack_coalescing_engine",
    srcs = ["ack_coalescing_engine.cc"],
    hdrs = ["ack_coalescing_engine.h"],
    deps = [
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:constants",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon:falcon_bitmap",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_protocol_ack_coalescing_engine",
        "//isekai/host/falcon:falcon_types",
        "//isekai/host/falcon:falcon_utils",
    ],
)

cc_test(
    name = "ack_coalescing_engine_test",
    srcs = ["ack_coalescing_engine_test.cc"],
    deps = [
        ":ack_coalescing_engine",
        "//isekai/common:config_cc_proto",
        "//isekai/common:constants",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_bitmap",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_protocol_ack_coalescing_engine",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "event_response_format_adapter_test",
    srcs = ["event_response_format_adapter_test.cc"],
    deps = [
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "//isekai/host/falcon:event_response_format_adapter",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_model",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_utils",
    hdrs = ["falcon_utils.h"],
    deps = [
        ":falcon_types",
        "//isekai/common:status_util",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_types",
        "//isekai/host/falcon:falcon_utils",
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "falcon_types",
    hdrs = ["falcon_types.h"],
    deps = [
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_types",
    ],
)

cc_library(
    name = "base_connection_scheduler",
    srcs = ["base_connection_scheduler.cc"],
    hdrs = ["base_connection_scheduler.h"],
    deps = ["//isekai/host/falcon:falcon_protocol_base_connection_scheduler"],
)

cc_library(
    name = "rx_packet_buffer_fetch_arbiter",
    srcs = ["rx_packet_buffer_fetch_arbiter.cc"],
    hdrs = ["rx_packet_buffer_fetch_arbiter.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_types",
        ":sram_dram_reorder_engine",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_inter_host_rx_round_robin_policy",
        "//isekai/host/rnic:memory_interface",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "sram_dram_reorder_engine",
    srcs = ["sram_dram_reorder_engine.cc"],
    hdrs = ["sram_dram_reorder_engine.h"],
    deps = [
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "falcon_component_interfaces",
    hdrs = ["falcon_component_interfaces.h"],
    deps = [
        ":sram_dram_reorder_engine",
        "//isekai/common:model_interfaces",
        "//isekai/host/rnic:memory_interface",
        "@com_google_absl//absl/status",
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
        "//isekai/host/falcon:packet_metadata_transformer",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "packet_metadata_transformer_test",
    srcs = ["packet_metadata_transformer_test.cc"],
    deps = [
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_component_interfaces",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "sram_dram_reorder_engine_test",
    srcs = ["sram_dram_reorder_engine_test.cc"],
    deps = [
        ":falcon_model",
        ":falcon_types",
        ":sram_dram_reorder_engine",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/host/falcon",
        "//isekai/host/falcon:falcon_connection_state",
        "//isekai/host/falcon:falcon_testing_helpers",
        "//isekai/host/falcon:falcon_types",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)
