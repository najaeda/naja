FROM gcr.io/hdl-containers/verilator:latest AS verilator

FROM gcr.io/hdl-containers/yosys:latest
COPY --from=verilator /usr/local/bin/ /usr/local/bin/
COPY --from=verilator /usr/local/share/verilator/ /usr/local/share/verilator/
RUN apt-get update && apt-get -y install \
    git \
    pkg-config \
    wget make  \
    g++ \
    flex bison \
    libboost-dev \
    python3-dev \
    libfl-dev capnproto libcapnp-dev libtbb-dev
# Put your own Dockfile commands here
RUN rm -rf /var/lib/apt/lists/* \
  && wget https://github.com/Kitware/CMake/releases/download/v3.24.1/cmake-3.24.1-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /opt/cmake-3.24.1 \
      && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.24.1 \
      && rm /tmp/cmake-install.sh \
      && ln -s /opt/cmake-3.24.1/bin/* /usr/local/bin
COPY / .
RUN mkdir install
RUN cmake . -DCMAKE_INSTALL_PREFIX=/install && make -j$(nproc) && make install

WORKDIR /regress
CMD make clean; PYTHONPATH=/install/lib/python LD_LIBRARY_PATH=/install/lib make NAJA_EDIT=/install/bin/naja_edit PRIMITIVES=/primitives/xilinx.py
