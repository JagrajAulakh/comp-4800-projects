## Prerequisites
Make sure you have the following installed:

### gcc
- [WSL on windows](https://learn.microsoft.com/en-us/windows/wsl/install)
- MacOS: `brew install gcc`
- Linux
	- Arch based: `pacman -S gcc`
	- Debian based: `apt install gcc`


Verify installation:
```bash
$ gcc -v
...
gcc version 12.2.0 (GCC)
```

Version doesn't matter, as long as it's somewhat recent like > `10.0.0`

### make
- Use WSL on Windows
- MacOS: `brew install make`
- Linux
	- Arch based: `pacman -S make`
	- Debian based: `apt install make`

Verify installation:
```bash
$ make -v
GNU Make 4.3
...
```


<!-- TODO: Cairo installation -->
