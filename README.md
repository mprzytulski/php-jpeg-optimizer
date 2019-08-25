## Dependencies
 * libjpeg-turbo (https://github.com/libjpeg-turbo/libjpeg-turbo)
 * mozjpeg (https://github.com/mozilla/mozjpeg)
 * libiqa (https://sourceforge.net/projects/iqa/)

## Install mozjpeg

https://github.com/mozilla/mozjpeg

```
git clone https://github.com/mozilla/mozjpeg
cd mozjpeg
cmake -G"Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/
make install
```

## Build iqa as shared library

```
wget https://master.dl.sourceforge.net/project/iqa/1.1.1%20Release/iqa_1.1.1_src.tar.gz
tar -xf iqa_1.1.1_src.tar.gz
cd ./iqa_1.1.1
gcc -shared -I./include -fPIC -lm -O2 -Wall -o libiqa.so ./source/ssim.c ./source/convolve.c ./source/decimate.c ./source/math_utils.c ./source/ms_ssim.c ./source/mse.c ./source/psnr.c
cp libiqa.so /usr/lib/x86_64-linux-gnu/
```

## Build extension

```
git clone https://github.com/mprzytulski/php-jpeg-optimizer
phpize
./configure
make
```

## Usage

```
jpeg_optimize((string) source, (string)destination, [JPEG_OPTIMIZER_QUALITY_*], [JPEG_OPTIMIZER_METHOD_*], [(int) jpegMin], [(int) jpegMax], [(int) attempts])
```

# Created based on

https://github.com/danielgtaylor/jpeg-archive
