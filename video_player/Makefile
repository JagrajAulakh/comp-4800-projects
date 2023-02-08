SOURCE_FILES = ./a2.c
OUTPUT=a2.out

CFLAGS=`pkg-config --cflags cairo gtk4 libavcodec libavutil libavformat libswscale`
LIBS=`pkg-config --libs cairo gtk4 libavcodec libavutil libavformat libswscale`

build: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)
