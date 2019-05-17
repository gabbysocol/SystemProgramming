#!/bin/bash
truncate -s 0 $2
exec 2>error.txt

directories=$(find $1 -type d -printf "%p\n")
IFS=$'\n'

for dir in $directories
do
	files=$(find $dir -maxdepth 1 -type f -printf "%p\n")
	fsize_sum=0
	fcount=0
	max=-1
	filename="-"
	for file in $files
	do
		fcount=$(($fcount+1))
		fsize_sum=$(($fsize_sum + $(wc -c<$file)))
		if [ $(wc -c<$file) -gt $max ]
		then
			max=$(wc -c<$file)
			filename=$file
		fi
	done
	echo $(realpath $dir) $fcount $fsize_sum $(basename $filename) >> $2
done
