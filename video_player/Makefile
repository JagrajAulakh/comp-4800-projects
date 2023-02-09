SOURCE_FILES = ./main.c ./firework.c ./firework.h
OUTPUT=main.out

CFLAGS=`pkg-config --cflags cairo gtk4`
LIBS=`pkg-config --libs cairo gtk4`

build: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc -ggdb $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT) -lm

firework: firework.c
	mkdir -p bin/ && \
	gcc -ggdb $(CFLAGS) $(LIBS) firework.c -o bin/firework -lm

run:
	bin/$(OUTPUT)

compile_db: Makefile
	bear -- make

clean:
	rm bin/$(OUTPUT)
