#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>
#include <string>

struct Code {
    uint16_t code; //actual code
    int32_t len; //code length
    uint8_t extra_bits = 0; //extra bits for code
    uint16_t value; //original value
};

//huffman tree class for deflate, put this in private for deflate later
class HuffmanTree{
    private:
    struct Member{
        uint16_t value;
        int32_t len;
        uint16_t code;
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
            Member mm = {300, -1, 0, 0};
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
                    Member mm = {300, -1, 0, 0};
                    ptr->right = std::make_shared<Member>(mm);
                }
                ptr = ptr->right;
            } else {
                //check if left is open
                if (!ptr->left) {
                    Member mm = {300, -1, 0, 0};
                    ptr->left = std::make_shared<Member>(mm);
                }
                ptr = ptr->left;
            }

            // ptr->len += m.len;
            //check if on last ptr
            if (bit_offset == 0) {
                ptr->value = m.value;
                ptr->extra_bits = m.extra_bits;
                ptr->code = m.code;
                ptr->len = m.len;
                break;
            }
        }
    }
    Code getCodeFromCompressed (uint32_t compressed_code, std::shared_ptr<Member> ptr) {
        if (ptr->code == compressed_code && ptr->len > 0) {
            return {ptr->code, ptr->len, ptr->extra_bits, ptr->value};
        }
        Code left = {0, -1};
        if (ptr->left != nullptr) {
            left = getCodeFromCompressed(compressed_code, ptr->left);
        }
        if (left.len > 0) {
            return left;
        }
        Code right = {0, -1};
        if (ptr->right != nullptr) {
            right = getCodeFromCompressed(compressed_code, ptr->right);
        }
        if (right.len > 0) {
            return right;
        }
        return {0, -1, 0, 300};
    }

    Code getCodeFromUncompressed (uint32_t uncompressed_code, std::shared_ptr<Member> ptr) {
        if (ptr->value == uncompressed_code && ptr->len > 0) {
            return {ptr->code, ptr->len, ptr->extra_bits, ptr->value};
        }
        Code left = {0, -1};
        if (ptr->left != nullptr) {
            left = getCodeFromUncompressed(uncompressed_code, ptr->left);
        }
        if (left.len > 0) {
            return left;
        }
        Code right = {0, -1};
        if (ptr->right != nullptr) {
            right = getCodeFromUncompressed(uncompressed_code, ptr->right);
        }
        if (right.len > 0) {
            return right;
        }
        return {0, -1, 0, 300};
    }

    void decodeCodes (std::vector<Code>& codes, std::shared_ptr<Member> ptr) {
        if (ptr->len != -1) {
            codes.push_back({ptr->code, ptr->len, ptr->extra_bits, ptr->value});
        }
        if (ptr->left != nullptr) {
            decodeCodes(codes, ptr->left);
        }
        if (ptr->right != nullptr) {
            decodeCodes(codes, ptr->right);
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
            membs.push_back({i.value, i.len, i.code, i.extra_bits});
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
            membs[i].code = next_code[len];
            next_code[len]++;
        }

        //LOOP THROUGH AND DETERMINE WHERE TO PLACE MEMBS BASED ON THEIR CODE

        while (membs.size() > 0) {
            insert(membs[0]);
            membs.erase(membs.begin());
        }





        return true;
    }
    // returns the codes that makeup the tree
    std::vector<Code> decode () {
        std::vector<Code> codes;
        decodeCodes(codes, head);

        return codes;
    }

    std::string flatten () {
        return "";
    }

    // already have the input decoded to proper format
    Code getUncompressedCode (uint32_t compressed_code) {
        return getCodeFromCompressed(compressed_code, head); 
    }
    // Input is uncompressed code
    Code getCompressedCode (uint32_t uncompressed_code) {
        return getCodeFromUncompressed(uncompressed_code, head);
    }

};


struct Match {
        uint16_t offset; //offset from start to match
        uint16_t length; //length of match
        uint8_t follow_code; //code after match
        int32_t dif; // difference from match to original
        Match () {
            offset = 0;
            length = 0;
            follow_code = 0;
            dif = 0;
        }
        Match (uint16_t offset, uint16_t length, uint8_t follow_code, int32_t dif) {
            this->offset = offset;
            this->length = length;
            this->follow_code = follow_code;
            this->dif = dif;
        }
        //do this
        bool overlaps (Match m) {
            return (!(this->offset + this->length < m.offset || m.offset + m.length < this->offset)) && 
            (this->offset + this->length <= m.offset + m.length || m.offset + m.length <= this->offset + this->length);
        }
};

