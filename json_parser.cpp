#include <iostream>
#include <ostream>
#include <variant>
#include <unordered_map>
#include <map>
#include <vector>
#include <fstream>
#include <memory>
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
            t_val = val;
        }

        ~Token(){}

        std::ostream& operator<<(std::ostream& os){
            os << "Token("<< this->t_type << " " << this->t_val << ")";
            return os;
        }
};

/*Json Structured Types, ts might be jank not sure yet*/
struct JsonObj;
struct JsonArray;
/*using JsonVal = std::variant<std::unique_ptr<std::string>, std::unique_ptr<JsonObj>, std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<JsonArr>>;*/
using JsonVal = std::variant<std::string, double, int, JsonObj>;

struct JsonObj{
    std::string name;
    std::unordered_map<std::string,JsonVal> contents;

    friend std::ostream& operator<<(std::ostream& os, const JsonObj& obj){
        os << "Object Name: " << obj.name << "\n";
        for (const auto& [namestr,value]: obj.contents){
            os << "  "<< namestr << ":";
            std::visit([&os](const auto& arg){ os << arg <<"\n";}, value);
        }
        return os;
    }
    
};

struct JsonArray{
    std::string name;
    std::map<int,JsonVal> contents;
};

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
        Token<V> getNextToken(){
            Token<V> tok;
            /*eof*/
            if (pos>=text.length()){
                tok = Token<V>(EOFTYPE,curChar);
                return tok;
            }
            /*string*/
            else if (curChar == "\""){
                tok = Token<V>(STRING,processString());
                return tok;

            }
            /*number*/
            else if (curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'){
                std::string pnumstr = processNumber();

                if (pnumstr.contains(".")){
                    double pnum = std::stod(pnumstr);
                    tok = Token<V>(NUMBER,pnum);
                }
                else{
                    int pnum = std::stoi(pnumstr);
                    tok = Token<V>(NUMBER,pnum);
                }
                return tok;
            }
            /*json object*/
            else if(curChar[0] == '{'){
                
                JsonObj jo = createObject<V>();
                tok = Token<V>(OBJECT, jo);
                return tok;
            }
            /*json array*/
            /*else if(curChar[0] == '['){*/
            /*    JsonArray ja = createArray();*/
            /*    tok = Token<V>(ARRAY, ja);*/
            /*    return tok;*/
            /*}*/
            /*newline*/
            else if(curChar[0] == '\n'||curChar[0] == ':'||curChar[0] == ','){
                pos++;
                curChar = text[pos];
                tok = getNextToken<V>();
                if(tok.t_type == EOFTYPE){
                    return tok;
                }
                return tok;
            }
            
            /*whitespace*/
            else{
                skipWhitespace();
                tok = getNextToken<V>();
                return tok;
            }
        }

        void eat(){}


        /*Parser*/
        template<typename V>
        JsonObj createObject(){
            JsonObj retObj;
            pos++;
            curChar = text[pos];
            while(curChar[0] !='}'){
                std::string name = std::get<std::string>(getNextToken<V>().t_val);
                JsonVal val = getNextToken<V>().t_val;
                retObj.contents.emplace(name,val);
                pos++;
                curChar = text[pos];

            }
            curChar = text[++pos];

            return retObj;
        }
        JsonArray createArray(){}

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

       void skipWhitespace(){
           while(curChar == " "){
               curChar = text[++pos];
           }
       }

        template<typename V>
        void expr(){
            Token<V> curToken = getNextToken<V>();

            while(curToken.t_type != EOFTYPE){
                parsed.contents.push_back(curToken.t_val);
                curToken = getNextToken<V>();
            }
        }

        void deJSONify()
        {
            for(auto& val : parsed.contents)
            {
                std::visit([](auto&& v){std::cout << v << std::endl;}, val);
            }
        }
        
};


/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
/*-----------------------------------------Execution From Here---------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

int main(){
    std::fstream jsonFile;
    jsonFile.open("test.json", std::ios::in);

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
