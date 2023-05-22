#!/bin/bash

# Build in the src directory.
cd src                                      && \
bison -d parse.ypp -Wother -Wcounterexample && \
flex lex.l                                  && \
g++ parse.tab.cpp lex.yy.c -o methanol      && \
rm parse.tab.* lex.yy.*                     && \

# Return to the original directory.
cd -                            && \
cp src/methanol methanol.exe    && \
chmod +x methanol.exe