# FAT Filesystem driver implementation

## Info

This project was originally a homework assignment for an operating systems class to partially implement the FAT whitepaper in order to read the root directory of a given FAT32 image.
The current code is a refactored and extended version of that assignment.

## TODOs (atm)

- Create tests 
- Create a more extensive driver

## Structure
```
project
│
├──include
│
├──src
│   │
│   └──driver.c     # test driver
│
├──fat32.img        # test image
│
└──Makefile
```

## make commands

make all        
make clean      
make dirs       
make driver     
make help       
make test       # TODO

## driver

driver mounts `fat32.img`, prints the root directory, and unmounts the image.