#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>

//https://minitoolz.com/tools/online-deflate-inflate-decompressor/
//https://minitoolz.com/tools/online-deflate-compressor/
//https://github.com/madler/zlib

struct Code {
    uint16_t code; //actual code
    int32_t len; //code length
    uint8_t extra_bits = 0; //extra bits for code
};

//huffman tree class for deflate, put this in private for deflate later
class HuffmanTree{
    private:
    struct Member{
        uint16_t code;
        int32_t len;
        uint16_t compressed_code;
        uint8_t extra_bits = 0;
        std::shared_ptr<Member> left = nullptr;
        std::shared_ptr<Member> right = nullptr;

    };
    std::shared_ptr<Member> head;
    //from right to left
    uint8_t extract1Bit(uint16_t c, uint16_t n) {
        return (c >> n) & 1;
    }

    void reinsert (std::vector<Member>& members, Member m) {
        for (uint32_t i = 0; i < members.size(); i++) {
            if (m.len < members[i].len) {
                members.insert(members.begin() + i, m);
                return;
            }
        }
        members.insert(members.end(), m);
    }

    int32_t findMaxCode (std::vector<Code>& codes) {
        int32_t o = codes[0].code;
        for (int32_t i = 0; i < codes.size(); i++) {
            if (codes[i].code > o) {
                o = codes[i].code;
            }
        }
        return o;
    }

    int16_t countCodeLength (std::vector<Member>& membs, int32_t code_length) {
        int16_t count = 0;
        for (auto& i : membs) {
            if (i.len == code_length) {
                count++;
            }
        }
        return count;
    }

    void insert (Member m) {
        if (!head) {
            Member mm = {0, -1};
            head = std::make_shared<Member>(mm);
        }
        std::shared_ptr<Member> ptr = head;
        std::vector<Member> members;
        uint16_t bit_offset = m.len;
        while (true) {
            bit_offset--;
            // check if go left or right
            uint8_t val = extract1Bit(m.code, bit_offset);

            // left = 0 right = 1
            if (val) {
                //check if right is open
                if (!ptr->right) {
                    Member mm = {0, -1};
                    ptr->right = std::make_shared<Member>(mm);
                }
                ptr = ptr->right;
            } else {
                //check if left is open
                if (!ptr->left) {
                    Member mm = {0, -1};
                    ptr->left = std::make_shared<Member>(mm);
                }
                ptr = ptr->left;
            }

            // ptr->len += m.len;
            //check if on last ptr
            if (bit_offset == 0) {
                ptr->code = m.code;
                ptr->len = m.len;
                break;
            }
        }
    }
    public:
    HuffmanTree () {
        head = nullptr;
    }

    HuffmanTree (std::vector<Code> codes) {
        encode(codes);
    }
    //copy constructor, not really, ptr is the same lol
    HuffmanTree (const HuffmanTree& huff) {
        head = huff.head;
    }
    HuffmanTree (HuffmanTree& huff) {
        head = huff.head;
    }

    bool encode (std::vector<Code> codes) {
        if (codes.size() < 1) {
            return false;
        }
        struct {
            bool operator()(Code a, Code b) const { return a.len < b.len; }
        } compareCodeL;

        std::sort(codes.begin(), codes.end(), compareCodeL);
        std::vector<Member> membs;
        for (auto& i : codes) {
            membs.push_back({i.code, i.len, i.code, i.extra_bits});
        }
        std::vector<Member> saved_memb = membs;
        int16_t code = 0;
        int16_t next_code[16];
        int16_t bl_count[16];
        for (uint32_t i = 0; i < 16; i++) {
            bl_count[i] = countCodeLength(membs, i);
        }
        bl_count[0] = 0;
        for (int32_t bits = 1; bits <= 15; bits++) {
            code = (code + bl_count[bits - 1]) << 1;
            next_code[bits] = code;
        }

        for (int32_t i = 0; i < membs.size(); i++) {
            int32_t len = membs[i].len;
            membs[i].compressed_code = next_code[len];
            next_code[len]++;
        }

        //LOOP THROUGH AND DETERMINE WHERE TO PLACE MEMBS BASED ON THEIR CODE

        while (membs.size() > 0) {
            insert(membs[0]);
            membs.erase(membs.begin());
        }





        return true;
    }

    std::string flatten () {
        return "";
    }

    uint32_t decode_bits (uint32_t input) {

        return 300; //never possible in a deflate compressed block
    }

};


struct Match {
        uint16_t offset; //offset from match
        uint16_t length; //length of match
        uint8_t follow_code; //code after match
        Match () {
            offset = 0;
            length = 0;
            follow_code = 0;
        }
        Match (uint16_t offset, uint16_t length, uint8_t follow_code) {
            this->offset = offset;
            this->length = length;
            this->follow_code = follow_code;
        }
        //do this
        bool overlaps (Match m) {
            return (!(this->offset + this->length < m.offset || m.offset + m.length < this->offset)) && 
            (this->offset + this->length <= m.offset + m.length || m.offset + m.length <= this->offset + this->length);
        }
};


