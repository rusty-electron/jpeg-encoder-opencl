#pragma once
#include <stdio.h>
#include <cstdint>
#include <iostream>
#include <cstring>

int readPPMImage(const char *, size_t *, size_t *, uint8_t **);
int writePPMImage(const char *, size_t, size_t, uint8_t *);