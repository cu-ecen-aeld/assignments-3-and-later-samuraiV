#!/bin/sh

if [ "$#" -ne 2 ]; then
  echo "incorrect number of arguments"
  echo "usage: $0 <filesdir> <searchstr>"
  exit 1
elif ! [ -d "$1" ]; then
  echo "$1 is not a directory"
  exit 1
fi

varFilesDir="$1"
varSearchStr="$2"

varTotalFiles=$(find $varFilesDir -type f | wc -l)
varMatchedFiles=$(grep -r $varSearchStr $varFilesDir | wc -l)

echo "The number of files are $varTotalFiles and the number of matching lines are $varMatchedFiles"
