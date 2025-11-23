#include <string>
#include <iostream>
#include <vector>
#include <cctype>
#include <sstream>
#include <fstream>
#include <exception>
using namespace std;

enum TokenType {
    PROGRAM, BEGIN, END, ID, NUM, ASSIGN, IF, THEN, WHILE, DO,
    PLUS, MINUS, MUL, DIV, MOD, LPAREN, RPAREN, SEMICOLON, RELOP, DOT, UNKNOWN, END_OF_FILE,
    AND, OR, NOT, ELSE, BREAK
};

struct Token {
    TokenType type;
    string value;
    int line;
    int col;
};

struct ASTNode {
    string value;
    vector<ASTNode*> children;
    ~ASTNode() { 
        for (auto child : children) {
            delete child;
        }
    }

    void print(int level = 0) {
        cout << string(level * 2, ' ') << value << endl;
        for (auto child : children) {
            child->print(level + 1);
        }
    }
};

class Lexer {
    string source;
    size_t pos = 0;
    int line = 1;
    int col = 1;

    void skipWhitespace() {
        while (pos < source.size() && isspace(source[pos])) {
            if (source[pos] == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            pos++;
        }
    }

    void skipComment() {
        while (pos < source.size() && source[pos] != '}') {
            if (source[pos] == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            pos++;
        }
        if (pos < source.size()) {
            pos++; // 跳过 }
            col++;
        }
    }

public:
    Lexer(const string& src) : source(src) {
        if (source.size() >= 3 && (unsigned char)source[0] == 0xEF && (unsigned char)source[1] == 0xBB && (unsigned char)source[2] == 0xBF) {
            source = source.substr(3);
        }
    }

    Token getNextToken() {
        skipWhitespace();

        if (pos >= source.size()) {
            return { END_OF_FILE, "", line, col };
        }

        if (source[pos] == '{') {
            skipComment();
            return getNextToken();
        }

        if (isalpha(source[pos])) {
            int start_col = col;
            string id;
            // 修复：使用正确的变量名 pos
            while (pos < source.size() && (isalnum(source[pos]) || source[pos] == '_')) {
                id += source[pos];
                pos++;
                col++;
            }

            TokenType type = ID;
            if (id == "program") type = PROGRAM;
            else if (id == "begin") type = BEGIN;
            else if (id == "end") type = END;
            else if (id == "if") type = IF;
            else if (id == "then") type = THEN;
            else if (id == "while") type = WHILE;
            else if (id == "do") type = DO;
            else if (id == "and") type = AND;
            else if (id == "or") type = OR;
            else if (id == "not") type = NOT;
            else if (id == "else") type = ELSE;
            else if (id == "break") type = BREAK;
            else if (id == "mod") type = MOD;

            return { type, id, line, start_col };
        }

        if (isdigit(source[pos])) {
            int start_col = col;
            string num;
            while (pos < source.size() && isdigit(source[pos])) {
                num += source[pos];
                pos++;
                col++;
            }
            return { NUM, num, line, start_col };
        }

        char ch = source[pos];
        int start_col = col;
        pos++;
        col++;

        switch (ch) {
            case '+': return { PLUS, string(1, ch), line, start_col };
            case '-': return { MINUS, string(1, ch), line, start_col };
            case '*': return { MUL, string(1, ch), line, start_col };
            case '/': return { DIV, string(1, ch), line, start_col };
            case '(': return { LPAREN, string(1, ch), line, start_col };
            case ')': return { RPAREN, string(1, ch), line, start_col };
            case ';': return { SEMICOLON, string(1, ch), line, start_col };
            case '.': return { DOT, string(1, ch), line, start_col };
            case ':':
                if (pos < source.size() && source[pos] == '=') {
                    pos++;
                    col++;
                    return { ASSIGN, ":=", line, start_col };
                }
                return { UNKNOWN, string(1, ch), line, start_col };
            case '<':
                if (pos < source.size() && source[pos] == '=') {
                    pos++;
                    col++;
                    return { RELOP, "<=", line, start_col };
                } else if (pos < source.size() && source[pos] == '>') {
                    pos++;
                    col++;
                    return { RELOP, "<>", line, start_col };
                }
                return { RELOP, "<", line, start_col };
            case '>':
                if (pos < source.size() && source[pos] == '=') {
                    pos++;
                    col++;
                    return { RELOP, ">=", line, start_col };
                }
                return { RELOP, ">", line, start_col };
            case '=': return { RELOP, "=", line, start_col };
            case '!':
                if (pos < source.size() && source[pos] == '=') {
                    pos++;
                    col++;
                    return { RELOP, "!=", line, start_col };
                }
                return { UNKNOWN, string(1, ch), line, start_col };
            default:
                return { UNKNOWN, string(1, ch), line, start_col };
        }
    }
};

class Parser {
    Lexer& lexer;
    Token currentToken;

    void error(const string& message, ostream& out) {
        out << "错误：" << message 
            << "（在行 " << currentToken.line << ", 列 " << currentToken.col 
            << "，遇到 '" << currentToken.value << "'）" << endl;
        throw runtime_error("语法错误");
    }

    void eat(TokenType type, ostream& out) {
        if (currentToken.type == type) {
            currentToken = lexer.getNextToken();
        } else {
            string expected;
            switch(type) {
                case PROGRAM: expected = "program"; break;
                case BEGIN: expected = "begin"; break;
                case END: expected = "end"; break;
                case SEMICOLON: expected = "分号"; break;
                case DOT: expected = "点号"; break;
                case ID: expected = "标识符"; break;
                case ASSIGN: expected = ":="; break;
                case IF: expected = "if"; break;
                case THEN: expected = "then"; break;
                case WHILE: expected = "while"; break;
                case DO: expected = "do"; break;
                case ELSE: expected = "else"; break;
                case BREAK: expected = "break"; break;
                case PLUS: expected = "+"; break;
                case MINUS: expected = "-"; break;
                case MUL: expected = "*"; break;
                case DIV: expected = "/"; break;
                case MOD: expected = "mod"; break;
                case LPAREN: expected = "("; break;
                case RPAREN: expected = ")"; break;
                case RELOP: expected = "关系运算符"; break;
                default: expected = "特定符号"; break;
            }
            error("应为 '" + expected + "'", out);
        }
    }

    ASTNode* Program(ostream& out) {
        auto node = new ASTNode{ "程序" };
        
        if (currentToken.type != PROGRAM) {
            error("程序必须以program关键字开始", out);
        }
        node->children.push_back(new ASTNode{ "关键字: " + currentToken.value });
        eat(PROGRAM, out);
        
        if (currentToken.type != ID) {
            error("program后必须跟程序名", out);
        }
        node->children.push_back(new ASTNode{ "程序名: " + currentToken.value });
        eat(ID, out);
        
        if (currentToken.type != SEMICOLON) {
            error("程序名后必须有分号", out);
        }
        eat(SEMICOLON, out);
        
        node->children.push_back(Block(out));
        
        if (currentToken.type != DOT) {
            error("程序必须以点号结束", out);
        }
        eat(DOT, out);
        
        return node;
    }

    ASTNode* Block(ostream& out) {
        auto node = new ASTNode{ "块" };
        eat(BEGIN, out);
        node->children.push_back(StmtList(out));
        eat(END, out);
        return node;
    }

    ASTNode* StmtList(ostream& out) {
        auto node = new ASTNode{ "语句列表" };
        if (currentToken.type == END) {
            return node;
        }
        node->children.push_back(Stmt(out));
        while (currentToken.type == SEMICOLON) {
            eat(SEMICOLON, out);
            if (currentToken.type == END) {
                break;
            }
            node->children.push_back(Stmt(out));
        }
        return node;
    }

    ASTNode* Stmt(ostream& out) {
        if (currentToken.type == ID) return AssignStmt(out);
        if (currentToken.type == IF) return IfStmt(out);
        if (currentToken.type == WHILE) return WhileStmt(out);
        if (currentToken.type == BREAK) return BreakStmt(out);
        if (currentToken.type == BEGIN) {
            auto node = new ASTNode{ "语句块" };
            node->children.push_back(Block(out));
            return node;
        }
        
        // 如果是数字，给出更具体的错误信息
        if (currentToken.type == NUM) {
            error("语句不能以数字开头，可能是赋值语句左值错误", out);
        }
        
        error("应为赋值语句、if语句、while语句、break语句或语句块", out);
        return nullptr;
    }

    ASTNode* AssignStmt(ostream& out) {
        auto node = new ASTNode{ "赋值语句" };
        
        // 检查左值是否为标识符
        if (currentToken.type != ID) {
            error("赋值语句左值必须是标识符", out);
        }
        node->children.push_back(new ASTNode{ "左值: " + currentToken.value });
        eat(ID, out);
        
        eat(ASSIGN, out);
        node->children.push_back(Expr(out));
        return node;
    }

    ASTNode* IfStmt(ostream& out) {
        auto node = new ASTNode{ "If语句" };
        eat(IF, out);
        node->children.push_back(Cond(out));
        eat(THEN, out);
        if (currentToken.type == BEGIN) {
            node->children.push_back(Block(out));
        } else {
            node->children.push_back(Stmt(out));
        }
        if (currentToken.type == ELSE) {
            eat(ELSE, out);
            if (currentToken.type == BEGIN) {
                node->children.push_back(Block(out));
            } else {
                node->children.push_back(Stmt(out));
            }
        }
        return node;
    }

    ASTNode* WhileStmt(ostream& out) {
        auto node = new ASTNode{ "While语句" };
        eat(WHILE, out);
        node->children.push_back(Cond(out));
        eat(DO, out);
        if (currentToken.type == BEGIN) {
            node->children.push_back(Block(out));
        } else {
            node->children.push_back(Stmt(out));
        }
        return node;
    }

    ASTNode* Expr(ostream& out) {
        auto node = Term(out);
        while (currentToken.type == PLUS || currentToken.type == MINUS) {
            auto op = new ASTNode{ currentToken.value };
            eat(currentToken.type, out);
            op->children.push_back(node);
            op->children.push_back(Term(out));
            node = op;
        }
        return node;
    }

    ASTNode* Term(ostream& out) {
        auto node = Factor(out);
        while (currentToken.type == MUL || currentToken.type == DIV || currentToken.type == MOD) {
            auto op = new ASTNode{ currentToken.value };
            eat(currentToken.type, out);
            op->children.push_back(node);
            op->children.push_back(Factor(out));
            node = op;
        }
        return node;
    }

    ASTNode* Factor(ostream& out) {
        if (currentToken.type == NUM) {
            auto node = new ASTNode{ "数字: " + currentToken.value };
            eat(NUM, out);
            return node;
        }
        else if (currentToken.type == ID) {
            auto node = new ASTNode{ "变量: " + currentToken.value };
            eat(ID, out);
            return node;
        }
        else if (currentToken.type == LPAREN) {
            eat(LPAREN, out);
            auto node = Expr(out);
            eat(RPAREN, out);
            return node;
        }
        error("应为数字、变量或括号表达式", out);
        return nullptr;
    }

    ASTNode* Cond(ostream& out) {
    auto node = new ASTNode{ "条件表达式" };
    
    // 处理逻辑非
    if (currentToken.type == NOT) {
        auto notNode = new ASTNode{ "逻辑非: " + currentToken.value };
        eat(NOT, out);
        notNode->children.push_back(Cond(out));
        return notNode;
    }
    
    // 处理括号条件
    if (currentToken.type == LPAREN) {
        eat(LPAREN, out);
        
        // 先解析括号内的内容，可能是表达式或条件
        auto leftNode = Expr(out); // 先尝试解析为表达式
        
        // 检查下一个token是关系运算符还是右括号
        if (currentToken.type == RELOP) {
            // 是关系表达式: (expr relop expr)
            node->children.push_back(leftNode);
            node->children.push_back(new ASTNode{ "关系符: " + currentToken.value });
            eat(RELOP, out);
            node->children.push_back(Expr(out));
        } else if (currentToken.type == RPAREN) {
            // 只有表达式，没有关系运算符 - 这是错误
            error("括号内的条件表达式必须包含关系运算符", out);
        } else {
            // 可能是逻辑运算符或其他情况，我们暂时不支持
            error("括号内的条件表达式格式错误", out);
        }
        
        eat(RPAREN, out);
        
        // 处理括号后的逻辑运算符
        while (currentToken.type == AND || currentToken.type == OR) {
            auto op = new ASTNode{ "逻辑符: " + currentToken.value };
            eat(currentToken.type, out);
            op->children.push_back(node);
            op->children.push_back(Cond(out));
            node = op;
        }
        return node;
    }
    
    // 处理基本关系表达式（无括号）
    node->children.push_back(Expr(out));
    
    if (currentToken.type != RELOP) {
        error("条件表达式中缺少关系运算符", out);
    }
    
    node->children.push_back(new ASTNode{ "关系符: " + currentToken.value });
    eat(RELOP, out);
    node->children.push_back(Expr(out));
    
    // 处理逻辑运算符
    while (currentToken.type == AND || currentToken.type == OR) {
        auto op = new ASTNode{ "逻辑符: " + currentToken.value };
        eat(currentToken.type, out);
        op->children.push_back(node);
        op->children.push_back(Cond(out));
        node = op;
    }
    
    return node;
}

    ASTNode* BreakStmt(ostream& out) {
        auto node = new ASTNode{ "Break语句" };
        eat(BREAK, out);
        return node;
    }

public:
    Parser(Lexer& lexer) : lexer(lexer), currentToken(this->lexer.getNextToken()) {}

    ASTNode* parse(ostream& out) {
        try {
            ASTNode* root = Program(out);
            if (currentToken.type != END_OF_FILE) {
                error("程序结束后有多余内容", out);
            }
            return root;
        }
        catch (const exception& e) {
            return nullptr;
        }
    }
};

int main() {
    ifstream inFile("input.txt");
    if (!inFile.is_open()) {
        cerr << "错误：无法打开输入文件！" << endl;
        return 1;
    }

    stringstream buffer;
    buffer << inFile.rdbuf();
    string source = buffer.str();

    Lexer lexer(source);
    Parser parser(lexer);

    ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        cerr << "错误：无法打开输出文件！" << endl;
        return 1;
    }

    ASTNode* root = parser.parse(outFile);
    if (root) {
        outFile << "该程序是正确的。" << endl;
        root->print();
        delete root;
    } else {
        outFile << "该程序有语法错误。" << endl;
    }

    inFile.close();
    outFile.close();
    return 0;
}