#include "ast.hh"

namespace mc {

std::vector<AST *> *global_prog = nullptr;

const char *const BinaryExpr::OpStr[] = {",", "=", "==", "!=", "<", ">", "||",
                                         "&&", "+", "-", "*", "/", "%"};

bool BinaryExpr::isLhsValue() const {
    return opt == BinOp::ASSIGN;
}

const char *const UnaryExpr::OpStr[] = {"-", "!"};


}; // namespace mc