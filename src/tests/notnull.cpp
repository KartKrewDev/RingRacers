#include "testbase.hpp"
#include <catch2/catch_test_macros.hpp>

#include "../cxxutil.hpp"

namespace {
class A {
public:
	virtual bool foo() = 0;
};
class B : public A {
public:
	virtual bool foo() override final { return true; };
};
} // namespace

TEST_CASE("NotNull<int*> is constructible from int*") {
	int a = 0;
	REQUIRE(srb2::NotNull(static_cast<int*>(&a)));
}

TEST_CASE("NotNull<A*> is constructible from B* where B inherits from A") {
	B b;
	REQUIRE(srb2::NotNull(static_cast<B*>(&b)));
}

TEST_CASE("NotNull<A*> dereferences to B& to call foo") {
	B b;
	srb2::NotNull<A*> a {static_cast<B*>(&b)};
	REQUIRE(a->foo());
}