//https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossless/lz77/concept.htm
//https://en.wikipedia.org/wiki/LZ77_and_LZ78
class LZ77 {
    private:
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
                if (length > 2) {
                    matches.push_back(Match(i, length, (window_index + length < buffer.size()) ? buffer[window_index + length] : 0, window_index - i));
                }
            }
        }
        for (int i = 0; i < matches.size();) {
            bool er = false;
            for (int j = 0; j < prev_matches.size() && i < matches.size(); j++) {
                // when a matches is erased creates issue somehow here
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
        return (uint32_t)buffer.size() - window_index;
    }
    void copyBuffer (std::vector<uint8_t>& buf) {
        for (uint8_t i : buf) {
            buffer.push_back(i);
        }
    }
    public:
    
    LZ77 (uint32_t size) {
        this->size = size;
        this->window_index = 0;
    }
    //maybe just return the matches vector and the inflate function can use to compress
    std::vector<Match> findBufferRuns (std::vector<uint8_t>& buf) {
        copyBuffer(buf);
        std::vector<Match> matches;
        //loop through buffer and find the longest matches
        //figure out why this is going out of bounds
        while (remainingWindow() > 0) {
            findLongestMatch();
        }
        //loop through buffer and edit it to compress streaks
        for (int i = 0; i < buffer.size();) {
            bool match = false;
            int j = 0;
            for (; j < prev_matches.size(); j++) {
                if (prev_matches[j].offset == i) {
                    match = true;
                    matches.push_back(prev_matches[j]);
                    i += prev_matches[j].length;
                    break;
                }
            }
            if (!match) {
                i++;
            } else {
                prev_matches.erase(prev_matches.begin() + j);
            }
        }
        return {};
    }
};

struct Range {
    uint32_t start;
    uint32_t end;
    uint32_t code;
};

class RangeLookup {
private:
    std::vector <Range> ranges;
public:
    RangeLookup () {
    }
    void addRange (Range r) {
        ranges.push_back(r);
    }
    Range lookup (uint32_t length) {
        for (Range i : ranges) {
            if (length >= i.start && length <= i.end) {
                return i;
            }
        }
        return {0, 0, 0};
    }
};

class Bitstream {
private:
    uint8_t bit_offset;
    uint32_t offset;
    std::vector<uint8_t> data;
    
    //from right to left
    uint8_t extract1Bit(uint32_t c, uint16_t n) {
        return (c >> n) & 1;
    }
    uint8_t extract1BitLeft (uint32_t c, uint16_t n) {
        return ((c << n) & 0b10000000000000000000000000000000) >> 31;
    }
    uint8_t extract1BitLeft (uint8_t c, uint8_t n) {
        return ((c << n) & 0b10000000) >> 7;
    }
public:
    Bitstream () {
        bit_offset = 0;
        offset = 0;
        data.push_back(0);
    }
    // copy constructor
    Bitstream (const Bitstream& b) {
        data = b.data;
        bit_offset = b.bit_offset;
    }
    void addBits (uint32_t val, uint8_t count) {
        for (uint32_t i = 0; i < count; i++, bit_offset++) {
            if (bit_offset > 7) {
                data.push_back(0);
                offset++;
                bit_offset = 0;
            }
            uint8_t bit = extract1Bit(val, i);
            data[offset] |= (bit << bit_offset);
        }
    }
    void nextByteBoundary () {
        
        data.push_back(0);
        bit_offset = 0;
        offset++;
    }
    std::vector<uint8_t> getData () {
        return data;
    }
    void addRawBuffer (std::vector<uint8_t> buffer) {
        for (auto& i : buffer) {
            addBits(i, 8);
        }
    }
    size_t getSize () {
        if (bit_offset == 0) {
            return data.size() - 1;
        }
        return data.size();
    }
};
// https://github.com/ebiggers/libdeflate/blob/master/lib/deflate_compress.c
// https://pzs.dstu.dp.ua/ComputerGraphics/ic/bibl/huffman.pdf
class CodeMap {
private:
    uint32_t codes[300];
public:
    CodeMap () {
        std::memset(codes, 0, sizeof(uint32_t) * 300);
    }
    CodeMap (const CodeMap& c) {
        std::memcpy(codes, c.codes, sizeof(uint32_t) * 300);
    }
    void addOccur (uint32_t code) {
        if (code < 300) {
            codes[code] += 1;
        }
    }
    uint32_t getOccur (uint32_t code) {
        if (code < 300) {
            return codes[code];
        }
        return 0;
    }
    std::vector<Code> generateCodes () {   
        struct t_code {
            uint32_t occurs;
            uint32_t value;
        };
        std::vector <t_code> temp_codes;
        for (uint32_t i = 0; i < 300; i++) {
            uint32_t occurs = getOccur(i);
            if (occurs) {
                // std::cout << "hit: " << i << ", occurs: " << occurs << "\n";
                temp_codes.push_back({occurs, i});
            }
        }

        struct compare_tcode {
            inline bool operator() (const t_code& t1, const t_code& t2) {
                return t1.occurs > t2.occurs;
            }
        };
        std::sort(temp_codes.begin(), temp_codes.end(), compare_tcode());
        int32_t len = 2;
        uint32_t code = 0;
        std::vector <Code> out_codes;
        for (t_code i : temp_codes) {
             // check if len needs to be increased for this one, so if the amount has surpassed the allowed number of codes of the bits
            uint32_t allowed = std::pow(2, len);
            if (code + 1 > allowed) {
                len++;
            }
            out_codes.push_back({(uint16_t)code, len, 0, (uint16_t)i.value});
            code++;
        }
        return out_codes;
    }
};

