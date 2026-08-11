#pragma once
#include <doctest/doctest.h>
#include "include/factorial.h"
#include "include/prime_factorial.h"

TEST_CASE("testing the factorial function") {
    CHECK(factorial(1) == 1);
    CHECK(factorial(2) == 2);
    CHECK(factorial(3) == 6);
    CHECK(factorial(10) == 3628800);
    INFO("The factorial function");
}

TEST_CASE("testing the prime factorial function")
{
   CHECK(prime_factorial(1) == 1);
   CHECK(prime_factorial(2) == 1);
   CHECK(prime_factorial(4) == 2);
}

TEST_CASE("[string] testing std::string")
{
    std::string a("omg");
    CHECK(a == "omg");
}

TEST_CASE("[math] basic stuff")  // skipped because it has math in the name
{
    CHECK(6 > 5);
}