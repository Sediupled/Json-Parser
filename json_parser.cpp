#include <iostream>
#include <ostream>
#include <variant>
#include <unordered_map>
#include <map>
#include <vector>
#include <fstream>
#include <memory>
#include <filesystem>
#include <stdexcept>
#include <regex>

/*Token Types*/
enum TokenType{
    STRING,
    NUMBER,
    BOOLEAN,
    NULLTYPE,
    NAME,
    EOFTYPE,
    OBJECT,
    ARRAY,
    NAMESEP,
    VALSEP,
    ENDOBJ,
    ENDARR
};

int numAllocs = 0;
int numTokenCopies = 0;
int numTokenMoves = 0;

void* operator new (std::size_t count)
{
    numAllocs++;

    return malloc(count);
}
struct JsonObj;
struct JsonArray;
using JsonVal = std::variant<std::string, double, long long, bool, std::nullptr_t, std::unique_ptr<JsonObj>, std::unique_ptr<JsonArray>>;

/*Json Structured Types, ts might be jank not sure yet*/
struct JsonObj{
    std::unordered_map<std::string,JsonVal> contents;
    int indentVal;
    friend std::ostream& operator<<(std::ostream& os, const JsonObj& obj);
};

struct JsonArray{
    std::map<int,JsonVal> contents;
    int indentVal;
    friend std::ostream& operator<<(std::ostream& os, const JsonArray& arr);
};


/*Token Class*/
class Token{
    public:
        TokenType t_type;
        JsonVal t_val;

        Token(TokenType type,JsonVal&& val) : t_type(type), t_val(std::move(val)){}
        Token(Token& token) : t_type(token.t_type), t_val(std::move(token.t_val)){
            numTokenCopies++;
        }
        Token(Token&& token) noexcept : t_type(token.t_type), t_val(std::move(token.t_val)){
            numTokenMoves++;
        }

        Token& operator=(Token& other){
            if(&other == this)
            {
                return *this;
            }
            t_type = other.t_type;
            t_val = std::move(other.t_val);
            numTokenCopies++;
            return *this;
        }
        Token& operator=(Token&& other) noexcept {
            if(&other == this)
            {
                return *this;
            }
            t_type = other.t_type;
            t_val = std::move(other.t_val);
            other.t_val = nullptr;
            numTokenMoves++;
            return *this;
        }

        
};

inline std::ostream& operator<<(std::ostream& os, const JsonObj& obj){
    os << "{" << "\n";
    for (const auto& [namestr,value]: obj.contents){
        for (int i = 0; i<obj.indentVal;i++){
            os << " ";
        }
        os << namestr << ":";
        std::visit([&os](const auto& arg){ 
                if constexpr(requires {*arg;}){
                    if (arg) os << *arg <<"\n";
                }else{
                    os << arg << "\n";
                }
            }, value);
    }
    if(obj.indentVal > 2){
        for (int i = 0; i<obj.indentVal-2;i++){
            os << " ";
        }
    }
    os << "}" << "\n";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const JsonArray& arr){
    os << "[" << "\n";
    for (const auto& [index,value]: arr.contents){
        for (int i = 0; i<arr.indentVal;i++){
            os << " ";
        }
        std::visit([&os](const auto& arg){ 
                if constexpr(requires {*arg;}){
                    if (arg) os << *arg <<"\n";
                }else{
                    os << arg << "\n";
                }
            }, value);
    }
    if(arr.indentVal > 2){
        for (int i = 0; i<arr.indentVal-2;i++){
            os << " ";
        }
    }
    os << "]" << "\n";
    return os;
}


/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------Interpreter From Here---------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

class Interpreter{
    public:
        int pos = 0;
        std::string curChar;
        std::string text;
        std::vector<JsonVal> parsed;
        Interpreter(std::string&& textFromFile){
            text = std::move(textFromFile);
            curChar = text[pos];
        }
        ~Interpreter(){}

        /*Lexer*/

        void advance() {
            curChar = text[++pos];
        }

