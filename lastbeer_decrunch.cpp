//=============================================================================
// Simple decruncher for Eichhof Lastbeer game archive BEER.DAT.
// It unpacks archive files and also extracts sound samples as PCM WAVs.
//
// usage: lastbeer_decrunch.exe [beer_dat_path]
// Result will be written to subfolder .\beer and .\beer\snd
// 
// (c) 2023, Stanislav Maslan, V1.0, s.maslan@seznam.cz
// Distributed under MIT license, https://opensource.org/licenses/MIT.
// https://github.com/smaslan/eichhof-lastbeer-decruncher
//=============================================================================

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include "other.h"

// archive format ID string
#define TYPE_ID_STR std::string("ALPHA-HELIX COMBINER VER 3.3")

// disable data write for debug
#define WRITE_DATA 1 

// usage string
#define INFO_STR "Eichhof Lastbeer BEER.DAT archive decruncher.\n(c) 2023, Stanislav Maslan, V1.0, s.maslan@seznam.cz\n"

// usage string
#define USAGE_STR "usage: lastbeer_decrunch.exe [beer_dat_path]\n" \
                  "  content will be written to ""./beer"",\n" \
                  "  decoded PCM WAVs to ""./beer/snd""\n"


int main(int argc,char* argv[])
{
    std::cout << INFO_STR << "\n";

    if(argc > 2)
    {
        std::cerr << "Error: Extra parameters detected!\n";
        std::cout << USAGE_STR;
        return(1);
    }

    // default beer path
    std::filesystem::path path = "BEER.DAT";
    if(argc == 2)
    {
        // explicit beer path
        path = argv[1];
    }    
    
    // open source archive
    std::ifstream fr(path,std::ios::in | std::ios::binary);
    if(!fr)
    {
        std::cerr << string_format("Error: Cannot open \"%ls\"!\n",path.c_str());
        std::cout << USAGE_STR;
        return(1);
    }

    std::cout << "Decrunching archive " << path << ":\n";

    // check type ID string
    std::string type_id(TYPE_ID_STR.length(),'\0');
    fr.read((char*)type_id.data(),type_id.length());
    if(type_id.compare(TYPE_ID_STR) != 0)
    {
        std::cerr << string_format("Error: Not a BEER.DAT? Wrong type ID string \"%s\"\n!",type_id.c_str());
        std::cout << USAGE_STR;
        return(1);
    }

    try
    {
        // dunno what it is...
        int some_head_stuff = istream_read_u16(fr);
        int some_head_stuff2 = istream_read_u16(fr);

        // files in archive
        int items_count = istream_read_u16(fr);

        // make result dir
        auto cwd = std::filesystem::current_path();
        auto result_fld = cwd / "beer";
        std::filesystem::create_directory(result_fld);

        // make decompressed sound dir
        auto snd_fld = result_fld / "snd";
        std::filesystem::create_directory(snd_fld);

        for(int k = 0; k < items_count; k++)
        {
            // get name
            std::string file_name(14,'\0');        
            fr.read((char*)file_name.data(),file_name.length());
            file_name.resize(strlen(file_name.c_str()));
        
            // get size
            int file_size = istream_read_u32(fr);

            // some stuff
            int file_flags = istream_read_u16(fr);

            // position in data
            int file_pos = istream_read_u32(fr);

            std::cout << " - " <<file_name << ": size=" << file_size << ", flag=" << file_flags << "\n";

            // read file data
            std::vector<uint8_t> data(file_size);
            auto pos = fr.tellg();
            fr.seekg(file_pos);
            fr.read((char*)data.data(),file_size);
            fr.seekg(pos);

            // write to result file
            #if WRITE_DATA
                auto file_path = result_fld / file_name;
                std::ofstream fw(file_path,std::ios::out | std::ios::binary);
                fw.write((char*)data.data(),data.size());
                fw.close();
            #endif

            // is SND?
            if(std::filesystem::path(file_name).extension().compare(".SND") != 0)
                continue;

            // SND data start
            uint8_t* sdata = data.data();

            // by default one sample only
            int snd_count = 1;

            // is it multi sample file?
            if(wildcmp("LEVEL*",file_name.c_str()))
            {
                // yaha, LEVELx.SND: contains multiple samples
                snd_count = *(uint16_t*)sdata; sdata += sizeof(uint16_t);

                // skip whateveritis for each sample
                sdata += snd_count*sizeof(uint32_t);
            }

            // for each sample within SND:
            for(int s = 0; s < snd_count; s++)
            {
                // make result file name
                auto file_name_base = std::filesystem::path(file_name).stem().string();
                std::string wav_name;
                if(snd_count > 1)
                    wav_name = file_name_base + string_format("_%02d.WAV",s);
                else
                    wav_name = file_name_base + ".WAV";
                auto wav_path = snd_fld / wav_name;

                // priority?
                int snd_priority = *(uint16_t*)sdata; sdata += sizeof(uint16_t);

                // sample rate [Hz]
                int snd_rate = 1000*(int)*(uint16_t*)sdata; sdata += sizeof(uint16_t);

                // flags?
                int snd_flags = *(uint16_t*)sdata; sdata += sizeof(uint16_t);
                int is_adpcm = snd_flags & 0x01;

                // data size
                int snd_size = *(uint32_t*)sdata; sdata += sizeof(uint32_t);

                std::cout << "   - " << wav_name << ": size=" << snd_size << ", rate=" << snd_rate << "Hz, " << (is_adpcm?"ADPCM":"PCM") << "\n";

                // decode PCM data buffer
                std::vector<uint8_t> pcm;

                if(is_adpcm)
                {
                    // 4-bit ADPCM data:
                
                    // note: following chunk of code goes from original source (with minor bug fixes):
                    //       https://github.com/kilobyte/lastbeer
                
                    /*   This is something called 8 to 4 bit ADPCM */
                    /*   Decoding algorithm taken from
                    /*   http://wiki.multimedia.cx/index.php?title=Creative_8_bits_ADPCM */
                    pcm.assign(1 + 2*(snd_size - 1),0x80);
                    unsigned char* new_sample = pcm.data();
                    int byte;
                    unsigned char value;
                    int i;

                    int step,shift,limit,sign;
                    step = 0;
                    shift = 0;
                    limit = 5;

                    byte = sdata[0];
                    new_sample[0] = byte;

                    for(i = 1; i < snd_size; i++) {
                        value = (sdata[i] & 0xf0) >> 4;

                        sign = (value & 0x08) ? -1 : 1; /* Test bit 3 */
                        value &= 0x07; /* Clear bit 3 */

                        byte += sign * (value << (step + shift));
                        if(byte > 0xff)
                            byte = 0xff;
                        if(byte < 0x00)
                            byte = 0x00;

                        new_sample[2*i - 1] = byte; /* fixed by -1 */

                        if(value >= limit)
                            step++;
                        else if(value == 0)
                            step--;
                        if(step > 3)
                            step = 3;
                        if(step < 0)
                            step = 0;

                        /*********************************************/

                        /* Exactly the same, but for the second nybble */
                        value = (sdata[i] & 0x0f);

                        sign = (value & 0x08) ? -1 : 1; /* Test bit 3 */
                        value &= 0x07; /* Clear bit 3 */

                        byte += sign * (value << (step + shift));
                        if(byte > 0xff)
                            byte = 0xff;
                        if(byte < 0x00)
                            byte = 0x00;
                        new_sample[2*i + 0] = byte; /* fixed by -1 */

                        if(value >= limit)
                            step++;
                        else if(value == 0)
                            step--;
                        if(step > 3)
                            step = 3;
                        if(step < 0)
                            step = 0;
                    }
                }
                else
                {
                    // unsigned 8-bit PCM data
                    pcm.assign(sdata,&sdata[snd_size]);
                }            
                sdata += snd_size;
                snd_size = pcm.size();                                   
            
                // write RIFF WAVE
                #if WRITE_DATA
                    std::ofstream fw(wav_path,std::ios::out | std::ios::binary);            
                    fw.write("RIFF",4);
                    ostream_write_u32(fw, snd_size + 36);
                    fw.write("WAVE",4);
                    fw.write("fmt ",4);
                    ostream_write_u32(fw, 16);
                    ostream_write_u16(fw, 1); // PCM
                    ostream_write_u16(fw, 1); // mono
                    ostream_write_u32(fw, snd_rate); // sample rate
                    ostream_write_u32(fw, snd_rate*1*1); // byte rate
                    ostream_write_u16(fw, 1*1); // block align
                    ostream_write_u16(fw, 8); // bits
                    fw.write("data",4);
                    ostream_write_u32(fw, snd_size);
                    fw.write((char*)pcm.data(),pcm.size()); // data
                    fw.close();
                #endif
            }        
        }
    }
    catch(...)
    {
        std::cerr << "\nError: Decoding archive failed.\n";
        std::cout << USAGE_STR;
        return(0);
    }
    
    std::cout << "\nDone!\n";
    return(0);
}

