#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
// 弾幕パターン：ニュートンのゆりかご
// ============================================================================
static void ShotPendulum(sEnemyShotSet* pEnemyShotSet)
{
    static int phase = 0; // 0:待機, 1:スイング, 2:崩壊

    // 初期化フェーズ
    if (pEnemyShotSet->count == 0) {
        phase = 1;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 5つのおもりと糸を生成
        for (int i = 0; i < 5; i++) {
            // おもり (大玉・白)
            sEnemyShot* pendulum = new sEnemyShot;
            pendulum->kind = img_enemyShotLargeBall[6];
            pendulum->x = 240.0 + (i - 2) * 40.0;
            pendulum->y = 60.0;
            pendulum->param_i[0] = i;       // インデックス (0~4)
            pendulum->speed = 0.0;          // メインルーチンの自動移動を無効化
            pendulum->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pendulum->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pendulum;
            pEnemyShotSet->pEnemyShotHead->prev = pendulum;

            // 糸 (短レーザー・白)
            sEnemyShot* stringShot = new sEnemyShot;
            stringShot->kind = img_enemyShotLaser[6];
            stringShot->x = 240.0 + (i - 2) * 40.0;
            stringShot->y = 30.0;           // レーザー中心をy=30にすればy=0~60をカバー
            stringShot->muki = DX_PI / 2.0; // 下向き
            stringShot->param_i[0] = i + 5; // インデックス (5~9)
            stringShot->speed = 0.0;
            stringShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            stringShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = stringShot;
            pEnemyShotSet->pEnemyShotHead->prev = stringShot;
        }
    }

    // スイング周期: 240フレーム (片道120フレーム)
    int t = pEnemyShotSet->count % 240;

    // HPが一定以下になったら崩壊フェーズへ移行
    if (phase == 1 && enemy.hp <= 100) {
        phase = 2;
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    if (phase == 1) {
        // 【スイング処理】
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int idx = pShot->param_i[0];

            if (idx >= 0 && idx < 5) { // おもりの座標更新
                double px = 240.0 + (idx - 2) * 40.0; // 支点X
                double py = 0.0;                      // 支点Y
                double length = 80.0;                 // 糸の長さ
                double theta = 0.0;

                // 右端(idx=4)が左へスイング (t=0~119)
                if (idx == 4 && t < 120) {
                    theta = (DX_PI / 4.0) * cos((t / 120.0) * DX_PI);
                }
                // 左端(idx=0)が右へスイング (t=120~239)
                else if (idx == 0 && t >= 120) {
                    theta = -(DX_PI / 4.0) * cos(((t - 120) / 120.0) * DX_PI);
                }

                pShot->x = px + length * sin(theta);
                pShot->y = py + length * (1.0 - cos(theta)) + 20.0; // +20.0で初期位置を調整
                pShot->speed = 0.0;
            }
            else if (idx >= 5 && idx < 10) { // 糸の座標・角度更新
                int pendulumIdx = idx - 5;
                double px = 240.0 + (pendulumIdx - 2) * 40.0;
                double py = 0.0;
                double length = 80.0;
                double theta = 0.0;

                if (pendulumIdx == 4 && t < 120) {
                    theta = (DX_PI / 4.0) * cos((t / 120.0) * DX_PI);
                }
                else if (pendulumIdx == 0 && t >= 120) {
                    theta = -(DX_PI / 4.0) * cos(((t - 120) / 120.0) * DX_PI);
                }

                double bx = px + length * sin(theta);
                double by = py + length * (1.0 - cos(theta)) + 20.0;

                // 糸は支点とおもりの中間に配置し、向きをおもりへ合わせる
                pShot->x = (px + bx) / 2.0;
                pShot->y = (py + by) / 2.0;
                pShot->muki = atan2(by - py, bx - px);
                pShot->speed = 0.0;
            }
            pShot = pShot->next;
        }

        // 【衝突時の衝撃波】 (t=60:右→左衝突, t=180:左→右衝突)
        if (t == 60 || t == 180) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 中央の3つのおもり (idx = 1, 2, 3) から衝撃波を展開
            for (int k = 1; k <= 3; k++) {
                double cx = 240.0 + (k - 2) * 40.0;
                double cy = 60.0;
                for (int j = 0; j < 16*3; j++) {
                    sEnemyShot* shock = new sEnemyShot;
                    shock->kind = img_enemyShotSmallBall[3]; // シアンで目立たせる
                    shock->x = cx;
                    shock->y = cy;
                    shock->muki = (j / 16.0/3) * 2.0 * DX_PI;
                    shock->speed = 3.5;
                    shock->param_i[0] = 99;
                    shock->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    shock->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = shock;
                    pEnemyShotSet->pEnemyShotHead->prev = shock;
                }
            }
        }
    }
    else if (phase == 2) {
        // 【崩壊処理】 糸が切れ、おもりが落下しながら弾をばらまく
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int idx = pShot->param_i[0];
            if (idx >= 0 && idx < 5) { // おもり
                // 初回のみ落下速度と角度を付与
                if (pShot->speed == 0.0) {
                    pShot->speed = 2.0 + GetRand(20) / 10.0-1.5;
                    pShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
                }

                // 落下中にランダムで中玉をばらまく
                if (pEnemyShotSet->count % 5 == 0) {
                    sEnemyShot* scatter = new sEnemyShot;
                    scatter->kind = img_enemyShotMediumBall[8]; // 橙
                    scatter->x = pShot->x;
                    scatter->y = pShot->y;
                    scatter->muki = GetRand(360) / 180.0 * DX_PI;
                    scatter->speed = 2.5;
                    scatter->param_i[0] = 99;
                    scatter->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    scatter->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = scatter;
                    pEnemyShotSet->pEnemyShotHead->prev = scatter;
                }
            }
            pShot = pShot->next;
        }
    }

    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// ============================================================================
// 敵本体のパターンエントリーポイント
// ============================================================================
void EnemyPat_NewtonsCradle_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // パターン開始タイミング
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPendulum;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}