        Token getNextToken(int indentVal = 2){
            while(curChar != ""){
                /*eof*/
                if (pos>=text.length()){
                    return Token(EOFTYPE,"");
                }
                /*string*/
                else if (curChar == "\""){
                    try{
                        return Token(STRING,processString());
                    } catch (std::runtime_error& e){
                        throw e;
                    }
                }
                /*boolean*/
                else if (curChar == "t" ||curChar == "f"){
                    try{
                        return Token(BOOLEAN, processBool());
                    } catch (std::runtime_error& e){
                        throw e;
                    }
                }
                else if (curChar == "n"){
                    try{
                        return Token(NULLTYPE, processNull());
                    } catch (std::runtime_error& e){
                        throw e;
                    }
                }
                /*number*/
                else if (curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '-'){
                    std::string pnumstr = processNumber();
                    if(
                            pnumstr.length()>1 &&
                            ((pnumstr[0]== '0' && (pnumstr[1] != '.' && pnumstr[1] != 'E' && pnumstr[1] != 'e'))|| pnumstr[0] == '.')
                    )
                    {
                        throw std::runtime_error("Here 1 Bad Number "+ pnumstr +" at pos " + std::to_string(pos));

                    }
                    else if( pnumstr.substr(0,2) == "-."){
                        throw std::runtime_error("Bad Number at pos " + std::to_string(pos));
                    }

                    if (pnumstr.contains(".")){
                        return Token(NUMBER,std::stod(pnumstr));
                    }
                    else {
                        return Token(NUMBER,std::stoll(pnumstr));
                    }
                }
                /*json object*/
                else if(curChar[0] == '{'){
                    try{
                    JsonObj jo = createObject(indentVal);
                    std::unique_ptr<JsonObj>jp = std::make_unique<JsonObj>(std::move(jo));
                    return Token(OBJECT, std::move(jp));
                    } catch(std::runtime_error& e){
                        throw e;
                    }
                }
                /*json array*/
                else if(curChar[0] == '['){
                    try {
                    JsonArray ja = createArray(indentVal);
                    std::unique_ptr<JsonArray>jap = std::make_unique<JsonArray>(std::move(ja));
                    return Token(ARRAY, std::move(jap));
                    } catch (std::runtime_error& e){
                        throw e;
                    }
                }
                // end-array or end-object
                else if(curChar == "}"){
                    return Token(ENDOBJ, "}");
                }
                else if(curChar == "]"){
                    return Token(ENDARR, "]");
                }
                /*skipping characters*/
                else if(curChar == " "|| curChar[0] == '\n'||curChar[0] == '\t'||curChar[0] == '\r'){
                    skipWhitespace();
                    continue;
                }
                // name,val separators
                // TODO: Make The Tokens actually take name and valseps
                else if(curChar == ":"){
                    advance();
                    return Token(NAMESEP, ":");
                }
                else if(curChar == ","){
                    advance();
                    return Token(VALSEP, ",");
                }
                else{
                    throw std::runtime_error("Bad Token " + curChar + " at pos " + std::to_string(pos));
                }
            }
        }

