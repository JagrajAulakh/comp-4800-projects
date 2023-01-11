
SOURCE_FILES = ./main.c
OUTPUT=main.out

CFLAGS=`pkg-config --cflags cairo`
LIBS=`pkg-config --libs cairo`

main: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)

clean:
	rm bin/$(OUTPUT)
