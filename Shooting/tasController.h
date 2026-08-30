#pragma once

extern bool g_isTasMode;

void iniGameForTas();

// Lose状態に遷移した直後に呼ぶ。TASモード時は10フレーム巻き戻しを行う。
// 戻り値: true なら巻き戻しを実行した
bool TAS_OnLoseState();

// 毎フレームのゲーム更新後に呼び、countの最大値を記録する
void TAS_ResetMaxCount();
void TAS_UpdateMaxCount(int currentCount);
