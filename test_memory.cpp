#include <catch2/catch.hpp>
#include "memory.hpp"

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
