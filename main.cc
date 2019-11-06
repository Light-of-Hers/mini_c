#include "ast.hh"
#include "config.hh"
#include "eyr_emitter.hh"
#include "type_checker.hh"
#include <cstdio>
#include <iostream>

extern int yyparse();

extern FILE *yyin;
extern int yylineno;

extern void yyerror(const char *s) {
    std::cerr << "error:" << yylineno << ": " << s << std::endl;
    exit(1);
}

int main(int argc, char **argv) {
    using namespace mc;
    using namespace std;
    using namespace eyr;

    if (argc > 1)
        yyin = fopen(argv[1], "r");

    yyparse();

#ifdef DEBUG
    ASTPrinter printer(cout);
    printer.print(*global_prog);
#endif
    TypeChecker checker;
    auto ok = checker.check(*global_prog);
    (void)ok;
#ifdef DEBUG
    std::cout << std::boolalpha;
    std::cout << std::endl << "Type Check OK: " << ok << std::endl;
    if (!ok)
        return 1;
#endif
    EyrEmitter emitter;
    auto mod = emitter.emit(*global_prog);
    std::cout << *mod;
}