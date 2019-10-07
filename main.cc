#include "ast.hh"
#include "type_checker.hh"
#include "eyr_emitter.hh"
#include <cstdio>
#include <iostream>

extern int yyparse();

extern FILE *yyin;

extern void yyerror(const char *s) {
    std::cout << "unknown token: " << s << std::endl;
}

int main(int argc, char **argv) {
    using namespace mc;
    using namespace std;
    using namespace eyr;

    if (argc > 1)
        yyin = fopen(argv[1], "r");

    yyparse();

    ASTPrinter printer(cout);
    printer.print(*global_prog);

    TypeChecker checker;
    auto ok = checker.check(*global_prog);
    std::cout << std::boolalpha;
    std::cout << ok << std::endl;

    EyrEmitter emitter;
    auto mod = emitter.emit(*global_prog);
    std::cout << *mod;
}