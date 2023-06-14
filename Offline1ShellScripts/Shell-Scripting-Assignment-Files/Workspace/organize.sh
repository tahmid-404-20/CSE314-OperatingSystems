#!/bin/bash

# recursively(dfs) the root folder and move the .c, .java, .py files to target folder
fileType=""
testCount=""

execute() {

    filePath="$2/$fileType/$5"

    c=C
    java=Java
    python=Python

    # compile the program
    if [ $fileType == $c ]; then
        gcc "$filePath"/main.c -o "$filePath"/main.out

    elif [ $fileType == $java ]; then

        javac "$filePath"/Main.java
    fi

    # execute tests
    matched=0
    notMatched=0
    for ((j = 1; j <= $testCount; j++)); do
        file="out$j.txt"
        touch "$filePath"/$file

        if [ $fileType == $c ]; then
            "$filePath"/main.out <"$3/test$j.txt" >"$filePath"/$file

        elif [ $fileType == $java ]; then
            java -cp "$filePath" Main <"$3/test$j.txt" >"$filePath"/$file

        elif [ $fileType == $python ]; then
            python3 "$filePath"/main.py <"$3/test$j.txt" >"$filePath"/$file
        fi

        # match the tests
        if [ -z "$(diff $4/ans$j.txt $filePath/$file)" ]; then
            matched=$(($matched + 1))
        else
            notMatched=$(($notMatched + 1))
        fi
    done

    echo "$5,$fileType,$matched,$notMatched" >>"$2"/result.csv
}

visit() {

    if [ -d "$1" ]; then
        for i in "$1"/*; do
            visit "$i" $2 $3
        done
    elif [ -f "$1" ]; then
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
            fileType="C"
        elif [ $javaExt == $java ]; then
            mkdir "$3"/Java/$2
            mv "$1" "$3"/Java/$2/Main.java
            fileType="Java"
        elif [ $pyExt == $python ]; then
            mkdir "$3"/Python/$2
            mv "$1" "$3"/Python/$2/main.py
            fileType="Python"
        fi

    fi
}

if [ $# -lt 4 ]; then
    echo "Usage:"
    echo "./organize.sh <submission folder> <target folder> <test folder> <answer folder> [-v] [-noexecute]"
    echo ""
    echo "-v: verbose"
    echo "-noexecute: do not execute code files"

else
    # remove the targets folder, create directories and subdirectories
    rm -rf $2
    mkdir $2
    mkdir $2/C
    mkdir $2/Java
    mkdir $2/Python

    uzip="unzipped"
    rm -rf "$1"/$uzip

    # checking the switches
    verbose=0
    isExecuted=1
    for i in $*; do
        if [ $i == "-v" ]; then
            verbose=1
        elif [ $i == "-noexecute" ]; then
            isExecuted=0
        fi
    done

    testCount=$(ls "$3" | wc -l)

    if [ $isExecuted == 1 ]; then
        touch "$2"/result.csv
        echo "student_id,type,matched,not_matched" >>"$2"/result.csv
    fi

    # go through the files in submissions folder
    for i in "$1"/*; do
        if [ -f "$i" ]; then
            fileName="$i"

            stdId=${fileName%.zip}
            stdId=${stdId: -7}
            # echo $stdId

            mkdir "$1"/$uzip
            unzip -qq "$i" -d "$1"/$uzip

            if [ $verbose == 1 ]; then
                echo "Organizing files of $stdId"
            fi

            visit "$1"/$uzip $stdId $2
            rm -r "$1"/$uzip

            if [ $isExecuted == 1 ]; then
                if [ $verbose == 1 ]; then
                    echo "Executing files of $stdId"
                fi
                execute "$1" "$2" "$3" "$4" $stdId
            fi
        fi
    done

fi

# Now compiling and executing the programs

# # C program
# for i in "$2"/C/*
# do
#     gcc "$i"/main.c -o "$i"/main.out

#     # echo ls "$3/" | wc -l
#     for((j=1;j<=$testCount;j++))
#     do
#        file="out$j.txt"
#        touch "$i"/$file
#        "$i"/main.out < "$3/test$j.txt" > "$i"/$file

#     #    echo "$i"/main "$3/test$j.txt" "$i"/$file
#     done
# done

# # Java
# for i in "$2"/Java/*
# do
#     javac "$i"/Main.java

#     for((j=1;j<=$testCount;j++))
#     do
#        file="out$j.txt"
#        touch "$i"/$file
#        java -cp "$i" Main < "$3/test$j.txt" > "$i"/$file

#     #    echo "$i"/Main "$3/test$j.txt" "$i"/$file
#     done
# done

# # Python
# for i in "$2"/Python/*
# do

#     for((j=1;j<=$testCount;j++))
#     do
#        file="out$j.txt"
#        touch "$i"/$file
#        python3 "$i"/main.py < "$3/test$j.txt" > "$i"/$file

#     #    echo "$i"/Main "$3/test$j.txt" "$i"/$file
#     done
# done
