SOURCE_FILES = ./main.c
OUTPUT=main.out

CFLAGS=-ggdb `pkg-config --cflags libpulse`
LIBS=`pkg-config --libs libpulse`

main: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)

run:
	bin/$(OUTPUT)

compile_db: Makefile
	bear -- make

clean:
	rm bin/$(OUTPUT)
