#include "catch2/catch.hpp"
#include "tokenizer/tokenizer.h"
#include "fmt/core.h"

#include <sstream>
#include <vector>

// 下面是示例如何书写测试用例
TEST_CASE("Test hello world.") {

    std::string input =
            /*"BEGIN\n"
            "END\n"
            "VAR\n"
            "CONST\n"
            "PRINT +\n"
            "123-/*=();\n"*/
            //"2147483648"
            "begin\n"
            "var a=0;\n"
            "a = 2*5*10;\n"
            "print(a);\n"
            "end\n";
    ;
    std::stringstream ss;
    ss.str(input);
    miniplc0::Tokenizer tkz(ss);
    std::vector<miniplc0::Token> output = {
            miniplc0::Token(miniplc0::BEGIN,std::string("BEGIN"),std::pair(0,0),std::pair(0,5)),
            miniplc0::Token(miniplc0::END,std::string("END"),std::pair(1,0),std::pair(1,3)),
            miniplc0::Token(miniplc0::VAR,std::string("VAR"),std::pair(2,0),std::pair(2,3)),
            miniplc0::Token(miniplc0::CONST,std::string("CONST"),std::pair(3,0),std::pair(3,5)),
            miniplc0::Token(miniplc0::PRINT,std::string("PRINT"),std::pair(4,0),std::pair(4,5)),
            miniplc0::Token(miniplc0::PLUS_SIGN,std::string("+"),std::pair(4,6),std::pair(4,7)),
            miniplc0::Token(miniplc0::UNSIGNED_INTEGER,123,std::pair(5,0),std::pair(5,3)),
            miniplc0::Token(miniplc0::MINUS_SIGN,std::string("-"),std::pair(5,3),std::pair(5,4)),
            miniplc0::Token(miniplc0::DIVISION_SIGN,std::string("/"),std::pair(5,4),std::pair(5,5)),
            miniplc0::Token(miniplc0::MULTIPLICATION_SIGN,std::string("*"),std::pair(5,5),std::pair(5,6)),
            miniplc0::Token(miniplc0::EQUAL_SIGN,std::string("="),std::pair(5,6),std::pair(5,7)),
            miniplc0::Token(miniplc0::LEFT_BRACKET,std::string("("),std::pair(5,7),std::pair(5,8)),
            miniplc0::Token(miniplc0::RIGHT_BRACKET,std::string(")"),std::pair(5,8),std::pair(5,9)),
            miniplc0::Token(miniplc0::SEMICOLON,std::string(";"),std::pair(5,9),std::pair(5,10))
            //miniplc0::Token(miniplc0::UNSIGNED_INTEGER,2147483647,std::pair(0,0),std::pair(0,10)),
    };
    auto result = tkz.AllTokens();
    if (result.second.has_value()) {
        FAIL();
    }
    REQUIRE( (result.first == output) );


}