        /*Parser*/
        JsonObj createObject(int indentVal = 2){
            JsonObj retObj;
            retObj.indentVal = indentVal;
            int nextIndent = indentVal+2;
            advance();
            skipWhitespace();
            // Empty Object Case
            if(curChar == "}"){
                advance();
                return retObj;
            }
            while(true){
                try{
                    std::string name = std::get<std::string>(getNextToken().t_val);
                    if (name == "}" || name == "]") throw std::runtime_error("Bad token "+ name +" near pos " +  std::to_string(pos));
                    std::cout << "curKey" << name << std::endl;
                    if (!checkColon()) throw std::runtime_error("Missing Colon at pos " +  std::to_string(pos));
                    Token tok = getNextToken(nextIndent);
                    JsonVal& val = tok.t_val;
                    TokenType toktype = tok.t_type;
                    if (toktype == ENDOBJ || toktype == ENDARR) throw std::runtime_error("Bad token near pos " +  std::to_string(pos));
                    retObj.contents.emplace(name,std::move(val));
                    TokenType c_or_e = getNextToken(nextIndent).t_type;        
                    if (c_or_e == ENDOBJ){
                        break;
                    }
                    if(c_or_e == ENDARR){
                        throw std::runtime_error("Bad Token ] at pos " +  std::to_string(pos));
                    }
                    if (c_or_e != VALSEP){
                        throw std::runtime_error("Missing Comma at pos " +  std::to_string(pos));
                    }
            
            
                }
                catch(const std::runtime_error& e){
                    throw e;
                }

                while(curChar[0] == '\n'){advance();}
                skipWhitespace();

            }
            advance();
            return retObj;
        }
        JsonArray createArray(int indentVal = 2){
            JsonArray retArr;
            retArr.indentVal = indentVal;
            int nextIndent = indentVal + 2;
            advance();
            skipWhitespace();
            int curIdx = 0;
            while(true){
                Token t  = getNextToken(nextIndent);
                JsonVal& val = t.t_val;
                TokenType typ = t.t_type;
                if(typ == ENDOBJ || typ == ENDARR || typ == NAMESEP || typ == VALSEP){
                    std::string strval = std::get<std::string>(val);
                    std::string msg = "Bad token" + strval +"at pos " + std::to_string(pos);
                    throw std::runtime_error(msg);
                }
                retArr.contents.emplace(curIdx,std::move(val));

                TokenType c_or_e = getNextToken(nextIndent).t_type;        
                if (c_or_e == ENDARR){
                    break;
                }
                if(c_or_e == ENDOBJ){
                    throw std::runtime_error("Bad Token } at pos " +  std::to_string(pos));
                }
                if (c_or_e != VALSEP){
                    throw std::runtime_error("Missing Comma at pos " +  std::to_string(pos));
                }

                skipWhitespace();

                curIdx++;

            }
            advance();
            return retArr;
        }

        // Leaves CurChar at first element of next valid json token
        std::string processNumber(){
            std::string numStr;
            while(curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'||curChar[0] == 'E'||curChar[0] == 'e'||curChar[0] == '-'||curChar[0] == '+'){
                numStr += curChar;
                advance();
            }
            return numStr;
        }

        // Leaves CurChar at first element of next valid json token
        std::string processString(){
            std::string retstr;
            std::regex esc_reg(R"(\\)");
            retstr += curChar;
            advance();
            while(curChar != "\""){
                if(std::regex_match(curChar, esc_reg))
                {
                    if(!validEscape()){
                        throw std::runtime_error("Unexpected escape sequence in string");
                    }
                }
                if(static_cast<int>(curChar[0]) < 32)
                {
                    throw std::runtime_error("Unescaped escape sequence here possibly");
                }
                retstr+= curChar;
                advance();
            }

            retstr +=curChar;
            advance();
            return retstr;
        }

        // Leaves CurChar at first element of next valid json token
        bool processBool(){
            if(curChar == "t" && "true"== text.substr(pos,4)){
                pos+=4;
                curChar = text[pos];
                return true;
            }
            if(curChar == "f" && "false"== text.substr(pos,5)){
                pos+=5;
                curChar = text[pos];
                return false;
            }
            else{
                throw std::runtime_error("Bad literal at " + std::to_string(pos));
            }
        }

        std::nullptr_t processNull(){
            if (curChar == "n" && "null" == text.substr(pos,4)){
                pos+=4;
                curChar = text[pos];
                return nullptr;
            } else{
                throw std::runtime_error("Bad literal at pos " + std::to_string(pos));
            }
        }

        // Leaves CurChar at first element of next valid json token
       void skipWhitespace(){
           while(curChar == " "|| curChar[0] == '\n'|| curChar[0] == '\t'|| curChar[0] == '\r'){
               advance();
           }
       }

