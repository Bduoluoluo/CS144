#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap (uint64_t n, Wrap32 zero_point) {
  // Your code here.
  return Wrap32(zero_point + static_cast<uint32_t>(n));
}

uint64_t Wrap32::unwrap (Wrap32 zero_point, uint64_t checkpoint) const {
  // Your code here.
  uint64_t cycle = 1ul << 32;
  uint64_t abs_seqno_0 = this->raw_value_ - zero_point.raw_value_;
  abs_seqno_0 += (checkpoint - abs_seqno_0) / cycle * cycle;
  uint64_t diff_0 = (abs_seqno_0 >= checkpoint) ? (abs_seqno_0 - checkpoint) : (checkpoint - abs_seqno_0);
  uint64_t abs_seqno_1 = abs_seqno_0 + cycle;
  uint64_t diff_1 = (abs_seqno_1 >= checkpoint) ? (abs_seqno_1 - checkpoint) : (checkpoint - abs_seqno_1);
  return (diff_0 <= diff_1) ? abs_seqno_0 : abs_seqno_1;
}
