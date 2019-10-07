%{
    #include "ast.hh"
    #include "type.hh"
    #include <vector>
    using namespace std;
    using namespace mc;

    static Type* tmp_base;

    #define BIN(x, a, b) new BinaryExpr(BinaryExpr::BinOp::x, a, b)
    #define UN(x, a) new UnaryExpr(UnaryExpr::UnOp::x, a)

    extern int yylex();
    extern void yyerror(const char* s);
%}

%start tops

%union {
    vector<AST*>* asts;
    AST* ast;
    DeclStmt* decl;
    Field* field;
    vector<Field*>* fields;
    Type* type;
    FuncDefn* func_defn;
    vector<Stmt*>* stmts;
    Stmt* stmt;
    IfStmt* if_stmt;
    WhileStmt* while_stmt;
    ReturnStmt* ret_stmt;
    Expr* expr;
    CallExpr* call;
    RefExpr* ref;
    vector<Expr*>* exprs;
    string* s;
    int i;
}

%token <i> NUM
%token <s> ID
%token IF ELSE WHILE RETURN EQ NE OR AND INT

%type <asts> tops
%type <ast> top
%type <decl> func_decl var_decl
%type <field> param
%type <fields> param_list param_list_
%type <type> base_part array_part 
%type <func_defn> func_defn
%type <stmts> comp_stmt
%type <stmt> stmt
%type <if_stmt> if_stmt
%type <while_stmt> while_stmt
%type <ret_stmt> ret_stmt
%type <expr> expr_ expr ass_expr cmp_expr or_expr and_expr add_expr mul_expr rem_expr factor
%type <exprs> arg_list arg_list_ array_index
%type <call> call
%type <ref> ref

%nonassoc ')'
%nonassoc ELSE

%%

// vector<AST*>*
tops:
    %empty
        { $$ = global_prog = new std::vector<AST*>; }
    | tops top  
        { $1->push_back($2); $$ = $1; }
    ;

// AST*
top:
    var_decl
        { $$ = $1; }
    | func_defn
        { $$ = $1; }
    | func_decl
        { $$ = $1; }
    ;

// DeclStmt*
var_decl:
    base_part ID array_part ';'
        { $$ = new DeclStmt(*$2, $3); delete $2; }
    ;

// Field*
param:
    base_part ID array_part
        { $$ = new Field(*$2, $3); delete $2; }
    | base_part ID '[' ']' array_part
        { $$ = new Field(*$2, new VariantArrayType($5)); delete $2; }
    ;

// vector<Field*>*
param_list:
    param 
        { $$ = new vector<Field*>{$1}; }
    | param_list ',' param 
        { $1->push_back($3); $$ = $1; }
    ;

// vector<Field*>*
param_list_:
    %empty 
        { $$ = new vector<Field*>; }
    | param_list 
        { $$ = $1; }
    ;

// Type*
base_part:
    INT 
        { $$ = tmp_base = new IntType; }
    ;

// Type*
array_part:
    %empty
        { $$ = tmp_base; }
    | '[' NUM ']' array_part
        { $$ = new ArrayType($2, $4); }
    ;

// FuncDefn*
func_defn:
    base_part ID '(' param_list_ ')' '{' comp_stmt '}'
        { $$ = new FuncDefn($1, *$2, *$4, *$7); delete $2; delete $4; delete $7; }
    ;

// DeclStmt*
func_decl:
    base_part ID '(' param_list_ ')' ';'
        { $$ = new DeclStmt(*$2, new FuncType($1, *$4)); delete $2; delete $4; }
    ;

// vector<Stmt*>*
comp_stmt:
    %empty
        { $$ = new vector<Stmt*>; }
    | comp_stmt stmt 
        { if ($2) $1->push_back($2); $$ = $1; }
    ;

