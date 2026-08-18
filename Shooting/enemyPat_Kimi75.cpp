// enemyPat_pentagram.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 五芒星弾幕「星降りの儀式」
// Phase 0 (count  0) : 予告 — 頂点に大玉を5つ配置
// Phase 1 (count 25) : 星の描画 — 大・中・小の3重五芒星を稜線で描く
// Phase 2 (count 60) : 活性化 — 稜線弾が中心に向かって加速
// Phase 3 (count110) : 星屑の拡散 — 中心から星型に波状拡散
// Phase 4 (count160) : 星の崩壊 — 残存弾が稜線に垂直に外へ飛散
static void ShotPentagram(sEnemyShotSet* pEnemyShotSet)
{
    // --- 初期化 ---
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0;                 // フェーズ
        pEnemyShotSet->param_i[1] = pEnemyShotSet->kind % 9; // 基準色 (0〜8)
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;  // 中心X
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;  // 中心Y

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    int  phase = pEnemyShotSet->param_i[0];
    int  baseColor = pEnemyShotSet->param_i[1];
    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];

    // 五芒星の頂点角度（上から時計回り、DXLib座標系）
    const double angles[5] = {
        -DX_PI / 2.0,                     // 上
        -DX_PI / 2.0 + 2.0 * DX_PI / 5.0, // 右上 (-18°)
        -DX_PI / 2.0 + 4.0 * DX_PI / 5.0, // 右下 (54°)
        -DX_PI / 2.0 + 6.0 * DX_PI / 5.0, // 左下 (126°)
        -DX_PI / 2.0 + 8.0 * DX_PI / 5.0  // 左上 (198° = -162°)
    };

    // 一筆書き順序: 上(0)→左上(4)→右下(2)→右上(1)→左下(3)→上(0)
    const int order[5] = { 0, 4, 2, 1, 3 };

    // ============================================================
    // Phase 0: 予告 — 頂点に大玉を配置（静止）
    // ============================================================
    if (phase == 0 && pEnemyShotSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double r = 130.0;
            pShot->x = cx + r * cos(angles[i]);
            pShot->y = cy + r * sin(angles[i]);
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotLargeBall[baseColor];
            pShot->param_i[0] = 10; // 予告弾マーク

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        pEnemyShotSet->param_i[0] = 1;
    }

    // ============================================================
    // Phase 1: 星の描画 — 3重五芒星（大・中・小）
    // ============================================================
    if (phase == 1 && pEnemyShotSet->count == 25) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const double radii[3] = { 130.0, 85.0, 45.0 };
        // 3重星で色をずらすと虹のような見た目になる
        int colors[3] = {
            baseColor,
            (baseColor + 3) % 9,
            (baseColor + 6) % 9
        };

        for (int star = 0; star < 3; star++) {
            double r = radii[star];
            double vx[5], vy[5];
            for (int i = 0; i < 5; i++) {
                vx[i] = cx + r * cos(angles[i]);
                vy[i] = cy + r * sin(angles[i]);
            }

            for (int line = 0; line < 5; line++) {
                int from = order[line];
                int to = order[(line + 1) % 5];
                double dx = vx[to] - vx[from];
                double dy = vy[to] - vy[from];
                double len = sqrt(dx * dx + dy * dy);
                int num = (int)(len / 10.0 * 2); // 10px間隔で配置

                for (int j = 1; j < num; j++) {
                    double t = (double)j / num;
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = vx[from] + dx * t;
                    pShot->y = vy[from] + dy * t;
                    pShot->muki = atan2(dy, dx); // 稜線の方向を保存
                    pShot->speed = 0.0;          // 初期は静止

                    // 星のサイズに応じて弾種を変化
                    if (star == 0) {
                        pShot->kind = img_enemyShotSmallBall[colors[star]];
                    }
                    else if (star == 1) {
                        pShot->kind = img_enemyShotScale[colors[star]];
                    }
                    else {
                        pShot->kind = img_enemyShotDiamond[colors[star]];
                    }

                    pShot->param_i[0] = 20 + line;   // 稜線弾マーク + 稜線番号
                    pShot->param_d[0] = pShot->muki; // 稜線の角度（崩壊時に使用）

                    // 稜線の中点から中心への方向を計算
                    double midX = (vx[from] + vx[to]) * 0.5;
                    double midY = (vy[from] + vy[to]) * 0.5;
                    pShot->param_d[1] = atan2(cy - midY, cx - midX);

                    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
        pEnemyShotSet->param_i[0] = 2;
    }

    // ============================================================
    // Phase 2: 活性化 — 稜線弾が中心へ向かって加速
    // ============================================================
    if (phase == 2 && pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] >= 20 && pShot->param_i[0] < 30) {
                // 中心方向へ向きを変え、速度を与える
                pShot->muki = pShot->param_d[1];
                pShot->speed = 1.2 + GetRand(15) / 10.0; // 1.2 〜 2.7
            }
            pShot = pShot->next;
        }
        pEnemyShotSet->param_i[0] = 3;
    }

    // ============================================================
    // Phase 3: 星屑の拡散 — 中心から星型に波状拡散
    // ============================================================
    if (phase == 3 && pEnemyShotSet->count == 110) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int numBurst = 40 * 2;
        for (int i = 0; i < numBurst; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = cx;
            pShot->y = cy;

            double baseAngle = (double)i / numBurst * 2.0 * DX_PI;

            // 最も近い五芒星の頂点方向を探す
            double nearest = baseAngle;
            double minDiff = 999.0;
            for (int k = 0; k < 5; k++) {
                double diff = baseAngle - angles[k];
                while (diff > DX_PI)  diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                if (fabs(diff) < minDiff) {
                    minDiff = fabs(diff);
                    nearest = angles[k];
                }
            }

            // 頂点方向に35%引き寄せることで、星型の波を形成
			double pull = (nearest - baseAngle) * 0.35;
			pShot->muki = baseAngle + pull;

			pShot->speed = 2.0 + GetRand(25) / 10.0; // 2.0 〜 4.5

			// 弾種：中玉と銃弾を交互に
			if (i % 2 == 0) {
				pShot->kind = img_enemyShotMediumBall[(baseColor + i % 3) % 9];
			}
			else {
				pShot->kind = img_enemyShotBullet[(baseColor + i % 3) % 9];
			}
			pShot->param_i[0] = 30; // 拡散弾マーク

			pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
			pShot->next = pEnemyShotSet->pEnemyShotHead;
			pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
			pEnemyShotSet->pEnemyShotHead->prev = pShot;
		}
		pEnemyShotSet->param_i[0] = 4;
	}

	// ============================================================
	// Phase 4: 星の崩壊 — 残存稜線弾が稜線に垂直に外へ飛散
	// ============================================================
	if (phase == 4 && pEnemyShotSet->count == 160) {
		if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
		PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

		sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pShot != pEnemyShotSet->pEnemyShotHead) {
			if (pShot->param_i[0] >= 20 && pShot->param_i[0] < 30) {
				// 稜線に垂直な方向（±90°）に飛散
				double perp = pShot->param_d[0] + (GetRand(1) == 0 ? DX_PI / 2.0 : -DX_PI / 2.0);
				pShot->muki = perp;
				pShot->speed = 2.0 + GetRand(20) / 10.0; // 2.0 〜 4.0
				pShot->param_i[0] = 40; // 崩壊弾マーク
			}
			pShot = pShot->next;
		}
		pEnemyShotSet->param_i[0] = 5; // 終了フェーズ
	}

	// ============================================================
	// 毎フレームの弾移動処理
	// ============================================================
	sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
	while (pShot != pEnemyShotSet->pEnemyShotHead) {
		int type = pShot->param_i[0];

		if (type == 10) {
			// 予告弾：微かに脈動（見た目演出）
			// 実際の位置は変えず、speed=0のまま
		}
		else if (type >= 20 && type < 30) {
			// 稜線弾：Phase 2で中心へ加速、それ以外は静止
			// speedはPhase 2の初期化時に設定済み
			pShot->x += pShot->speed * cos(pShot->muki);
			pShot->y += pShot->speed * sin(pShot->muki);
		}
		else if (type == 30) {
			// 拡散弾：直進
			pShot->x += pShot->speed * cos(pShot->muki);
			pShot->y += pShot->speed * sin(pShot->muki);
		}
		else if (type == 40) {
			// 崩壊弾：垂直に外へ飛散
			pShot->x += pShot->speed * cos(pShot->muki);
			pShot->y += pShot->speed * sin(pShot->muki);
		}

		pShot = pShot->next;
	}
}

