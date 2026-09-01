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
#include <format>

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

struct JsonObj;
struct JsonArray;
using JsonVal = std::variant<std::string, double, int, bool, std::nullptr_t, std::unique_ptr<JsonObj>, std::unique_ptr<JsonArray>>;

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

        Token(TokenType type,JsonVal val) : t_type(type), t_val(std::move(val)){}
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

struct JsonContents{
    std::vector<JsonVal> contents;
};




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
        JsonContents parsed;
        Interpreter(std::string textFromFile){
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
                    return Token(EOFTYPE,curChar);
                }
                /*string*/
                else if (curChar == "\""){
                    return Token(STRING,processString());
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
                else if (curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'){
                    std::string pnumstr = processNumber();

                    if (pnumstr.contains(".")){
                        double pnum = std::stod(pnumstr);
                        return Token(NUMBER,pnum);
                    }
                    else{
                        int pnum = std::stoi(pnumstr);
                        return Token(NUMBER,pnum);
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
                    return Token(ENDOBJ, curChar);
                }
                else if(curChar == "]"){
                    return Token(ENDARR, curChar);
                }
                /*skipping characters*/
                else if(curChar == " "|| curChar[0] == '\n'||curChar[0] == '\t'||curChar[0] == '\r'){
                    skipWhitespace();
                    continue;
                }
                // name,val separators
                else if(curChar == ":"){
                    advance();
                    return Token(NAMESEP, curChar);
                }
                else if(curChar == ","){
                    advance();
                    return Token(VALSEP, curChar);
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
                    if (!checkColon()) throw std::runtime_error("Missing Colon at pos " +  std::to_string(pos));
                    JsonVal val = getNextToken(nextIndent).t_val;
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
            while(curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'||curChar[0] == 'E'||curChar[0] == 'e'){
                numStr += curChar;
                advance();
            }
            return numStr;
        }

        // Leaves CurChar at first element of next valid json token
        std::string processString(){
            std::string retstr;
            retstr += curChar;
            advance();
            while(curChar!= "\""){
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

        void expr(){
            Token curToken = getNextToken();

            while(curToken.t_type != EOFTYPE){
                parsed.contents.push_back(std::move(curToken.t_val));
                try{
                    curToken = getNextToken();
                } catch (std::runtime_error& e){
                    throw e;
                }
            }
        }

        void deJSONify()
        {
            for(auto& val : parsed.contents)
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

void runTest(const std::string&filename){
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

    Interpreter interp(textFromFile);
    try{
        interp.expr();
    } catch(std::runtime_error& e){
        throw e;
    }
    interp.deJSONify();
}

int main(int argc, char* argv[]){
    if(argc>2){
        std::cerr << "too many arguments" << "\n";
        return EXIT_FAILURE; 
    }
    namespace fs = std::filesystem;
    fs::path curdir = argv[1];

    if (fs::is_empty(curdir)){
        std::cerr << "File or Dir is Empty" << "\n";
        return EXIT_FAILURE; 
    }

    const std::string SUCCESS = "\033[32m";
    const std::string FAILURE = "\033[31m";
    const std::string TEST = "\033[33m";

    for(const auto& entry: fs::directory_iterator(curdir)){
        std::string fn = entry.path().extension().string();
        if (fn == ".json"){
            try{
            std::cout << TEST << "---------------------TESTING FILE "+entry.path().filename().string() +"------------------" << std::endl;
            runTest(entry.path().string());
            } catch(std::runtime_error& e){
                std::cerr << FAILURE << e.what() << "\n";

            std::cout << FAILURE << "---------------------TEST "+entry.path().filename().string() +" FAILED------------------" << std::endl;
                continue;
            }
            std::cout << SUCCESS << "---------------------TEST "+entry.path().filename().string() +" PASSED------------------" << std::endl;
        }
    }
}
