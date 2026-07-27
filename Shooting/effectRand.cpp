#include "effectRand.h"

static uint32_t effectRandSeed = 42;

// 線形合同法（LCG）
static uint32_t lcg(uint32_t x) {
    return 1664525u * x + 1013904223u;
}

void effectRandSetSeed(uint32_t seed) {
    effectRandSeed = seed;
}

int effectRandInt(int max) {
    if (max <= 0) return 0;
    effectRandSeed = lcg(effectRandSeed);
    return (int)(effectRandSeed % max);
}

int effectRandRange(int min, int max) {
    if (min > max) { int t = min; min = max; max = t; }
    int range = max - min + 1;
    if (range <= 0) return min;
    return min + effectRandInt(range);
}

double effectRandDouble() {
    effectRandSeed = lcg(effectRandSeed);
    return (double)effectRandSeed / (double)UINT32_MAX;
}