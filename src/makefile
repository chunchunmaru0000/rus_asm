тест: асм
	./асм тест.асм тест -о 0
	xxd тест
	chmod +x тест
	./тест

асм:
	gcc main.c flgs.c gner.c cmnd.c lbel.c pser.c lsts.c tzer.c -o асм


WARNS := 0
DEBUG := 0
TESTS := ../tests/
BUILD := ../tests/build
TS := $(wildcard $(TESTS)/*.асм)
BS := $(patsubst $(TESTS)/%.асм, $(BUILD)/%, $(TS))

b:
	mkdir -p $(BUILD)

$(BUILD)/%: $(TESTS)%.асм асм b
	@./асм $< $@ -о $(DEBUG) -п $(WARNS)
	@if [ "$(basename $(notdir $<))" = "тест_вывод_числа" ]; then \
		chmod +x $@ ; $@ ; \
	fi

all: $(BS)
	@./асм тест.асм тест -о $(DEBUG) -п $(WARNS)
	@./асм тест_заглавными.асм тест_заглавными -о $(DEBUG) -п $(WARNS)
	@chmod +x тест_заглавными
	./тест_заглавными < t.txt
	rm -rf $(BUILD)
	rm тест
	rm тест_заглавными
