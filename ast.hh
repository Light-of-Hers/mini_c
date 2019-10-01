#ifndef __MC_AST_HH__
#define __MC_AST_HH__

#include "type.hh"
#include <iostream>
#include <string>
#include <utility>

namespace mc {

struct ASTVisitor;
struct AST;
struct FuncDefn;
struct Stmt;
struct IfStmt;
struct WhileStmt;
struct ReturnStmt;
struct BlockStmt;
struct DeclStmt;
struct Expr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct RefExpr;
struct NumExpr;

#define VISITS()                                                                   \
    V(FuncDefn)                                                                \
    V(IfStmt)                                                                  \
    V(WhileStmt)                                                               \
    V(ReturnStmt)                                                              \
    V(BlockStmt)                                                               \
    V(DeclStmt)                                                                \
    V(BinaryExpr)                                                              \
    V(UnaryExpr)                                                               \
    V(RefExpr)                                                                 \
    V(CallExpr)                                                                \
    V(NumExpr)

struct ASTVisitor {
#define V(x) virtual void visit(x *ast) = 0;
    VISITS()
#undef V
};

struct ASTPrinter : public ASTVisitor {
#define V(x) void visit(x *ast) override;
    VISITS()
#undef V

    explicit ASTPrinter(std::ostream &os) : os(os) {}

    void print(AST *ast);

private:
    std::vector<int> depths;
    std::ostream &os;
};

struct AST {
    virtual void accept(ASTVisitor &vis) = 0;

    virtual ~AST() = default;
};

#define OVERRIDE()                                                             \
    void accept(ASTVisitor &vis) override { vis.visit(this); }

struct FuncDefn : public AST {
    OVERRIDE()

    FuncDefn(Type *r, std::string n, std::vector<Field *> ps,
             std::vector<Stmt *> b)
            : ret_type(r), name(std::move(n)), params(std::move(ps)),
              body(std::move(b)) {
        type = new FuncType(r, ps);
    }
    ~FuncDefn() override;

    Type *getRetType() const { return ret_type; }
    const std::string &getName() const { return name; }
    const std::vector<Field *> &getParams() const { return params; }
    const std::vector<Stmt *> &getBody() const { return body; }
    Type *getType() const { return type; }

private:
    Type *type;
    Type *ret_type;
    std::string name;
    std::vector<Field *> params;
    std::vector<Stmt *> body;
};

struct Stmt : public AST {
};

struct IfStmt : public Stmt {
    OVERRIDE()

    IfStmt(Expr *c, Stmt *t, Stmt *a = nullptr) : cond(c), then(t), alter(a) {}
    ~IfStmt() override;

    Expr *getCond() const { return cond; }
    Stmt *getThen() const { return then; }
    Stmt *getAlter() const { return alter; }

private:
    Expr *cond;
    Stmt *then;
    Stmt *alter;
};

struct WhileStmt : public Stmt {
    OVERRIDE()

    WhileStmt(Expr *t, Stmt *l) : test(t), loop(l) {}
    ~WhileStmt() override;

    Expr *getTest() const { return test; }
    Stmt *getLoop() const { return loop; }

private:
    Expr *test;
    Stmt *loop;
};

struct BlockStmt : public Stmt {
    OVERRIDE()

    explicit BlockStmt(std::vector<Stmt *> ss) : stmts(std::move(ss)) {}
    ~BlockStmt() override;

    const std::vector<Stmt *> &getStmts() const { return stmts; }

private:
    std::vector<Stmt *> stmts;
};

struct ReturnStmt : public Stmt {
    OVERRIDE()

    explicit ReturnStmt(Expr *v) : value(v) {}
    ~ReturnStmt() override;

    Expr *getValue() const { return value; }

private:
    Expr *value;
};

struct DeclStmt : public Stmt {
    OVERRIDE()

    DeclStmt(const std::string &s, Type *t) : var(s, t) {}
    explicit DeclStmt(const Field &v) : var(v) {}

    const Field &getVar() const { return var; }

private:
    Field var;
};

struct Expr : public Stmt {
    virtual bool isLhsValue() const { return false; }

    Type *getType() const { return type; }
    void setType(Type *t) { type = t; }

protected:
    Type *type{}; // 类型检查时使用
};

struct BinaryExpr : public Expr {
    OVERRIDE()
    // bool isLhsValue() const override;

    enum class BinOp {
        COMMA = 0, ASSIGN, EQ, NE, LT, GT, OR, AND, ADD, SUB, MUL, DIV, REM,
    };
    static const char *const OpStr[];

    BinaryExpr(BinOp op, Expr *l, Expr *r) : opt(op), lhs(l), rhs(r) {}
    ~BinaryExpr() override;

    BinOp getOpt() const { return opt; }
    Expr *getLhs() const { return lhs; }
    Expr *getRhs() const { return rhs; }

    bool isLhsValue() const override;

private:
    BinOp opt;
    Expr *lhs;
    Expr *rhs;
};

struct UnaryExpr : public Expr {
    OVERRIDE()

    enum class UnOp {
        NEG = 0,
        NOT,
    };
    static const char *const OpStr[];

    UnaryExpr(UnOp op, Expr *r) : opt(op), opr(r) {}
    ~UnaryExpr() override;

    UnOp getOpt() const { return opt; }
    Expr *getOpr() const { return opr; }

private:
    UnOp opt;
    Expr *opr;
};

struct RefExpr : public Expr {
    OVERRIDE()

    bool isLhsValue() const override { return true; }

    RefExpr(std::string n, std::vector<int> i)
            : name(std::move(n)), index(std::move(i)) {}

    const std::string &getName() const { return name; }
    const std::vector<int> &getIndex() const { return index; }

private:
    std::string name;
    std::vector<int> index;
};

struct CallExpr : public Expr {
    OVERRIDE()

    CallExpr(std::string n, std::vector<Expr *> as)
            : name(std::move(n)), args(std::move(as)) {}
    ~CallExpr() override;

    const std::string &getName() const { return name; }
    const std::vector<Expr *> &getArgs() const { return args; }

private:
    std::string name;
    std::vector<Expr *> args;
};

struct NumExpr : public Expr {
    OVERRIDE()

    explicit NumExpr(int n) : num(n) {}

    int getNum() const { return num; }

private:
    int num;
};

#undef OVERRIDE

extern std::vector<AST *> *prog;

}; // namespace mc

#endif