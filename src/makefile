all: асм
	./асм тест.асм тест -о 0
	./асм тест_заглавными.асм тест_заглавными -о 0

тест: асм
	./асм тест.асм тест
	xxd тест
	chmod +x тест
	./тест

тест1: асм
	cat тест1.асм
	./асм тест1.асм тест1
	xxd тест1
	rm тест1

быть: асм
	./асм быть.асм быть
	xxd быть

асм:
	gcc main.c flgs.c gner.c cmnd.c pser.c lists.c tzer.c -o асм

test1:
	fasm test1.asm
	xxd test1
	cat test1.asm
