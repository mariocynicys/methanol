clear
bison -d parse.ypp -Wother -Wcounterexample
flex lex.l
g++ parse.tab.cpp lex.yy.c -o methanol
rm parse.tab.* lex.yy.*