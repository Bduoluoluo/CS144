#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route (const uint32_t route_prefix, const uint8_t prefix_length, const optional<Address> next_hop, const size_t interface_num) {
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  // Your code here.
  this->route_table_[{route_prefix, prefix_length}] = {next_hop, interface_num};
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route () {
  // Your code here.
  for (auto interface : this->_interfaces) {
    while ((*interface).datagrams_received().size()) {
      InternetDatagram dgram = (*interface).datagrams_received().front();
      (*interface).datagrams_received().pop();

      if (dgram.header.ttl == 0 || (-- dgram.header.ttl) == 0)
        continue;
      dgram.header.compute_checksum(); // ttl has changed
      
      uint32_t ip = dgram.header.dst;
      pair<std::optional<Address>, size_t> choice{};
      bool is_chosen = false;
      for (const auto& [key, value] : this->route_table_) {
        const auto& [route_prefix, prefix_length] = key;

        if ((prefix_length == 0 || ip >> (32 - prefix_length) == route_prefix >> (32 - prefix_length)) && (is_chosen == false || prefix_length > choice.second)) {
          choice = value;
          is_chosen = true;
        }
      }

      if (is_chosen == true) {
        const auto& [next_hop, interface_num] = choice;
        (*this->interface(interface_num)).send_datagram(dgram, next_hop.has_value() ? next_hop.value() : Address::from_ipv4_numeric(ip));
      }
    }
  }
}
