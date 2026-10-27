# numcsv

`numcsv` reads a CSV file and computes basic statistics for one numeric column.

## Build

```sh
make
```

Or compile directly:

```sh
cc -std=c11 -O2 -o numcsv src/main.c -lm
```

## Usage

```sh
numcsv data.csv
numcsv -c 2 -H data.csv
```

Options:

- `-c N` selects column N (1-based, default 1)
- `-H` skips the first row as a header

Output includes count, minimum, maximum, mean, and median.
