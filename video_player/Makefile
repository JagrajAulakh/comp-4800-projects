
SOURCE_FILES = ./main.c
OUTPUT=main.out

CFLAGS=`pkg-config --cflags cairo gtk+-3.0`
LIBS=`pkg-config --libs cairo gtk+-3.0`

main: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)

compile_db: Makefile
	bear -- make

clean:
	rm bin/$(OUTPUT)
