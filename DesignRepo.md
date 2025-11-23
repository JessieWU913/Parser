# Pascal-like 语法分析器 - 设计文档
---
## 项目概述

### 项目目标
开发一个能够解析类Pascal语言的语法分析器，实现词法分析、语法分析和抽象语法树构建功能。

### 核心特性
- 类Pascal语法解析
- 详细的错误定位和报告
- 抽象语法树可视化
- 中文错误信息输出

## 系统架构

### 整体架构图
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  输入文件    │ -> │  词法分析器   │ -> │  语法分析器   │ -> │  抽象语法树  │
│ (input.txt) │    │  (Lexer)    │    │  (Parser)   │    │   (AST)     │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
                                                                │
                                                                ↓
                                                         ┌─────────────┐
                                                         │  输出结果    │
                                                         │ (output.txt)│
                                                         └─────────────┘
```

### 组件设计

#### 1. 词法分析器 (Lexer)
**职责**：将源代码字符流转换为标记(token)流

**核心数据结构**：
```cpp
enum TokenType {
    PROGRAM, BEGIN, END, ID, NUM, ASSIGN, IF, THEN, WHILE, DO,
    PLUS, MINUS, MUL, DIV, MOD, LPAREN, RPAREN, SEMICOLON, 
    RELOP, DOT, UNKNOWN, END_OF_FILE, AND, OR, NOT, ELSE, BREAK
};

struct Token {
    TokenType type;
    string value;
    int line;
    int col;
};
```

**主要方法**：
- `getNextToken()`: 获取下一个标记
- `skipWhitespace()`: 跳过空白字符
- `skipComment()`: 跳过注释

#### 2. 语法分析器 (Parser)
**职责**：根据语法规则解析标记流，构建抽象语法树

**设计模式**：递归下降分析

**语法规则**：
```
Program    → program ID ; Block .
Block      → begin StmtList end
StmtList   → Stmt ( ; Stmt )*
Stmt       → AssignStmt | IfStmt | WhileStmt | BreakStmt | Block
AssignStmt → ID := Expr
IfStmt     → if Cond then Stmt [else Stmt]
WhileStmt  → while Cond do Stmt
BreakStmt  → break
Expr       → Term ((PLUS | MINUS) Term)*
Term       → Factor ((MUL | DIV | MOD) Factor)*
Factor     → NUM | ID | LPAREN Expr RPAREN
Cond       → [NOT] (Expr RELOP Expr | LPAREN Cond RPAREN) 
             ((AND | OR) Cond)*
```

#### 3. 抽象语法树 (AST)
**职责**：以树形结构表示程序的语法结构

**核心数据结构**：
```cpp
struct ASTNode {
    string value;
    vector<ASTNode*> children;
    
    // 构造和析构方法
    ~ASTNode();
    void print(int level = 0);
};
```

## 详细设计

### 词法分析设计

#### 标记识别规则
1. **关键字**：`program`, `begin`, `end`, `if`, `then`, `while`, `do`, `and`, `or`, `not`, `else`, `break`, `mod`
2. **标识符**：以字母开头，后跟字母、数字或下划线
3. **数字**：整数序列
4. **运算符**：
   - 算术：`+`, `-`, `*`, `/`
   - 关系：`<`, `>`, `=`, `!=`, `<=`, `>=`
   - 赋值：`:=`
5. **分隔符**：`(`, `)`, `;`, `.`

#### 错误处理
- 遇到无法识别的字符标记为 `UNKNOWN`
- 保持行号和列号跟踪，用于错误定位

### 语法分析设计

#### 递归下降分析策略
每个非终结符对应一个解析函数：
- `Program()`: 解析程序结构
- `Block()`: 解析语句块
- `Stmt()`, `StmtList()`: 解析语句和语句列表
- `Expr()`, `Term()`, `Factor()`: 解析表达式
- `Cond()`: 解析条件表达式

#### 错误恢复机制
- 使用异常处理进行错误恢复
- 提供错误位置和原因

### 抽象语法树设计

#### 节点类型
- **程序节点**：包含程序名和程序体
- **语句节点**：赋值、if、while、break等
- **表达式节点**：算术运算、关系运算、逻辑运算
- **叶子节点**：标识符、数字

#### 树形结构示例
```
程序
├── 程序名: demo
└── 块
    ├── 赋值语句
    │   ├── 左值: x
    │   └── 表达式
    │       ├── 数字: 1
    │       ├── +
    │       └── 表达式
    │           ├── 数字: 2
    │           ├── *
    │           └── 数字: 3
    └── If语句
        ├── 条件表达式
        │   ├── 变量: a
        │   ├── >
        │   └── 数字: 0
        └── 赋值语句
            ├── 左值: b
            └── 数字: 1
```

## 实现细节

### 文件结构
```
Parser/
├── parser.cpp          # 主程序文件
├── input.txt           # 测试输入文件
├── output.txt          # 结果输出文件
├── README.md           # 项目说明
└── design_repo.md      # 设计文档
```

### 关键算法

#### 1. 词法分析算法
```cpp
Token Lexer::getNextToken() {
    skipWhitespace();
    if (pos >= source.size()) return END_OF_FILE;
    
    if (isalpha(ch)) {
        // 处理标识符和关键字
        return processIdentifier();
    } else if (isdigit(ch)) {
        // 处理数字
        return processNumber();
    } else {
        // 处理运算符和分隔符
        return processOperator();
    }
}
```

#### 2. 语法分析算法
```cpp
ASTNode* Parser::parse() {
    try {
        ASTNode* root = Program();
        if (currentToken != END_OF_FILE) {
            error("程序结束后有多余内容");
        }
        return root;
    } catch (exception& e) {
        return nullptr;
    }
}
```

#### 3. 错误处理算法
```cpp
void Parser::error(const string& message) {
    // 记录错误位置和原因
    errorLog << "错误：" << message 
             << "（在行 " << line << ", 列 " << col << "）" << endl;
    throw ParseError(message);
}
```

## 测试用例设计
```pascal
// 测试用例1：基础语法
program basic;
begin
    x := 1;
    if a > 0 then b := 1
end.

// 测试用例2：复杂表达式
program expressions;
begin
    result := (a + b) * c - d / e mod f
end.

// 测试用例3：嵌套结构
program nested;
begin
    while i < 10 do begin
        if (i mod 2) = 0 then
            sum := sum + i;
        i := i + 1
    end
end.
```

## 性能考虑

### 时间复杂度
- **词法分析**：O(n)，n为源代码长度
- **语法分析**：O(n)，递归下降分析
- **AST构建**：O(n)，每个节点处理一次

### 空间复杂度
- **标记存储**：O(n)
- **AST存储**：O(n)
- **递归栈**：O(d)，d为语法嵌套深度

## 扩展性设计

### 可扩展的语法规则
通过修改语法规则和对应的解析函数，可以轻松扩展支持的语法结构。

### 模块化设计
各组件之间松散耦合，便于单独测试和修改。

### 错误处理扩展
当前的错误处理框架支持添加更多类型的语法错误检测。

## ❌限制和约束

### 语法限制
1. 不支持过程、函数定义
2. 不支持数组、记录等复杂数据类型
3. 不支持浮点数运算
4. 条件表达式中括号内必须是完整的关系表达式

### 技术限制
1. 单文件实现，未进行代码分离
2. 错误恢复机制相对简单
3. 不支持语法错误自动修复

## 部署和使用

### 编译要求
- C++14 兼容编译器
- 标准模板库支持

### 运行环境
- 支持文件读写的操作系统
- 足够的内存处理目标程序