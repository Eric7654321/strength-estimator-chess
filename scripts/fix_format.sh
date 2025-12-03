#! /bin/zsh
# only for clang-format
git diff -U0 | clang-format-diff -p1 -i
# If you want to only reformat lines that differ from clang-format's expected formatting: