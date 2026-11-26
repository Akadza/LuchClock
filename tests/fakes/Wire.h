#pragma once
#include <stdint.h>
struct FakeWire {
    uint8_t address=0, value=127;
    bool connected=true;
    void begin(int,int) {}
    void setClock(unsigned long) {}
    void beginTransmission(uint8_t a) {address=a;}
    uint8_t endTransmission() {return connected && address==0x20 ? 0:1;}
    void write(uint8_t) {}
    uint8_t requestFrom(uint8_t a,uint8_t n) {return connected && a==0x20?n:0;}
    int read() {return value;}
};
extern FakeWire Wire;
