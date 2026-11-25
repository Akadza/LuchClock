#include "../src/encoder/position_math.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    uint32_t out=123;
    CHECK(position_math::advanceClockPosition(65024,512,out) && out==65536);
    CHECK(position_math::advanceClockPosition(786400,512,out) && out==480);
    CHECK(position_math::advanceClockPosition(0,-512,out) && out==785920);
    out=123; CHECK(!position_math::advanceClockPosition(786432,0,out) && out==123);
    for(uint32_t p=0;p<786432;++p) {
        CHECK(position_math::advanceClockPosition(p,1,out) && out==(p==786431?0:p+1));
        CHECK(position_math::advanceClockPosition(p,-1,out) && out==(p==0?786431:p-1));
        CHECK(position_math::advanceClockPosition(p,786432,out) && out==p);
        CHECK(position_math::advanceClockPosition(p,-786432,out) && out==p);
    }
    CHECK(position_math::advanceClockPosition(0,INT32_MAX,out) && out==524287);
    CHECK(position_math::advanceClockPosition(0,INT32_MIN,out) && out==262144);
    out=123; CHECK(!position_math::advanceClockPosition(UINT32_MAX,1,out) && out==123);
    std::cout << "Clock position OK\n";
}
