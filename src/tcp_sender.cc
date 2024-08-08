#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <iostream>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight () const {
  // Your code here.
  return this->reader().bytes_popped() + this->SYN_ + this->FIN_ - this->last_seqno_;
}

uint64_t TCPSender::consecutive_retransmissions () const {
  // Your code here.
  return this->consecutive_retransmissions_count_;
}

void TCPSender::push (const TransmitFunction& transmit) {
  // Your code here.
  TCPSenderMessage msg;
  uint64_t seqno = this->reader().bytes_popped() + this->SYN_ + this->FIN_;
  uint64_t window_size = this->window_size_ - seqno;

  msg.seqno = Wrap32::wrap(seqno, this->isn_);
  msg.SYN = false;
  msg.FIN = false;  
  msg.RST = false;
  msg.payload.clear();

  if (this->input_.has_error()) {
    if (this->RST_ == true) return;
    msg.RST = true;
    this->RST_ = true;
  } else {
    if (this->SYN_ == false) {
      msg.SYN = true;
      this->SYN_ = true;
    }
    msg.payload = this->reader().peek().substr(0, min(window_size, TCPConfig::MAX_PAYLOAD_SIZE));
    this->input_.reader().pop(msg.payload.size());
    window_size -= msg.payload.size();
    if (this->FIN_ == false && window_size > 0 && this->reader().is_finished()) {
      msg.FIN = true;
      this->FIN_ = true;
    }
  }
  
  if (msg.sequence_length() > 0 || msg.RST == true) {
    transmit(msg);
    this->outstanding_data_.push({msg, seqno});
    if (this->timer_flag_ == false) {
      this->timer_flag_ = true;
      this->timer_ = this->current_RTO_ms_;
    }
    
    push(transmit);
  }
}

TCPSenderMessage TCPSender::make_empty_message() const {
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap(this->reader().bytes_popped() + this->SYN_ + this->FIN_, this->isn_);
  msg.SYN = false;
  msg.FIN = false;  
  msg.RST = this->input_.has_error();
  msg.payload.clear();
  return msg;
}

void TCPSender::receive (const TCPReceiverMessage& msg) {
  // Your code here.
  if (msg.RST == true) {
    this->input_.set_error();
    return;
  }

  if (msg.ackno.has_value()) {
    uint64_t seqno = msg.ackno.value().unwrap(this->isn_, this->last_seqno_);
    if (seqno > this->reader().bytes_popped() + this->SYN_ + this->FIN_)
      return; // impossible ackno
    this->last_seqno_ = max(this->last_seqno_, seqno);

    this->window_size_ = max(this->window_size_, seqno + msg.window_size);
    if (msg.window_size == 0) {
      this->window_size_ ++;
      this->zero_flag_ = true;
    } else
      this->zero_flag_ = false;

    if (this->outstanding_data_.size() && seqno >= this->outstanding_data_.front().second + this->outstanding_data_.front().first.sequence_length()) {
      this->current_RTO_ms_ = this->initial_RTO_ms_;
      this->timer_flag_ = false;
      this->consecutive_retransmissions_count_ = 0;

      while (this->outstanding_data_.size() && seqno >= this->outstanding_data_.front().second + this->outstanding_data_.front().first.sequence_length())
        this->outstanding_data_.pop();
      if (this->outstanding_data_.size()) {
        this->timer_flag_ = true;
        this->timer_ = this->current_RTO_ms_;
      }
    }
  } else {
    this->window_size_ = max(this->window_size_, static_cast<uint64_t>(msg.window_size));
    if (msg.window_size == 0) {
      this->window_size_ ++;
      this->zero_flag_ = true;
    } else
      this->zero_flag_ = false;
  }
}

void TCPSender::tick (uint64_t ms_since_last_tick, const TransmitFunction& transmit) {
  // Your code here.
  this->timer_ -= min(this->timer_, ms_since_last_tick);
  if (this->timer_flag_ && this->timer_ == 0) {
    if (this->outstanding_data_.size())
      transmit(this->outstanding_data_.front().first);

    if (this->zero_flag_ == false) {
      this->consecutive_retransmissions_count_ ++;
      this->current_RTO_ms_ *= 2;      
    }

    this->timer_ = this->current_RTO_ms_;
  }
}
