package(default_visibility = ["//isekai:__subpackages__"])

licenses(["notice"])

cc_library(
    name = "gating_variable",
    hdrs = ["gating_variable.h"],
    deps = ["@com_google_glog//:glog"],
)

cc_test(
    name = "gating_variable_test",
    srcs = ["gating_variable_test.cc"],
    deps = [
        ":gating_variable",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_counters",
    srcs = ["falcon_counters.cc"],
    hdrs = [
        "falcon_counters.h",
    ],
)

cc_test(
    name = "falcon_counters_test",
    srcs = ["falcon_counters_test.cc"],
    deps = [
        ":falcon_model",
        ":falcon_utils",
        "//isekai/common:default_config_generator",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_histograms",
    srcs = ["falcon_histograms.cc"],
    hdrs = [
        "falcon_histograms.h",
    ],
    deps = [
        ":falcon_counters",
        ":falcon_histograms_cc_proto",
        "//isekai/common:tdigest",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "falcon_histograms_test",
    srcs = ["falcon_histograms_test.cc"],
    deps = [
        ":falcon_histograms",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stats_manager",
    srcs = ["stats_manager.cc"],
    hdrs = [
        "stats_manager.h",
    ],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_counters",
        ":falcon_histograms",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "falcon_model",
    srcs = ["falcon_model.cc"],
    hdrs = [
        "falcon_model.h",
    ],
    deps = [
        ":falcon_component_factories",
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_histograms",
        ":falcon_inter_host_rx_scheduler",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_resource_credits",
    srcs = [
        "falcon_resource_credits.cc",
    ],
    hdrs = [
        "falcon_resource_credits.h",
    ],
    deps = ["//isekai/common:config_cc_proto"],
)

cc_test(
    name = "falcon_resource_credits_test",
    srcs = ["falcon_resource_credits_test.cc"],
    deps = [
        ":falcon_resource_credits",
        "//isekai/common:config_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "falcon_component_interfaces",
    hdrs = [
        "falcon_component_interfaces.h",
    ],
    deps = [
        ":falcon_bitmap",
        ":falcon_connection_state",
        ":falcon_counters",
        ":falcon_histograms",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_resource_credits",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "falcon_protocol_resource_manager",
    srcs = ["falcon_protocol_resource_manager.cc"],
    hdrs = ["falcon_protocol_resource_manager.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_resource_credits",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_ack_coalescing_engine",
    srcs = ["falcon_protocol_ack_coalescing_engine.cc"],
    hdrs = ["falcon_protocol_ack_coalescing_engine.h"],
    deps = [
        ":falcon_bitmap",
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_protocol_ack_nack_scheduler",
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:falcon_types",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "falcon_protocol_ack_coalescing_engine_test",
    srcs = ["falcon_protocol_ack_coalescing_engine_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_ack_coalescing_engine",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_connection_state",
    srcs = ["falcon_connection_state.cc"],
    hdrs = ["falcon_connection_state.h"],
    deps = [
        ":falcon_bitmap",
        ":falcon_histograms",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_types",
        ":falcon_utils",
        ":gating_variable",
        "//isekai/common:constants",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "falcon_protocol_rate_update_engine",
    srcs = ["falcon_protocol_rate_update_engine.cc"],
    hdrs = ["falcon_protocol_rate_update_engine.h"],
    deps = [
        ":event_response_format_adapter",
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_rate_update_engine_adapter",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "//isekai/host/falcon/rue:latency_generator",
        "//isekai/host/falcon/rue/algorithm",
        "//isekai/host/falcon/rue/algorithm:config_cc_proto",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_ack_nack_scheduler",
    srcs = ["falcon_protocol_ack_nack_scheduler.cc"],
    hdrs = [
        "falcon_protocol_ack_nack_scheduler.h",
    ],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_histograms",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_types",
    hdrs = ["falcon_types.h"],
    deps = [
        "//isekai/common:constants",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "@com_google_absl//absl/time",
    ],
)

proto_library(
    name = "falcon_histograms_proto",
    srcs = ["falcon_histograms.proto"],
    deps = ["//isekai/common:tdigest_proto"],
)

cc_proto_library(
    name = "falcon_histograms_cc_proto",
    deps = [":falcon_histograms_proto"],
)

cc_library(
    name = "falcon_protocol_connection_state_manager",
    srcs = ["falcon_protocol_connection_state_manager.cc"],
    hdrs = ["falcon_protocol_connection_state_manager.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:bind_front",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "falcon_component_factories",
    hdrs = ["falcon_component_factories.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_inter_host_rx_scheduler",
        ":falcon_protocol_ack_coalescing_engine",
        ":falcon_protocol_ack_nack_scheduler",
        ":falcon_protocol_admission_control_manager",
        ":falcon_protocol_buffer_reorder_engine",
        ":falcon_protocol_connection_state_manager",
        ":falcon_protocol_fast_connection_scheduler",
        ":falcon_protocol_fast_retransmission_scheduler",
        ":falcon_protocol_packet_reliability_manager",
        ":falcon_protocol_packet_type_based_connection_scheduler",
        ":falcon_protocol_rate_update_engine",
        ":falcon_protocol_resource_manager",
        ":falcon_protocol_retransmission_scheduler",
        ":falcon_protocol_round_robin_arbiter",
        ":packet_metadata_transformer",
        ":stats_manager",
        "//isekai/host/falcon/gen2:ack_coalescing_engine",
        "//isekai/host/falcon/gen2:ack_nack_scheduler",
        "//isekai/host/falcon/gen2:admission_control_manager",
        "//isekai/host/falcon/gen2:arbiter",
        "//isekai/host/falcon/gen2:connection_scheduler",
        "//isekai/host/falcon/gen2:connection_state_manager",
        "//isekai/host/falcon/gen2:falcon_component_interfaces",
        "//isekai/host/falcon/gen2:inter_host_rx_scheduler",
        "//isekai/host/falcon/gen2:packet_metadata_transformer",
        "//isekai/host/falcon/gen2:rate_update_engine",
        "//isekai/host/falcon/gen2:reliability_manager",
        "//isekai/host/falcon/gen2:reorder_engine",
        "//isekai/host/falcon/gen2:resource_manager",
        "//isekai/host/falcon/gen2:retransmission_scheduler",
        "//isekai/host/falcon/gen2:rx_packet_buffer_fetch_arbiter",
        "//isekai/host/falcon/gen2:sram_dram_reorder_engine",
        "//isekai/host/falcon/gen2:stats_manager",
        "//isekai/host/falcon/gen3:ack_coalescing_engine",
        "//isekai/host/falcon/gen3:ack_nack_scheduler",
        "//isekai/host/falcon/gen3:admission_control_manager",
        "//isekai/host/falcon/gen3:arbiter",
        "//isekai/host/falcon/gen3:connection_scheduler",
        "//isekai/host/falcon/gen3:connection_state_manager",
        "//isekai/host/falcon/gen3:inter_host_rx_scheduler",
        "//isekai/host/falcon/gen3:packet_metadata_transformer",
        "//isekai/host/falcon/gen3:rate_update_engine",
        "//isekai/host/falcon/gen3:reliability_manager",
        "//isekai/host/falcon/gen3:reorder_engine",
        "//isekai/host/falcon/gen3:resource_manager",
        "//isekai/host/falcon/gen3:retransmission_scheduler",
        "//isekai/host/falcon/gen3:stats_manager",
        "//isekai/host/rnic:memory_interface",
        "@com_google_absl//absl/log",
    ],
)

cc_library(
    name = "falcon_bitmap",
    srcs = ["falcon_bitmap.cc"],
    hdrs = ["falcon_bitmap.h"],
    deps = [
        "//isekai/common:constants",
        "@com_google_absl//absl/log:check",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_connection_scheduler_types",
    srcs = ["falcon_protocol_connection_scheduler_types.cc"],
    hdrs = ["falcon_protocol_connection_scheduler_types.h"],
    deps = [
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "weighted_round_robin_policy",
    hdrs = ["weighted_round_robin_policy.h"],
    deps = [
        ":falcon_protocol_connection_scheduler_types",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_inter_connection_round_robin_policy",
    srcs = ["falcon_protocol_inter_connection_round_robin_policy.cc"],
    hdrs = ["falcon_protocol_inter_connection_round_robin_policy.h"],
    deps = [
        ":falcon_component_interfaces",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/meta:type_traits",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_intra_connection_round_robin_policy",
    srcs = ["falcon_protocol_intra_connection_round_robin_policy.cc"],
    hdrs = ["falcon_protocol_intra_connection_round_robin_policy.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_protocol_connection_scheduler_types",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_inter_host_rx_scheduler",
    srcs = ["falcon_inter_host_rx_scheduler.cc"],
    hdrs = [
        "falcon_inter_host_rx_scheduler.h",
    ],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_inter_host_rx_round_robin_policy",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/functional:any_invocable",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_inter_host_rx_round_robin_policy",
    srcs = ["falcon_inter_host_rx_round_robin_policy.cc"],
    hdrs = ["falcon_inter_host_rx_round_robin_policy.h"],
    deps = [
        ":falcon_component_interfaces",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/meta:type_traits",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_admission_control_manager",
    srcs = ["falcon_protocol_admission_control_manager.cc"],
    hdrs = ["falcon_protocol_admission_control_manager.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_histograms",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/common:token_bucket",
        "//isekai/host/falcon",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "falcon_protocol_packet_reliability_manager",
    srcs = ["falcon_protocol_packet_reliability_manager.cc"],
    hdrs = ["falcon_protocol_packet_reliability_manager.h"],
    deps = [
        ":falcon_bitmap",
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:net_address",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/common:token_bucket",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:falcon_types",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_prod",
    ],
)

cc_library(
    name = "falcon_component_test_infrastructure",
    testonly = 1,
    srcs = ["falcon_component_test_infrastructure.cc"],
    hdrs = ["falcon_component_test_infrastructure.h"],
    deps = [
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_resource_manager",
        ":falcon_resource_credits",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:net_address",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/fabric:constants",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:falcon_model",
        "//isekai/host/falcon/gen3:falcon_model",
        "//isekai/host/falcon/gen3:falcon_utils",
        "//isekai/host/rdma:rdma_base_model",
        "//isekai/host/rdma:rdma_component_interfaces",
        "//isekai/host/rdma:rdma_falcon_model",
        "//isekai/host/rnic:connection_manager",
        "//isekai/host/rnic:traffic_shaper_model",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest",
        "@omnetpp",
    ],
)

cc_test(
    name = "falcon_component_test_infrastructure_test",
    srcs = ["falcon_component_test_infrastructure_test.cc"],
    deps = [
        ":falcon_component_test_infrastructure",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:net_address",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_protocol_buffer_reorder_engine",
    srcs = ["falcon_protocol_buffer_reorder_engine.cc"],
    hdrs = ["falcon_protocol_buffer_reorder_engine.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_utils",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_retransmission_scheduler",
    srcs = ["falcon_protocol_retransmission_scheduler.cc"],
    hdrs = ["falcon_protocol_retransmission_scheduler.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_protocol_inter_connection_round_robin_policy",
        ":falcon_protocol_intra_connection_round_robin_policy",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_round_robin_arbiter",
    srcs = ["falcon_protocol_round_robin_arbiter.cc"],
    hdrs = ["falcon_protocol_round_robin_arbiter.h"],
    deps = [
        ":falcon_component_interfaces",
        "//isekai/common:config_cc_proto",
        "//isekai/common:environment",
        "//isekai/common:status_util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "falcon_utils",
    srcs = ["falcon_utils.cc"],
    hdrs = ["falcon_utils.h"],
    deps = [
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_types",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
    ],
)

cc_library(
    name = "falcon_protocol_fast_connection_scheduler",
    srcs = ["falcon_protocol_fast_connection_scheduler.cc"],
    hdrs = ["falcon_protocol_fast_connection_scheduler.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_histograms",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_types",
        ":falcon_utils",
        ":weighted_round_robin_policy",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "falcon_protocol_fast_connection_scheduler_test",
    srcs = ["falcon_protocol_fast_connection_scheduler_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_protocol_fast_retransmission_scheduler",
    srcs = ["falcon_protocol_fast_retransmission_scheduler.cc"],
    hdrs = ["falcon_protocol_fast_retransmission_scheduler.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_histograms",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_types",
        ":falcon_utils",
        ":weighted_round_robin_policy",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/time",
    ],
)

cc_test(
    name = "falcon_protocol_fast_retransmission_scheduler_test",
    srcs = ["falcon_protocol_fast_retransmission_scheduler_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon_protocol_base_connection_scheduler",
    srcs = ["falcon_protocol_base_connection_scheduler.cc"],
    hdrs = ["falcon_protocol_base_connection_scheduler.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_intra_packet_type_round_robin_policy",
    srcs = ["falcon_protocol_intra_packet_type_round_robin_policy.cc"],
    hdrs = ["falcon_protocol_intra_packet_type_round_robin_policy.h"],
    deps = [
        ":falcon_component_interfaces",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "falcon_protocol_inter_packet_type_weighted_round_robin_policy",
    srcs = ["falcon_protocol_inter_packet_type_weighted_round_robin_policy.cc"],
    hdrs = ["falcon_protocol_inter_packet_type_weighted_round_robin_policy.h"],
    deps = [":falcon_component_interfaces"],
)

cc_library(
    name = "falcon_protocol_packet_type_based_connection_scheduler",
    srcs = ["falcon_protocol_packet_type_based_connection_scheduler.cc"],
    hdrs = ["falcon_protocol_packet_type_based_connection_scheduler.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_protocol_base_connection_scheduler",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_protocol_inter_packet_type_round_robin_policy",
        ":falcon_protocol_inter_packet_type_weighted_round_robin_policy",
        ":falcon_protocol_intra_packet_type_round_robin_policy",
        ":falcon_utils",
        "//isekai/common:model_interfaces",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "falcon_protocol_inter_packet_type_round_robin_policy",
    srcs = ["falcon_protocol_inter_packet_type_round_robin_policy.cc"],
    hdrs = ["falcon_protocol_inter_packet_type_round_robin_policy.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_protocol_connection_scheduler_types",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "falcon_testing_helpers",
    testonly = 1,
    srcs = ["falcon_testing_helpers.cc"],
    hdrs = ["falcon_testing_helpers.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_ack_coalescing_engine",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_protocol_packet_reliability_manager",
        ":falcon_protocol_rate_update_engine",
        ":falcon_protocol_resource_manager",
        ":falcon_rate_update_engine_adapter",
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:ack_coalescing_engine",
        "//isekai/host/falcon/gen2:falcon_model",
        "//isekai/host/falcon/gen2:falcon_types",
        "//isekai/host/falcon/gen2:falcon_utils",
        "//isekai/host/falcon/gen3:falcon_model",
        "//isekai/host/falcon/gen3:falcon_utils",
        "//isekai/host/falcon/rue",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "falcon_rate_update_engine_adapter",
    srcs = ["falcon_rate_update_engine_adapter.cc"],
    hdrs = ["falcon_rate_update_engine_adapter.h"],
    deps = [
        ":falcon_connection_state",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "event_response_format_adapter",
    srcs = ["event_response_format_adapter.cc"],
    hdrs = ["event_response_format_adapter.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_rate_update_engine_adapter",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:falcon_types",
        "//isekai/host/falcon/gen2:reliability_manager",
        "//isekai/host/falcon/rue",
        "//isekai/host/falcon/rue/algorithm",
        "//isekai/host/falcon/rue/algorithm:config_cc_proto",
        "//isekai/host/falcon/rue/algorithm:dram_state_manager",
        "//isekai/host/falcon/rue/algorithm:stateful_algorithm",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
    ],
)

cc_library(
    name = "packet_metadata_transformer",
    srcs = ["packet_metadata_transformer.cc"],
    hdrs = ["packet_metadata_transformer.h"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        "//isekai/common:config_cc_proto",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "@com_google_absl//absl/log:check",
    ],
)

cc_test(
    name = "packet_metadata_transformer_test",
    srcs = ["packet_metadata_transformer_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "event_response_format_adapter_test",
    srcs = ["event_response_format_adapter_test.cc"],
    deps = [
        ":event_response_format_adapter",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_rate_update_engine_adapter_test",
    srcs = ["falcon_rate_update_engine_adapter_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_rate_update_engine",
        ":falcon_rate_update_engine_adapter",
        ":falcon_testing_helpers",
        ":falcon_utils",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/log:check",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_buffer_reorder_engine_test",
    srcs = ["falcon_protocol_buffer_reorder_engine_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_counters",
        ":falcon_model",
        ":falcon_protocol_buffer_reorder_engine",
        ":falcon_testing_helpers",
        ":falcon_utils",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:environment",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:reorder_engine",
        "//isekai/host/falcon/gen3:reorder_engine",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_connection_state_test",
    srcs = ["falcon_connection_state_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen3:falcon_utils",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_connection_state_manager_test",
    srcs = ["falcon_protocol_connection_state_manager_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_protocol_connection_state_manager",
        ":falcon_testing_helpers",
        ":falcon_utils",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:simple_environment",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_resource_manager_test",
    srcs = ["falcon_protocol_resource_manager_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_resource_manager",
        ":falcon_resource_credits",
        ":falcon_testing_helpers",
        ":falcon_types",
        ":falcon_utils",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/fabric:constants",
        "//isekai/host/falcon",
        "//isekai/host/falcon/gen2:resource_manager",
        "//isekai/host/falcon/gen3:resource_manager",
        "//isekai/host/falcon/rue",
        "//isekai/host/rdma:rdma_falcon_model",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "falcon_protocol_packet_reliability_manager_test",
    srcs = ["falcon_protocol_packet_reliability_manager_test.cc"],
    deps = [
        ":falcon_bitmap",
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_protocol_packet_reliability_manager",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:constants",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/container:btree",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_rate_update_engine_test",
    srcs = ["falcon_protocol_rate_update_engine_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_counters",
        ":falcon_model",
        ":falcon_protocol_rate_update_engine",
        ":falcon_testing_helpers",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "//isekai/host/falcon/rue",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_inter_connection_round_robin_policy_test",
    srcs = ["falcon_protocol_inter_connection_round_robin_policy_test.cc"],
    deps = [
        ":falcon_protocol_inter_connection_round_robin_policy",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_intra_connection_round_robin_policy_test",
    srcs = ["falcon_protocol_intra_connection_round_robin_policy_test.cc"],
    deps = [
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_protocol_intra_connection_round_robin_policy",
        "@com_google_absl//absl/status:statusor",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_connection_scheduler_types_test",
    srcs = ["falcon_protocol_connection_scheduler_types_test.cc"],
    deps = [
        ":falcon_protocol_connection_scheduler_types",
        "@com_google_absl//absl/container:btree",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_ack_nack_scheduler_test",
    srcs = ["falcon_protocol_ack_nack_scheduler_test.cc"],
    deps = [
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_ack_nack_scheduler",
        ":falcon_testing_helpers",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/log:check",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_admission_control_manager_test",
    srcs = ["falcon_protocol_admission_control_manager_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_admission_control_manager",
        ":falcon_testing_helpers",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "falcon_bitmap_test",
    srcs = ["falcon_bitmap_test.cc"],
    deps = [
        ":falcon_bitmap",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_component_test",
    srcs = ["falcon_component_test.cc"],
    deps = [
        ":falcon_component_test_infrastructure",
        ":falcon_connection_state",
        ":falcon_counters",
        ":falcon_model",
        ":falcon_protocol_resource_manager",
        ":falcon_resource_credits",
        ":falcon_utils",
        "//isekai/common:common_util",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_scheduler_queue_length_test",
    srcs = ["falcon_scheduler_queue_length_test.cc"],
    deps = [
        ":falcon_component_test_infrastructure",
        ":falcon_counters",
        ":falcon_model",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/common:status_util",
        "//isekai/host/falcon",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_retransmission_scheduler_test",
    srcs = ["falcon_protocol_retransmission_scheduler_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_testing_helpers",
        ":falcon_types",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_round_robin_arbiter_test",
    srcs = ["falcon_protocol_round_robin_arbiter_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_round_robin_arbiter",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_intra_packet_type_round_robin_policy_test",
    srcs = ["falcon_protocol_intra_packet_type_round_robin_policy_test.cc"],
    deps = [
        ":falcon_protocol_intra_packet_type_round_robin_policy",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_inter_packet_type_weighted_round_robin_policy_test",
    srcs = ["falcon_protocol_inter_packet_type_weighted_round_robin_policy_test.cc"],
    deps = [
        ":falcon_protocol_inter_packet_type_weighted_round_robin_policy",
        "//isekai/host/falcon",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_packet_type_based_connection_scheduler_test",
    srcs = ["falcon_protocol_packet_type_based_connection_scheduler_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_packet_type_based_connection_scheduler",
        ":falcon_testing_helpers",
        ":falcon_utils",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:model_interfaces",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_protocol_inter_packet_type_round_robin_policy_test",
    srcs = ["falcon_protocol_inter_packet_type_round_robin_policy_test.cc"],
    deps = [
        ":falcon_protocol_connection_scheduler_types",
        ":falcon_protocol_inter_packet_type_round_robin_policy",
        "//isekai/common:status_util",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_inter_host_rx_round_robin_policy_test",
    srcs = ["falcon_inter_host_rx_round_robin_policy_test.cc"],
    deps = [
        ":falcon_inter_host_rx_round_robin_policy",
        "//isekai/common:status_util",
        "@com_google_absl//absl/status",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "falcon_inter_host_rx_scheduler_test",
    srcs = ["falcon_inter_host_rx_scheduler_test.cc"],
    deps = [
        ":falcon_model",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:simple_environment",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "gen1_ack_coalescing_engine_test",
    srcs = ["gen1_ack_coalescing_engine_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_testing_helpers",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/time",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "gen1_reliability_manager_test",
    srcs = ["gen1_reliability_manager_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_testing_helpers",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:packet",
        "//isekai/host/falcon",
        "@com_google_cel_cpp//internal:testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "gen1_rate_update_engine_test",
    srcs = ["gen1_rate_update_engine_test.cc"],
    deps = [
        ":falcon_component_interfaces",
        ":falcon_connection_state",
        ":falcon_model",
        ":falcon_protocol_rate_update_engine",
        ":falcon_testing_helpers",
        "//isekai/common:config_cc_proto",
        "//isekai/common:default_config_generator",
        "//isekai/common:simple_environment",
        "//isekai/host/falcon/rue",
        "//isekai/host/rnic:connection_manager",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "falcon",
    hdrs = ["falcon.h"],
)
