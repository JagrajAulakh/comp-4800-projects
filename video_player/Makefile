SOURCE_FILES = ./polish.c
OUTPUT=polish.out

CFLAGS=`pkg-config --cflags cairo gtk4`
LIBS=`pkg-config --libs cairo gtk4`

build: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)