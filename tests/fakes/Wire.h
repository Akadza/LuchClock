#pragma once
#include <stdint.h>
struct FakeWire {
 uint8_t address=0,value=127,reg=0,readIndex=0;
 bool connected=true,asConnected=false;
 uint16_t angle=0;uint8_t status=0x20;
 bool shortRead=false;
 unsigned configWrites=0;
 void begin(int,int){}
 void setClock(unsigned long){}
 void beginTransmission(uint8_t a){address=a;}
 uint8_t endTransmission(bool=true){return (address==0x20&&connected)||(address==0x36&&asConnected)?0:1;}
 void write(uint8_t v){if(address==0x36){reg=v;if(v!=0x0b&&v!=0x0c)++configWrites;}}
 uint8_t requestFrom(uint8_t a,uint8_t n){address=a;readIndex=0;return !shortRead&&((a==0x20&&connected)||(a==0x36&&asConnected))?n:0;}
 int read(){if(address==0x20)return value;if(reg==0x0b)return status;return readIndex++==0?angle>>8:angle&255;}
};
extern FakeWire Wire;
