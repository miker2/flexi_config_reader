# We only run clang-13 on 20.04 (and whatever ships on its libc), the other compilers run on 22.04
# unfortunately we have to duplicate the postprocessing steps
FROM ubuntu:20.04 AS base-ubuntu20
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates wget gpg
RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
RUN echo 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-13 main' | tee -a /etc/apt/sources.list.d/llvm.list
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2> /dev/null \
    | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main' \
    | tee /etc/apt/sources.list.d/kitware.list
RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo \
    build-essential \
    git \
    gdb \
    python3 \
    python3-dev \
    python3-pip \
    python3-pybind11 \
    python3-pybindgen \
    pybind11-dev \
    libc++-13-dev \
    libc++abi-13-dev \
    clang-13 \
    clang-format-13 \
    clang-tidy-13 \
    lld-13 \
    ninja-build \
    cmake

# project root will be mounted at /usr/src/flexi_cfg
COPY <<EOF /usr/bin/cmake-setup
#!/bin/bash
cmake /usr/src/flexi_cfg -DCMAKE_BUILD_TYPE=Debug -DENABLE_CLANG_TIDY:BOOL=ON -DCFG_ENABLE_DEBUG:BOOL=ON -DCFG_ENABLE_PARSER_TRACE:BOOL=OFF -DCFG_EXAMPLES:BOOL=ON -DCFG_PYTHON_BINDINGS:BOOL=ON -G Ninja
EOF
RUN chmod +x /usr/bin/cmake-setup

WORKDIR /usr/src/flexi_cfg_build/_deps

ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
  && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
  && chmod 0440 /etc/sudoers.d/$USERNAME

# compose mounts volumes as root
COPY <<EOF /usr/bin/entrypoint
#!/bin/bash
set -e
# Function to handle termination signal
term_handler() {
    echo "Termination signal received. Cleaning up..."
    # Insert cleanup commands here
    exit 0
}
# Trap termination signals (SIGTERM, SIGINT)
trap 'term_handler' SIGTERM SIGINT

sudo chown -R ${USER_UID}:${USER_GID} /usr/src/flexi_cfg_build
echo "== Executing == \$@ =="
exec "\$@"
echo "== DONE =="
sleep 2
EOF
RUN chmod +x /usr/bin/entrypoint

COPY <<EOF /usr/bin/build-and-test
#!/bin/bash
set -e
# Function to handle termination signal
term_handler() {
    echo "Termination signal received. Cleaning up..."
    # Insert cleanup commands here
    exit 0
}
# Trap termination signals (SIGTERM, SIGINT)
trap 'term_handler' SIGTERM SIGINT
cmake-setup && ninja all && ninja test
result="\$?"
echo -- BUILD DONE -- RESULT = \$result --
exit \$result
EOF
RUN chmod +x /usr/bin/build-and-test

COPY <<EOF /home/${USERNAME}/.bashrc
echo -e "======================================================================================="
echo -e "FlexiConfig interactive test environment:"
echo -e ""
echo -e "commands:  cmake-setup               # prepare cmake build"
echo -e "           build-and-test            # builds all targets, tidy, runs tests"
echo -e "           ninja all                 # builds all targets, tidy"
echo -e "           ctest <...>               # runs tests"
echo -e "           tests/utils_test          # run any gtest directly"
echo -e "           gdb tests/utils_test      # run with gdb"
echo -e "======================================================================================="
EOF

USER ${USERNAME}
WORKDIR /usr/src/flexi_cfg_build

ENV PATH=${PATH}:/home/${USERNAME}/.local/bin
ENV TEST_DIR=env

ENTRYPOINT ["/usr/bin/entrypoint"]

# ----------------------------------------------------------------------------------------------------------------

FROM ubuntu:22.04 AS base
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates wget gpg
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2> /dev/null \
    | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' \
    | tee /etc/apt/sources.list.d/kitware.list
RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo \
    build-essential \
    gdb \
    git \
    python3 \
    python3-dev \
    python3-pip \
    python3-pybind11 \
    python3-pybindgen \
    pybind11-dev \
    gcc-12 \
    g++-12 \
    clang-13 \
    clang-format-13 \
    clang-tidy-13 \
    lld-13 \
    clang-14 \
    clang-format-14 \
    clang-tidy-14 \
    lld-14 \
    clang-15 \
    clang-format-15 \
    clang-tidy-15 \
    lld-15 \
    ninja-build \
    cmake

