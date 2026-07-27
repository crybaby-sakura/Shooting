#pragma once
#include <cstdint>

// エフェクト用決定論的乱数（リプレイ再現用）
void effectRandSetSeed(uint32_t seed);
int  effectRandInt(int max);           // 0 ～ max-1 の整数
int  effectRandRange(int min, int max); // min ～ max の整数
double effectRandDouble();             // 0.0 ～ 1.0 未満の実数