// Stmt*
stmt:
    '{' comp_stmt '}'
        { $$ = new BlockStmt(*$2); delete $2; }
    | if_stmt
        { $$ = $1; }
    | while_stmt
        { $$ = $1; }
    | ret_stmt
        { $$ = $1; }
    | var_decl
        { $$ = $1; }
    | func_decl
        { $$ = $1; }
    | expr_ ';'
        { $$ = $1; }
    ;

// IfStmt*
if_stmt:
    IF '(' expr ')' stmt
        { $$ = new IfStmt($3, $5); }
    | IF '(' expr ')' stmt ELSE stmt
        { $$ = new IfStmt($3, $5, $7); }
    ;

// WhileStmt*
while_stmt:
    WHILE '(' expr ')' stmt
        { $$ = new WhileStmt($3, $5); }
    ;

// ReturnStmt*
ret_stmt:
    RETURN expr ';'
        { $$ = new ReturnStmt($2); }
    ;

// Expr*
expr_:
    %empty
        { $$ = nullptr; }
    | expr
        { $$ = $1; }
    ;

// Expr*
expr:
    ass_expr
        { $$ = $1; }
    | expr ',' ass_expr
        { $$ = BIN(COMMA, $1, $3); }
    ;

// Expr*
ass_expr:
    cmp_expr
        { $$ = $1; }
    | ref '=' ass_expr
        { $$ = BIN(ASSIGN, $1, $3); }
    ;

// Expr*
cmp_expr:
    or_expr
        { $$ = $1; }
    | or_expr EQ or_expr
        { $$ = BIN(EQ, $1, $3); }
    | or_expr NE or_expr 
        { $$ = BIN(NE, $1, $3); }
    | or_expr '<' or_expr
        { $$ = BIN(LT, $1, $3); }
    | or_expr '>' or_expr
        { $$ = BIN(GT, $1, $3); }
    ;

// Expr*
or_expr: 
    and_expr
        { $$ = $1; }
    | or_expr OR and_expr
        { $$ = BIN(OR, $1, $3); }
    ;

// Expr*
and_expr:
    add_expr
        { $$ = $1; }
    | and_expr AND add_expr
        { $$ = BIN(AND, $1, $3); } 
    ;

// Expr*
add_expr:
    mul_expr
        { $$ = $1; }
    | add_expr '+' mul_expr
        { $$ = BIN(ADD, $1, $3); }
    | add_expr '-' mul_expr
        { $$ = BIN(SUB, $1, $3); }
    ;

// Expr*
mul_expr:
    rem_expr
        { $$ = $1; }
    | mul_expr '*' rem_expr
        { $$ = BIN(MUL, $1, $3); }
    | mul_expr '/' rem_expr
        { $$ = BIN(DIV, $1, $3); }
    ;

// Expr*
rem_expr:
    factor
        { $$ = $1; }
    | rem_expr '%' factor
        { $$ = BIN(REM, $1, $3); }
    ;

// Expr*
factor:
    NUM 
        { $$ = new NumExpr($1); }
    | ref
        { $$ = $1; }
    | call
        { $$ = $1; }
    | '(' expr ')'
        { $$ = $2; }
    | '-' factor
        { $$ = UN(NEG, $2); }
    | '!' factor
        { $$ = UN(NOT, $2); }
    ;

// CallExpr*
call:
    ID '(' arg_list_ ')'
        { $$ = new CallExpr(*$1, *$3); delete $1; delete $3; }
    ;

// vector<Expr*>*
arg_list:
    ass_expr
        { $$ = new vector<Expr*>{$1}; }
    | arg_list ',' ass_expr
        { $1->push_back($3); $$ = $1; }
    ;

// vector<Expr*>*
arg_list_:
    %empty
        { $$ = new vector<Expr*>; }
    | arg_list
        { $$ = $1; }
    ;

// RefExpr*
ref:
    ID array_index
        { $$ = new RefExpr(*$1, *$2); delete $1; delete $2; }
    ;

// vector<Expr*>*
array_index:
    %empty
        { $$ = new vector<Expr*>; }
    | array_index '[' expr ']'
        { $1->push_back($3); $$ = $1; }
    ;

%%