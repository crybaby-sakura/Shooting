#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：炭火の熱気！パチパチ焼き栗のホクホク爆ぜ
// ============================================================
static void Shot_YakiGuri(sEnemyShotSet* pEnemyShotSet)
{
    // 【フェーズ0: 初期化】セット生成時に「焼き栗」を発射
    if (pEnemyShotSet->count == 0) {
        // 予告音ではなく、中くらいの発射音で「ポン」と栗を放るイメージ
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 一度に3個の栗を左右にばらけて発射
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + (i - 1) * 70.0; // 左右にずらす
            pShot->y = pEnemyShotSet->y;
            // 上向き(-PI/2)を基準に少しばらけさせる
            pShot->muki = -DX_PI / 2.0 + (GetRand(40) - 20) / 100.0;
            pShot->speed = 3.5;

            // 素材: 橙(8)の中楕円弾(10.5x7.0) ＝ 焼き栗の皮
            pShot->kind = img_enemyShotMediumOval[8];
            pShot->param_i[0] = 0; // 状態0: 飛び込み中

            // 双方向リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 【フェーズ1〜3: 弾の挙動更新】
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除時に備えて次を保存

        if (pShot->param_i[0] == 0) {
            // 状態0: 飛び込み（減速させて画面上部で滞空させる）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.94; // 徐々に減速

            if (pShot->count > 45) {
                pShot->param_i[0] = 1; // 状態1: 滞空へ移行
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 状態1: 滞空（炭火でじっくり焼かれているようにゆらゆら揺れる）
            pShot->x += sin(pShot->count * 0.15) * 0.6;
            pShot->y += cos(pShot->count * 0.12) * 0.4;

            if (pShot->count > 100) {
                // 【ギミック: パチパチ爆ぜる！】
                double bx = pShot->x;
                double by = pShot->y;

                // 元の「栗」弾は役目を終えたのでリストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                // 爆発音（軽いパチッという音）
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                // 1. 火の粉（外側）：橙の小玉を放射状に撒き散らす
                for (int i = 0; i < 12; i++) {
                    sEnemyShot* pFire = new sEnemyShot;
                    pFire->x = bx;
                    pFire->y = by;
                    pFire->muki = (i / 12.0) * 2.0 * DX_PI + (GetRand(20) - 10) / 100.0;
                    pFire->speed = 3.0 + GetRand(20) / 10.0;
                    pFire->kind = img_enemyShotSmallBall[8]; // 橙(8)の小玉
                    pFire->param_i[0] = 2; // 状態2: 爆発後

                    pFire->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFire->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFire;
                    pEnemyShotSet->pEnemyShotHead->prev = pFire;
                }

                // 2. 中身（内側）：黄色い鱗弾を8方向に飛び散らせる（ホクホク感）
                for (int i = 0; i < 8*2; i++) {
                    sEnemyShot* pNaka = new sEnemyShot;
                    pNaka->x = bx;
                    pNaka->y = by;
                    pNaka->muki = (i / 8.0/2) * 2.0 * DX_PI;
                    pNaka->speed = 4.5 + GetRand(30) / 10.0;
                    pNaka->kind = img_enemyShotScale[1]; // 黄(1)の鱗弾(4.0x3.0)＝米粒
                    pNaka->param_i[0] = 2; // 状態2: 爆発後

                    pNaka->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNaka->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNaka;
                    pEnemyShotSet->pEnemyShotHead->prev = pNaka;
                }

                // 3. 湯気（残り香）：白い短レーザーを上方向にゆらゆら上昇させる
                for (int i = 0; i < 3; i++) {
                    sEnemyShot* pYuge = new sEnemyShot;
                    pYuge->x = bx + (GetRand(20) - 10);
                    pYuge->y = by;
                    pYuge->muki = -DX_PI / 2.0; // 真上
                    pYuge->speed = 1.2 + GetRand(10) / 10.0;
                    pYuge->kind = img_enemyShotLaser[6]; // 白(6)の短レーザー(64.0x4.0)＝湯気
                    pYuge->param_i[0] = 3; // 状態3: 湯気
                    pYuge->margin = 80;

                    pYuge->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pYuge->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pYuge;
                    pEnemyShotSet->pEnemyShotHead->prev = pYuge;
                }
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 状態2: 爆発後の弾（火の粉・中身）の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.985; // 空気抵抗で少し減速
        }
        else if (pShot->param_i[0] == 3) {
            // 状態3: 湯気の移動（横にゆらゆらしながら上昇）
            pShot->x += sin(pShot->count * 0.2) * 1.5;
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_RoastChestnut_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 160.0; // 少し下げて発射位置を確保
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復する動き（焼き栗屋台の親父のイメージ）
        enemy.x += 1.2 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 約2秒(120フレーム)ごとに「焼き栗セット」を発射
    if (count % 120 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot_YakiGuri;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}