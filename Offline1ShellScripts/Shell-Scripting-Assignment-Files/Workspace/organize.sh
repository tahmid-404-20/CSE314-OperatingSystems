#!/bin/bash

# recursively(dfs) the root folder and move the .c, .java, .py files to target folder
visit() {
    
    if [ -d "$1" ]
    then
        for i in "$1"/*
        do
            visit "$i" $2 $3
        done
    elif [ -f "$1" ]
    then
        # echo "$1"
        str="$1"
        cExt=${str: -2}
        javaExt=${str: -5}
        pyExt=${str: -3}

        c=.c
        java=.java
        python=.py


        if [ $cExt == $c ]; then
            mkdir "$3"/C/$2
            mv "$1" "$3"/C/$2/main.c
        elif [ $javaExt == $java ]; then
            mkdir "$3"/Java/$2
            mv "$1" "$3"/Java/$2/Main.java
        elif [ $pyExt == $python ]; then
            mkdir "$3"/Python/$2
            mv "$1" "$3"/Python/$2/main.py
        fi

    fi     
}

# remove the targets folder, create directories and subdirectories
rm -rf $2
mkdir  $2
mkdir $2/C
mkdir $2/Java
mkdir $2/Python

uzip="unzipped"
rm -rf "$1"/$uzip

# go through the files in submissions folder
for i in "$1"/* 
do
    if [ -f "$i" ]; then
        fileName="$i"
        
        stdId=${fileName%.zip}
        stdId=${stdId: -7}
        # echo $stdId

        mkdir "$1"/$uzip
        unzip -qq "$i" -d "$1"/$uzip
        visit "$1"/$uzip $stdId $2
        rm -r "$1"/$uzip
        
    fi
done

# Now compiling and executing the programs
testCount=`ls "$3" | wc -l`

# C program
for i in "$2"/C/*
do  
    gcc "$i"/main.c -o "$i"/main.out

    # echo ls "$3/" | wc -l
    for((j=1;j<=$testCount;j++))
    do
       file="out$j.txt"
       touch "$i"/$file 
       "$i"/main.out < "$3/test$j.txt" > "$i"/$file

    #    echo "$i"/main "$3/test$j.txt" "$i"/$file
    done
done


# Java
for i in "$2"/Java/*
do  
    javac "$i"/Main.java

    for((j=1;j<=$testCount;j++))
    do
       file="out$j.txt"
       touch "$i"/$file 
       java -cp "$i" Main < "$3/test$j.txt" > "$i"/$file

    #    echo "$i"/Main "$3/test$j.txt" "$i"/$file
    done
done


# Python
for i in "$2"/Python/*
do  

    for((j=1;j<=$testCount;j++))
    do
       file="out$j.txt"
       touch "$i"/$file 
       python3 "$i"/main.py < "$3/test$j.txt" > "$i"/$file

    #    echo "$i"/Main "$3/test$j.txt" "$i"/$file
    done
done