# project root will be mounted at /usr/src/flexi_cfg
COPY <<EOF /usr/bin/cmake-setup
#!/bin/bash
cmake /usr/src/flexi_cfg -DCMAKE_BUILD_TYPE=Debug -DENABLE_CLANG_TIDY:BOOL=ON -DCFG_ENABLE_DEBUG:BOOL=ON -DCFG_ENABLE_PARSER_TRACE:BOOL=OFF -DCFG_EXAMPLES:BOOL=ON -DCFG_PYTHON_BINDINGS:BOOL=ON -G Ninja
EOF
RUN chmod +x /usr/bin/cmake-setup

WORKDIR /usr/src/flexi_cfg_build/_deps

ARG USERNAME=user
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
  && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
  && chmod 0440 /etc/sudoers.d/$USERNAME

# compose mounts volumes as root
COPY <<EOF /usr/bin/entrypoint
#!/bin/bash
set -e
# Function to handle termination signal
term_handler() {
    echo "Termination signal received. Cleaning up..."
    # Insert cleanup commands here
    exit 0
}
# Trap termination signals (SIGTERM, SIGINT)
trap 'term_handler' SIGTERM SIGINT

sudo chown -R ${USER_UID}:${USER_GID} /usr/src/flexi_cfg_build
echo "== Executing == \$@ =="
exec "\$@"
echo "== DONE =="
sleep 2
EOF
RUN chmod +x /usr/bin/entrypoint

COPY <<EOF /usr/bin/build-and-test
#!/bin/bash
set -e
# Function to handle termination signal
term_handler() {
    echo "Termination signal received. Cleaning up..."
    # Insert cleanup commands here
    exit 0
}
# Trap termination signals (SIGTERM, SIGINT)
trap 'term_handler' SIGTERM SIGINT
cmake-setup && ninja all && ninja test
result="\$?"
echo -- BUILD DONE -- RESULT = \$result --
exit \$result
EOF
RUN chmod +x /usr/bin/build-and-test

COPY <<EOF /home/${USERNAME}/.bashrc
echo -e "======================================================================================="
echo -e "FlexiConfig interactive test environment:"
echo -e ""
echo -e "commands:  cmake-setup               # prepare cmake build"
echo -e "           build-and-test            # builds all targets, tidy, runs tests"
echo -e "           ninja all                 # builds all targets, tidy"
echo -e "           ctest <...>               # runs tests"
echo -e "           tests/utils_test          # run any gtest directly"
echo -e "           gdb tests/utils_test      # run with gdb"
echo -e "======================================================================================="
EOF

USER ${USERNAME}
WORKDIR /usr/src/flexi_cfg_build

ENV PATH=${PATH}:/home/${USERNAME}/.local/bin
ENV TEST_DIR=env

ENTRYPOINT ["/usr/bin/entrypoint"]

# ----------------------------------------------------------------------------------------------------------------

# 20.04 for clang-13
FROM base-ubuntu20 AS clang-13-ubuntu20
ENV CC=/usr/bin/clang-13
ENV CXX=/usr/bin/clang++-13
# clang-13 libc++ for C++20 (e.g. <span>)
ENV CXXFLAGS="-stdlib=libc++ -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"
ENV LDFLAGS="-stdlib=libc++ -g -fuse-ld=/usr/bin/ld.lld-13"
ENV CLANG_TOOLS_PATH=/usr/lib/llvm-13
CMD ["/usr/bin/build-and-test"]

FROM base AS gcc-12
ENV CC=/usr/bin/gcc-12
ENV CXX=/usr/bin/g++-12
ENV CXXFLAGS="-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"
ENV LDFLAGS="-g"
CMD ["/usr/bin/build-and-test"]

FROM base AS clang-13
ENV CC=/usr/bin/clang-13
ENV CXX=/usr/bin/clang++-13
ENV CLANG_TOOLS_PATH=/usr/lib/llvm-13
ENV CXXFLAGS="-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"
ENV LDFLAGS="-g -fuse-ld=/usr/bin/ld.lld-13"
CMD ["/usr/bin/build-and-test"]

FROM base AS clang-14
ENV CC=/usr/bin/clang-14
ENV CXX=/usr/bin/clang++-14
ENV CLANG_TOOLS_PATH=/usr/lib/llvm-14
ENV CXXFLAGS="-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"
ENV LDFLAGS="-g -fuse-ld=/usr/bin/ld.lld-14"
CMD ["/usr/bin/build-and-test"]

FROM base AS clang-15
ENV CC=/usr/bin/clang-15
ENV CXX=/usr/bin/clang++-15
ENV CLANG_TOOLS_PATH=/usr/lib/llvm-15
ENV CXXFLAGS="-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer"
ENV LDFLAGS="-g -fuse-ld=/usr/bin/ld.lld-15"
CMD ["/usr/bin/build-and-test"]

FROM base AS clean
CMD [ "/bin/bash", "-c", "rm -rf _deps/*"]
