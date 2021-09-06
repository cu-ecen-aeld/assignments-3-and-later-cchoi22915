#!/bin/bash
#Writer Script
#Chris Choi
#AESD CU Boulder Fall 2021

#Accepts the following arguments: the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile; the second argument is a text string which will be written within this file, referred to below as writestr

#Exits with value 1 error and print statements if any of the arguments above were not specified

#Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. Exits with value 1 and error print statement if the file could not be created.

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
        echo "Found file $1.";
    else
    	touch $1
	if [ -f "$1" ]; 
	then
		echo "Created file $1.";
	else
		echo "ERROR: $1 is not a valid directory or file... exitting.";
		exit 1
	fi
    fi
fi

echo "$2" >> "$1"
