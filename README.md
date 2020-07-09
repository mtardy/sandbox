# Basic calculator

## Requirements

Please install [peg/leg](https://www.piumarta.com/software/peg/) manually or with Homebrew:
```bash
$ brew install peg
```

Please install [the Boehm-Demers-Weiser conservative garbage collector](https://www.hboehm.info/gc/) manually or with Homebrew:
```bash
$ brew install bdw-gc
```

## Build

```bash
$ make
```

## Usage

```bash
$ echo "a=2+3 a*2" | ./calc
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
