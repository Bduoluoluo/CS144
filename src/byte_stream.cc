#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream (uint64_t capacity) : capacity_(capacity), bytesPushed(0), bytesPoped(0), error_(false), close_(false) {
  this->buffer_.clear();
}

bool Writer::is_closed () const {
  // Your code here.
  return this->close_;
}

void Writer::push (string data) {
  // Your code here.
  data = data.substr(0, available_capacity());
  bytesPushed += data.size();
  buffer_ += data;
}

void Writer::close () {
  // Your code here.
  this->close_ = true;
}

uint64_t Writer::available_capacity () const {
  // Your code here.
  return this->capacity_ - this->buffer_.size();
}

uint64_t Writer::bytes_pushed () const {
  // Your code here.
  return this->bytesPushed;
}

bool Reader::is_finished() const {
  // Your code here.
  return this->close_ && !this->buffer_.size();
}

uint64_t Reader::bytes_popped () const {
  // Your code here.
  return this->bytesPoped;
}

string_view Reader::peek () const {
  // Your code here.
  return this->buffer_;
}

void Reader::pop (uint64_t len) {
  // Your code here.
  len = min(len, this->buffer_.size());
  this->bytesPoped += len;
  this->buffer_ = this->buffer_.substr(len);
}

uint64_t Reader::bytes_buffered () const {
  // Your code here.
  return this->buffer_.size();
}
