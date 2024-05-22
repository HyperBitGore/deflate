#include "deflate.h"


int main(){
    Deflate def;
    HuffmanTree hf;
    hf.encode({{'a', 8}, {'b', 6}, {'c', 5}, {'d', 4}, {'e', 3}, {'f', 2}});
    std::ofstream of;
    of.open("uncompressed.test", std::ios::binary);
    //first byte
    uint8_t b = 1;
    of.write(reinterpret_cast<char*>(&b), sizeof(uint8_t));
    //len = 100 and nlen = 27
    uint32_t p = (100) | (27 << 16); 
    of.write(reinterpret_cast<char*>(&p), sizeof(uint32_t));
    for (uint8_t i = 0; i < 100; i++) {
        of << i;
    }
    of.close();
    Deflate::decode("uncompressed.test", "uncompressed.parsed");
}
