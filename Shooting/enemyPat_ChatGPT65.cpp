// enemyPat_tmp_regenerate_hydra.cpp
// 再生九頭竜「リジェネレイト・ヒュドラ」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace
{
	constexpr int HYDRA_MAX_HEADS = 12;

	struct HydraHead
	{
		bool active = false;
		bool branched = false;
		int generation = 0;
		int spawnCount = 0;
		double baseAngle = 0.0;
		double amplitude = 0.0;
		double waveSpeed = 0.0;
		double phase = 0.0;
		double length = 0.0;
		double lengthSpeed = 0.0;
		double branchBias = 0.0;
		double tipX = 0.0;
		double tipY = 0.0;
		sEnemyShotSet* pNeckSet = nullptr;
	};

	static HydraHead g_hydraHeads[HYDRA_MAX_HEADS];
	static int g_hydraHeadCount = 0;
	static int g_hydraMuki = 1;

	static double NormalizeAngle(double a)
	{
		while (a > DX_PI) a -= DX_PI * 2.0;
		while (a < -DX_PI) a += DX_PI * 2.0;
		return a;
	}

	static void AddHydraShot(sEnemyShotSet* pSet, double x, double y, double angle, double speed, int kind)
	{
		sEnemyShot* pShot = new sEnemyShot;
		pShot->x = x;
		pShot->y = y;
		pShot->muki = angle;
		pShot->speed = speed;
		pShot->kind = kind;
		pShot->prev = pSet->pEnemyShotHead->prev;
		pShot->next = pSet->pEnemyShotHead;
		pSet->pEnemyShotHead->prev->next = pShot;
		pSet->pEnemyShotHead->prev = pShot;
	}

	// ------------------------------------------------------------
	// 蛇の首本体
	// ------------------------------------------------------------
	static void ShotHydraNeck(sEnemyShotSet* pSet)
	{
		int headIndex = pSet->param_i[0];
		if (headIndex < 0 || headIndex >= HYDRA_MAX_HEADS) return;
		if (!g_hydraHeads[headIndex].active) return;

		HydraHead& h = g_hydraHeads[headIndex];

		// 初回だけ弾列を生成
		if (pSet->count == 0)
		{
			const int SEGMENTS = 28;

			for (int i = 0; i < SEGMENTS; ++i)
			{
				sEnemyShot* pShot = new sEnemyShot;
				pShot->x = enemy.x;
				pShot->y = enemy.y + 12.0;
				pShot->kind = (i == SEGMENTS - 1)
					? img_enemyShotMediumBall[2]
					: img_enemyShotSmallBall[2];
				pShot->margin = 9999;

				pShot->param_i[0] = i;          // セグメント番号
				pShot->param_i[1] = SEGMENTS;   // 総数

				pShot->prev = pSet->pEnemyShotHead->prev;
				pShot->next = pSet->pEnemyShotHead;
				pSet->pEnemyShotHead->prev->next = pShot;
				pSet->pEnemyShotHead->prev = pShot;
			}
		}

		int life = count - h.spawnCount;

		double currentLength = h.length + life * h.lengthSpeed * 0.09;

		sEnemyShot* pShot = pSet->pEnemyShotHead->next;
		while (pShot != pSet->pEnemyShotHead)
		{
			int seg = pShot->param_i[0];
			int total = pShot->param_i[1];

			double t = (double)seg / (total - 1);

			// セグメントごとに位相をずらして蛇行させる
			double wave = sin(h.phase + life * h.waveSpeed - t * 4.5) * h.amplitude;
			double angle = h.baseAngle + wave + h.branchBias;

			double len = currentLength * t;

			pShot->x = enemy.x + cos(angle) * len;
			pShot->y = enemy.y + 12.0 + sin(angle) * len;

			// 先端位置を保存
			if (seg == total - 1)
			{
				h.tipX = pShot->x;
				h.tipY = pShot->y;
			}

			pShot = pShot->next;
		}
	}

	static void SpawnHydraHead(double angle, int generation, double branchBias)
	{
		if (g_hydraHeadCount >= HYDRA_MAX_HEADS) return;

		for (int i = 0; i < HYDRA_MAX_HEADS; ++i)
		{
			if (!g_hydraHeads[i].active)
			{
				HydraHead& h = g_hydraHeads[i];
				h.active = true;
				h.branched = false;
				h.generation = generation;
				h.spawnCount = count;
				h.baseAngle = angle;
				h.amplitude = (generation == 0) ? 0.28 : 0.18;
				h.waveSpeed = 0.045 + generation * 0.01;
				h.phase = (GetRand(628) / 100.0);
				h.length = 18.0 + generation * 8.0;
				h.lengthSpeed = (generation == 0) ? 1.5 : 1.2;
				h.branchBias = branchBias;
				h.tipX = enemy.x;
				h.tipY = enemy.y;
				++g_hydraHeadCount;

				// 首の弾列セットを作成
				sEnemyShotSet* pSet = new sEnemyShotSet;
				pSet->count = 0;
				pSet->patternFunc = ShotHydraNeck;
				pSet->param_i[0] = i;

				pSet->pEnemyShotHead = new sEnemyShot;
				pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
				pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

				pSet->prev = enemyShotSetHead.prev;
				pSet->next = &enemyShotSetHead;
				enemyShotSetHead.prev->next = pSet;
				enemyShotSetHead.prev = pSet;

				h.pNeckSet = pSet;

				break;
			}
		}
	}

	static void ShotHydraRegenerate(sEnemyShotSet* pSet)
	{
		if (pSet->count == 0)
		{
			int headIndex = pSet->param_i[0];
			bool frenzy = pSet->param_i[1] != 0;
			int generation = pSet->param_i[2];

			if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
			PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

			if (headIndex < 0 || headIndex >= HYDRA_MAX_HEADS || !g_hydraHeads[headIndex].active) return;

			HydraHead& h = g_hydraHeads[headIndex];
			double base = atan2(player.y - h.tipY, player.x - h.tipX);
			base += (GetRand(40) - 20) / 180.0 * DX_PI;

			int way = frenzy ? 5 : (generation >= 2 ? 5 : (generation == 1 ? 4 : 3));
			double spread = frenzy ? 0.16 : 0.24;
			double speedBase = frenzy ? 3.8 : (2.2 + generation * 0.3);
			int color = frenzy ? 1 : (generation == 0 ? 2 : 5);

			for (int i = 0; i < way; ++i)
			{
				double offset = (i - (way - 1) * 0.5) * spread;
				AddHydraShot(pSet, h.tipX, h.tipY, base + offset, speedBase + i * 0.08, img_enemyShotScale[color]);
			}
		}

		sEnemyShot* pShot = pSet->pEnemyShotHead->next;
		while (pShot != pSet->pEnemyShotHead)
		{
			pShot->x += pShot->speed * cos(pShot->muki);
			pShot->y += pShot->speed * sin(pShot->muki);
			pShot = pShot->next;
		}
	}

	static void ShotHydraBurst(sEnemyShotSet* pSet)
	{
		if (pSet->count == 0)
		{
			if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
			PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

			for (int i = 0; i < 6; ++i)
			{
				double angle = pSet->muki + (i - 2.5) * 0.10;
				AddHydraShot(pSet, pSet->x, pSet->y, angle, 1.7 + i * 0.12, img_enemyShotLargeBall[5]);
			}
		}

		sEnemyShot* pShot = pSet->pEnemyShotHead->next;
		while (pShot != pSet->pEnemyShotHead)
		{
			if (pShot->count == 70 && pShot->kind == img_enemyShotLargeBall[5])
			{
				for (int i = 0; i < 16; ++i)
				{
					double angle = i * (DX_PI * 2.0 / 16.0);
					AddHydraShot(pSet, pShot->x, pShot->y, angle, 2.4 + (i % 4) * 0.18, img_enemyShotSmallBall[5]);
				}
			}

			pShot->x += pShot->speed * cos(pShot->muki);
			pShot->y += pShot->speed * sin(pShot->muki);
			pShot = pShot->next;
		}
	}

	static void CreateHydraShotSet(double x, double y, double angle, sEnemyShotSet::PatternFunc func, int headIndex, bool frenzy, int generation)
	{
		sEnemyShotSet* pSet = new sEnemyShotSet;
		pSet->count = 0;
		pSet->patternFunc = func;
		pSet->x = x;
		pSet->y = y;
		pSet->muki = angle;
		pSet->param_i[0] = headIndex;
		pSet->param_i[1] = frenzy ? 1 : 0;
		pSet->param_i[2] = generation;

		pSet->pEnemyShotHead = new sEnemyShot;
		pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
		pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

		pSet->prev = enemyShotSetHead.prev;
		pSet->next = &enemyShotSetHead;
		enemyShotSetHead.prev->next = pSet;
		enemyShotSetHead.prev = pSet;
	}

	static void UpdateHydraHeads()
	{
		for (int i = 0; i < HYDRA_MAX_HEADS; ++i)
		{
			HydraHead& h = g_hydraHeads[i];
			if (!h.active) continue;

			int life = count - h.spawnCount;
			double currentLength = h.length + life * h.lengthSpeed * 0.09;
			double wave = sin(h.phase + life * h.waveSpeed);
			double angle = h.baseAngle + wave * h.amplitude + h.branchBias;

			h.tipX = enemy.x + cos(angle) * currentLength;
			h.tipY = enemy.y + 12.0 + sin(angle) * currentLength;

			int interval = (h.generation >= 2) ? 49 : (h.generation == 1 ? 52 : 55);
			if (life > 18 && life % interval == 0)
			{
				bool frenzy = life < 60;
				CreateHydraShotSet(h.tipX, h.tipY, angle, ShotHydraRegenerate, i, frenzy, h.generation);
			}

			if (!h.branched && h.generation < 2 && g_hydraHeadCount < HYDRA_MAX_HEADS)
			{
				int branchTime = 180 + h.generation * 120;
				if (life >= branchTime)
				{
					h.branched = true;
					SpawnHydraHead(angle - 0.35, h.generation + 1, -0.18);
					SpawnHydraHead(angle + 0.35, h.generation + 1, 0.18);

					if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
					PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
				}
			}
		}
	}
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Hydra_ChatGPT()
{
	if (count == 1)
	{
		enemy.x = 240.0;
		enemy.y = 56.0;
		enemy.maxHp = enemy.hp = 200;

		g_hydraMuki = 1;
		g_hydraHeadCount = 0;

		for (int i = 0; i < HYDRA_MAX_HEADS; ++i)
		{
			g_hydraHeads[i] = HydraHead{};
		}

		for (int i = 0; i < 8; ++i)
		{
			double angle = -DX_PI * 0.90 + i * (DX_PI * 1.80 / 7.0);
			SpawnHydraHead(angle, 0, 0.0);
		}

		if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
		PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
	}
	else
	{
		enemy.x += 0.75 * (double)g_hydraMuki;
		if (count % 180 == 90) g_hydraMuki *= -1;
	}

	UpdateHydraHeads();

	if (g_hydraHeadCount >= HYDRA_MAX_HEADS && count % 120 == 1)
	{
		double aim = atan2(player.y - enemy.y, player.x - enemy.x);
		CreateHydraShotSet(enemy.x, enemy.y + 8.0, aim, ShotHydraBurst, -1, false, 0);
	}
}
