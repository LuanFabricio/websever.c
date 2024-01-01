#!/bin/bash

set -x

# Compiling server
gcc -Wall -Wextra -o server.out server.c

# Compiling handler module
gcc -Wall -Wextra -o handlers.so -fPIC -shared handlers.c

# Compiling request module
gcc -Wall -Wextra -o request.so -fPIC -shared request.c

# Compiling dynamic page module
gcc -Wall -Wextra -o pages/something/[id].so  -fPIC -shared pages/something/[id].c