//https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossless/lz77/concept.htm
//https://en.wikipedia.org/wiki/LZ77_and_LZ78
//switch to sliding window instead of current two buffer design
    //need to figure out how to only pick proper matches instead of matching every single run of characters
        //legit encode the data in the sliding window (a-doi)
        //keep list of longest matches found, and if a new match overlaps an old one and is longer, replace it
//also get proper formatting of lz done
    //have the huffman tree as variable and lookup the code for the the triple
class LZ77 {
    private:

    HuffmanTree huff_tree;
    std::vector <uint8_t> buffer;
    std::vector <Match> prev_matches;
    uint32_t window_index;
    uint32_t size;

    //tries to find longest match, and moves window forward by 1 byte, if no match found, just returns an empty Match
    void findLongestMatch () {
        std::vector<Match> matches;
        for (int i = window_index - 1; i >= 0; i--) {
            if (buffer[i] == buffer[window_index]) {
                uint32_t length = 0;
                int j;
                for (j = i; j < window_index && window_index + length < buffer.size() && buffer[j] == buffer[window_index + length]; j++, length++);
                if (length > 1) {
                    matches.push_back(Match(i, length, (window_index + length < buffer.size()) ? buffer[window_index + length] : 0));
                }
            }
        }
        for (int i = 0; i < matches.size();) {
            bool er = false;
            for (int j = 0; j < prev_matches.size() && matches.size() > 0; j++) {
                if (prev_matches[j].overlaps(matches[i])) {
                    if (prev_matches[j].length < matches[i].length) {
                        prev_matches[j] = matches[i];
                    } else {
                        er = true;
                        matches.erase(matches.begin() + i);
                    }           
                }
            }
            if (!er) {
                i++;
            }
        }
        Match longest;
        if (matches.size() > 0) {
            longest = matches[0];
            for (auto& i : matches) {
                if (i.length > longest.length) {
                    longest = i;
                }
            }
            prev_matches.push_back(longest);
        } else {
            longest.follow_code = buffer[window_index];
        }
        window_index += (longest.length > 0) ? longest.length : 1;
    }

    uint32_t remainingWindow () {
        return buffer.size() - window_index;
    }
    public:
    
    LZ77 (uint32_t size, HuffmanTree huff) {
        this->size = size;
        this->window_index = 0;
        this->huff_tree = huff;
    }
    void copyBuffer (std::vector<uint8_t>& buf) {
        for (uint8_t i : buf) {
            buffer.push_back(i);
        }
    }
    //maybe just return the matches vector and the inflate function can use to compress
    std::vector<uint8_t> compressBuffer () {
        std::vector <uint8_t> buf;
        std::vector<Match> matches;
        std::stringstream ss;
        //loop through the prev-matches and edit the buffer, then output the edited buffer
        while (remainingWindow() > 0) {
            findLongestMatch();
            /* if (m.length > 0) {
                ss.write(reinterpret_cast<char*>(&m.offset), sizeof(uint16_t));
                ss.write(reinterpret_cast<char*>(&m.length), sizeof(uint16_t));
                ss.write(reinterpret_cast<char*>(&m.follow_code), sizeof(uint8_t));
            } else {
                ss << m.follow_code;
            } */
        }
        for (int i = 0; i < buffer.size();) {
            bool match = false;
            int j = 0;
            for (; j < prev_matches.size(); j++) {
                if (prev_matches[j].offset == i) {
                    match = true;
                    ss.write(reinterpret_cast<char*>(&prev_matches[j].offset), sizeof(uint16_t));
                    ss.write(reinterpret_cast<char*>(&prev_matches[j].length), sizeof(uint16_t));
                    ss.write(reinterpret_cast<char*>(&prev_matches[j].follow_code), sizeof(uint8_t));
                    i += prev_matches[j].length;
                    break;
                }
            }
            if (!match) {
                ss << buffer[i];
                i++;
            } else {
                prev_matches.erase(prev_matches.begin() + j);
            }
        }
        for (auto& i : ss.str()) {
            buf.push_back(i);
        }
        //copy the stream to the buffer
        matches.clear();
        return buf;
    }
};

/*
    * Data elements are packed into bytes in order of
    increasing bit number within the byte, i.e., starting
    with the least-significant bit of the byte.
    * Data elements other than Huffman codes are packed
    starting with the least-significant bit of the data
    element.
    * Huffman codes are packed starting with the most-
    significant bit of the code.
*/

//add LZ77 compression/decompression
    //figure out why there is a seg fault, lol
