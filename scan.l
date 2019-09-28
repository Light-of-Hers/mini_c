%{
    #include "ast.hh"
    #include "type.hh"
    using namespace std;
    using namespace mc;

    #include <vector>
    #include PARSER_H
%}

%option nounput noyywrap

alpha   [a-zA-Z]
digit   [0-9]

%%

"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"return"    { return RETURN; }
"int"       { return INT; }
"=="        { return EQ; }
"!="        { return NE; }
"||"        { return OR; }
"&&"        { return AND; }

[+\-*/%\[\](){},;=<>]  { return yytext[0]; }

{digit}+    { yylval.i = atoi(yytext); return NUM; }

({alpha}|_)({alpha}|{digit}|_)* { yylval.s = new string(yytext); return ID; }

[ \t\n\r] {}

%%