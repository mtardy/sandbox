# Javascrypt

## Build the project
### Requirements

Please install [peg/leg](https://www.piumarta.com/software/peg/) manually or with Homebrew: 
```bash
$ brew install peg
```

Please install [the Boehm-Demers-Weiser conservative garbage collector](https://www.hboehm.info/gc/) manually or with Homebrew:
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
$ docker build . --tag mtardy/javascrypt
$ docker run -it mtardy/javascrypt
```

---

## Usage

```bash
$ echo "a=2+3 a*2" | ./calc
```
or
```bash
$ ./calc < file
```

## Tests

Simple tests:
```bash
$ ./test.sh
```

Custom test:
```bash
$ ./calc < test.txt
```
