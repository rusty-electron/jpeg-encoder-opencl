#!/bin/sh

# Clean build directory if it exists
if [ -d "build" ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

# Create build directory
mkdir build

# cd into build directory
cd build
# generate build files
cmake ..
# build project and check exit status
# loop until build is successful
# use do while loop to ensure build is attempted at least once
count=0
while true; do
    make
    # break if build is successful or 5 attempts have been made
    if [ $? -eq 0 ] || [ $count -eq 5 ]; then
        break
    fi
    # increment count
    count=$((count+1))
    sleep 1
done