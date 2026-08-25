#include <iostream>
#include <variant>
#include <unordered_map>
#include <map>
#include <vector>
#include <fstream>
#include <memory>

/*Token Types*/
enum TokenType{
    STRING,
    NUMBER,
    BOOLEAN,
    NULLTYPE,
    BEGINARR,
    BEGINOBJ,
    ENDARR,
    ENDOBJ,
    NAME,
    NAMESEP,
    VALSEP,
    EOFTYPE
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
struct JsonArr;
/*using JsonVal = std::variant<std::unique_ptr<std::string>, std::unique_ptr<JsonObj>, std::unique_ptr<int>, std::unique_ptr<double>, std::unique_ptr<JsonArr>>;*/
using JsonVal = std::variant<std::string, double, int>;

struct JsonObj{
    std::string name;
    std::unordered_map<std::string,JsonVal> contents;
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
            if (curChar == "\""){
                tok = Token<V>(STRING,processString());
            }
            else if (curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'){
                std::string pnumstr= processNumber();

                if (pnumstr.contains(".")){
                    double pnum = std::stod(pnumstr);
                    tok = Token<V>(NUMBER,pnum);
                }
                else{
                    int pnum = std::stoi(pnumstr);
                    tok = Token<V>(NUMBER,pnum);
                }
            }
            else if(curChar[0] == '\n'){
                pos++;
                curChar = text[pos];
                tok = getNextToken<V>();
            }
            else if (pos>=text.length()){
                tok = Token<V>(EOFTYPE,curChar);
                return tok;
            }
            pos++;
            curChar = text[pos];
            return tok;
        }

        void eat(){}


        /*Parser*/
        void createObj(){}
        void createArray(){}

        std::string processNumber(){
            std::string numStr;
            while(curChar[0] >= '0' && curChar[0] <= '9' || curChar[0] == '.'||curChar[0] == 'E'||curChar[0] == 'e'){
                numStr += curChar;
                curChar = text[++pos];
            }
            pos--;
            numStr.erase(std::remove(numStr.begin(), numStr.end(), '\n'), numStr.end());
            return numStr;
        }

/*Returns with pos still on last character of string, generally "*/
        std::string processString(){
            std::string retstr;
            retstr += curChar;
            curChar = text[++pos];
            while(curChar!= "\""){
                retstr+= curChar;
                curChar = text[++pos];
            }

            retstr +=curChar;

            return retstr;
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
        /*for(auto v: textFromFile){*/
        /*    std::cout << v << std::endl;*/
        /*}*/
    }

    Interpreter interp(textFromFile);
    interp.expr<JsonVal>();
    interp.deJSONify();

}
