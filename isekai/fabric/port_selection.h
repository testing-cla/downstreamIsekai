#ifndef ISEKAI_FABRIC_PORT_SELECTION_H_
#define ISEKAI_FABRIC_PORT_SELECTION_H_
#include <cstdint>
#include <memory>

#include "isekai/common/config.pb.h"
#include "isekai/fabric/network_routing_table.h"
#include "isekai/fabric/packet_util.h"

namespace isekai {

// The abstract class for output port selection for packet.
class OutputPortSelection {
 public:
  virtual ~OutputPortSelection() {}
  // Pure virtual function for output port selection for packet. The function
  // could be override to uses different schemes, e.g. weighted cost multipath
  // selection, random selection, round robin selection.
  virtual uint32_t SelectOutputPort(const TableOutputOptions& output_options,
                                    omnetpp::cMessage* packet, int stage) = 0;
};

// The class for output port selection with WCMP (weighted cost multipath
// selection) for packet.
class WcmpOutputPortSelection : public OutputPortSelection {
 public:
  // Selected the output port from output_options for the packet. The function
  // uses weighted cost multipath selection.
  uint32_t SelectOutputPort(const TableOutputOptions& output_options,
                            omnetpp::cMessage* packet, int stage) override;
};

// The factory for generating object for output port selection for packet.
class OutputPortSelectionFactory {
 public:
  // Returns object for output port selection for packet according to
  // port_selection_scheme.
  static std::unique_ptr<OutputPortSelection> GetOutputPortSelectionScheme(
      RouterConfigProfile::PortSelectionPolicy port_selection_scheme);
};

}  // namespace isekai

#endif  // ISEKAI_FABRIC_PORT_SELECTION_H_
