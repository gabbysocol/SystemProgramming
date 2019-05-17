#!/bin/bash


{
find "$(readlink -f "$1")" -size +${2}c -size -${3}c -type f -printf "%p  %s\n" > $4 
find $1 -type f | wc -l 
} 2>/tmp/error.txt


cat /tmp/error.txt | while read line 
do
 	echo $(basename $0) $line >&2
done