#include "isekai/common/packet.h"

#include <cstdint>
#include <vector>

#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "isekai/common/net_address.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"

namespace isekai {
namespace {

TEST(PacketTest, DebugString) {
  Packet packet = {};

  packet.metadata.traffic_class = 1;
  packet.metadata.timestamp = 2;
  packet.metadata.timing_wheel_timestamp = absl::Nanoseconds(100);
  ASSERT_OK_THEN_ASSIGN(packet.metadata.destination_ip_address,
                        Ipv6Address::OfString("2001:db8:85a2::1"));
  packet.metadata.source_bifurcation_id = 0;
  packet.metadata.destination_bifurcation_id = 1;
  packet.metadata.scid = 8;

  packet.falcon.protocol_type = falcon::ProtocolType::kRdma;
  packet.packet_type = falcon::PacketType::kPullRequest;
  packet.falcon.ack_req = false;
  packet.falcon.dest_cid = 4;
  packet.falcon.rrbpsn = 5;
  packet.falcon.rdbpsn = 5;
  packet.falcon.psn = 6;
  packet.falcon.rsn = 7;

  packet.rdma.opcode = Packet::Rdma::Opcode::kReadRequest;
  packet.rdma.inline_payload_length = 9;
  packet.rdma.request_length = 10;
  packet.rdma.sgl = {20, 50, 70};
  packet.rdma.dest_qp_id = 11;
  packet.rdma.rsn = 12;

  // clang-format off
  EXPECT_EQ(packet.DebugString(),
            R"({
  metadata {
    traffic_class: 1
    timestamp: 2
    timing_wheel_timestamp: 100ns
    destination_ip_address: 2001:db8:85a2::1
    source_bifurcation_id: 0
    destination_bifurcation_id: 1
    scid: 8
  }
  falcon {
    protocol_type: kRdma
    packet_type: kPullRequest
    ack_req: false
    dest_cid: 4
    rrbpsn: 5
    rdbpsn: 5
    psn: 6
    rsn: 7
  }
  rdma {
    opcode: kReadRequest
    inline_payload_length: 9
    request_length: 10
    sgl: [20, 50, 70]
    dest_qp_id: 11
    rsn: 12
  }
}
)");
  // clang-format on
}

}  // namespace
}  // namespace isekai
