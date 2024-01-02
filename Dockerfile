# Creates an image to run valgrind on macOS.

FROM alpine:latest

RUN apk add g++ make cmake vim ncurses-dev valgrind

WORKDIR /tmp
RUN wget "https://cmocka.org/files/1.1/cmocka-1.1.7.tar.xz"
RUN tar -xf cmocka-1.1.7.tar.xz

WORKDIR /tmp/cmocka-1.1.7
RUN mkdir build

WORKDIR /tmp/cmocka-1.1.7/build
RUN cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ..
RUN make
RUN make install
