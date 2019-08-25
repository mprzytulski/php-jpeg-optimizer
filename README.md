## Dependencies
 * libjpeg
 * mozjpeg
 *

## Install mozjpeg

https://github.com/mozilla/mozjpeg

```
git clone https://github.com/mozilla/mozjpeg
cd mozjpeg
cmake -G"Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/
make install
```

## Install iqa


```
wget https://master.dl.sourceforge.net/project/iqa/1.1.1%20Release/iqa_1.1.1_src.tar.gz
tar -xf iqa_1.1.1_src.tar.gz
cd ./iqa_1.1.1
RELEASE=1 make
cp ./include/* /usr/include
cp ./build/release/* /usr/lib/x86_64-linux-gnu/
```