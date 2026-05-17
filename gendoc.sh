#!/bin/sh
rm -rf docs
doxygen
cd docs
git add .
