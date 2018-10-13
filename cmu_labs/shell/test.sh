#!/bin/bash

# if no args we run all tests from 1 to 16
n1=1
n2=16

# if args we use those
if [ "$1" != "" ]; then
    n1=$1
    n2=$2
    # if only one arg we run only one test
    if [ "$2" == "" ]; then
        n2=$n1
    fi
fi

# run all tests from n1 to n2, stop at first failed test
for i in $(seq $n1 $n2); do
    if [ "$i" -lt "10" ]; then
        i="0$i"
    fi

    outA=$(make test$i)
    outB=$(make rtest$i)

    # remove pid and jid from () and []
    outA1=$(echo $outA | sed -e 's/([^()]*)/()/g' -e 's/\[[^][]*\]/[]/g')
    outB1=$(echo $outB | sed -e 's/([^()]*)/()/g' -e 's/\[[^][]*\]/[]/g')

    # change tsh in tshref
    outA1=$(echo $outA1 | sed -e 's/.\/tsh/.\/tshref/g')

    # change make test in make rtest
    outA1=$(echo $outA1 | sed -e 's/make test/make rtest/g')

    # remove 5 digits numbers for 'ps a' output
    outA1=$(echo $outA1 | sed -e 's/[0-9][0-9][0-9][0-9][0-9]/*/g')
    outB1=$(echo $outB1 | sed -e 's/[0-9][0-9][0-9][0-9][0-9]/*/g')

    if [ "$outA1" = "$outB1" ]; then
        printf "Test %s passed\n" "$i"
    else
        printf "\nTest %s failed !!!\n\n" "$i"
        printf "You->\n%s\n\n" "$outA"
        printf "Reference->\n%s\n" "$outB"
        break
    fi
done
