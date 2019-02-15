// This is a test for the memory class that we use as the memory backend for the
// page table walker tests.

#include <catch2/catch.hpp>
#include "memory.hpp"

using operation_type = memory<uint32_t>::operation_type;

TEST_CASE("Memory can read and write data", "[memory]")
{
  memory<uint32_t> mem;

  SECTION("Reading uninitialized memory throws") {
    REQUIRE_THROWS_AS(mem.read(0), accessed_uninitialized_memory);
  }

  SECTION("Written memory can be read") {
    mem.write(0, 123);
    mem.write(12, 456);

    REQUIRE(mem.read(0) == 123);
    REQUIRE(mem.read(12) == 456);
  }

  SECTION("New writes overwrite older ones") {
    mem.write(0, 123);
    mem.write(0, 456);

    REQUIRE(mem.read(0) == 456);
  }
}

// Return a function that sets a boolean value when it's called.
static auto bool_setter(bool *b)
{
  return [b] (...) { *b = true; };
}

TEST_CASE("Asynchronous handlers are called")
{
  memory<uint32_t> mem;

  SECTION("A handler is called for the correct operation") {
    bool called = false;

    mem.execute_after(operation_type::WRITE, 4, bool_setter(&called));

    mem.write(4, 0);
  }

  SECTION("A handler is not called for the wrong address or operation") {
    bool called = false;

    mem.execute_after(operation_type::READ, 4, bool_setter(&called));

    mem.write(0, 0);
    mem.write(4, 0);

    CHECK_FALSE(called);
  }

  SECTION("A handler is called after the operation it matched on") {
    uint32_t read_value = ~0;

    mem.write(0, 1);
    mem.execute_after(operation_type::WRITE, 0,
                             [&read_value] (memory<uint32_t> *m) { read_value = m->read(0); });
    mem.write(0, 2);

    REQUIRE(read_value == 2);
  }

  SECTION("A handler is called only once") {
    int count = 0;

    mem.execute_after(operation_type::WRITE, 0, [&count] (...) { count++; });

    mem.write(0, 1);
    mem.write(0, 1);

    REQUIRE(count == 1);
  }
}

TEST_CASE("Operations are counted correctly")
{
  memory<uint32_t> mem;

  REQUIRE(mem.count_operations(operation_type::WRITE, 0) == 0);
  mem.write(0, 0);
  REQUIRE(mem.count_operations(operation_type::WRITE, 0) == 1);
  mem.write(0, 1);
  REQUIRE(mem.count_operations(operation_type::WRITE, 0) == 2);
  mem.read(0);
  REQUIRE(mem.count_operations(operation_type::WRITE, 0) == 2);
  REQUIRE(mem.count_operations(operation_type::READ, 0) == 1);
}
