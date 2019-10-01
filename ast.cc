#include "ast.hh"

namespace mc {

FuncDefn::~FuncDefn() {
    delete (ret_type);
    for (auto p : params)
        delete (p);
    for (auto s : body)
        delete (s);
}

IfStmt::~IfStmt() {
    delete (cond);
    if (then)
        delete (then);
    if (alter)
        delete (alter);
}

WhileStmt::~WhileStmt() {
    delete (test);
    if (loop)
        delete (loop);
}

BlockStmt::~BlockStmt() {
    for (auto s : stmts)
        delete (s);
}

ReturnStmt::~ReturnStmt() {
    delete (value);
}

BinaryExpr::~BinaryExpr() {
    delete (lhs);
    delete (rhs);
}

UnaryExpr::~UnaryExpr() {
    delete (opr);
}

CallExpr::~CallExpr() {
    for (auto arg: args)
        delete (arg);
}

std::vector<AST *> *prog = nullptr;

const char *const BinaryExpr::OpStr[] = {",", "=", "==", "!=", "<", ">", "||",
                                         "&&", "+", "-", "*", "/", "%"};
bool BinaryExpr::isLhsValue() const {
    if (opt == BinOp::ASSIGN)
        return true;
    else if (opt == BinOp::COMMA)
        return rhs->isLhsValue();
    return false;
}

const char *const UnaryExpr::OpStr[] = {"-", "!"};


}; // namespace mc