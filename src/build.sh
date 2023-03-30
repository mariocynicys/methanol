clear
bison -d parse.ypp -Wother -Wcounterexample && echo "YPP OK" || echo "YPP FAIL"
flex lex.l && echo "LEX OK" || echo "LEX FAIL"
g++ parse.tab.cpp lex.yy.c -o methanol && echo "G++ OK" || echo "G++ FAIL"
rm parse.tab.* lex.yy.*