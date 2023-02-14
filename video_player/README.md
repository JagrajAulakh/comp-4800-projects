This program takes command line arguments: a video file, an output directory, and an
optional frame number. It will extract the frame number (or frame 1 of not specified)
and output 5 different grayscale images to the output directory.

# Compiling and running the code

```bash
# -- COMPILE --
$ make

# -- RUN --
$ bin/grayscale.out path/to/video outputdir frame

# example
$ bin/grayscale.out ./sample.mp4 out 20

```

