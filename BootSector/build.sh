#!/bin/sh
/Users/sunny/src/opt/local/cross/bin/i686-clover-linux-gnu-gcc -c -o bin/St32_64m.o St32_64m.S
/Users/sunny/src/opt/local/cross/bin/i686-clover-linux-gnu-ld --oformat binary -o bin/St32_64m.com bin/St32_64m.o -Ttext 0 -Map bin/St32_64m.map
rm bin/St32_64m.o bin/St32_64m.map
