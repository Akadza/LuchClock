#pragma once
#include <Arduino.h>
// Контракт байтового хранилища. Форматы записей определяются отдельно.
class IStorage {
public:
 virtual ~IStorage()=default;
 virtual void begin(size_t size)=0;
 virtual void read(size_t address,void* data,size_t size)=0;
 virtual void write(size_t address,const void* data,size_t size)=0;
 virtual bool commit()=0;
 template<class T> void get(size_t address,T& value){read(address,&value,sizeof(T));}
 template<class T> void put(size_t address,const T& value){write(address,&value,sizeof(T));}
};
IStorage& storage();
