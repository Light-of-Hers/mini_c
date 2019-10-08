#include "ast.hh"

namespace mc {

std::vector<AST *> *global_prog = nullptr;

const char *const BinaryExpr::OpStr[] = {",", "=", "==", "!=", "<", ">", "||",
                                         "&&", "+", "-", "*", "/", "%"};
bool BinaryExpr::isLhsValue() const {
    if (opt == BinOp::ASSIGN)
        return true;
    return false;
}

const char *const UnaryExpr::OpStr[] = {"-", "!"};


}; // namespace mc