#include "analyser.h"

#include <climits>
#include <sstream>
namespace miniplc0 {
    std::pair<std::vector<Instruction>, std::optional<CompilationError>> Analyser::Analyse() {
        auto err = analyseProgram();
        if (err.has_value())
            return std::make_pair(std::vector<Instruction>(), err);
        else
            return std::make_pair(_instructions, std::optional<CompilationError>());
    }

    // <程序> ::= 'begin'<主过程>'end'
    std::optional<CompilationError> Analyser::analyseProgram() {
        // 示例函数，示例如何调用子程序
        auto err = c0Program();
        if (err.has_value())
            return err;
        return {};
    }

    //<C0-program> ::={<variable-declaration>}{<function-definition>}
    //CO项目
    std::optional<CompilationError> Analyser::c0Program() {
        auto var = variableDeclaration();
        if (var.has_value())
            return var;
        auto fuc = functionDefinition();
        if (fuc.has_value())
            return fuc;
        //没有主函数的情况
        if (!isFunction("main"))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoMainFunction);
        return {};
    }
    //基础C0，定义类型为int；
            //<variable-declaration> ::=[<const-qualifier>]<type-specifier><init-declarator-list>';'
            std::optional<CompilationError> Analyser::variableDeclaration() {
            std::string  type;
            while (true){
            type = "int";
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType()!=TokenType::CONST&&next.value().GetType()!=TokenType::INT){
                unreadToken();
                return {};
            }

            //const情况
            if (next.value().GetType()==TokenType::CONST){
                if(!next.has_value())
                    return {};
                type = "const_int";
            }
            else unreadToken();
            //int情况
            next = nextToken();
            if(!next.has_value()) {
                unreadToken();
                return {};
            }
            //判断是否可能是函数
            //
            next = nextToken();
            if(!next.has_value()) {
                unreadToken();
                return {};
            }
            next = nextToken();
            if(!next.has_value()) {
                unreadToken();
                return {};
            }
            unreadToken();
            unreadToken();
            //判断为函数
            if(next.value().GetType()==TokenType::LEFT_BRACKET){
                unreadToken();
                if(type=="const_int")
                    unreadToken();
                return {};
            }
            //initDeclarationList
            auto err = initDeclarationList(type);
            if(err.has_value())
                return err;

            //;
            next = nextToken();
            if(!next.has_value())
                return {};
            if (next.value().GetType() != TokenType::SEMICOLON)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            }
    }

    //<init-declarator-list> ::=<init-declarator>{','<init-declarator>}
    std::optional<CompilationError> Analyser::initDeclarationList(std::string type) {
        auto err = initDeclaration(type);
        if(err.has_value())
            return err;
        while(true){
            //,
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType()!=TokenType::COMMA){
                unreadToken();
                return {};
            }
            err = initDeclaration(type);
            if(err.has_value())
                return err;
        }
    }
    //
    /*


     <initializer> ::='='<expression>*/
    //<init-declarator> ::=<identifier>[<initializer>]
    std::optional<CompilationError> Analyser::initDeclaration(std::string type) {
        auto var = nextToken();
        if (!var.has_value() || var.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        if (isDeclared(var.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
        //预读，判断之后是否有表达式
        auto next = nextToken();
        unreadToken();

        if (next.value().GetType() != TokenType::EQUAL_SIGN &&
            next.value().GetType() != TokenType::SEMICOLON&&
                next.value().GetType() != TokenType::COMMA) {
            return {};
        }
        //初始化的情况
        if(next.value().GetType()==EQUAL_SIGN){
            //<初始化>
            auto err = initializer();
            if(err.has_value())
                return err;
            if(type=="const_int"){
                addConstant(var.value());
            }
            else{
                addVariable(var.value());
            }
        }
        else{
            //const未被初始化
            if(type=="const_int"){
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
            }
            else addUninitializedVariable(var.value());
        }

        return {};
    }
    //<initializer> ::='='<expression>
    std::optional<CompilationError> Analyser::initializer() {
        auto next = nextToken();
        if(next.value().GetType()!=TokenType::EQUAL_SIGN)
            return {};
        auto err = expression();
        if(err.has_value())
            return err;
        return {};
    }

    //函数定义
    std::optional<CompilationError> Analyser::functionDefinition() {
        while(true){
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType()!=TokenType::VOID&&next.value().GetType()!=TokenType::INT)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrFunctionHasNoReturnValue);
            //标识符
            //如果函数被定义，报错，否则添加到函数名表中
            auto var = nextToken();
            if (!var.has_value() || var.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if(isFunction(var.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrFunctionHasDefined);
            else addFunctions(var.value());
            //参数*/
            auto err = parameterClause();
            if(err.has_value())
                return err;
            //语句序列
            err = compoundStatement();
            if(err.has_value())
                return err;
        }

    }

    std::optional<CompilationError> Analyser::parameterClause() {
        //(
        auto next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType()!=TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //预读
        next = nextToken();
        unreadToken();
        if(!next.has_value())
            return {};
        //参数列表
        if(next.value().GetType()!=TokenType::RIGHT_BRACKET){
            auto err = parameterDeclarationList();
            if (err.has_value())
                return err;
        }
        //)
        else next = nextToken();
        return {};
    }
    //参数列表
    std::optional<CompilationError> Analyser::parameterDeclarationList() {
        auto err = parameterDeclaration();
        if(err.has_value())
            return err;
        while(true){
            //,
            auto next = nextToken();
            if(!next.has_value()){
                unreadToken();
                return {};
            }
            if(next.value().GetType()==TokenType::RIGHT_BRACKET)
                return {};
            if(next.value().GetType()!=TokenType::COMMA)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
            //函数声明
            err = parameterDeclaration();
            if(err.has_value())
                return err;
        }
    }
    //参数声明
    std::optional<CompilationError> Analyser::parameterDeclaration() {
        auto next = nextToken();
        if (!next.has_value())
            return {};
        //const情况
        if (next.value().GetType() == TokenType::CONST) {
            next = nextToken();
            if (!next.has_value())
                return {};
            if (next.value().GetType() != TokenType::INT) {
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
            }
        }
        //int情况
        else if (next.value().GetType() != TokenType::INT)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //标识符
        next = nextToken();
        if(!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        return {};
    }

    //函数调用
    std::optional<CompilationError> Analyser::functionCall() {
            //标识符
        auto next = nextToken();
        if(next.value().GetType()!=TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //(
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType()!=TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //表达式列表
        //预读
        next = nextToken();
        if(next.value().GetType()!=TokenType::RIGHT_BRACKET){
            auto err = expressionList();
            if(err.has_value())
                return err;
        }
        //)
        if(!next.has_value())
            return {};
        if(next.value().GetType()!=TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        return {};
    }
    //表达式列表
    std::optional<CompilationError> Analyser::expressionList() {
        auto err = expression();
        if(err.has_value())
            return err;
        while(true){
            //,
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType()!=TokenType::COMMA)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
            //expression
            err = expression();
            if(err.has_value())
                return err;
        }
    }

    //函数主体
    std::optional<CompilationError> Analyser::compoundStatement() {
        //{
        auto next = nextToken();
        if(!next.has_value())
            return {};

        if(next.value().GetType()!=TokenType::LEFT_LARGE_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //函数声明
        auto err = variableDeclaration();
        if(err.has_value())
            return err;

        //语句序列
        err = statementSeq();
        if(err.has_value())
            return err;

        //}
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType()!=TokenType::RIGHT_LARGE_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);

        return {};
    }

    //语句序列
    std::optional<CompilationError> Analyser::statementSeq() {
        while(true){
            auto next = nextToken();
            unreadToken();

            if( next.value().GetType() !=TokenType::LEFT_LARGE_BRACKET&&
                next.value().GetType() !=TokenType::RETURN&&
                next.value().GetType() !=TokenType::IF&&
                next.value().GetType() !=TokenType::WHILE&&
                next.value().GetType() !=TokenType::SCAN&&
                next.value().GetType() !=TokenType::PRINT&&
                next.value().GetType() !=TokenType::SEMICOLON&&
                next.value().GetType() !=TokenType::IDENTIFIER
                    )
             return {};

            auto err = statement();
            if(err.has_value())
                return err;
        }
    }
    std::optional<CompilationError> Analyser::statement() {
        std::optional<CompilationError> err;
        //预读
        auto next = nextToken();
        unreadToken();
        if (!next.has_value())
            return {};
        switch (next.value().GetType()) {
            //语句序列
            case TokenType::LEFT_LARGE_BRACKET: {
                auto err = statementSeq();
                if (err.has_value())
                    return err;
                next = nextToken();
                if (next.value().GetType() != TokenType::RIGHT_LARGE_BRACKET)
                    return {};
                break;
            }
            //返回语句
            case TokenType::RETURN: {
                unreadToken();
                err = jumpStatement();
                if (err.has_value())
                    return err;
                break;
            }
            //条件语句
            case TokenType::IF: {
                err = conditionStatement();
                if (err.has_value())
                    return err;
                break;
            }
            //while语句
            case TokenType::WHILE: {
                err = loopStatement();
                if (err.has_value())
                    return err;
                break;
            }
            //scan语句
            case TokenType::SCAN: {
                err = scanStatement();
                if (err.has_value())
                    return err;
                break;
            }
            //print语句
            case TokenType::PRINT: {
                err = printStatement();
                if (err.has_value())
                    return err;
                break;
            }
            case TokenType::IDENTIFIER: {
                err = assignmentExpression();
                if (err.has_value())
                    return err;
                //;
                next = nextToken();
                if(next.value().GetType()!=TokenType::SEMICOLON)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
                break;
            }
            //;
            case TokenType::SEMICOLON: {
                break;
            }
            default:
                break;
        }
        return {};
    }

    //while语句
    std::optional<CompilationError> Analyser::loopStatement() {
        //while
        auto next = nextToken();
        //(
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() !=TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //条件
        auto err = condition();
        if (err.has_value())
            return err;
        //)
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() !=TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //语句；
        err = statement();
        if (err.has_value())
            return err;
        return {};

    }
    //返回语句
    std::optional<CompilationError> Analyser::jumpStatement() {
        auto next = nextToken();
        next = nextToken();
        if(!next.has_value())
            return {};
        //返回值
        auto err = expression();
        if (err.has_value())
            return err;
        //;
        next = nextToken();
        if (next.value().GetType()!=TokenType::SEMICOLON){
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        }
        return {};
    }
    std::optional<CompilationError> Analyser::conditionStatement() {
        //if
        auto next = nextToken();
        //(
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() !=TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //条件
        auto err = condition();
        if (err.has_value())
            return err;
        //)
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() !=TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //if语句后接的语句
        err = statement();
        if( err.has_value())
            return err;
        //判断是否有else
        next = nextToken();
        if(next.value().GetType() ==TokenType::ELSE){
            err = statement();
            if (err.has_value())
                return err;
        }
        else unreadToken();
        return {};
    }
    //输出
    std::optional<CompilationError> Analyser::printStatement() {
        //print
        auto next = nextToken();
        //{
        next = nextToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() !=TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        next = nextToken();
        unreadToken();
        if(!next.has_value())
            return {};
        if(next.value().GetType() !=TokenType::RIGHT_BRACKET)
        {
            //输出列表
            auto err = printlist();
            if(err.has_value())
                return err;
        }
        //}
        next = nextToken();
        if(!next.has_value())
            return {};
        return {};
    }
    std::optional<CompilationError> Analyser::printlist() {
        auto err = printable();
        if(err.has_value())
            return err;
        while (true)
        {
            //,
            auto next = nextToken();
            if(!next.has_value())
                return {};
            if(next.value().GetType() !=TokenType::COMMA)
                return {};
            //printable
            err = printable();
            if(err.has_value())
                return err;
        }
    }
    std::optional<CompilationError> Analyser::printable() {
        auto err = expression();
        if(err.has_value())
            return err;
        return {};
    }
    //输入语句
    std::optional<CompilationError> Analyser::scanStatement() {
        auto next = nextToken();
        //(
        next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //标识符
        next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //)
        next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        //；
        next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::SEMICOLON) {
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        }
        return {};
    }
    /*<function-definition> ::=<type-specifier><identifier><parameter-clause><compound-statement>
    <parameter-clause> ::='(' [<parameter-declaration-list>] ')'<parameter-declaration-list> ::=<parameter-declaration>{','<parameter-declaration>}
    <parameter-declaration> ::=[<const-qualifier>]<type-specifier><identifier>
    <function-call> ::=<identifier> '(' [<expression-list>] ')'
    <expression-list> ::=<expression>{','<expression>}*/
    //函数声明

//    // <主过程> ::= <常量声明><变量声明><语句序列>
//    // 需要补全
//    std::optional<CompilationError> Analyser::analyseMain() {
//        // 完全可以参照 <程序> 编写
//
//        // <常量声明>
//        auto con = analyseConstantDeclaration();
//        if(con.has_value())
//            return con;
//        // <变量声明>
//        auto var = analyseVariableDeclaration();
//        if(var.has_value())
//            return var;
//        // <语句序列>
//        auto sta = analyseStatementSequence();
//        if(sta.has_value())
//            return sta;
//        return {};
//    }

    // <常量声明> ::= {<常量声明语句>}
    // <常量声明语句> ::= 'const'<标识符>'='<常表达式>';'
   /*std::optional<CompilationError> Analyser::analyseConstantDeclaration() {
        // 示例函数，示例如何分析常量声明

        // 常量声明语句可能有 0 或无数个
        while (true) {
            // 预读一个 token，不然不知道是否应该用 <常量声明> 推导
            auto next = nextToken();
            if (!next.has_value())
                return {};
            // 如果是 const 那么说明应该推导 <常量声明> 否则直接返回
            if (next.value().GetType() != TokenType::CONST) {
                unreadToken();
                return {};
            }

            // <常量声明语句>
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if (isDeclared(next.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
            addConstant(next.value());

            // '='
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);

            // <常表达式>
            int32_t val;
            auto err = analyseConstantExpression(val);
            if (err.has_value())
                return err;

            // ';'
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            // 生成一次 LIT 指令加载常量
            _instructions.emplace_back(Operation::LIT, val);
        }
        return {};
    }*/
    /*//定义类型
    std::optional<CompilationError> Analyser::analyseTypeSpecifier() {
        auto err = analyseTypeSpecifier();
        if(err.has_value())
            return err;
    }*/

    //简单定义类型
    /*std::optional<CompilationError> Analyser::analyseSimpleTypeSpecifier() {
        auto next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::VOID||next.value().GetType() != TokenType::UNSIGNED_INTEGER){
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecificType);
        }
    }*/


    // <变量声明> ::= {<变量声明语句>}
    // <变量声明语句> ::= 'var'<标识符>['='<表达式>]';'
    // 需要补全
    /*std::optional<CompilationError> Analyser::variableDeclaration() {
        // 变量声明语句可能有一个或者多个
        while (true) {
            // 预读一个 token，不然不知道是否应该用 <变量声明> 推导
            auto next = nextToken();
            if (!next.has_value())
                return {};
            // 如果是 int 那么说明应该推导 <变量声明> 否则直接返回
            if (next.value().GetType() != TokenType::INT) {
                unreadToken();
                return {};
            }
            // <标识符>
            auto var = nextToken();
            if (!var.has_value() || var.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if (isDeclared(next.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
            // 变量可能没有初始化，仍然需要一次预读
            //判断变量是否初始化
            next = nextToken();
            if (next.value().GetType() != TokenType::EQUAL_SIGN &&
                next.value().GetType() != TokenType::SEMICOLON) {
                return {};
            }
            // 初始化的情况
            if(next.value().GetType() == TokenType::EQUAL_SIGN){
                // '<表达式>'
                auto err = analyseExpression();
                if (err.has_value())
                    return err;
                next = nextToken();
                // ";"
                if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
                addVariable(var.value());
                _instructions.emplace_back(Operation::LIT, getIndex(var.value().GetValueString()));
                return {};
            }
                //未初始化的情况
            else{
                next = nextToken();
                if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
                addUninitializedVariable(var.value());
                _instructions.emplace_back(Operation::LIT, 0);
                return {};
            }
        }
    }*/
    //定义类型
    /*<type-specifier>         ::= <simple-type-specifier>
    <simple-type-specifier>  ::= 'void'|'int'
    <const-qualifier>        ::= 'const'*/

    //常量定义





    //表达式定义
    /*<assignment-expression> ::=<identifier><assignment-operator><expression>
    <condition> ::=<expression>[<relational-operator><expression>]
    <expression> ::=<additive-expression>
    <additive-expression> ::=<multiplicative-expression>{<additive-operator><multiplicative-expression>}
    <multiplicative-expression> ::=<unary-expression>{<multiplicative-operator><unary-expression>}
    <unary-expression> ::=[<unary-operator>]<primary-expression>
    <primary-expression> ::='('<expression>')'|<identifier>|<integer-literal>|<function-call>*/


    //赋值表达式
    std::optional<CompilationError> Analyser::assignmentExpression() {
        auto next = nextToken();
        if (!next.has_value())
            return {};
        //const不能被赋值
        if (next.value().GetType() != TokenType::IDENTIFIER){
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        }
        //const不能被赋值
        if (isConstant(next.value().GetValueString()))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstNotAssigned);
        next = nextToken();
        if (!next.has_value())
            return {};
        if (next.value().GetType() != TokenType::EQUAL_SIGN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        auto err = expression();
        if (err.has_value())
            return err;
        return {};
    }

    //表达式
    //<expression> ::= <additive-expression>
    std::optional<CompilationError> Analyser::expression(){
        auto err = additiveExpression();
        if (err.has_value())
            return err;
        return {};
    }
    std::optional<CompilationError> Analyser::condition() {
        auto err = expression();
        if (err.has_value())
            return err;
        //预读
        auto next = nextToken();
        unreadToken();
        if (!next.has_value())
            return {};
        //没有比较符的情况，直接返回
        if(next.value().GetType()==TokenType::RIGHT_BRACKET){
            return {};
        }
        else if (next.value().GetType()!= TokenType::LESS_THAN&&
                next.value().GetType()!= TokenType::GREAT_THAN&&
                next.value().GetType()!= TokenType::LESS_THAN&&
                next.value().GetType()!= TokenType::GREAT_EQUAL&&
                next.value().GetType()== TokenType::EQUAL_EQUAL&&
                next.value().GetType()== TokenType::NOR_EQUAL
                ) {
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        }
        else{
            //读入比较符
            next = nextToken();
            err = expression();
            if (err.has_value())
                return err;
            return {};
        }
    }
    //加法型表达式
    //<additive-expression> ::= <multiplicative-expression>{<additive-operator><multiplicative-expression>}
    std::optional<CompilationError> Analyser::additiveExpression() {
        auto err = multiplicativeExpression();
        if (err.has_value())
            return err;
        while(true){
            auto  next = nextToken();
            if (!next.has_value())
                return {};
            if (next.value().GetType()!= TokenType::PLUS_SIGN&&next.value().GetType()!= TokenType::MINUS_SIGN){
                unreadToken();
                return {};
            }
            err = multiplicativeExpression();
            if (err.has_value())
                return err;
            return {};
        }
    }
    //乘法型表达式
    //<multiplicative-expression> ::=   <unary-expression>{<multiplicative-operator><unary-expression>}
    std::optional<CompilationError> Analyser::multiplicativeExpression() {
        auto err = unaryExpression();
        if (err.has_value())
            return err;
        while(true){
            auto  next = nextToken();
            if (!next.has_value())
                return {};
            if (next.value().GetType()!= TokenType::MULTIPLICATION_SIGN&&next.value().GetType()!= TokenType::MINUS_SIGN){
                unreadToken();
                return {};
            }
            err = multiplicativeExpression();
            if (err.has_value())
                return err;
            return {};
        }
    }
    //一元表达式
    //<unary-expression> ::= [<unary-operator>]<primary-expression>
    std::optional<CompilationError> Analyser::unaryExpression() {
        auto  next = nextToken();
        if (next.value().GetType()!= TokenType::PLUS_SIGN&&next.value().GetType()!= TokenType::MINUS_SIGN){
            unreadToken();
        }
        auto err = primaryExpression();
        if (err.has_value())
            return err;
        return {};
    }
    //主表达式
    std::optional<CompilationError> Analyser::primaryExpression() {

        auto next = nextToken();
        if (!next.has_value())
            return {};
        if(next.value().GetType()!=IDENTIFIER&&
        next.value().GetType()!=LEFT_BRACKET&&
        next.value().GetType()!=INTEGER&&
        next.value().GetType()!=HEX_INT
        )
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
        switch (next.value().GetType()){
            case TokenType ::LEFT_BRACKET:{
                auto err = expression();
                if(err.has_value())
                    return err;
                //)
                next = nextToken();
                if (!next.has_value())
                    return {};
                if (next.value().GetType() != TokenType::RIGHT_BRACKET)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrLackSomething);
                break;
            }
            case TokenType ::IDENTIFIER:{
                next = nextToken();
                unreadToken();
                if(next.value().GetType()==TokenType::LEFT_BRACKET){
                    auto err = functionCall();
                    if (err.has_value())
                        return err;
                }
                break;
            }
            case TokenType ::INTEGER:{
                return {};
                break;
            }
            case TokenType ::HEX_INT:{
                return {};
                break;
            }
            default:
                break;
        }
        //(
        return {};
        //标识符
    }


    // <语句序列> ::= {<语句>}
    // <语句> :: = <赋值语句> | <输出语句> | <空语句>
    // <赋值语句> :: = <标识符>'='<表达式>';'
    // <输出语句> :: = 'print' '(' <表达式> ')' ';'
    // <空语句> :: = ';'
    // 需要补全
   /* std::optional<CompilationError> Analyser::analyseStatementSequence() {
        while (true) {
            // 预读
            auto next = nextToken();
            if (!next.has_value())
                return {};
            unreadToken();
            if (next.value().GetType() != TokenType::IDENTIFIER &&
                next.value().GetType() != TokenType::PRINT &&
                next.value().GetType() != TokenType::SEMICOLON) {
                return {};
            }
            std::optional<CompilationError> err;
            switch (next.value().GetType()) {
                case TokenType::IDENTIFIER:{
                    auto err = analyseAssignmentStatement();
                    if (err.has_value())
                        return err;
                    break;
                }
                case TokenType::PRINT:{
                    auto err = analyseOutputStatement();
                    if (err.has_value())
                        return err;
                    break;
                }
                case TokenType::SEMICOLON:{
                    return {};
                    break;
                }
                    // 这里需要你针对不同的预读结果来调用不同的子程序
                    // 注意我们没有针对空语句单独声明一个函数，因此可以直接在这里返回
                default:
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
                    break;
            }
        }
        return {};
    }*/

    /*// <常表达式> ::= [<符号>]<无符号整数>
    // 需要补全
    std::optional<CompilationError> Analyser::analyseConstantExpression(int32_t& out) {
        std::stringstream s;
        std::string str;
        int num;
        auto next = nextToken();
        if (!next.has_value())
            return {};
        switch (next.value().GetType()) {
            case TokenType::UNSIGNED_INTEGER: {
                s << next.value().GetValueString();
                s >> num;
                out = std::any_cast<int32_t>(num);
                break;
            }
            case TokenType::MINUS_SIGN: {
                next = nextToken();
                s << next.value().GetValueString();
                s >> num;
                out = std::any_cast<int32_t>(next.value().GetValue());
                break;
            }
            case TokenType::PLUS_SIGN: {
                next = nextToken();
                s << next.value().GetValueString();
                s >> num;
                out = std::any_cast<int32_t>(next.value().GetValue());
                break;
            }

            default:{
                return {};
            }
        }*/

        // out 是常表达式的结果
        //		// 这里你要分析常表达式并且计算结果
        //		// 注意以下均为常表达式
        //		// +1 -1 1
        //		// 同时要注意是否溢出
    /*    return {};
    }

    // <表达式> ::= <项>{<加法型运算符><项>}
    std::optional<CompilationError> Analyser::analyseExpression() {
        // <项>
        auto err = analyseItem();
        if (err.has_value())
            return err;

        // {<加法型运算符><项>}
        while (true) {
            // 预读
            auto next = nextToken();
            if (!next.has_value())
                return {};
            auto type = next.value().GetType();
            if (type != TokenType::PLUS_SIGN && type != TokenType::MINUS_SIGN) {
                unreadToken();
                return {};
            }

            // <项>
            err = analyseItem();
            if (err.has_value())
                return err;

            // 根据结果生成指令
            if (type == TokenType::PLUS_SIGN)
                _instructions.emplace_back(Operation::ADD, 0);
            else if (type == TokenType::MINUS_SIGN)
                _instructions.emplace_back(Operation::SUB, 0);
        }
        return {};
    }

    // <赋值语句> ::= <标识符>'='<表达式>';'
    // 需要补全
    std::optional<CompilationError> Analyser::analyseAssignmentStatement() {
        //标识符
        auto var = nextToken();
        if (!var.has_value() || var.value().GetType() != TokenType::IDENTIFIER)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
        //"="
        auto next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
        //表达式
        auto err = analyseExpression();
        if (err.has_value())
            return err;
        //";"
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
        _instructions.emplace_back(Operation::STO, getIndex(var.value().GetValueString()));
        // 这里除了语法分析以外还要留意
        // 标识符声明过吗？
        // 标识符是常量吗？
        // 需要生成指令吗？
        return {};
    }

    // <输出语句> ::= 'print' '(' <表达式> ')' ';'
    std::optional<CompilationError> Analyser::analyseOutputStatement() {
        // 如果之前 <语句序列> 的实现正确，这里第一个 next 一定是 TokenType::PRINT
        auto next = nextToken();

        // '('
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

        // <表达式>
        auto err = analyseExpression();
        if (err.has_value())
            return err;

        // ')'
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

        // ';'
        next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

        // 生成相应的指令 WRT
        _instructions.emplace_back(Operation::WRT, 0);
        return {};
    }

    // <项> :: = <因子>{ <乘法型运算符><因子> }
    // 需要补全
    std::optional<CompilationError> Analyser::analyseItem() {
        // 可以参考 <表达式> 实现
        // <因子>
        auto err = analyseFactor();
        if (err.has_value())
            return err;
        // {<乘法型运算符><项>}
        while (true) {
            // 预读
            auto next = nextToken();
            if (!next.has_value())
                return {};
            auto type = next.value().GetType();
            if (type != TokenType::MULTIPLICATION_SIGN && type != TokenType::DIVISION_SIGN) {
                unreadToken();
                return {};
            }

            // 因子>
            err = analyseFactor();
            if (err.has_value())
                return err;

            // 根据结果生成指令
            if (type == TokenType::MULTIPLICATION_SIGN)
                _instructions.emplace_back(Operation::MUL, 0);
            else if (type == TokenType::DIVISION_SIGN)
                _instructions.emplace_back(Operation::DIV, 0);
        }
        return {};
    }

    // <因子> ::= [<符号>]( <标识符> | <无符号整数> | '('<表达式>')' )
    // 需要补全
    std::optional<CompilationError> Analyser::analyseFactor() {
        // [<符号>]
        auto next = nextToken();
        auto prefix = 1;
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        if (next.value().GetType() == TokenType::PLUS_SIGN)
            prefix = 1;
        else if (next.value().GetType() == TokenType::MINUS_SIGN) {
            prefix = -1;
            _instructions.emplace_back(Operation::LIT, 0);
        }
        else
            unreadToken();

        // 预读
        next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        if (next.value().GetType() != TokenType::IDENTIFIER &&
            next.value().GetType() != TokenType::UNSIGNED_INTEGER &&
            next.value().GetType() != TokenType::LEFT_BRACKET) {
            return {};
        }
        switch (next.value().GetType()) {
            case IDENTIFIER:{
                if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
                _instructions.emplace_back(Operation::LOD, getIndex(next.value().GetValueString()));
                ;
                break;
            }
            case UNSIGNED_INTEGER:{
                _instructions.emplace_back(Operation::LOD, getIndex(next.value().GetValueString()));
                break;
            }
            case LEFT_BRACKET:{
                //表达式
                next = nextToken();
                auto err = analyseExpression();
                if (err.has_value())
                    return err;
                //")"
                next = nextToken();
                if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);
                break;
            }
                // 这里和 <语句序列> 类似，需要根据预读结果调用不同的子程序
                // 但是要注意 default 返回的是一个编译错误
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
        }

        // 取负
        if (prefix == -1)
            _instructions.emplace_back(Operation::SUB, 0);
        return {};
    }*/

    std::optional<Token> Analyser::nextToken() {
        if (_offset == _tokens.size())
            return {};
        // 考虑到 _tokens[0..._offset-1] 已经被分析过了
        // 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
        _current_pos = _tokens[_offset].GetEndPos();
        return _tokens[_offset++];
    }

    void Analyser::unreadToken() {
        if (_offset == 0)
            DieAndPrint("analyser unreads token from the begining.");
        _current_pos = _tokens[_offset - 1].GetEndPos();
        _offset--;
    }

    void Analyser::_add(const Token& tk, std::map<std::string, int32_t>& mp) {
        if (tk.GetType() != TokenType::IDENTIFIER)
            DieAndPrint("only identifier can be added to the table.");
        mp[tk.GetValueString()] = _nextTokenIndex;
        _nextTokenIndex++;
    }

    void Analyser::addVariable(const Token& tk) {
        _add(tk, _vars);
    }

    void Analyser::addConstant(const Token& tk) {
        _add(tk, _consts);
    }

    void Analyser::addUninitializedVariable(const Token& tk) {
        _add(tk, _uninitialized_vars);
    }

    void Analyser::addFunctions(const Token& tk) {
        _add(tk, _functions);
    }
    //添加函数名
    /*void Analyser::addFunctions(const Token& tk) {
        _add(tk, _functions);
    }*/
    int32_t Analyser::getIndex(const std::string& s) {
        if (_uninitialized_vars.find(s) != _uninitialized_vars.end())
            return _uninitialized_vars[s];
        else if (_vars.find(s) != _vars.end())
            return _vars[s];
        else
            return _consts[s];
    }

    bool Analyser::isDeclared(const std::string& s) {
        return isConstant(s) || isUninitializedVariable(s) || isInitializedVariable(s);
    }

    bool Analyser::isUninitializedVariable(const std::string& s) {
        return _uninitialized_vars.find(s) != _uninitialized_vars.end();
    }
    bool Analyser::isInitializedVariable(const std::string&s) {
        return _vars.find(s) != _vars.end();
    }

    bool Analyser::isConstant(const std::string&s) {
        return _consts.find(s) != _consts.end();
    }
    bool Analyser::isFunction(const std::string&s){
        return _functions.find(s) != _functions.end();
    }
}