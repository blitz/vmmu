#include <catch2/catch.hpp>
#include "pt_walk.hpp"

TEST_CASE("TLB entries match correctly", "[tlb_entry]")
{
  SECTION("Match mask generation works") {
    tlb_entry entry {0, 0, 32, {}};
    REQUIRE(entry.match_mask() == 0xFFFFFFFF00000000ULL);
  }

  SECTION("Linear address match and are translated") {
    tlb_entry entry {0xffff888000000000ULL,
                     0x0000123000000000ULL,
                     64 - 30,   // Gigabyte page
                     {}};

    REQUIRE( entry.translate(0xffff88803fffffffULL));
    REQUIRE(*entry.translate(0xffff88803fffffffULL) == 0x000012303fffffffUll);
  }

}

// TODO Test tlb_entry::allows()
