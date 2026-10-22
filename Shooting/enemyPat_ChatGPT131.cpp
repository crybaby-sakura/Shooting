#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// バブルソート弾幕
//
// 8個の弾を「値を持つ要素」に見立て、隣接する2発を比較する。
// 左の値 > 右の値なら、2発が上下に分かれる弧を描いて入れ替わる。
// 走査のたびに大きい値が右端へ移動していくため、
// バブルソートのアルゴリズムそのものが弾幕として見える構成。
// ============================================================

namespace
{
	const int kBubbleCount = 8;
	const double kSlotSpacing = 48.0;
	const double kFirstSlotX = 72.0;

	const int kCompareWaitFrames = 8;
	const int kSwapFrames = 20;
	const int kSortedWaitFrames = 70;
	const int kSortedPulseFrames = 24;
	const double kSwapArcHeight = 24.0;
	const double kReturnLerp = 0.26;

	// sEnemyShotSet::param_i
	const int P_LANE = 0;
	const int P_PHASE = 1;
	const int P_COMPARE_INDEX = 2;
	const int P_PASS_END = 3;
	const int P_SWAPPED_THIS_PASS = 4;
	const int P_SWAP_START_COUNT = 5;
	const int P_WAIT_START_COUNT = 6;
	const int P_CYCLE = 7;

	// phase
	const int PHASE_COMPARE = 0;
	const int PHASE_SWAP = 1;
	const int PHASE_SORTED = 2;

	// sEnemyShot::param_i
	const int S_VALUE = 0;
	const int S_SLOT = 1;

	double SlotX(int slot)
	{
		return kFirstSlotX + kSlotSpacing * slot;
	}

	double LaneY(int lane)
	{
		// 3段に分散して配置し、画面全体を横切る「データ列」に見せる。
		return 240.0 + 100.0 * lane;
	}

