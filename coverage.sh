#!/bin/bash
set -e  
set -x  

BUILD_DIR=build
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

cmake .. -DCMAKE_BUILD_TYPE=Debug -DCODE_COVERAGE=ON

cmake --build .

ctest

lcov --capture \
     --directory . \
     --output-file coverage.info \
     --rc branch_coverage=1 \
     --ignore-errors mismatch

lcov --remove coverage.info '/usr/*' '*/_deps/*' \
     --output-file coverage_filtered.info \
     --rc branch_coverage=1

genhtml coverage_filtered.info \
    --output-directory coverage_html \
    --rc branch_coverage=1


if command -v xdg-open &>/dev/null; then
    xdg-open coverage_html/index.html
elif command -v open &>/dev/null; then
    open coverage_html/index.html
fi

