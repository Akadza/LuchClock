#include "../src/encoder/position_math.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
int main() {
    using position_math::applyOffset;
    CHECK(applyOffset(1000,512)==1512);
    CHECK(applyOffset(65024,512)==0);
    CHECK(applyOffset(65520,32)==16);
    CHECK(applyOffset(0,65024)==65024);
    CHECK(applyOffset(16,65504)==65520);
    CHECK(applyOffset(65535,65535)==65534);
    for (uint32_t a=0;a<65536;++a) {
        CHECK(applyOffset(a,0)==a);
        CHECK(applyOffset(a,1)==(a==65535?0:a+1));
        CHECK(applyOffset(a,65535)==(a==0?65535:a-1));
        CHECK(applyOffset(0,a)==a);
        CHECK(applyOffset(applyOffset(a,32768),32768)==a);
    }
    for (uint32_t a=0;a<65536;a+=16) {
        uint32_t expected=a;
        for (uint32_t o=0;o<65536;o+=16) {
            CHECK(applyOffset(a,o)==expected);
            expected += 16;
            if (expected==65536) expected=0;
        }
    }
    std::cout << "Offset OK\n";
}
