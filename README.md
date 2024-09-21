# clux - json and json-schema library for C

## Compile and install
```
make
sudo make install
sudo ldconfig # on linux
```

## Test examples
```
cd tests/whatever
CFLAGS="-std=c11 -Wpedantic -Wall -Wextra -O2" LDLIBS="-lclux" make demo && ./demo
```