class DynamicHuffLengthCompressor {
private:
    std::vector<Code> codes;

    uint32_t countRepeats (std::vector<uint8_t>& bytes, uint32_t index) {
        uint32_t count = 0;
        for (uint32_t i = index + 1; i < bytes.size(); i++) {
            if (bytes[i] == bytes[index]) {
                count++;
            } else {
                return count;
            }
        }
        return count;
    }
    HuffmanTree constructTree (std::vector<uint8_t>& bytes) {
        CodeMap cm;
        for (uint32_t i = 0; i < bytes.size();) {
            uint32_t reps = countRepeats(bytes, i);
            if (reps > 2) {
                if (bytes[i] == 0) {
                    if (reps <= 10) {
                        cm.addOccur(17);
                        i += reps;
                    } else {
                        cm.addOccur(18);
                        i += (reps <= 138) ? reps : 138;
                    }
                } else {
                    cm.addOccur(16);
                    i += (reps <= 6) ? reps : 6;
                }
            } else {
                cm.addOccur(bytes[i]);
                i++;
            }
        }
        std::vector<Code> t_codes = cm.generateCodes();
        for (auto& i : codes) {
            for (auto& j : t_codes) {
                if (i.value == j.value) {
                    i.code = j.code;
                    i.len = j.len;
                    j.extra_bits = i.extra_bits;
                }
            }
        }
        return HuffmanTree(t_codes);
    }

    void writeCodeLengthTree (Bitstream& bs) {
        uint32_t max = 4;
        for (uint32_t i = 4; i < codes.size(); i++) {
            if (codes[i].len > 0) {
                max = i;
            }
        }
        max = max - 4;
        bs.addBits(max, 4);
        for (uint32_t i = 0; i < max + 4; i++) {
            bs.addBits(codes[i].len, 3);
        }
        bs.addBits(0, 4);
    }
public:
    DynamicHuffLengthCompressor() {
        codes = {
            {0, 0, 2, 16},
            {0, 0, 3, 17},
            {0, 0, 7, 18},
            {0, 0, 0, 0},
            {0, 0, 0, 8},
            {0, 0, 0, 7},
            {0, 0, 0, 9},
            {0, 0, 0, 6},
            {0, 0, 0, 10},
            {0, 0, 0, 5},
            {0, 0, 0, 11},
            {0, 0, 0, 4},
            {0, 0, 0, 12},
            {0, 0, 0, 3},
            {0, 0, 0, 13},
            {0, 0, 0, 2},
            {0, 0, 0, 14},
            {0, 0, 0, 1},
            {0, 0, 0, 15},
        };
    }

