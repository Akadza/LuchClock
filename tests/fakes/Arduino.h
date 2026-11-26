#pragma once
#include <stdint.h>
#include <string>
#include <cstdio>
#include <cstring>
class String : public std::string {
public:
    using std::string::string;
    String() = default;
    String(const std::string& s):std::string(s){}
    explicit String(unsigned int n):std::string(std::to_string(n)){}
};
#define F(x) x
#define LOW 0
#define HIGH 1
#define OUTPUT 1
unsigned long millis();
void digitalWrite(uint8_t pin, uint8_t value);
void pinMode(uint8_t pin, uint8_t mode);
struct FakeSerial {
    template<class... T> void printf(const char*, T...) {}
    template<class T> void println(const T&) {}
};
extern FakeSerial Serial;
