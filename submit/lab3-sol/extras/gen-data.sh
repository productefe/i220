#!/usr/bin/bash

RANDOM=789
if ! pwd | grep lab3-sol$ > /dev/null
then
    echo "you must run this script from your lab3-sol directory"
    exit 1
fi

rm -rf data;
mkdir data; cd data

for d in `seq 3 3 20`
do
    v1=$((97 + $RANDOM%26))
    dir=`printf $(printf '\\%o' $v1)`-$d
    mkdir $dir
    pushd $dir >/dev/null
    nFiles=$((70 + 7*$RANDOM%9))
    for n in `seq 1 7 $((7*$nFiles))`
    do
	x1=$(($RANDOM%4))
	{ [ $x1 -eq 0 ] && c1=x; } || \
	{ [ $x1 -eq 1 ] && c1=z; } || \
	{ [ $x1 -eq 2 ] && c1=k; } || \
	{ [ $x1 -eq 3 ] && c1=j; }
	x2=$((97 + $RANDOM%26))
	c2=`printf $(printf '\\%o' $x2)`
	x3=$((97 + $RANDOM%26))
	c3=`printf $(printf '\\%o' $x3)`
	z=$(($RANDOM%4))
	{ [ $z -eq 0 ] && ext=ts; } || \
	{ [ $z -eq 1 ] && ext=c; } || \
	{ [ $z -eq 2 ] && ext=java; } || \
	{ [ $z -eq 3 ] && ext=js; }
	fName=$c2$c1$c3-$n.$ext
	printf "%*s\n" $(($RANDOM%50)) 'hello world' > $fName
	touch -d "2024-$(($RANDOM%12 + 1))-$(($RANDOM%28 + 1))" $fName
    done
    popd >/dev/null
    touch -d "2024-$(($RANDOM%12 + 1))-$(($RANDOM%28 + 1))" $dir
done
exit
which printf;
for s in `seq 7 5 100`
do
    printf "%*s" $(($s%10)) 'hello world';
done