// 敵本体のパターン
void EnemyPat_Pentagram_Kimi()
{
	static int muki;
	static int shot_count;

	if (count == 1) {
		// ゲーム画面は 480x480
		enemy.x = 240.0;
		enemy.y = 160.0;
		enemy.maxHp = enemy.hp = 200; // 200で固定
		muki = 1;
		shot_count = 0;
	}
	else {
		// ゆっくり左右に揺れる
		enemy.x += 0.6 * (double)muki;
		if (count % 180 == 90) muki *= -1;
	}

	// 約4秒ごとに五芒星弾幕を発射
	if (count % 240 == 1) {
		sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
		pEnemyShotSet->count = 0;
		pEnemyShotSet->patternFunc = ShotPentagram;
		pEnemyShotSet->x = enemy.x;
		pEnemyShotSet->y = enemy.y + 15.0;
		pEnemyShotSet->muki = 0.0; // 五芒星は自機狙いではなく幾何学的
		pEnemyShotSet->kind = shot_count++;

		pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
		pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
		pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

		pEnemyShotSet->prev = enemyShotSetHead.prev;
		pEnemyShotSet->next = &enemyShotSetHead;
		enemyShotSetHead.prev->next = pEnemyShotSet;
		enemyShotSetHead.prev = pEnemyShotSet;
	}
}