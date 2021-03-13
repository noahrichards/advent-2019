#!/bin/bash

day=$1
mkdir ${day}
cp template/* ${day}/
sed -i.bak -e "s/\${day}/${day}/" ${day}/BUILD
rm ${day}/BUILD.bak
