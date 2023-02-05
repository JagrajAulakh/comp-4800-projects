SOURCE_FILES = ./grayscale.c
OUTPUT=grayscale.out

CFLAGS=`pkg-config --cflags cairo gtk4`
LIBS=`pkg-config --libs cairo gtk4 libavcodec libavutil libavformat libswscale`

main: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)

run:
	bin/$(OUTPUT)

compile_db: Makefile
	bear -- make

clean:
	rm bin/$(OUTPUT)
