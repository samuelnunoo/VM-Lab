#
# Makefile for the CS 105 Virtual Memory Homework
#
# Eleanor Birrell
# Pomona College
# March 19, 2020
#
SHELL = /bin/sh

all: vm

vm: vm.c
	rm -rf files
	mkdir files
	gcc -o vm vm.c

clean:
	rm -rf files
	rm -rf vm
	rm -rf *~
	rm -rf *.pg