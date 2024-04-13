#!/bin/sh

if [ "$#" -ne 2 ]; then
  echo "incorrect number of arguments"
  echo "usage: $0 <filesdir> <searchstr>"
  exit 1
fi

varWriteFile="$1"
varWriteStr="$2"

varWriteDir="$(dirname $varWriteFile)"

mkdir -p $varWriteDir

if [ "$?" -ne 0 ]; then
  echo "$varWriteDir could not be created!"
  exit 1
fi

touch $varWriteFile

if [ "$?" -ne 0 ]; then
  echo "$varWriteDir could not be created!"
  exit 1
fi

echo $varWriteStr >$varWriteFile
