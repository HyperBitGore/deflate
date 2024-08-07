#include "deflate.hpp"


int main(){
    Deflate def;
    HuffmanTree hf;
    hf.encode(
        {{'a', 3}, {'b', 3}, {'c', 3}, {'d', 3}, {'e', 3}, {'f', 2}, {'g', 4}, {'h', 4}}
        );
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
    Deflate::inflate("uncompressed.test", "uncompressed.parsed");

    Deflate::deflate("lz.test", "lz.compressed");
    
    Match m1(32, 5, 'c');
    Match m2(34, 2, 'h');
    std::cout << "Overlaps: " << m1.overlaps(m2) << "\n";

}
