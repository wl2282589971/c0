#include "tokenizer/tokenizer.h"

#include <cctype>
#include <sstream>
#include <cstring>

namespace miniplc0 {

	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
		if (!_initialized)
			readAll();
		if (_rdr.bad())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
		if (isEOF())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
		auto p = nextToken();
		if (p.second.has_value())
			return std::make_pair(p.first, p.second);
		auto err = checkToken(p.first.value());
		if (err.has_value())
			return std::make_pair(p.first, err.value());
		return std::make_pair(p.first, std::optional<CompilationError>());
	}

	std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
		std::vector<Token> result;
		while (true) {
			auto p = NextToken();
			if (p.second.has_value()) {
				if (p.second.value().GetCode() == ErrorCode::ErrEOF)
					return std::make_pair(result, std::optional<CompilationError>());
				else
					return std::make_pair(std::vector<Token>(), p.second);
			}
			result.emplace_back(p.first.value());
		}
	}

	// 注意：这里的返回值中 Token 和 CompilationError 只能返回一个，不能同时返回。
	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
		// 用于存储已经读到的组成当前token字符
		std::stringstream ss;
		// 分析token的结果，作为此函数的返回值
		std::pair<std::optional<Token>, std::optional<CompilationError>> result;
		// <行号，列号>，表示当前token的第一个字符在源代码中的位置
		std::pair<int64_t, int64_t> pos;
		// 记录当前自动机的状态，进入此函数时是初始状态
		DFAState current_state = DFAState::INITIAL_STATE;
		// 这是一个死循环，除非主动跳出
		// 每一次执行while内的代码，都可能导致状态的变更
		while (true) {
			// 读一个字符，请注意auto推导得出的类型是std::optional<char>
			// 这里其实有两种写法
			// 1. 每次循环前立即读入一个 char
			// 2. 只有在可能会转移的状态读入一个 char
			// 因为我们实现了 unread，为了省事我们选择第一种
			auto current_char = nextChar();
			// 针对当前的状态进行不同的操作
			switch (current_state) {

				// 初始状态
				// 这个 case 我们给出了核心逻辑，但是后面的 case 不用照搬。
			case INITIAL_STATE: {
				// 已经读到了文件尾
				if (!current_char.has_value())
					// 返回一个空的token，和编译错误ErrEOF：遇到了文件尾
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrEOF));

				// 获取读到的字符的值，注意auto推导出的类型是char
				auto ch = current_char.value();
				// 标记是否读到了不合法的字符，初始化为否
				auto invalid = false;

				// 使用了自己封装的判断字符类型的函数，定义于 tokenizer/utils.hpp
				// see https://en.cppreference.com/w/cpp/string/byte/isblank
				if (miniplc0::isspace(ch)) // 读到的字符是空白字符（空格、换行、制表符等）
					current_state = DFAState::INITIAL_STATE; // 保留当前状态为初始状态，此处直接break也是可以的
				else if (!miniplc0::isprint(ch)) // control codes and backspace
					invalid = true;
				else if (miniplc0::isdigit(ch)) // 读到的字符是数字
					current_state = DFAState::UNSIGNED_INTEGER_STATE; // 切换到无符号整数的状态
				else if (miniplc0::isalpha(ch)) // 读到的字符是英文字母
					current_state = DFAState::IDENTIFIER_STATE; // 切换到标识符的状态
				else {
					switch (ch) {
                        case '=': // 如果读到的字符是`=`，则切换到等于号的状态
                            current_state = DFAState::EQUAL_SIGN_STATE;
                            break;
                        case '-':
                            // 请填空：切换到减号的状态
                            current_state = DFAState::MINUS_SIGN_STATE;
                            break;
                        case '+':
                            // 请填空：切换到加号的状态
                            current_state = DFAState::PLUS_SIGN_STATE;
                            break;
                        case '*':
                            // 请填空：切换状态
                            current_state = DFAState::MULTIPLICATION_SIGN_STATE;
                            break;
                        case '/':
                            // 请填空：切换状态
                            current_state = DFAState::DIVISION_SIGN_STATE;
                            break;
                            ///// 请填空：
                            ///// 对于其他的可接受字符
                            ///// 切换到对应的状态
                        case ';':
                            current_state = DFAState::SEMICOLON_STATE;
                            break;
                        case '(':
                            current_state = DFAState::LEFTBRACKET_STATE;
                            break;
                        case ')':
                            current_state = DFAState::RIGHTBRACKET_STATE;
                            break;
                        /*case '[':
                            current_state = DFAState::LEFTBRACKET_MID_STATE;
                            break;
                        case ']':*/
                            current_state = DFAState::RIGHTBRACKET_MID_STATE;
                            break;
                        case '{':
                            current_state = DFAState::LEFTBRACKET_LARGE_STATE;
                            break;
                        case '}':
                            current_state = DFAState::RIGHTBRACKET_LARGE_STATE;
                            break;
                        case '>':
                            current_state = DFAState::GREAT_THAN_STATE;
                            break;
                        case '<':
                            current_state = DFAState::LESS_THAN_STATE;
                            break;
                        case ',':
                            current_state = DFAState::COMMA_STATE;
                            break;
                        case '!':
                            current_state = DFAState::NOR_STATE;
                            break;

					// 不接受的字符导致的不合法的状态
					default:
						invalid = true;
						break;
					}
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE)
					pos = previousPos(); // 记录该字符的的位置为token的开始位置
				// 读到了不合法的字符
				if (invalid) {
					// 回退这个字符
					unreadLast();
					// 返回编译错误：非法的输入
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE) // ignore white spaces
					ss << ch; // 存储读到的字符
				break;
			}

								// 当前状态是无符号整数
			case UNSIGNED_INTEGER_STATE: {
                auto ch = current_char.value();
                std::string token;
				// 请填空：
                // 如果当前已经读到了文件尾，则解析已经读到的字符串为整数
                //     解析成功则返回无符号整数类型的token，否则返回编译错误
                if(miniplc0::isdigit(ch)||miniplc0::isalpha(ch)) {
                    ss << ch;
                }
                else{
                    unreadLast();
                    ss>>token;
                    int len = token.size();
                    bool sign = true;
                    //判断是否为16进制数
                    if(token[0]=='0'&&(token[1]=='x'||token[len-1]=='X')){
                        for(int i=0;i<len-2;i++){
                            if(!(miniplc0::isdigit(token[i])||(token[i]>='a'&&token[i]<='f')||(token[i]>='A'&&token[i]<='F')))
                            {
                                sign = false;
                                break;
                            }
                        }
                        if(sign){
                            token = token.substr(2,len-2);
                            return std::make_pair(std::make_optional<Token>(TokenType::HEX_INT, token, pos, currentPos()), std::optional<CompilationError>());
                        }

                        else
                            return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
                    }
                    else{
                        for(int i=0;i<len-2;i++){
                            if(!(miniplc0::isdigit(token[i])))
                            {
                                sign = false;
                                break;
                            }
                        }
                        if(sign)
                            return std::make_pair(std::make_optional<Token>(TokenType::INTEGER, std::atoi(token.c_str()), pos, currentPos()), std::optional<CompilationError>());


                        else
                            return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
                    }
                }


                // 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
                //     解析成功则返回无符号整数类型的token，否则返回编译错误
				break;
			}
			//处理标识符以及字母开头十六进制的情况。
			case IDENTIFIER_STATE: {
                   // 请填空：
                   auto ch=current_char.value();
                   // 如果当前已经读到了文件尾，则解析已经读到的字符串
                   //     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
                   if(miniplc0::isdigit(ch)||miniplc0::isalpha(ch)){
                       ss << ch;
                   }
                       // 如果读到的是字符或字母，则存储读到的字符
                   else{
                    unreadLast();
                    std::string token;
                    ss >> token;
                    if(token=="const")
                        return std::make_pair(std::make_optional<Token>(TokenType::CONST, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="void")
                        return std::make_pair(std::make_optional<Token>(TokenType::VOID, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="int")
                        return std::make_pair(std::make_optional<Token>(TokenType::INT, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="char")
                        return std::make_pair(std::make_optional<Token>(TokenType::CHAR, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="double")
                        return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="struct")
                        return std::make_pair(std::make_optional<Token>(TokenType::STRUCT, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="if")
                        return std::make_pair(std::make_optional<Token>(TokenType::IF, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="else")
                        return std::make_pair(std::make_optional<Token>(TokenType::ELSE, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="switch")
                        return std::make_pair(std::make_optional<Token>(TokenType::SWITCH, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="case")
                        return std::make_pair(std::make_optional<Token>(TokenType::CASE, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="default")
                        return std::make_pair(std::make_optional<Token>(TokenType::DEFAULT, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="while")
                        return std::make_pair(std::make_optional<Token>(TokenType::WHILE, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="for")
                        return std::make_pair(std::make_optional<Token>(TokenType::FOR, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="do")
                        return std::make_pair(std::make_optional<Token>(TokenType::DO, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="return")
                        return std::make_pair(std::make_optional<Token>(TokenType::RETURN, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="break")
                        return std::make_pair(std::make_optional<Token>(TokenType::BREAK, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="continue")
                        return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="print")
                        return std::make_pair(std::make_optional<Token>(TokenType::PRINT, token, pos, currentPos()), std::optional<CompilationError>());
                    else if(token=="scan")
                        return std::make_pair(std::make_optional<Token>(TokenType::SCAN, token, pos, currentPos()), std::optional<CompilationError>());
                    else return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, token, pos, currentPos()), std::optional<CompilationError>());
                }
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串
				//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
				break;
			}

								   // 如果当前状态是加号
			case PLUS_SIGN_STATE: {
				// 请思考这里为什么要回退，在其他地方会不会需要
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()), std::optional<CompilationError>());
			}
								  // 当前状态为减号的状态
			case MINUS_SIGN_STATE:{
				// 请填空：回退，并返回减号token
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()), std::optional<CompilationError>());
			}
			case MULTIPLICATION_SIGN_STATE:{
                unreadLast();
                return std::make_pair(std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, '*', pos, currentPos()), std::optional<CompilationError>());
			}
			case DIVISION_SIGN_STATE:{
			    unreadLast();
			    return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()), std::optional<CompilationError>());
			}

			case EQUAL_SIGN_STATE:{
                char ch = current_char.value();
                std::string token;
                if (ch == '='){
                    return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_EQUAL, '==', pos, currentPos()), std::optional<CompilationError>());
                }
                else{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, '=', pos, currentPos()), std::optional<CompilationError>());
                }
			}
			case SEMICOLON_STATE:{
			    unreadLast();
			    return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, ';', pos, currentPos()), std::optional<CompilationError>());
			}
			case LEFTBRACKET_STATE:{
			    unreadLast();
			    return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, '(', pos, currentPos()), std::optional<CompilationError>());
			}
			case RIGHTBRACKET_STATE:{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, ')', pos, currentPos()), std::optional<CompilationError>());
			}
			/*case LEFTBRACKET_MID_STATE:{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::LEFT_MID_BRACKET, '[', pos, currentPos()), std::optional<CompilationError>());
			}
			case RIGHTBRACKET_MID_STATE:{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_MID_BRACKET, ']', pos, currentPos()), std::optional<CompilationError>());
			}*/
			case LEFTBRACKET_LARGE_STATE:{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::LEFT_LARGE_BRACKET, '{', pos, currentPos()), std::optional<CompilationError>());
			}
			case RIGHTBRACKET_LARGE_STATE:{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_LARGE_BRACKET, '}', pos, currentPos()), std::optional<CompilationError>());
			}
			case GREAT_THAN_STATE:{
                char ch = current_char.value();
                std::string token;
                if (ch == '='){
                    return std::make_pair(std::make_optional<Token>(TokenType::GREAT_EQUAL, '>=', pos, currentPos()), std::optional<CompilationError>());
                }
                else{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::GREAT_THAN, '>', pos, currentPos()), std::optional<CompilationError>());
                }
			}
			case LESS_THAN_STATE:{
                char ch = current_char.value();
                std::string token;
                if (ch == '='){
                    return std::make_pair(std::make_optional<Token>(TokenType::LESS_EQUAL, '<=', pos, currentPos()), std::optional<CompilationError>());
                }
                else{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::LESS_THAN, '<', pos, currentPos()), std::optional<CompilationError>());
                }
			}
			case COMMA_STATE:{
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::COMMA, ',', pos, currentPos()), std::optional<CompilationError>());
			}
			case NOR_STATE:{
			    char ch = current_char.value();
			    std::string token;
			    if (ch == '='){
                    return std::make_pair(std::make_optional<Token>(TokenType::NOR_EQUAL, '!=', pos, currentPos()), std::optional<CompilationError>());
			    }
			    else{
			        unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::NOR, '!', pos, currentPos()), std::optional<CompilationError>());
			    }
			}







								   // 请填空：
								   // 对于其他的合法状态，进行合适的操作
								   // 比如进行解析、返回token、返回编译错误

								   // 预料之外的状态，如果执行到了这里，说明程序异常
			default:{
                DieAndPrint("unhandled state.");
                break;
			}

			}
		}
		// 预料之外的状态，如果执行到了这里，说明程序异常
		return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
	}

	std::optional<CompilationError> Tokenizer::checkToken(const Token& t) {
		switch (t.GetType()) {
			case IDENTIFIER: {
				auto val = t.GetValueString();
				if (miniplc0::isdigit(val[0]))
					return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second, ErrorCode::ErrInvalidIdentifier);
				break;
			}
		default:
			break;
		}
		return {};
}

