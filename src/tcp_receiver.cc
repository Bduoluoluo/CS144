#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive (TCPSenderMessage message) {
  // Your code here.
  if (message.RST == true) {
    this->reader().set_error();
    return;
  }
  
  if (message.SYN) {
    this->ISN_ = true;
    this->zero_pointer_ = message.seqno;
  } else if (this->ISN_ == false)
    return;

  this->reassembler_.insert(message.seqno.unwrap(this->zero_pointer_, this->writer().bytes_pushed()) - !message.SYN, message.payload, message.FIN);
  this->rev_base_ = this->writer().bytes_pushed() + this->ISN_ + this->writer().is_closed();
}

TCPReceiverMessage TCPReceiver::send () const {
  // Your code here.
  TCPReceiverMessage message;
  if (this->ISN_ == true)
    message.ackno = Wrap32::wrap(this->rev_base_, this->zero_pointer_);
  message.window_size = min((uint64_t) UINT16_MAX, this->writer().available_capacity());
  message.RST = this->reader().has_error();
  return message;
}