	sEnemyShot* FindShotBySlot(sEnemyShotSet* pEnemyShotSet, int slot)
	{
		sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pShot != pEnemyShotSet->pEnemyShotHead) {
			if (pShot->param_i[S_SLOT] == slot) {
				return pShot;
			}
			pShot = pShot->next;
		}
		return nullptr;
	}

	void MoveShotTowardHome(sEnemyShot* pShot, double targetX, double targetY)
	{
		pShot->x += (targetX - pShot->x) * kReturnLerp;
		pShot->y += (targetY - pShot->y) * kReturnLerp;
	}

	void SetBubbleValue(sEnemyShot* pShot, int value)
	{
		pShot->param_i[S_VALUE] = value;
		// 色を値として扱う。中玉は輪郭が見やすく、交換の判別もしやすい。
		pShot->kind = img_enemyShotLargeBall[value % 8];
	}

	// lane / cycle / slot から、毎回異なる初期列を作る。
	// どの列も「大きい値が左側にある」または「未整列」の状態になるようにしている。
	int InitialValue(int lane, int cycle, int slot)
	{
		static const int patterns[3][4][kBubbleCount] =
		{
			{
				{7, 6, 5, 4, 3, 2, 1, 0},
				{6, 4, 7, 1, 5, 2, 3, 0},
				{7, 2, 6, 1, 5, 0, 4, 3},
				{5, 7, 3, 6, 1, 4, 0, 2}
			},
			{
				{3, 7, 1, 6, 0, 5, 2, 4},
				{6, 1, 5, 0, 7, 2, 4, 3},
				{4, 0, 7, 2, 6, 1, 5, 3},
				{7, 3, 5, 1, 6, 0, 4, 2}
			},
			{
				{5, 1, 6, 2, 7, 3, 0, 4},
				{2, 6, 0, 5, 3, 7, 1, 4},
				{7, 4, 1, 6, 0, 5, 2, 3},
				{3, 0, 6, 2, 5, 1, 7, 4}
			}
		};

		return patterns[lane % 3][cycle % 4][slot];
	}

	void ResetBubbleRow(sEnemyShotSet* pEnemyShotSet)
	{
		const int lane = pEnemyShotSet->param_i[P_LANE];
		const int cycle = pEnemyShotSet->param_i[P_CYCLE];

		sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pShot != pEnemyShotSet->pEnemyShotHead) {
			const int slot = pShot->param_i[S_SLOT];
			SetBubbleValue(pShot, InitialValue(lane, cycle, slot));
			pShot->x = SlotX(slot);
			pShot->y = LaneY(lane);
			pShot = pShot->next;
		}

		pEnemyShotSet->param_i[P_PHASE] = PHASE_COMPARE;
		pEnemyShotSet->param_i[P_COMPARE_INDEX] = 0;
		pEnemyShotSet->param_i[P_PASS_END] = kBubbleCount - 1;
		pEnemyShotSet->param_i[P_SWAPPED_THIS_PASS] = 0;
		pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
	}

	void StartSwap(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pLeft, sEnemyShot* pRight)
	{
		pEnemyShotSet->param_i[P_PHASE] = PHASE_SWAP;
		pEnemyShotSet->param_i[P_SWAP_START_COUNT] = pEnemyShotSet->count;

		// 交換開始時点の座標を保存。ここから左右の目的地へ弧を描かせる。
		pLeft->param_d[0] = pLeft->x;
		pLeft->param_d[1] = pLeft->y;
		pRight->param_d[0] = pRight->x;
		pRight->param_d[1] = pRight->y;

		pEnemyShotSet->param_i[P_SWAPPED_THIS_PASS] = 1;

		if (pEnemyShotSet->param_i[P_LANE] == 0) {
			if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
			PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
		}
	}

	void FinishComparisonAndAdvance(sEnemyShotSet* pEnemyShotSet)
	{
		pEnemyShotSet->param_i[P_PHASE] = PHASE_COMPARE;

		int compareIndex = pEnemyShotSet->param_i[P_COMPARE_INDEX];
		const int passEnd = pEnemyShotSet->param_i[P_PASS_END];

		if (compareIndex >= passEnd - 1) {
			// この走査で一度も交換が起きなかったなら既に整列済み。
			if (pEnemyShotSet->param_i[P_SWAPPED_THIS_PASS] == 0) {
				pEnemyShotSet->param_i[P_PHASE] = PHASE_SORTED;
				pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
				if (pEnemyShotSet->param_i[P_LANE] == 0) {
					if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
					PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
				}
				return;
			}

			// バブルソートの1走査終了。最大値が末尾に確定する。
			pEnemyShotSet->param_i[P_PASS_END] = passEnd - 1;
			pEnemyShotSet->param_i[P_COMPARE_INDEX] = 0;
			pEnemyShotSet->param_i[P_SWAPPED_THIS_PASS] = 0;
			pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
			return;
		}

		++compareIndex;
		pEnemyShotSet->param_i[P_COMPARE_INDEX] = compareIndex;
		pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
	}

	// 1つの列をバブルソートとして進行させる。
	static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
	{
		if (pEnemyShotSet->count == 0) {
			pEnemyShotSet->param_i[P_LANE] = pEnemyShotSet->kind % 3;
			pEnemyShotSet->param_i[P_PHASE] = PHASE_COMPARE;
			pEnemyShotSet->param_i[P_COMPARE_INDEX] = 0;
			pEnemyShotSet->param_i[P_PASS_END] = kBubbleCount - 1;
			pEnemyShotSet->param_i[P_SWAPPED_THIS_PASS] = 0;
			pEnemyShotSet->param_i[P_SWAP_START_COUNT] = 0;
			pEnemyShotSet->param_i[P_WAIT_START_COUNT] = 0;
			pEnemyShotSet->param_i[P_CYCLE] = 0;

			// 列ごとに少し待ち時間をずらす。
			pEnemyShotSet->param_i[P_WAIT_START_COUNT] = -pEnemyShotSet->param_i[P_LANE] * 4;

			const int lane = pEnemyShotSet->param_i[P_LANE];
			const double laneY = LaneY(lane);

			for (int slot = 0; slot < kBubbleCount; ++slot) {
				sEnemyShot* pShot = new sEnemyShot;
				pShot->x = SlotX(slot);
				pShot->y = laneY;
				pShot->muki = 0.0;
				pShot->speed = 0.0;
				pShot->param_i[S_SLOT] = slot;
				pShot->param_i[S_VALUE] = InitialValue(pEnemyShotSet->param_i[P_LANE], 0, slot);
				pShot->kind = img_enemyShotLargeBall[pShot->param_i[S_VALUE] % 8];

				pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
				pShot->next = pEnemyShotSet->pEnemyShotHead;
				pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
				pEnemyShotSet->pEnemyShotHead->prev = pShot;
			}
		}

		const int lane = pEnemyShotSet->param_i[P_LANE];
		const double laneY = LaneY(lane);
		const int phase = pEnemyShotSet->param_i[P_PHASE];

		if (phase == PHASE_SWAP) {
			const int compareIndex = pEnemyShotSet->param_i[P_COMPARE_INDEX];
			sEnemyShot* pLeft = FindShotBySlot(pEnemyShotSet, compareIndex);
			sEnemyShot* pRight = FindShotBySlot(pEnemyShotSet, compareIndex + 1);

			if (pLeft == nullptr || pRight == nullptr) {
				pEnemyShotSet->param_i[P_PHASE] = PHASE_COMPARE;
				pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
				return;
			}

			double t = (double)(pEnemyShotSet->count - pEnemyShotSet->param_i[P_SWAP_START_COUNT]) / kSwapFrames;
			if (t < 0.0) t = 0.0;
			if (t > 1.0) t = 1.0;

			const double eased = t * t * (3.0 - 2.0 * t);
			const double arc = sin(DX_PI * eased) * kSwapArcHeight;

			const double leftTargetX = SlotX(compareIndex + 1);
			const double rightTargetX = SlotX(compareIndex);
			const double leftStartX = pLeft->param_d[0];
			const double leftStartY = pLeft->param_d[1];
			const double rightStartX = pRight->param_d[0];
			const double rightStartY = pRight->param_d[1];

			pLeft->x = leftStartX + (leftTargetX - leftStartX) * eased;
			pLeft->y = leftStartY + (laneY - leftStartY) * eased - arc;

			pRight->x = rightStartX + (rightTargetX - rightStartX) * eased;
			pRight->y = rightStartY + (laneY - rightStartY) * eased + arc;

			// 他の要素は自分の位置へ戻しておく。
			sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
			while (pShot != pEnemyShotSet->pEnemyShotHead) {
				if (pShot != pLeft && pShot != pRight) {
					MoveShotTowardHome(pShot, SlotX(pShot->param_i[S_SLOT]), laneY);
				}
				pShot = pShot->next;
			}

			if (t >= 1.0) {
				const int leftSlot = pLeft->param_i[S_SLOT];
				pLeft->param_i[S_SLOT] = pRight->param_i[S_SLOT];
				pRight->param_i[S_SLOT] = leftSlot;

				FinishComparisonAndAdvance(pEnemyShotSet);
			}
			return;
		}

		if (phase == PHASE_SORTED) {
			const int elapsed = pEnemyShotSet->count - pEnemyShotSet->param_i[P_WAIT_START_COUNT];

			// 整列完了時に、列全体が外へ膨らんで戻る「確定演出」。
			sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
			while (pShot != pEnemyShotSet->pEnemyShotHead) {
				const int slot = pShot->param_i[S_SLOT];
				const double center = (kBubbleCount - 1) * 0.5;
				const double direction = (slot < center) ? -1.0 : 1.0;
				double pulse = 0.0;
				if (elapsed >= 0 && elapsed < kSortedPulseFrames) {
					const double t = (double)elapsed / kSortedPulseFrames;
					pulse = sin(DX_PI * t) * 16.0;
				}
				MoveShotTowardHome(pShot, SlotX(slot) + direction * pulse, laneY);
				pShot = pShot->next;
			}

			if (elapsed >= kSortedWaitFrames) {
				++pEnemyShotSet->param_i[P_CYCLE];
				ResetBubbleRow(pEnemyShotSet);

				// リセット時だけ予告音を鳴らし、「次の並べ替え」が始まることを示す。
				if (pEnemyShotSet->param_i[P_LANE] == 0) {
					if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
					PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
				}
			}
			return;
		}

		// PHASE_COMPARE
		{
			sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
			while (pShot != pEnemyShotSet->pEnemyShotHead) {
				MoveShotTowardHome(pShot, SlotX(pShot->param_i[S_SLOT]), laneY);
				pShot = pShot->next;
			}

			const int elapsed = pEnemyShotSet->count - pEnemyShotSet->param_i[P_WAIT_START_COUNT];
			const int compareIndex = pEnemyShotSet->param_i[P_COMPARE_INDEX];
			const int passEnd = pEnemyShotSet->param_i[P_PASS_END];

			if (passEnd <= 0) {
				pEnemyShotSet->param_i[P_PHASE] = PHASE_SORTED;
				pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
				return;
			}

			if (elapsed < kCompareWaitFrames) {
				return;
			}

			sEnemyShot* pLeft = FindShotBySlot(pEnemyShotSet, compareIndex);
			sEnemyShot* pRight = FindShotBySlot(pEnemyShotSet, compareIndex + 1);
			if (pLeft == nullptr || pRight == nullptr) {
				pEnemyShotSet->param_i[P_WAIT_START_COUNT] = pEnemyShotSet->count;
				return;
			}

			// 「左 > 右」なら交換。この判定がバブルソートの中心。
			if (pLeft->param_i[S_VALUE] > pRight->param_i[S_VALUE]) {
				StartSwap(pEnemyShotSet, pLeft, pRight);
			}
			else {
				FinishComparisonAndAdvance(pEnemyShotSet);
			}
		}
	}
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_BubbleSort_ChatGPT()
{
	static int muki;
	static bool initialized;

	if (count == 1) {
		enemy.x = 240.0;
		enemy.y = 40.0;
		enemy.maxHp = enemy.hp = 200;
		muki = 1;
		initialized = false;
	}
	else {
		enemy.x += 0.92 * (double)muki;
		if (count % 120 == 60) {
			muki *= -1;
		}
	}

	// 3列だけ生成。各列の内部でバブルソートが自律的に進む。
	if (!initialized && count == 1) {
		for (int lane = 0; lane < 3; ++lane) {
			sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
			pEnemyShotSet->count = 0;
			pEnemyShotSet->patternFunc = ShotBubbleSort;
			pEnemyShotSet->x = 240.0;
			pEnemyShotSet->y = LaneY(lane);
			pEnemyShotSet->muki = 0.0;
			pEnemyShotSet->kind = lane;
			pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
			pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
			pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

			pEnemyShotSet->prev = enemyShotSetHead.prev;
			pEnemyShotSet->next = &enemyShotSetHead;
			enemyShotSetHead.prev->next = pEnemyShotSet;
			enemyShotSetHead.prev = pEnemyShotSet;
		}
		initialized = true;
	}
}
