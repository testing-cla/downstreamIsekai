#include "isekai/common/ipv6_trie.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#include <string>

#include "absl/numeric/int128.h"
#include "glog/logging.h"

namespace isekai {

absl::uint128 HexIPv6AddressToBinaryFormat(
    const std::string& hex_ipv6_address) {
  in6_addr ipv6_addr;
  memcpy(&ipv6_addr, hex_ipv6_address.data(), sizeof(ipv6_addr));
  return absl::MakeUint128(
      static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[0])) << 32 |
          static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[1])),
      static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[2])) << 32 |
          static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[3])));
}

std::string HexIPv6AddressToString(const std::string& hex_ipv6_address) {
  static char addr_buf[INET6_ADDRSTRLEN];

  in6_addr ipv6_addr;
  memcpy(&ipv6_addr, hex_ipv6_address.data(), sizeof(ipv6_addr));
  inet_ntop(AF_INET6, &ipv6_addr, addr_buf, INET6_ADDRSTRLEN);

  return addr_buf;
}

absl::uint128 StringToIPAddressOrDie(const std::string& str) {
  in6_addr ipv6_addr;
  if (inet_pton(AF_INET6, str.c_str(), &ipv6_addr) > 0) {
    return absl::MakeUint128(
        static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[0])) << 32 |
            static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[1])),
        static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[2])) << 32 |
            static_cast<uint64_t>(ntohl(ipv6_addr.s6_addr32[3])));
  }
  LOG(FATAL) << "invalid ip address: " << str;
}

}  // namespace isekai
