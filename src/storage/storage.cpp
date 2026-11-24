#include "storage.h"
#include <EEPROM.h>
class EepromStorage final:public IStorage {
public:
 void begin(size_t size) override {EEPROM.begin(size);}
 void read(size_t address,void* data,size_t size) override {
  auto* p=static_cast<uint8_t*>(data);
  for(size_t i=0;i<size;++i)p[i]=EEPROM.read(address+i);
 }
 void write(size_t address,const void* data,size_t size) override {
  const auto* p=static_cast<const uint8_t*>(data);
  for(size_t i=0;i<size;++i)EEPROM.write(address+i,p[i]);
 }
 bool commit() override {return EEPROM.commit();}
};
IStorage& storage(){static EepromStorage instance;return instance;}
