#include <iostream>
#include <fstream>
#include <vector>

#include "lexer.hpp"

int main(int argc, char** argv) {
//    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
//    std::streamsize size = file.tellg();
//    file.seekg(0, std::ios::beg);
//
//    std::vector<char> buffer(size);
//    if (!file.read(buffer.data(), size))
//        return 65;
//
//    for (auto i : buffer) {
//        std::cout << i;
//    }
//
//    std::cout << std::endl;
//    std::cout << "it worked" << std::endl;

    std::vector<char> input{ '+' };
    for (auto c : input)
        std::cout << c << std::endl;
    Lexer lexer{&input, 0};
    std::cout << name(lexer.next().kind) << std::endl;

    return 0;
}
