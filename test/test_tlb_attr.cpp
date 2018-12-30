#include <catch2/catch.hpp>
#include "pt_walk.hpp"

TEST_CASE("TLB attributes", "[tlb_attr]")
{
  SECTION("Default attributes allow everything") {
    tlb_attr attr;

    CHECK(attr.is_w());
    CHECK(attr.is_u());
    CHECK_FALSE(attr.is_xd());
    CHECK_FALSE(attr.is_d());
  }

  SECTION("Attribute construction works") {
    tlb_attr attr(0, 0, 1, 1);

    CHECK_FALSE(attr.is_w());
    CHECK_FALSE(attr.is_u());
    CHECK(attr.is_xd());
    CHECK(attr.is_d());
  }
}

TEST_CASE("TLB entry combining", "[tlb_attr]")
{
  tlb_attr const attr_nothing{0, 0, 0, 0};
  tlb_attr const attr_w{1, 0, 0, 0};
  tlb_attr const attr_u{0, 1, 0, 0};
  tlb_attr const attr_xd{0, 0, 1, 0};
  tlb_attr const attr_d{0, 0, 0, 1};

  SECTION("User bits combine correctly") {
    CHECK      (tlb_attr::combine(attr_u,       attr_u).is_u());
    CHECK_FALSE(tlb_attr::combine(attr_nothing, attr_u).is_u());
  }

  SECTION("Write bits combine correctly") {
    CHECK      (tlb_attr::combine(attr_w,       attr_w).is_w());
    CHECK_FALSE(tlb_attr::combine(attr_nothing, attr_w).is_w());
  }

  SECTION("XD bits combine correctly") {
    CHECK(tlb_attr::combine(attr_xd,      attr_xd).is_xd());
    CHECK(tlb_attr::combine(attr_nothing, attr_xd).is_xd());
  }

  SECTION("Dirty bits combine correctly") {
    CHECK(tlb_attr::combine(attr_d,       attr_d).is_d());
    CHECK(tlb_attr::combine(attr_nothing, attr_d).is_d());
  }

}
