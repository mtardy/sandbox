#!/bin/bash
PROG=calc

make

echo "2+3" | ./${PROG} | cmp <(echo "5") && echo "test#1 passed!" || true
echo "2-3" | ./${PROG} | cmp <(echo "-1") && echo "test#2 passed!" || true
echo "2--3" | ./${PROG} | cmp <(echo "5") && echo "test#3 passed!" || true
echo "-2+3" | ./${PROG} | cmp <(echo "1") && echo "test#4 passed!" || true
echo "+2+3" | ./${PROG} | cmp <(echo "5") && echo "test#5 passed!" || true
echo "2*3" | ./${PROG} | cmp <(echo "6") && echo "test#6 passed!" || true
echo "4/2" | ./${PROG} | cmp <(echo "2") && echo "test#7 passed!" || true
echo "3/2" | ./${PROG} | cmp <(echo "1") && echo "test#8 passed!" || true
echo "5%2" | ./${PROG} | cmp <(echo "1") && echo "test#9 passed!" || true
echo "3*2+10*2" | ./${PROG} | cmp <(echo "26") && echo "test#10 passed!" || true
echo "-3*2+10*2" | ./${PROG} | cmp <(echo "14") && echo "test#11 passed!" || true
echo "- 5%  2 +2-6 / 2" | ./${PROG} | cmp <(echo "-2") && echo "test#12 passed!" || true

make clean