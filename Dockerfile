FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y gcc make git binutils libc6-dev gdb sudo clang lldb zsh
RUN adduser --disabled-password --gecos '' user
RUN echo 'user ALL=(root) NOPASSWD:ALL' > /etc/sudoers.d/user
RUN apt clean && rm -rf /var/lib/apt/lists/*
USER user
WORKDIR /home/user
