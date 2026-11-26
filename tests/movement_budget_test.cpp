#include "../src/encoder/movement_budget.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    position_math::MovementBudget budget(100, 512);
    budget.accrue(1000);
    CHECK(budget.consume(100));
    CHECK(!budget.consume(1));
    budget.accrue(100000);
    CHECK(!budget.consume(513));
    CHECK(budget.consume(512));
    CHECK(!budget.consume(1));
    position_math::MovementBudget fractional(91, 512);
    for (int i=0; i<40; ++i) fractional.accrue(25);
    CHECK(fractional.consume(91));
    CHECK(!fractional.consume(1));
    position_math::MovementBudget extreme(UINT32_MAX, 512);
    extreme.accrue(UINT32_MAX);
    CHECK(extreme.consume(512));
    CHECK(!extreme.consume(1));
    std::cout << "Movement budget OK\n";
}
