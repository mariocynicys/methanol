%{
    #include "parse.tab.hpp"
%}
%option yylineno
%option noyywrap

%%
    /* Data Types */
"int"     return INTEGER_TYPE_DECLARATION;
"flt"     return DOUBLE_TYPE_DECLARATION;
"log"     return LOGICAL_TYPE_DECLARATION;
"str"     return STRING_TYPE_DECLARATION;
"enum"    return ENUM_TYPE_DECLARATION;
"const"   return CONSTANT;

    /* Key Words */
"print"       return PRINT;
"if"          return IF;
"else"        return ELSE;
"while"       return WHILE;
"for"         return FOR;
"repeat"      return REPEAT;
"until"       return UNTIL;
"switch"      return SWITCH;
"case"        return CASE;
"default"     return DEFAULT;
"return"      return RETURN;

    /* Operators */
"+"     return PLUS;
"-"     return MINUS;
"*"     return MULT;
"/"     return DIV;
"!"     return NOT;
"&"     return AND;
"|"     return OR;
"=="    return EQ;
"!="    return NE;
">"     return GT;
"<"     return LT;
">="    return GTE;
"<="    return LTE;

\n          {} /* Newlines */
[ \t]+      {} /* Whitespaces */
\/\/[^\n]*  {} /* Comments */

    /* Strings */
\"[^"\n]*\" {
    yylval.STRING = strdup(yytext + 1);
    yylval.STRING[yyleng - 2] = 0;
    return STRING;
}

    /* Logicals */
true {
    yylval.LOGICAL = 1;
    return LOGICAL;
}
false {
    yylval.LOGICAL = 0;
    return LOGICAL;
}

    /* Numbers */
[0-9]+ {
    yylval.INTEGER = atoi(yytext);
    return INTEGER;
}
[0-9]*\.[0-9]+ {
    yylval.DOUBLE = atof(yytext);
    return DOUBLE;
}

    /* Identifiers */
[_a-zA-Z][_a-zA-Z0-9]* {
    yylval.IDENTIFIER = strdup(yytext);
    return IDENTIFIER;
}

    /* Other known characters */
[\.,;:=(){}[\]]             return *yytext;


    /* Unknown characters */
.                           return ERROR;
%%