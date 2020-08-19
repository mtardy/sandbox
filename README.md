# Sandbox

## Build the project
### Requirements

Please install [peg/leg](https://www.piumarta.com/software/peg/) manually or with a package manager, for example with Homebrew: 
```bash
$ brew install peg
```

Please install [the Boehm-Demers-Weiser conservative garbage collector](https://www.hboehm.info/gc/) manually or with a package manager, for example with Homebrew:
```bash
$ brew install bdw-gc
```

### Build

```bash
$ make
```

## Build with Docker

The Docker image provides a ready to go environment to experiment with the project. Just build the image locally and run an interactive shell inside the container:
```bash
$ docker build . --tag mtardy/sandbox
$ docker run -it mtardy/sandbox
```

---

## Usage
### Single input
You can pass your program:
* via the standard input
```bash
$ echo "a=2+3 a*2" | ./parse
```
```bash
$ ./parse < file
```
* via a file
```bash
$ ./parse file
```

### Multiple inputs
You can also pass multiple files and use `-` in any order as the standard input when mixing files and standard input, for example:
```bash
$ ./parse file1 - file2 < file3
```
```bash
$ echo "a=2+3 a*2" | ./parse file1 file2 -
```