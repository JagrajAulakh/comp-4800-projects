This program takes command line arguments: a video file, and the video's framerate. It
will decode and output the video's frames in two separate threads.

# Compiling and running the code

```bash
# -- COMPILE --
$ make

# -- RUN --
$ bin/a4.out path/to/video framerate

# example
$ bin/a4.out ./sample_no_audio.mp4 60

```

I have included and sample video file that I used for testing. My program works _for
sure_ with the included sample video.
