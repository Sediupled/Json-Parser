#include <iostream>
#include <variant>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>

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
using JsonVal = std::variant<std::string, std::unique_ptr<JsonObj>, int, double, std::unique_ptr<JsonArr>>;

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

template<typename V>
class Interpreter{
    public:
        int pos;
        char curChar;
        std::string text;
        JsonContents parsed;
        Interpreter(){
            pos = 0;
            curChar = text[pos];
        }
        ~Interpreter(){}

        /*Lexer*/

        void advance() {
        }

        void getNextToken(){
        }

        void eat(){}


        /*Parser*/
        void createObj(){}
        void createArray(){}
        void processNumber(){}
        void processString(TokenType strType){}

        void expr(){
            Token<V> curToken;

            while(curToken.t_type != EOFTYPE){
                curToken = getNextToken();
                if (curToken.t_type == STRING){
                    parsed.contents.push_back(curToken.t_val);
                }
            }
        }

        void deJSONify()
        {
            this->expr();
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
}
