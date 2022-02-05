#!/bin/bash
#
# File:   createrandomfile.bash
# Author: pilluh
#
# Created on 28 déc. 2015, 15:21:11
#
dd if=/dev/urandom of=random.data bs=10M count=30
