#!/usr/bin/env bash

set -e

make

rm -f log
mkfifo log

tmux \
	new-session 			'cat log' \; \
	split-window -h		'./raisin 2>log' \; \

rm -f log
