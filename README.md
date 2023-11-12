# clux - Ligtweight json and json-schema library for C

## Compile and install
```
make
sudo make install
```

## Test examples
```
cd tests/whatever
CFLAGS="-std=c11 -Wpedantic -Wall -Wextra -O2" LDLIBS="-lclux" make demo && ./demo
```