    Bitstream compress (uint32_t len_max, HuffmanTree huff, HuffmanTree dist) {
        std::vector<uint8_t> bytes;
        std::vector<Code> huff_codes = huff.decode();
        // constructing raw bytes of the table
        struct compare_code_value {
            inline bool operator() (const Code& code1, const Code& code2) {
                return code1.value < code2.value;
            }
        };
        std::sort(huff_codes.begin(), huff_codes.end(), compare_code_value());
        for (uint32_t i = 0; i < 257 + len_max; i++) {
            bool write = false;
            for (auto& j : huff_codes) {
                if (((uint32_t)j.value) == i) {
                    bytes.push_back(j.len);
                    write = true;
                    break;
                }
            } 
            if (!write) {
                bytes.push_back(0);
            }
        }
        std::vector<Code> dist_codes = dist.decode();
        std::sort(dist_codes.begin(), dist_codes.end(), compare_code_value());
        uint32_t max = dist_codes[dist_codes.size() - 1].value;
        for (uint32_t i = 0; i < 1 + max; i++) {
            bool write = false;
            for (auto& j : dist_codes) {
                if (j.value == i) {
                    bytes.push_back(j.len);
                    write = true;
                    break;
                }
            }
            if (!write) {
                bytes.push_back(0);
            }
        }
        // constructing the code length huffman tree
        // 3 bits for each code length
        // start by computing how often each code length character will be used
        HuffmanTree code_tree = constructTree(bytes);
        Bitstream bs;
        writeCodeLengthTree(bs);
        // compressing the table
        // for each byte we loop forward and see how many times it's repeated
        // depending on many times repeated we determine the proper code to use for that length of bytes
        for (uint32_t i = 0; i < bytes.size();) {
            uint32_t reps = countRepeats(bytes, i);
            uint32_t code = 0;
            uint32_t inc = 1;
            if (reps > 2) {
                if (bytes[i] == 0) {
                    if (reps <= 10) {
                        code = 17;
                        inc = (reps < 10) ? reps : 10;
                    } else {
                        code = 18;
                        inc = (reps < 138) ? reps : 138; 
                    }
                } else {
                    code = 16;
                    inc = (reps <= 6) ? reps : 6;
                }
            } else {
                code = bytes[i];
            }
            Code c = code_tree.getCompressedCode(code);
            bs.addBits(c.code, c.len);
            if (c.extra_bits > 0) {
                uint32_t start = 3;
                if (c.value == 18) {
                    start = 11;
                }
                uint32_t val = reps - start;
                bs.addBits(val, c.extra_bits);
            }
            i += inc;
        }

        return bs;
    }
};


/*
     However, we describe the compressed block format
         in below, as a sequence of data elements of various bit
         lengths, not a sequence of bytes.  We must therefore specify
         how to pack these data elements into bytes to form the final
         compressed byte sequence:

             * Data elements are packed into bytes in order of
               increasing bit number within the byte, i.e., starting
               with the least-significant bit of the byte.
             * Data elements other than Huffman codes are packed
               starting with the least-significant bit of the data
               element.
             * Huffman codes are packed starting with the most-
               significant bit of the code.

         In other words, if one were to print out the compressed data as
         a sequence of bytes, starting with the first byte at the
         *right* margin and proceeding to the *left*, with the most-
         significant bit of each byte on the left as usual, one would be
         able to parse the result from right to left, with fixed-width
         elements in the correct MSB-to-LSB order and Huffman codes in
         bit-reversed order (i.e., with the first bit of the code in the
         relative LSB position).
*/
// so reading huffman codes we read left to right versus regular data which is the basic right to left bit read
// https://www.rfc-editor.org/rfc/rfc1951#page-6
//https://minitoolz.com/tools/online-deflate-inflate-decompressor/
//https://minitoolz.com/tools/online-deflate-compressor/
//https://github.com/madler/zlib

// https://www.cs.ucdavis.edu/~martel/122a/deflate.html
//implement deflate itself
    //-test implementation (use libdeflate or tinydeflate)
    //-fixed uncompressed buffers being unreadable
    //-need to fix compressed buffers being unreadable
    //-maybe order of bits is wrong?
