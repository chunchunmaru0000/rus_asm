тест: асм
	./асм тест.асм тест -о 0
	xxd тест
	chmod +x тест
	./тест

асм:
	gcc main.c flgs.c gner.c \
	cmnd.c cmnd_arrs.c cmnd1.c cmnd2.c cmnd3.c \
	lbel.c pser.c pser_expr.c pser_wrds.c lsts.c tzer.c -o \
	асм

1: асм
	fasm test.asm
	./асм тест1.асм тест1
	xxd test
	xxd тест1
	#rm test
	#rm тест1

bg: асм
	./асм тест2.асм тест2

WARNS := 0
DEBUG := 0
OPT := 3
TESTS := ../tests/
BUILD := ../tests/build
TS := $(wildcard $(TESTS)/*.асм)
BS := $(patsubst $(TESTS)/%.асм, $(BUILD)/%, $(TS))

b:
	mkdir -p $(BUILD)

$(BUILD)/%: $(TESTS)%.асм асм b
	@./асм $< $@ -о $(DEBUG) -п $(WARNS) -с $(OPT)
	@if [ "$(basename $(notdir $<))" = "тест_вывод_числа" ]; then \
		chmod +x $@ ; $@ ; \
	fi

all: $(BS)
	@./асм тест.асм тест -о $(DEBUG) -п $(WARNS) -с $(OPT)
	@./асм тест_заглавными.асм тест_заглавными -о $(DEBUG) -п $(WARNS) -с $(OPT)
	@chmod +x тест_заглавными
	./тест_заглавными < t.txt
	rm -rf $(BUILD)
	rm тест
	rm тест_заглавными

t:
	gcc test.c -o test
	./test
