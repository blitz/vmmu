#pragma once

#include <algorithm>
#include <cstdint>
#include <forward_list>

#include "utilities.hpp"

// An exception that is thrown when the memory class is asked to access memory
// that has no defined value.
struct accessed_uninitialized_memory {
  uint64_t address;

  accessed_uninitialized_memory() = delete;
  accessed_uninitialized_memory(uint64_t address_) : address(address_) {}
};

// A memory backend for the page table walker that is useful for testing.
//
// It doesn't store a flat memory array, but instead records all memory
// operations. This is useful to check whether certain memory locations are not
// accessed multiple times (TOCTOU bugs!) or whether the correct atomic
// operations were used.
template <typename WORD>
class memory {

  enum class opcode { READ, WRITE };

  struct operation {
    opcode opc;
    uint64_t address;
    WORD value;

    operation() = delete;

    static operation write(uint64_t address, WORD value)
    {
      return operation(opcode::WRITE, address, value);
    }

    static operation read(uint64_t address, WORD value)
    {
      return operation(opcode::WRITE, address, value);
    }

    bool matches_read(uint64_t address_) const
    {
      return opc == opcode::WRITE and address == address_;
    }

  private:

    operation(opcode opc_, uint64_t address_, WORD value_)
      : opc(opc_), address(address_), value(value_)
    {}
  };

  // Log of all operations. From new to old, i.e. new operations are added to
  // the front.
  std::forward_list<operation> history;

  bool is_naturally_aligned(uint64_t address)
  {
    return (address % sizeof(WORD)) == 0;
  }

public:

  void write(uint64_t address, WORD value)
  {
    fast_assert(is_naturally_aligned(address));

    history.emplace_front(operation::write(address, value));
  }

  WORD read(uint64_t address)
  {
    fast_assert(is_naturally_aligned(address));

    auto it = std::find_if(history.begin(), history.end(),
                           [address] (auto const &op) { return op.matches_read(address); });

    if (it == history.end()) {
      throw accessed_uninitialized_memory { address };
    } else {
      history.emplace_front(operation::read(address, it->value));
      return it->value;
    }
  }
};
