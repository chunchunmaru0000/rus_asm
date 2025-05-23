#!/bin/bash

file="$1"
dict=(
	"mov быть"
	"cmp срав"
	"ja идв"
	"je идр"
	"jb идп"
	"jb идп"
)

for pair in "${dict[@]}"; do
	read -r old new <<< "$pair"
	was=$(grep -o "$old" "$file" | wc -l)
	sed -i "s/$old/$new/g" $file
	echo -e "$old\t->\t$new\t$was раз"
done
