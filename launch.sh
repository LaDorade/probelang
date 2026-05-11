#!/bin/bash

set -xe

docker run --rm \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp=unconfined \
    -v "$(pwd)":/app \
    c-debug \
    bash -c "make -B && ./main"
