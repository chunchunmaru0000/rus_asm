тест: асм
	./асм тест.асм тест -о 0
	xxd тест
	chmod +x тест
	./тест

ч: асм
	./асм чето.асм чето
	xxd чето

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

all: $(BS)
	@./асм тест.асм тест -о $(DEBUG) -п $(WARNS)
	@./асм тест_заглавными.асм тест_заглавными -о $(DEBUG) -п $(WARNS)
	rm -rf $(BUILD)
