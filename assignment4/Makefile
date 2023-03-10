SOURCE_FILES = ./a42.c
OUTPUT=a42.out

CFLAGS=`pkg-config --cflags cairo gtk4 libavcodec libavutil libavformat libswscale`
LIBS=`pkg-config --libs cairo gtk4 libavcodec libavutil libavformat libswscale`

build: $(SOURCE_FILES)
	mkdir -p bin/ && \
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o bin/$(OUTPUT)