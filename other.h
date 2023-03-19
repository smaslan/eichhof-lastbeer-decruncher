#ifndef other_H
#define other_H

#include <string>
//#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdarg.h>


std::string string_format(const std::string fmt,...);
bool iequals(const std::string& a,const std::string& b);

int wildcmp(const char* wild,const char* string);

std::string istream_read_string(std::ifstream& fr);
int ostream_write_u32(std::ofstream& fw,uint32_t val);
int ostream_write_u16(std::ofstream& fw,uint16_t val);
int ostream_write_i32(std::ofstream& fw,int32_t val);
uint32_t istream_read_u32(std::ifstream& fr);
uint16_t istream_read_u16(std::ifstream& fr);
int32_t istream_read_i32(std::ifstream& fr);


#endif


