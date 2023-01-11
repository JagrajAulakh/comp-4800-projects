
SOURCE_FILES = ./main.c
OUTPUT=main.out

CFLAGS=`pkg-config --cflags cairo`
LIBS=`pkg-config --libs cairo`

main: $(SOURCE_FILES)
	gcc $(CFLAGS) $(LIBS) $(SOURCE_FILES) -o $(OUTPUT)


clean:
	rm $(OUTPUT)
