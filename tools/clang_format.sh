#!/bin/sh

# A Simple format to run clang-format over every file in project directory
# WARNING: Only run this in the root directory of the project

find . -iname *.h -o -iname *.cc | xargs clang-format -i