       bool checkColon(){
            try {
               TokenType t = getNextToken().t_type;
               return ( t == NAMESEP);
            }catch(std::runtime_error& e){
                throw e;
            }
       }

       bool validEscape(){
           std::string matchee = text.substr(pos,5);
           std::cout << matchee << std::endl;
           std::cout << text << std::endl;
           std::regex pat1 (R"(\\(b|f|n|r|t|"|u[0-9a-fA-F]{4}|\\|/).*)");
           return std::regex_match(matchee, pat1);
       }

        void expr(){
            Token curToken = getNextToken();
            if(curToken.t_type == EOFTYPE)
            {
                throw std::runtime_error("Unexpected End-of-File");
            }
            if(curToken.t_type == ENDOBJ||curToken.t_type == ENDARR)
            {
                throw std::runtime_error("Unexpected " + std::get<std::string>(curToken.t_val));
            }
            parsed.push_back(std::move(curToken.t_val));
            skipWhitespace();
            if(pos<text.length())
            {
                throw std::runtime_error("Expected EOF");
            }
        }

        void deJSONify()
        {
            for(auto& val : parsed)
            {
                std::visit([](auto&& v){
                        if constexpr(requires {*v;}){
                            std::cout << *v << std::endl;
                        } else {
                            std::cout << v << std::endl;
                        }
                }, val);
            }
        }
        
};


/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
/*-----------------------------------------Execution From Here---------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

void runTest(const std::string& filename){
    std::fstream jsonFile;
    jsonFile.open(filename, std::ios::in);

    std::string textFromFile;
    std::string line;

    if (jsonFile.is_open()){
        while( getline(jsonFile, line))
        {
            textFromFile += line + '\n';
        }
    }

    Interpreter interp(std::move(textFromFile));
    try{
        interp.expr();
    } catch(std::runtime_error& e){
        throw e;
    }
    // interp.deJSONify();
}

int main(int argc, char* argv[]){
    if(argc>2){
        std::cerr << "too many arguments" << "\n";
        return EXIT_FAILURE; 
    }
    namespace fs = std::filesystem;
    fs::path curdir = argv[1];
    if (fs::is_regular_file(curdir) && curdir.extension().string() == ".json")
    {
        runTest(curdir.string());
        std::cout << numAllocs <<" Allocations done." << std::endl;
        std::cout << numTokenMoves <<" Token Moves done." << std::endl;
        std::cout << numTokenCopies <<" Token Copies done." << std::endl;
        return 0;
    }

    if (fs::is_empty(curdir)){
        std::cerr << "File or Dir is Empty" << "\n";
        return EXIT_FAILURE; 
    }

    const std::string SUCCESS = "\033[32m";
    const std::string FAILURE = "\033[31m";
    const std::string TEST = "\033[33m";

    int numPassed = 0;
    int numFailed = 0;

    for(const auto& entry: fs::directory_iterator(curdir)){
        std::string fn = entry.path().extension().string();
        if (fn == ".json"){
            try{
            std::cout << TEST << "---------------------TESTING FILE "+entry.path().filename().string() +"------------------" << std::endl;
            runTest(entry.path().string());
            } catch(std::runtime_error& e){
                numFailed++;
                std::cerr << FAILURE << e.what() << "\n";
            std::cout << FAILURE << "---------------------TEST "+entry.path().filename().string() +" FAILED------------------" << std::endl;
                continue;
            }
            numPassed++;
            std::cout << SUCCESS << "---------------------TEST "+entry.path().filename().string() +" PASSED------------------" << std::endl;
        }
    }

    std::cout << TEST << numAllocs <<" Allocations done." << std::endl;
    std::cout << TEST << numTokenMoves <<" Token Moves done." << std::endl;
    std::cout << TEST << numTokenCopies <<" Token Copies done." << std::endl;
    std::cout << SUCCESS <<numPassed <<" Tests Passed" << std::endl;
    std::cout << FAILURE <<numFailed <<" Tests Failed" << std::endl;
}
