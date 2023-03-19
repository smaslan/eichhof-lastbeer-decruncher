#include "other.h"

#include <string>
#include <filesystem>

// compare wildcard string to string
int wildcmp(const char* wild,const char* string)
{
    // Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
    const char* cp = NULL,* mp = NULL;

    while((*string) && (*wild != '*')) {
        if((*wild != *string) && (*wild != '?')) {
            return 0;
        }
        wild++;
        string++;
    }

    while(*string) {
        if(*wild == '*') {
            if(!*++wild) {
                return 1;
            }
            mp = wild;
            cp = string+1;
        }
        else if((*wild == *string) || (*wild == '?')) {
            wild++;
            string++;
        }
        else {
            wild = mp;
            string = cp++;
        }
    }

    while(*wild == '*') {
        wild++;
    }
    return !*wild;
}

std::string string_format(const std::string fmt,...) {
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while(1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap,fmt);
        int n = vsnprintf((char*)str.data(),size,fmt.c_str(),ap);
        va_end(ap);
        if(n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if(n > -1)  // Needed size returned
            size = n + 1;   // For null char
        else
            size *= 2;      // Guess at a larger size (OS specific)
    }
    return str;
}

// compare strings case insensitive
bool iequals(const std::string& a,const std::string& b)
{
    return std::equal(a.begin(),a.end(),
        b.begin(),b.end(),
        [](char a,char b) {
            return std::tolower(a) == std::tolower(b);
        });
}


// read string item with size prefix (16bit)
std::string istream_read_string(std::ifstream& fr)
{
    uint16_t len;
    fr.read((char*)&len,sizeof(uint16_t));
    std::string str(len,'\0');
    //str.reserve(len + 1);
    fr.read((char*)str.data(),len);
    str.resize(len-1);
    return(str);
}

// write u32 value
int ostream_write_u32(std::ofstream& fw,uint32_t val)
{
    fw.write((char*)&val,sizeof(uint32_t));
    return(0);
}

// write u16 value
int ostream_write_u16(std::ofstream& fw,uint16_t val)
{
    fw.write((char*)&val,sizeof(uint16_t));
    return(0);
}

// write i32 value
int ostream_write_i32(std::ofstream& fw,int32_t val)
{
    fw.write((char*)&val,sizeof(int32_t));
    return(0);
}

// read uint32_t
uint32_t istream_read_u32(std::ifstream& fr)
{
    uint32_t val;
    fr.read((char*)&val,sizeof(uint32_t));
    return(val);
}

// read uint16_t
uint16_t istream_read_u16(std::ifstream& fr)
{
    uint16_t val;
    fr.read((char*)&val,sizeof(uint16_t));
    return(val);
}

// read int32_t
int32_t istream_read_i32(std::ifstream& fr)
{
    int32_t val;
    fr.read((char*)&val,sizeof(int32_t));
    return(val);
}







