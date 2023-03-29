SOURCE_FILES = ./main.c
OUTPUT=main.out

CFLAGS=-ggdb `pkg-config --cflags libpulse libavformat libavcodec libavutil`
LIBS=`pkg-config --libs libpulse libavformat libavcodec libavutil` -lm

main: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(SOURCE_FILES) $(LIBS) -o bin/$(OUTPUT)

run:
	bin/$(OUTPUT)

compile_db: Makefile
	bear -- make

clean:
	rm bin/$(OUTPUT)
