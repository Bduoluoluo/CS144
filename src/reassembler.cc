#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::update_unpopped_bytes () {
  string unpopped_bytes = "";
  while (this->used_buffer_[this->unassembled_index_  % this->capacity_] && writer().available_capacity()) {
    unpopped_bytes.push_back(this->char_buffer_[this->unassembled_index_  % this->capacity_]);
    this->used_buffer_[this->unassembled_index_  % this->capacity_] = false;
    this->unassembled_index_ ++;
    this->pending_bytes_ --;
  }

  if (unpopped_bytes.size())
    output_.writer().push(unpopped_bytes);
  
  if (this->reach_last_substring_ && !this->pending_bytes_)
    output_.writer().close();
}

void Reassembler::insert (uint64_t first_index, string data, bool is_last_substring) {
  // Your code here.
  uint64_t counter = 0;
  for (auto ch : data) {
    if (first_index + counter >= this->unassembled_index_ + writer().available_capacity()) break;
    if (first_index + counter < this->unassembled_index_) {
      counter ++;
      continue;
    }

    if (this->used_buffer_[(first_index + counter)  % this->capacity_] == false) this->pending_bytes_ ++;
    this->char_buffer_[(first_index + counter)  % this->capacity_] = ch;
    this->used_buffer_[(first_index + counter)  % this->capacity_] = true;
    counter ++;
  }

  if (counter == data.size() && is_last_substring)
    this->reach_last_substring_ = true;

  update_unpopped_bytes();
}

uint64_t Reassembler::bytes_pending () const {
  // Your code here.
  return this->pending_bytes_;
}

uint64_t Reassembler::get_unassembled_index () const {
  return this->unassembled_index_;
}