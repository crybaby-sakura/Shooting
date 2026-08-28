// enemyPat_Tmp.cpp
// ブラウン運動をモチーフにした弾幕パターン
// 既存弾（小玉・中玉など）を組み合わせ、各弾が微小なランダム方向変化を繰り返すことで
// 不規則にジグザグしながら拡散する様子を表現する。
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：ブラウン拡散弾（ランダムウォーク弾幕）
// ============================================================
// 各弾は一定フレームごとに進行方向を微小にランダム変化させ、
// ブラウン運動のような不規則な軌跡を描きながら外側へ拡散する。
static void ShotBrownian(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ----- 発射時（count == 0）のみ弾を生成 -----
    if (pEnemyShotSet->count == 0) {
        // 中くらいの発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1セットあたりの弾数（12〜16発程度で雲状にする）
        const int num = 12 + GetRand(4);  // 12〜16

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            // 発射位置：敵中心から少しだけばらつかせる（粒子の初期位置の揺らぎ）
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);

            // 初期方向：基本は自機方向寄りだが、広めにばらつかせる
            // GetRand(x) は 0〜x の整数を返すので注意
            double baseMuki = pEnemyShotSet->muki;
            double randOffset = (GetRand(140) - 70) / 180.0 * DX_PI;  // ±約70度
            pEnemyShot->muki = baseMuki + randOffset;

            // 速度：中速〜やや遅めで拡散しやすくする
            pEnemyShot->speed = (140 + GetRand(80)) / 100.0;  // 1.4〜2.2

            // 弾種：小玉（粒子感）を基本に、少しだけ中玉を混ぜる
            // 色はシアン〜青系で統一感を出しつつ、ランダムで変化
            int color = 3 + GetRand(2);  // 3:シアン, 4:青, 5:マゼンタ あたり
            if (GetRand(3) == 0) {
                // たまに中玉を混ぜて密度感を出す
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----- 毎フレームの移動＆ブラウン運動シミュレーション -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ブラウン運動：一定間隔で進行方向を微小にランダム変化
        // pShot->count はメインルーチンで毎フレーム+1される
        // 変化間隔を 5〜7 フレーム程度にすると、ジグザグ感が出やすい
        if (pShot->count > 0 && pShot->count % 6 == 0) {
            // ±約18度の範囲でランダムに角度を加算
            // GetRand(36) は 0〜36 → -18〜+18 になる
            double delta = (GetRand(36) - 18) / 180.0 * DX_PI;
            pShot->muki += delta;
        }

        // 位置更新（通常の直進）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// 関数名は指定通り void EnemyPat_BrownianMotion_Grok() とする
// ============================================================
void EnemyPat_BrownianMotion_Grok()
{
    static int muki;
    static int shot_count;

    // 初期化（最初の1フレーム）
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;  // 固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵はゆっくり左右に往復（サンプルをベースに少しゆっくり）
        enemy.x += 0.7 * (double)muki;
        if (count % 160 == 80) muki *= -1;

        // 画面端で反転を確実にする
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
    }

    // 一定間隔でブラウン拡散弾を発射
    // 20フレームごとだと密度が高め、30フレームだと少し余裕あり
    if (count % 24 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBrownian;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // 自機狙いを基本方向にする
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