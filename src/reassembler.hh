#pragma once

#include "byte_stream.hh"
#include <vector>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler(ByteStream&& output) : output_(std::move(output)), capacity_(writer().available_capacity()), pending_bytes_(0), unassembled_index_(0), reach_last_substring_(false) {
    char_buffer_.resize(capacity_);
    used_buffer_.resize(capacity_);
  }

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;
  uint64_t get_unassembled_index () const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  void update_unpopped_bytes ();

  ByteStream output_; // the Reassembler writes to this ByteStream
  uint64_t capacity_;
  uint64_t pending_bytes_;
  uint64_t unassembled_index_;
  bool reach_last_substring_;
  std::vector<char> char_buffer_{};
  std::vector<bool> used_buffer_{};
};
