#ifndef ISEKAI_HOST_FALCON_GEN2_FALCON_TYPES_H_
#define ISEKAI_HOST_FALCON_GEN2_FALCON_TYPES_H_

#include <cstdint>
#include <utility>

#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_types.h"

namespace isekai {

// Holds the packet metadata that is used by the DRAM-Backed modules to uniquely
// identify work (fetch or evict to DRAM).
struct BufferFetchWorkId {
  uint32_t cid;
  uint32_t rsn;
  falcon::PacketType type;
  BufferFetchWorkId(uint32_t cid, uint32_t rsn, falcon::PacketType type)
      : cid(cid), rsn(rsn), type(type) {}
};

struct Gen2OutstandingPacketContext : OutstandingPacketContext {
  // For multipath connections, flow_id represents the flow ID that the
  // outstanding packet was transmitted on. This field is used for ACK unrolling
  // in Gen2.
  uint8_t flow_id = 0;

  Gen2OutstandingPacketContext() {}
  Gen2OutstandingPacketContext(uint32_t rsn_arg, falcon::PacketType type_arg,
                               uint8_t flow_id)
      : OutstandingPacketContext(rsn_arg, type_arg), flow_id(flow_id) {}
};

struct Gen2AckCoalescingKey : AckCoalescingKey {
  // For multipath connections, flow_id represents the flow ID that the
  // outstanding packet was transmitted on. This field is used to enable ACK
  // coalescing to happen at the <connection, flow> pair in Gen2.
  uint8_t flow_id = 0;

  Gen2AckCoalescingKey(uint32_t scid_arg, uint8_t flow_id)
      : AckCoalescingKey(scid_arg), flow_id(flow_id) {}
  Gen2AckCoalescingKey() {}
  template <typename H>
  friend H AbslHashValue(H state, const Gen2AckCoalescingKey& value) {
    state = H::combine(std::move(state), value.scid, value.flow_id);
    return state;
  }
  // Two variables are equal if they share the same scid and flow_id.
  inline bool operator==(const Gen2AckCoalescingKey& other) const {
    return other.scid == scid && other.flow_id == flow_id;
  }
};

typedef Gen2AckCoalescingKey Gen2RueKey;

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_FALCON_TYPES_H_
