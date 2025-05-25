тест: асм
	./асм тест.асм тест
	xxd тест
	chmod +x тест
	./тест

асм:
	gcc main.c flgs.c gner.c cmnd.c pser.c lists.c tzer.c -o асм


TESTS := ../tests/
BUILD := ../tests/build
TS := $(wildcard $(TESTS)/*.асм)
BS := $(patsubst $(TESTS)/%.асм, $(BUILD)/%, $(TS))

$(BUILD)/%: $(TESTS)%.асм асм
	mkdir -p $(BUILD)
	./асм $< $@ -о 0

all: $(BS)
	./асм тест.асм тест -о 0
	./асм тест_заглавными.асм тест_заглавными -о 0
	rm -rf $(BUILD)
