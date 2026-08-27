#include <iostream>
#include <cctype>
#include <ostream>
#include <variant>
#include <unordered_map>
#include <map>
#include <vector>
#include <fstream>
#include <memory>
#include <format>
#include <filesystem>

/*Token Types*/
enum TokenType{
    STRING,
    NUMBER,
    BOOLEAN,
    NULLTYPE,
    NAME,
    EOFTYPE,
    OBJECT,
    ARRAY
};

/*Token Class Template*/
template<typename V>
class Token{
    public:
        TokenType t_type;
        V t_val;

        Token(){}
        Token(TokenType type,V val) {
            t_type = type;
            t_val = std::move(val);
        }

        std::ostream& operator<<(std::ostream& os){
            os << "Token("<< this->t_type << " " << this->t_val << ")";
            return os;
        }
};

/*Json Structured Types, ts might be jank not sure yet*/
struct JsonObj;
struct JsonArray;
using JsonVal = std::variant<std::string, double, int, bool, std::unique_ptr<JsonObj>, std::unique_ptr<JsonArray>>;

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
        std::visit([&os](const auto& arg){ os << arg <<"\n";}, value);
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
        int pos;
        std::string curChar;
        std::string text;
        JsonContents parsed;
        Interpreter(std::string textFromFile){
            pos = 0;
            text = textFromFile;
            curChar = text[pos];
        }
        ~Interpreter(){}

        /*Lexer*/

        void advance() {
        }

        template<typename V>
        Token<V> getNextToken(int indentVal = 2){
            Token<V> tok;
            /*eof*/
            if (pos>=text.length()){
                return Token<V>(EOFTYPE,curChar);
            }
            /*string*/
            else if (curChar == "\""){
                return Token<V>(STRING,processString());

            }
            /*boolean*/
            else if (curChar == "t" ||curChar == "f"){
                return Token<V>(BOOLEAN, processBool());
            }
            /*number*/
            else if (curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'){
                std::string pnumstr = processNumber();

                if (pnumstr.contains(".")){
                    double pnum = std::stod(pnumstr);
                    return Token<V>(NUMBER,pnum);
                }
                else{
                    int pnum = std::stoi(pnumstr);
                    return Token<V>(NUMBER,pnum);
                }
            }
            /*json object*/
            else if(curChar[0] == '{'){
                
                JsonObj jo = createObject<V>(indentVal);
                std::unique_ptr<JsonObj>jp = std::make_unique<JsonObj>(std::move(jo));
                return Token<V>(OBJECT, std::move(jp));
            }
            /*json array*/
            else if(curChar[0] == '['){
                JsonArray ja = createArray<V>(indentVal);
                std::unique_ptr<JsonArray>jap = std::make_unique<JsonArray>(std::move(ja));
                return Token<V>(ARRAY, std::move(jap));
            }
            /*newline*/
            else if(curChar[0] == '\n'||curChar[0] == ':'||curChar[0] == ','||curChar[0] == '\t'){
                pos++;
                curChar = text[pos];
                tok = getNextToken<V>(indentVal);
                if(tok.t_type == EOFTYPE){
                    return tok;
                }
                return tok;
            }
            
            /*whitespace*/
            else{
                skipWhitespace();
                tok = getNextToken<V>(indentVal);
                return tok;
            }
        }

        void eat(){}


        /*Parser*/
        template<typename V>
        JsonObj createObject(int indentVal = 2){
            JsonObj retObj;
            retObj.indentVal = indentVal;
            int nextIndent = indentVal+2;
            pos++;
            curChar = text[pos];
            while(curChar[0] !='}'){
                std::string name = std::get<std::string>(getNextToken<V>().t_val);
                JsonVal val = getNextToken<V>(nextIndent).t_val;
                retObj.contents.emplace(name,std::move(val));
                while(curChar[0] == '\n'){curChar = text[++pos];}
                skipWhitespace();

            }
            curChar = text[++pos];

            return retObj;
        }
        
        template<typename V>
        JsonArray createArray(int indentVal = 2){
            JsonArray retArr;
            retArr.indentVal = indentVal;
            int nextIndent = indentVal + 2;
            pos++;
            curChar = text[pos];
            int curIdx = 0;
            while(curChar[0] !=']'){
                JsonVal val = getNextToken<V>(nextIndent).t_val;
                retArr.contents.emplace(curIdx,std::move(val));
                while(curChar[0] == '\n'){curChar = text[++pos];}
                skipWhitespace();
                curIdx++;

            }
            curChar = text[++pos];

            return retArr;
        }

        std::string processNumber(){
            std::string numStr;
            while(curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'||curChar[0] == 'E'||curChar[0] == 'e'){
                numStr += curChar;
                curChar = text[++pos];
            }
            return numStr;
        }

        std::string processString(){
            std::string retstr;
            retstr += curChar;
            curChar = text[++pos];
            while(curChar!= "\""){
                retstr+= curChar;
                curChar = text[++pos];
            }

            retstr +=curChar;
            curChar = text[++pos];

            return retstr;
        }

        bool processBool(){
            bool retB = (curChar == "t");
            if(retB){
                pos+=4;
            }
            else{
                pos+=5;
            }

            curChar = text[pos];

            return retB;
        }

       void skipWhitespace(){
           while(curChar == " "){
               curChar = text[++pos];
           }
       }

        template<typename V>
        void expr(){
            Token<V> curToken = getNextToken<V>();

            while(curToken.t_type != EOFTYPE){
                parsed.contents.push_back(std::move(curToken.t_val));
                curToken = getNextToken<V>();
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

void runTest(std::string filename){
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
    interp.expr<JsonVal>();
    interp.deJSONify();
}

int main(){
    namespace fs = std::filesystem;
    fs::path curdir = ".";
    for(const auto& entry: fs::directory_iterator(curdir)){
        std::string fn = entry.path().filename().string();
        if (std::isdigit(static_cast<unsigned char>(fn[0]))){
            runTest(fn);
            std::cout << "--------------------------------------------------------" << std::endl;
        }
    }
}
