cmd=$1
file=tmp

echo -e "format binary\nuse64\norg 0\n$cmd" > "$file.asm"
fasm "$file.asm"
xxd "$file.bin"

rm "$file.asm"
rm "$file.bin"
