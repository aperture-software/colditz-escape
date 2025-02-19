ifeq ($(OS),Windows_NT)
    uname_S := Windows
else
    uname_S := $(shell uname -s)
endif

ifeq ($(uname_S), Windows)
    include Makefile.psp
endif
ifeq ($(uname_S), Linux)
    include Makefile.linux
endif
ifeq ($(uname_S), Darwin)
    include Makefile.mac
endif
