# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

FROM alpine:3.19.1 as builder

# Install required packages
RUN apk --no-cache add ca-certificates
RUN apk update && apk upgrade
RUN apk add --no-cache cmake make g++ \
        python3-dev capnproto capnproto-dev \
        bison flex-dev boost-dev onetbb-dev

# Set the working directory
WORKDIR /naja
COPY CMakeLists.txt /naja/
COPY src /naja/src
COPY test /naja/test
COPY cmake /naja/cmake
COPY primitives /naja/primitives
COPY thirdparty /naja/thirdparty

WORKDIR /naja/build
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --parallel 8 && \
    cmake --install . --prefix /naja/install    
RUN ctest -j8

FROM alpine:3.19.1 as release
RUN apk --no-cache add ca-certificates
RUN apk add --no-cache \
    libstdc++ capnproto python3

RUN addgroup -S naja && adduser -S naja -G naja
USER naja
COPY --chown=naja:naja --from=builder \
    ./naja/install/bin/naja_edit \
    ./naja/
COPY --chown=naja:naja --from=builder \
    ./naja/install/lib \
    ./naja/lib
ENV LD_LIBRARY_PATH /naja/lib
ENV PYTHONPATH /naja/lib/python
ENTRYPOINT [ "./naja/naja_edit" ]