//implement rest of inflate
//implement deflate itself
//add error checking and maybe test files lol
class Deflate{
private:
    //from right to left
    static uint8_t extract1Bit (uint8_t c, uint8_t n) {
        return (c >> n) & 1;
    }
    static std::vector<Code> generateFixedCodes () {
        std::vector <Code> fixed_codes;
        uint16_t i = 0;
        //regular alphabet
        for (; i < 144; i++) {
            fixed_codes.push_back({i, 8});
        }
        for (; i < 256; i++) {
            fixed_codes.push_back({i, 9});
        }
        uint8_t extra_bits = 0;
        for(; i < 280; i++) {
            switch (i) {
                case 265:
                    extra_bits = 1;
                break;
                case 269:
                    extra_bits = 2;
                break;
                case 273:
                    extra_bits = 3;
                break;
                case 277:
                    extra_bits = 4;
                break;
            }
            fixed_codes.push_back({i, 7, extra_bits});
        }
        for(; i < 288; i++) {
            switch (i) {
                case 281:
                    extra_bits = 5;
                break;
                case 285:
                    extra_bits = 0;
                break;
            }
            fixed_codes.push_back({i, 8, extra_bits});
        }
        return fixed_codes;
    }
    static std::vector <Code> generateFixedDistanceCodes () {
        std::vector <Code> fixed_codes;
        uint8_t extra_bits = 0;
        //regular alphabet
        for (uint16_t i = 0; i < 32; i++) {
            if (i >= 4) {
                extra_bits = (i / 2) - 1;
            } 
            fixed_codes.push_back({i, 5, extra_bits});
        }
        return fixed_codes;
    }

    static std::streampos getFileSize (std::string file) {
        std::streampos fsize = 0;
        std::ifstream fi (file, std::ios::binary);
        fsize = fi.tellg();
        fi.seekg(0, std::ios::end);
        fsize = fi.tellg() - fsize;
        fi.close();
        return fsize;
    }

public:
    Deflate () {

    }
    ~Deflate () {

    }
    //copy constructor
    Deflate (const Deflate& def) {

    }
    //move constructor
    Deflate (const Deflate&& def) {

    }
    //need struct to represent distance codes and their extra bits/lengths
    static void inflate (std::string file_path, std::string new_file) {
        //creating default huffman tree
        std::vector <Code> fixed_codes = generateFixedCodes();
        std::vector <Code> fixed_dist_codes = generateFixedDistanceCodes();
        HuffmanTree fixed_huffman(fixed_codes);
        HuffmanTree fixed_dist_huffman(fixed_dist_codes);
        //starting parse of file
        std::ofstream nf;
        nf.open(new_file, std::ios::binary);
        std::ifstream f;
        f.open(file_path, std::ios::binary);
        //reading bits
        bool e = true;
        while(e) {
            //read the first three bits
            char c = f.get();
            uint16_t len;
            uint16_t nlen;
            uint8_t bfinal = extract1Bit(c, 0);
            uint8_t btype = (extract1Bit(c, 1) | extract1Bit(c, 2));
            //rest of block parsing
            switch (btype) {
                //uncompressed block
                case 0:
                    //skip remaining bits
                    //read len bytes
                    len = f.get();
                    len |= (f.get() << 8);
                    //read nlen bytes
                    nlen = f.get();
                    // NOLINTNEXTLINE
                    nlen |= (f.get() << 8);
                    //read the rest of uncompressed block
                    for (uint32_t i = 0; i < len; i++) {
                        uint8_t n = f.get();
                        nf << n;
                    }
                break;
                //compressed alphabet
                    //0-255 literals
                    //256 end of block
                    //257-285 length codes
                //parse the blocks with uint16_ts since the values go up to 287, technically but the 286-287 don't participate
                //compressed with fixed huffman codes
                case 1:
                    
                break;
                //compressed with dynamic huffman codes
                case 2:
                
                break;
                //error block (reserved)
                case 3:
                    e = false;
                break;
            }
            //means we finished decoding the final block
            if (bfinal) {
                e = false;
            }
        }

        f.close();
        nf.close();
    }

    //rewrite the file reading to be proper
    static void deflate (std::string file_path, std::string new_file) {
        std::streampos sp = getFileSize(file_path);
        std::ifstream fi;
        fi.open(file_path, std::ios::binary | std::ios::ate);
        if (!fi) {
            return;
        }
        std::streamsize size = fi.tellg();
        fi.close();
        fi.open(file_path, std::ios::binary);
        std::ofstream of;
        of.open(new_file, std::ios::binary);
        if (!of) {
            return;
        }
        std::vector <Code> fixed_codes = generateFixedCodes();
        std::vector <Code> fixed_dist_codes = generateFixedDistanceCodes();
        HuffmanTree fixed_huffman(fixed_codes);
        HuffmanTree fixed_dist_huffman(fixed_dist_codes);
        LZ77 lz(2048, fixed_dist_huffman);
        uint8_t c;
        bool q = false;
        std::vector<uint8_t> buffer;
        while (!q) {
            Match m;
            std::streampos p = sp - fi.tellg();
            if (p > 0 && buffer.size() < 2048) {
                c = fi.get();
                buffer.push_back(c);
            } else {
                if (p <= 0) {
                    q = true;
                }
                lz.copyBuffer(buffer);
                buffer.clear();
                lz.compressBuffer();
            }
        }

        fi.close();
        of.close();
    }
};