//implement rest of inflate
//make it a command line utility
//add error checking and maybe test files lol
//optimize
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
        for (uint16_t code = 48; i < 144; i++, code++) {
            fixed_codes.push_back({code, 8, 0, i});
        }
        for (uint16_t code = 400; i < 256; i++, code++) {
            fixed_codes.push_back({code, 9, 0, i});
        }
        uint8_t extra_bits = 0;
        for(uint16_t code = 0; i < 280; i++, code++) {
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
            fixed_codes.push_back({code, 7, extra_bits, i});
        }
        for(uint16_t code = 192; i < 288; i++, code++) {
            switch (i) {
                case 281:
                    extra_bits = 5;
                break;
                case 285:
                    extra_bits = 0;
                break;
            }
            fixed_codes.push_back({code, 8, extra_bits, i});
        }
        return fixed_codes;
    }

    static HuffmanTree generateFixedHuffman () {
        std::vector <Code> fixed_codes = generateFixedCodes();
        return HuffmanTree(fixed_codes);
    }
    static std::vector <Code> generateFixedDistanceCodes () {
        std::vector <Code> fixed_codes;
        uint8_t extra_bits = 0;
        //regular alphabet
        for (uint16_t i = 0; i < 32; i++) {
            if (i >= 4) {
                extra_bits = (i / 2) - 1;
            } 
            fixed_codes.push_back({i, 5, extra_bits, i});
        }
        return fixed_codes;
    }

    static HuffmanTree generateFixedDistanceHuffman () {
        std::vector <Code> fixed_dist_codes = generateFixedDistanceCodes();
        return HuffmanTree(fixed_dist_codes);
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

    struct match_index_comp {
        inline bool operator() (const Match& first, const Match& second) {
            return first.offset < second.offset;
        }
    };


    static RangeLookup generateLengthLookup () {
        RangeLookup rl;
        rl.addRange({3, 3, 257});
        rl.addRange({4, 4, 258});
        rl.addRange({5, 5, 259});
        rl.addRange({6, 6, 260});
        rl.addRange({7, 7, 261});
        rl.addRange({8, 8, 262});
        rl.addRange({9, 9, 263});
        rl.addRange({10, 10, 264});
        rl.addRange({11, 12, 265});
        rl.addRange({13, 14, 266});
        rl.addRange({15, 16, 267});
        rl.addRange({17, 18, 268});
        rl.addRange({19, 22, 269});
        rl.addRange({23, 26, 270});
        rl.addRange({27, 30, 271});
        rl.addRange({31, 34, 272});
        rl.addRange({35, 42, 273});
        rl.addRange({43, 50, 274});
        rl.addRange({51, 58, 275});
        rl.addRange({59, 66, 276});
        rl.addRange({67, 82, 277});
        rl.addRange({83, 98, 278});
        rl.addRange({99, 114, 279});
        rl.addRange({115, 130, 280});
        rl.addRange({131, 162, 281});
        rl.addRange({163, 194, 282});
        rl.addRange({195, 226, 283});
        rl.addRange({227, 257, 284});
        rl.addRange({258, 258, 285});
        return rl;
    }

    static RangeLookup generateDistanceLookup () {
        RangeLookup rl;
        rl.addRange({1, 1, 0});
        rl.addRange({2, 2, 1});
        rl.addRange({3, 3, 2});
        rl.addRange({4, 4, 3});
        rl.addRange({5, 6, 4});
        rl.addRange({7, 8, 5});
        rl.addRange({9, 12, 6});
        rl.addRange({13, 16, 7});
        rl.addRange({17, 24, 8});
        rl.addRange({25, 32, 9});
        rl.addRange({33, 48, 10});
        rl.addRange({49, 64, 11});
        rl.addRange({65, 96, 12});
        rl.addRange({97, 128, 13});
        rl.addRange({129, 192, 14});
        rl.addRange({193, 256, 15});
        rl.addRange({257, 384, 16});
        rl.addRange({385, 512, 17});
        rl.addRange({513, 768, 18});
        rl.addRange({769, 1024, 19});
        rl.addRange({1025, 1536, 20});
        rl.addRange({1537, 2048, 21});
        rl.addRange({2049, 3072, 22});
        rl.addRange({3073, 4096, 23});
        rl.addRange({4097, 6144, 24});
        rl.addRange({6145, 8192, 25});
        rl.addRange({8193, 12288, 26});
        rl.addRange({12289, 16384, 27});
        rl.addRange({16385, 24576, 28});
        rl.addRange({24577, 32768, 29});
        return rl;
    }

    // deflate

    static Bitstream compressBuffer (std::vector<uint8_t>& buffer, std::vector<Match> matches, HuffmanTree tree, HuffmanTree dist_tree, uint32_t preamble, bool final) {
        // compress into huffman code format
        Bitstream bs;
        RangeLookup rl = generateLengthLookup();
        RangeLookup dl = generateDistanceLookup();
        // writing the block out to file
        uint32_t pre = (final) ? (preamble | 0b001) : preamble; 
        bs.addBits(pre, 3);
        bs.nextByteBoundary();
        if ((preamble & 0b100) == 4) {
            writeDynamicHuffmanTree(bs, matches, tree, dist_tree);
        }
        for (uint32_t i = 0; i < buffer.size(); i++) {
            if (matches.size() > 0 && i == matches[0].offset) {
                // output the code for the thing
                Range lookup = rl.lookup(matches[0].length);

                Code c = tree.getCompressedCode(lookup.code); //this is temp way to lookup 
                bs.addBits(c.code, c.len);
                // add extra bits to bitstream
                if (c.extra_bits > 0) {
                    uint32_t extra_bits = matches[0].length % lookup.start;
                    bs.addBits(extra_bits, c.extra_bits);
                }
                Range dist = dl.lookup(matches[0].dif);
                Code dic = dist_tree.getCompressedCode(dist.code);
                bs.addBits(dic.code, dic.len);
                if (dic.extra_bits > 0) {
                    uint32_t extra_bits = matches[0].dif % dist.start;
                    bs.addBits(extra_bits, dic.extra_bits);
                }
                matches.erase(matches.begin());
            } else {
                Code c = tree.getCompressedCode((uint32_t)buffer[i]);
                bs.addBits(c.code, c.len);
            }
        }
        Code endcode = tree.getCompressedCode(256);
        bs.addBits(endcode.code, endcode.len);
        return bs;
    }

    static Bitstream makeUncompressedBlock (std::vector<uint8_t>& buffer, bool final) {
        Bitstream bs;
        uint8_t pre = 0b000;
        if (final) {
            pre |= 1;
        }
        bs.addBits(pre, 3);
        bs.nextByteBoundary();
        bs.addBits(buffer.size(), 16);
        bs.addBits(~(buffer.size()), 16);
        bs.addRawBuffer(buffer);
        return bs;
    }

    static std::pair<HuffmanTree, HuffmanTree> constructDynamicHuffmanTree (std::vector<uint8_t>& buffer, std::vector<Match> matches) {
        CodeMap c_map;
        c_map.addOccur(256);
        CodeMap dist_codes;
        RangeLookup rl = generateLengthLookup();
        RangeLookup dl = generateDistanceLookup();
        for (uint32_t i = 0; i < buffer.size(); i++) {
            if (matches.size() > 0 && i == matches[0].offset) {
                Range lookup = rl.lookup(matches[0].length);
                Range dist = dl.lookup(matches[0].dif);
                c_map.addOccur(lookup.code);
                dist_codes.addOccur(dist.code);
                matches.erase(matches.begin());
            } else {
                c_map.addOccur(buffer[i]);
            }
        }
        HuffmanTree tree(c_map.generateCodes());
        HuffmanTree dist_tree(dist_codes.generateCodes());
        return std::pair<HuffmanTree, HuffmanTree> (tree, dist_tree);
    }

    static void writeDynamicHuffmanTree (Bitstream& bs, std::vector<Match>& matches, HuffmanTree tree, HuffmanTree dist_tree) {
        uint32_t matches_size = 0;
        if (matches.size() > 0) {
            matches_size = matches[0].length;
            for (auto& i : matches) {
                if (i.length > matches_size) {
                    matches_size = i.length;
                }
            }
        }
        // HLIT
        bs.addBits(matches_size, 5);
        std::vector<Code> tree_codes= tree.decode();
        std::vector<Code> dist_codes = dist_tree.decode();
        uint32_t dist_codes_size = 0;
        if (dist_codes.size() > 0) {
        dist_codes_size = dist_codes[0].value;
            for (auto& i : dist_codes) {
                if (i.value > dist_codes_size) {
                    dist_codes_size = i.value;
                }
            }
        }
        // HDIST
        bs.addBits(dist_codes_size, 5);
        // compress the code lengths and use that to generate the HCLEN
        DynamicHuffLengthCompressor compressor;
        bs.addRawBuffer(compressor.compress(matches_size, tree, dist_tree).getData());
    }
public:
    // not done
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

    // not done
    static size_t deflate (std::string file_path, std::string new_file) {
        std::streampos sp = getFileSize(file_path);
        std::ifstream fi;
        fi.open(file_path, std::ios::binary);
        if (!fi) {
            return 0;
        }
        std::ofstream of;
        of.open(new_file, std::ios::binary);
        if (!of) {
            return 0;
        }
        HuffmanTree fixed_dist_huffman = generateFixedDistanceHuffman();
        HuffmanTree fixed_huffman = generateFixedHuffman();
        uint8_t c;
        bool q = false;
        std::vector<uint8_t> buffer;
        size_t out_index = 0;
        while (!q) {
            std::streampos p = sp - fi.tellg();
            if (p > 0 && buffer.size() < 32768) {
                c = fi.get();
                buffer.push_back(c);
            } else {
                // writing the block out to file
                if (p <= 0) {
                    q = true;
                }
                LZ77 lz(32768);
                // finding the matches above length of 2
                std::vector<Match> matches = lz.findBufferRuns(buffer);
                std::sort(matches.begin(), matches.end(), match_index_comp());

                std::vector<uint8_t> output_buffer;
                std::pair<HuffmanTree, HuffmanTree> trees = constructDynamicHuffmanTree(buffer, matches);

                Bitstream bs_fixed = compressBuffer(buffer, matches, fixed_huffman, fixed_dist_huffman, 0b001, q);
                Bitstream bs_dynamic = compressBuffer(buffer, matches, trees.first, trees.second, 0b010, q);
                if (bs_fixed.getSize() < (buffer.size() + 5) && bs_fixed.getSize() < bs_dynamic.getSize()) {
                    output_buffer = bs_fixed.getData();
                } else if (bs_dynamic.getSize() < (buffer.size() + 5)) {
                    output_buffer = bs_dynamic.getData();
                } else {
                    output_buffer = makeUncompressedBlock(buffer, q).getData();
                }
                // compare size of bs
                for (auto& i : output_buffer) {
                    of << i;
                    out_index++;
                }
                buffer.clear();
            }
        }

        fi.close();
        of.close();
        return out_index;
    }

    static size_t deflate (char* data, size_t data_size, char* out_data, size_t out_data_size) {
        HuffmanTree fixed_dist_huffman = generateFixedDistanceHuffman();
        HuffmanTree fixed_huffman = generateFixedHuffman();
        uint8_t c;
        bool q = false;
        std::vector<uint8_t> buffer;
        size_t index = 0;
        size_t out_index = 0;
         while (!q) {
            for (; index < data_size && buffer.size() < 32768; index++) {
                buffer.push_back(data[index]);
            }
            // writing the block out to file
            if (index >= data_size) {
                q = true;
            }
            LZ77 lz(32768);
            // finding the matches above length of 2
            std::vector<Match> matches = lz.findBufferRuns(buffer);
            std::sort(matches.begin(), matches.end(), match_index_comp());

            std::vector<uint8_t> output_buffer;
            std::pair<HuffmanTree, HuffmanTree> trees = constructDynamicHuffmanTree(buffer, matches);

            Bitstream bs_fixed = compressBuffer(buffer, matches, fixed_huffman, fixed_dist_huffman, 0b010, q);
            Bitstream bs_dynamic = compressBuffer(buffer, matches, trees.first, trees.second, 0b100, q);
            Bitstream bs_out;
            if (bs_fixed.getSize() < (buffer.size() + 5) && bs_fixed.getSize() < bs_dynamic.getSize()) {
                bs_out = bs_fixed;
            } else if (bs_dynamic.getSize() < (buffer.size() + 5)) {
                bs_out = bs_fixed;
            } else {
                bs_out = makeUncompressedBlock(buffer, q);
            }
            // Bitstream bs_store = makeUncompressedBlock(buffer, q);
            // output_buffer = bs_store.getData();
            // compare size of bs
            output_buffer = bs_out.getData();
            for (int32_t i = 0; i < bs_out.getSize(); i++) {
                out_data[out_index] = output_buffer[i];
                out_index++;
            }
            buffer.clear();
        }
        return out_index;
    }
};