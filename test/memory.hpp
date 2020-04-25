#pragma once

#include <algorithm>
#include <cstdint>
#include <forward_list>
#include <functional>
#include <map>
#include <tuple>

#include "vmmu_utilities.hpp"

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
class memory
{
public:
  enum class operation_type { READ, WRITE };

private:
  using this_t = memory<WORD>;

  struct operation_head {
    operation_type opc;
    uint64_t address;

    // We need a comparison operator, because we use this struct in std::map
    // below.
    bool operator<(operation_head const &rhs) const
    {
      return opc < rhs.opc or (opc == rhs.opc and address < rhs.address);
    }

    bool operator==(operation_head const &rhs) const
    {
      return opc == rhs.opc and address == rhs.address;
    }

    operation_head() = delete;
    operation_head(operation_type opc_, uint64_t address_) : opc(opc_), address(address_) {}
  };

  struct operation : public operation_head {
    WORD value;

    operation() = delete;

    static operation write(uint64_t address, WORD value)
    {
      return operation(operation_type::WRITE, address, value);
    }

    static operation read(uint64_t address, WORD value)
    {
      return operation(operation_type::READ, address, value);
    }

    bool matches_read(uint64_t address_) const
    {
      return this->opc == operation_type::WRITE and this->address == address_;
    }

  private:
    operation(operation_type opc_, uint64_t address_, WORD value_)
        : operation_head(opc_, address_), value(value_)
    {
    }
  };

  // Log of all operations. From new to old, i.e. new operations are added to
  // the front.
  std::forward_list<operation> history;

  using async_handler_fn = std::function<void(this_t *)>;

  // Remember all async handlers here.
  std::map<operation_head, async_handler_fn> async_handlers;

  void maybe_execute_async_handler(operation_head const &op)
  {
    auto node = async_handlers.extract(op);

    if (not node)
      return;

    node.mapped()(this);
  }

  // Will execute a matching async handler on destruction. Can be used as a
  // scope guard.
  struct async_handler_guard {
    this_t *memory;
    operation_head op;

    async_handler_guard() = delete;
    async_handler_guard(this_t *memory_, operation_type type_, uint64_t address_)
        : memory(memory_), op(type_, address_)
    {
    }

    ~async_handler_guard() { memory->maybe_execute_async_handler(op); }
  };

  bool is_naturally_aligned(uint64_t address) { return (address % sizeof(WORD)) == 0; }

public:
  void write(uint64_t address, WORD value)
  {
    vmmu::fast_assert(is_naturally_aligned(address));
    async_handler_guard g {this, operation_type::WRITE, address};

    history.emplace_front(operation::write(address, value));
  }

  WORD read(uint64_t address)
  {
    vmmu::fast_assert(is_naturally_aligned(address));
    async_handler_guard g {this, operation_type::READ, address};

    auto it = std::find_if(history.begin(), history.end(),
                           [address](auto const &op) { return op.matches_read(address); });

    if (it == history.end()) {
      throw accessed_uninitialized_memory {address};
    } else {
      history.emplace_front(operation::read(address, it->value));
      return it->value;
    }
  }

  // Schedule a function to be called once after a specified memory operation.
  //
  // This can be used to modify memory at the right time to make cmpxchg
  // operations fail and increase test coverage of otherwise hard to reach code
  // paths.
  template <typename H>
  void execute_after(operation_type op_type, uint64_t address, H &&async_handler)
  {
    async_handlers.emplace(operation_head {op_type, address}, std::forward<H>(async_handler));
  }

  // Count the number of operations at a given address.
  size_t count_operations(operation_type op_type, uint64_t address) const
  {
    return std::count(history.begin(), history.end(), operation_head {op_type, address});
  }
};
