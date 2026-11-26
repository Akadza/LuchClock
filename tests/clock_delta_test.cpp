#include "../src/encoder/position_math.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    using position_math::clockDelta;
    int32_t d=123;
    CHECK(clockDelta(100,200,d)&&d==100);
    CHECK(clockDelta(786431,0,d)&&d==1);
    CHECK(clockDelta(0,786431,d)&&d==-1);
    CHECK(clockDelta(655360,720896,d)&&d==65536);
    for(uint32_t p=0;p<786432;++p) {
        CHECK(clockDelta(p,p,d)&&d==0);
        CHECK(clockDelta(p,p==786431?0:p+1,d)&&d==1);
        CHECK(clockDelta(p,p==0?786431:p-1,d)&&d==-1);
        const uint32_t opposite=p<393216?p+393216:p-393216;
        d=123;
        CHECK(!clockDelta(p,opposite,d)&&d==123);
        CHECK(clockDelta(p,opposite==0?786431:opposite-1,d)&&d==393215);
        CHECK(clockDelta(p,opposite==786431?0:opposite+1,d)&&d==-393215);
    }
    for(uint32_t invalid : {786432u,786433u,UINT32_MAX}) {
        d=123;
        CHECK(!clockDelta(invalid,0,d)&&d==123);
        CHECK(!clockDelta(0,invalid,d)&&d==123);
    }
    std::cout << "Clock delta OK\n";
}
