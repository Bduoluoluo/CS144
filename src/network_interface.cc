#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram (const InternetDatagram& dgram, const Address& next_hop) {
  // Your code here.
  uint32_t ip = next_hop.ipv4_numeric();
  if (this->ip_to_ethernet_.count(ip) && this->ip_to_ethernet_[ip].second) {
    transmit(get_ipv4_ethernet(this->ip_to_ethernet_[ip].first, serialize(dgram)));
  } else {
    this->waiting_dgram_[ip].push_back(dgram);
    if (this->last_arp_time_[ip] == 0) {
      this->last_arp_time_[ip] = ARP_TIME;
      transmit(get_request_ethernet(serialize(get_arp_request(ip))));
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame (const EthernetFrame& frame) {
  // Your code here.
  if (frame.header.dst != this->ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST)
    return;

  if (frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram{};
    if (!parse(dgram, frame.payload))
      return;
    this->datagrams_received_.push(dgram);
  } else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp{};
    if (!parse(arp, frame.payload))
      return;
    
    this->ip_to_ethernet_[arp.sender_ip_address] = {arp.sender_ethernet_address, ETHERNET_TIME};
    if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == this->ip_address_.ipv4_numeric())
      transmit(get_reply_ethernet(arp.sender_ethernet_address, serialize(get_arp_reply(arp.sender_ethernet_address, arp.sender_ip_address))));

    if (this->waiting_dgram_.count(arp.sender_ip_address) && this->waiting_dgram_[arp.sender_ip_address].size()) {
      for (const InternetDatagram& dgram : this->waiting_dgram_[arp.sender_ip_address])
        transmit(get_ipv4_ethernet(arp.sender_ethernet_address, serialize(dgram)));
      this->waiting_dgram_[arp.sender_ip_address].clear();
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick (const size_t ms_since_last_tick) {
  // Your code here.
  for (auto& elem : this->ip_to_ethernet_)
    elem.second.second -= min(elem.second.second, ms_since_last_tick);
  for (auto& elem : this->last_arp_time_)
    elem.second -= min(elem.second, ms_since_last_tick);
}


EthernetFrame NetworkInterface::get_ipv4_ethernet (const EthernetAddress& dst, const std::vector<std::string>& payload) const {
  return EthernetFrame {
    .header = EthernetHeader {
      .dst = dst,
      .src = this->ethernet_address_,
      .type = EthernetHeader::TYPE_IPv4
    },
    .payload = payload
  };
}

EthernetFrame NetworkInterface::get_request_ethernet (const std::vector<std::string>& payload) const {
  return EthernetFrame {
    .header = EthernetHeader {
      .dst = ETHERNET_BROADCAST,
      .src = this->ethernet_address_,
      .type = EthernetHeader::TYPE_ARP
    },
    .payload = payload
  };
}

EthernetFrame NetworkInterface::get_reply_ethernet (const EthernetAddress& dst, const std::vector<std::string>& payload) const {
  return EthernetFrame {
    .header = EthernetHeader {
      .dst = dst,
      .src = this->ethernet_address_,
      .type = EthernetHeader::TYPE_ARP
    },
    .payload = payload
  };
}

ARPMessage NetworkInterface::get_arp_request (const uint32_t target_ip) const {
  return ARPMessage {
    .opcode = ARPMessage::OPCODE_REQUEST,
    .sender_ethernet_address = this->ethernet_address_,
    .sender_ip_address = this->ip_address_.ipv4_numeric(),
    .target_ip_address = target_ip
  };
}

ARPMessage NetworkInterface::get_arp_reply (const EthernetAddress& target_ethernet, const uint32_t target_ip) const {
  return ARPMessage {
    .opcode = ARPMessage::OPCODE_REPLY,
    .sender_ethernet_address = this->ethernet_address_,
    .sender_ip_address = this->ip_address_.ipv4_numeric(),
    .target_ethernet_address = target_ethernet,
    .target_ip_address = target_ip
  };
}