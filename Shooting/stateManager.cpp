// stateManager.cpp
#include "stateManager.h"
#include "DxLib.h"
#include "imgSoundLoad.h" // currentBGMHandle, bgm_menu, loadStageBGM()
#include "initial.h"      // ini(), startNewGame()
#include "replay.h"       // startReplay(), replayActive
#include "stageData.h"    // stageData
#include "gv.h"
#include "fileOpenClose.h"
#include "masterpieceViewer.h"


// BGMオンオフ制御定数 (true:再生する, false:再生しない)
static constexpr bool BGM_ENABLED = true;

int stageNum = 0;
sCursor cursor = { 0, 0, 0 };
Joutai StateManager::currentState = Joutai::None;


Joutai StateManager::GetState()
{
	return currentState;
}

bool StateManager::ChangeState(Joutai newState)
{
	// 同じ状態への遷移は何もしない
	if (currentState == newState)
		return false;

	// 新しい状態に応じた初期化処理
	switch (newState)
	{
	case Joutai::Menu:
		replayActive = false;
		if constexpr (BGM_ENABLED) {
			if (currentBGMHandle != -1) {
				StopSoundMem(currentBGMHandle);
			}
			currentBGMHandle = bgm_menu;
			PlaySoundMem(bgm_menu, DX_PLAYTYPE_LOOP);
		}
		break;

	case Joutai::Game:
		// 遅延ロード: ゲーム開始前にBGMを必要に応じてロード
		loadStageBGM(stageNum);
		// BGM 制御：現在のステージ用に切り替え
		if constexpr (BGM_ENABLED) {
			int bgmHandle = stageData[stageNum].bgmHandle;
			if (currentBGMHandle != bgmHandle) {
				if (currentBGMHandle != -1) {
					StopSoundMem(currentBGMHandle);
				}
				currentBGMHandle = bgmHandle;
				if (bgmHandle != -1) {
					//ChangeVolumeSoundMem(200, bgmHandle); // BGM の音量を下げる
					PlaySoundMem(bgmHandle, DX_PLAYTYPE_LOOP);
				}
			}
		}
		startNewGame(); // iniGame + 乱数シード（joutaiFlag 代入は削除済み）

		// ここでプレイ回数を増やす（リプレイでない場合のみ）
		if (!replayActive && stageNum >= 0 && stageNum < (int)stageData.size()) {
			stageData[stageNum].playCount++;
			savePlayCount();   // 異常終了に備えてすぐ保存
		}

		break;

	case Joutai::Replay:
		// BGM 制御：ステージ BGM に切り替え
		if (!startReplay(stageNum))
			return false;  // リプレイファイル不存在など
		key[KEY_INPUT_NUMPAD4] = 0;
		key[KEY_INPUT_NUMPAD6] = 0;
		key[KEY_INPUT_NUMPAD8] = 0;
		key[KEY_INPUT_NUMPAD5] = 0;
		key[KEY_INPUT_V] = 1;
		key[KEY_INPUT_C] = 0;
		// 遅延ロード
		loadStageBGM(stageNum);
		if constexpr (BGM_ENABLED) {
			int bgmHandle = masterpieceMode ? bgm_masterpiece : stageData[stageNum].bgmHandle;
			if (currentBGMHandle != bgmHandle) {
				if (currentBGMHandle != -1) {
					StopSoundMem(currentBGMHandle);
				}
				currentBGMHandle = bgmHandle;
				if (bgmHandle != -1) {
					PlaySoundMem(bgmHandle, DX_PLAYTYPE_LOOP);
				}
			}
		}
		break;

	case Joutai::Win:
	case Joutai::Lose:
		// 特別な処理なし（BGM はそのまま）
		break;
	}

	currentState = newState;
	return true;
}