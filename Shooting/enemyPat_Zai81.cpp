// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：演算遅延ラグ・テレポーション
static void ShotLagTeleportation(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 予告音を鳴らす（「処理落ちが発生する」予感を演出）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 第1層：遅い大玉（赤）- ラグって止まっている時間が長い
        for (int i = 0; i < 12 * 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / 12.0 / 2) * i;
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤色の大玉

            pEnemyShot->param_i[0] = 0; // 状態: 0=移動中, 1=フリーズ中
            pEnemyShot->param_i[1] = 0; // フリーズ経過フレーム
            pEnemyShot->param_i[2] = 20; // フリーズ時間
            pEnemyShot->param_i[3] = 30; // 移動時間
            pEnemyShot->param_i[4] = 0; // 移動フレームカウンタ

            // フリーズ中にスキップされる座標の移動量（ワープ距離）
            pEnemyShot->param_d[0] = pEnemyShot->speed * pEnemyShot->param_i[2] * cos(pEnemyShot->muki);
            pEnemyShot->param_d[1] = pEnemyShot->speed * pEnemyShot->param_i[2] * sin(pEnemyShot->muki);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 第2層：速い中玉（青）- カクカクした動きで目が追いづらい
        for (int i = 0; i < 24 * 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / 24.0 / 2) * i + (DX_PI / 24.0 / 2);
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青色の中玉

            pEnemyShot->param_i[0] = 0; // 状態: 0=移動中, 1=フリーズ中
            pEnemyShot->param_i[1] = 0; // フリーズ経過フレーム
            pEnemyShot->param_i[2] = 10; // フリーズ時間（短め）
            pEnemyShot->param_i[3] = 15; // 移動時間（短め）
            pEnemyShot->param_i[4] = 0; // 移動フレームカウンタ

            // ワープ距離
            pEnemyShot->param_d[0] = pEnemyShot->speed * pEnemyShot->param_i[2] * cos(pEnemyShot->muki);
            pEnemyShot->param_d[1] = pEnemyShot->speed * pEnemyShot->param_i[2] * sin(pEnemyShot->muki);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 移動フレームをカウント
            pShot->param_i[4]++;

            // 規定の移動時間に達したらフリーズ状態へ遷移
            if (pShot->param_i[4] >= pShot->param_i[3]) {
                pShot->param_i[0] = 1; // フリーズへ
                pShot->param_i[1] = 0; // フリーズ経過時間リセット
                pShot->param_i[4] = 0; // 移動フレームリセット
            }
        }
        else if (pShot->param_i[0] == 1) {
            // フリーズ中（座標の更新をスキップすることで残像となる）
            // ※フリーズ中は見た目が止まっているが当たり判定も止まるため、
            //   「残像に当たった！」というヒヤリとする経験を提供する
            pShot->param_i[1]++;

            // 規定のフリーズ時間に達したらテレポーション
            if (pShot->param_i[1] >= pShot->param_i[2]) {
                // スキップされていた分の距離を一瞬で適用（ワープ）
                pShot->x += pShot->param_d[0];
                pShot->y += pShot->param_d[1];
                pShot->param_i[0] = 0; // 通常移動に戻る
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Lag_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 90フレームごとに発射
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLagTeleportation;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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