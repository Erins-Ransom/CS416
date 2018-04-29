#!/bin/bash

# make a big file
for i in `seq 1 1000`;
do 
	echo "writing stuff...\nNeed more filler: 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234546789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890" >> /tmp/erk58/mountdir/file.txt
done

# make some directories
mkdir /tmp/erk58/mountdir/dir1
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/file.txt

mkdir /tmp/erk58/mountdir/dir2
echo "I'm here!" >> /tmp/erk58/mountdir/dir2/file.txt

mkdir /tmp/erk58/mountdir/dir3
echo "I'm here!" >> /tmp/erk58/mountdir/dir3/file.txt

mkdir /tmp/erk58/mountdir/dir4
echo "I'm here!" >> /tmp/erk58/mountdir/dir4/file.txt

mkdir /tmp/erk58/mountdir/dir5
echo "I'm here!" >> /tmp/erk58/mountdir/dir5/file.txt

mkdir /tmp/erk58/mountdir/dir1/dirA
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirA/file.txt

mkdir /tmp/erk58/mountdir/dir1/dirB
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirB/file.txt

mkdir /tmp/erk58/mountdir/dir1/dirC
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirC/file.txt
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirC/file1.txt
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirC/file2.txt
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirC/file3.txt
echo "I'm here!" >> /tmp/erk58/mountdir/dir1/dirC/file4.txt

