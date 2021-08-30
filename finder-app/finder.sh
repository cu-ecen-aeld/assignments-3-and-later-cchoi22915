#!/bin/bash
#Finder script
#Chris Choi
#AESD CU Boulder Fall 2021
#Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir; the second argument is a text string which will be searched within these files, referred to below as searchstr

#Exits with return value 1 error and print statements if any of the parameters above were not specified

#Exits with return value 1 error and print statements if filesdir does not represent a directory on the filesystem

#Prints a message "The number of files are X and the number of matching lines are Y" where X is the number of files in the directory and all subdirectories and Y is the number of matching lines found in respective files.

echo "Finder script by Chris Choi."

if [ $# -lt 2 ]
then
	echo "ERROR: Incorrect arguments passed."
	echo "Exitting - argument 1 = directory to search, argument 2 = string to search."
	exit 1
fi

if [ -d "$1" ]; 
then
    echo "Searching in directory $1."
else
    if [ -f "$1" ]; 
    then
        echo "Found File $1.";
    else
        echo "ERROR: $1 is not a valid directory... exitting...";
        exit 1
    fi
fi

echo "Searching for: $2."
echo "The number of files are $(find "$1" -type f | wc -l) and the number of matching lines are $(grep -ior "$2" "$1" | wc -w)"