void Tokenizer::readAll() {
    if (_initialized)
        return;
    for (std::string tp; std::getline(_rdr, tp);)
        _lines_buffer.emplace_back(std::move(tp + "\n"));
    _initialized = true;
    _ptr = std::make_pair<int64_t, int64_t>(0, 0);
    return;
}

	// Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
	std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
		if (_ptr.first >= _lines_buffer.size())
			DieAndPrint("advance after EOF");
		if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
			return std::make_pair(_ptr.first + 1, 0);
		else
			return std::make_pair(_ptr.first, _ptr.second + 1);
	}

	std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
		return _ptr;
	}

	std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
		if (_ptr.first == 0 && _ptr.second == 0)
			DieAndPrint("previous position from beginning");
		if (_ptr.second == 0)
			return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
		else
			return std::make_pair(_ptr.first, _ptr.second - 1);
	}

	std::optional<char> Tokenizer::nextChar() {
		if (isEOF())
			return {}; // EOF
		auto result = _lines_buffer[_ptr.first][_ptr.second];
		_ptr = nextPos();
		return result;
	}

	bool Tokenizer::isEOF() {
		return _ptr.first >= _lines_buffer.size();
	}

	// Note: Is it evil to unread a buffer?
	void Tokenizer::unreadLast() {
		_ptr = previousPos();
	}

}