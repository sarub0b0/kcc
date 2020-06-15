FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y gcc make git binutils libc6-dev gdb sudo clang lldb zsh valgrind
RUN apt clean && rm -rf /var/lib/apt/lists/*
WORKDIR /root
