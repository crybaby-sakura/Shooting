// enemyPat_Thunderbolt.cpp
// 弾幕: 青天の霹靂「快晴の落雷」
//  ・「晴天」: 不気味なほど穏やかに、青と白の小弾がまばらに降り注ぐ(効果音も意図的に鳴らさない)
//  ・「霹靂」: 予兆なしに、短レーザーを縦に連結した"光の柱"が自機の直前位置や周辺へ高速落下
//  ・着弾地点では中玉が一旦静止し、レーザーが画面下に届く頃に全方向へ炸裂(雷の衝撃波)
//  ・レーザーは1本ずつ横にずらしてジグザグに見せ、雷らしい不規則さを演出
//  ※画面フラッシュ用の素材がないため、演出は弾の構成だけで表現している

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 晴天フェーズ: まばらに降り注ぐ小弾(青/白)
// ============================================================
static void ShotSunny(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 2発ずつ、ごくまばらに降らせる
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 20.0 + GetRand(440);                                   // 画面幅のどこか
            pEnemyShot->y = -10.0;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;   // ほぼ真下+微小な揺らぎ
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;                        // ゆっくり降る
            // 青(4)と白(6)の小玉を交互に: 「晴天空」の表現
            pEnemyShot->kind = img_enemyShotSmallBall[(pEnemyShotSet->kind + i) % 2 == 0 ? 4 : 6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 等速直線移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 霹靂フェーズ: 落雷の柱 + 着弾炸裂
//  param_i[0] = 落雷の本数
//  param_d[0]〜 = 各落雷のx座標(param_d[0]は自機直前位置狙い)
// ============================================================
static void ShotBolt(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 雷鳴
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int bolts = pEnemyShotSet->param_i[0];
        for (int b = 0; b < bolts; b++) {
            double bx = pEnemyShotSet->param_d[b];

            // --- 光の柱: 短レーザー(白)を縦に連結して高速落下 ---
            for (int k = 0; k < 12; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx + (GetRand(20) - 10);   // 1本ずつずらしてジグザグな雷に
                pEnemyShot->y = -32.0 - k * 58.0;          // 縦に連結(隙間なく柱に見せる)
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 20.0;                  // 高速: 反応時間は約25フレーム
                pEnemyShot->kind = img_enemyShotLaser[6];  // 白

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // --- 衝撃波: 中玉(白)を着弾点に置き、一旦静止しておく ---
            for (int k = 0; k < 14; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = 460.0;
                pEnemyShot->muki = (k / 14.0) * 2.0 * DX_PI + 0.1;  // 全方向
                pEnemyShot->speed = 0.0;                            // まだ動かない
                pEnemyShot->param_d[0] = 5.0 + GetRand(150) / 100.0;// 起動後の速度(ラフな炸裂感)
                pEnemyShot->param_i[0] = 25;                        // 起動フレーム(先端が画面下に届く頃)
                pEnemyShot->kind = img_enemyShotMediumBall[6];      // 白

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動 + 静止弾(衝撃波)の一斉起動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 速度0(=静止中)の弾が規定フレームに達したら炸裂
        if (pShot->speed == 0.0 && pShot->count >= pShot->param_i[0]) {
            pShot->speed = pShot->param_d[0];
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 落雷の予告音(音だけのセット。晴天の静けさを破る不穏な合図)
// ============================================================
static void ShotChargeWarn(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    static int muki;
    static int bolt_round;

    const int CYCLE = 600; // 1サイクル = 600フレーム(10秒)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        bolt_round = 0;
    }
    else {
        // 雲のようにゆったり左右に漂う
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
        if (enemy.x < 60.0)  enemy.x = 60.0;
        if (enemy.x > 420.0) enemy.x = 420.0;
    }

    int t = count % CYCLE;

    // ---- 晴天: 常時、まばらな小弾を降らせる(音なし=静けさの演出) ----
    if (t % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSunny;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = count / 10; // 小玉の青/白交代用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---- 予兆音: 落雷の30フレーム前から予告音だけ鳴らす ----
    if (t == 210 || t == 450) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChargeWarn;
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

    // ---- 霹靂: 予兆なしに落雷 ----
    //  1回目(3本): うち1本は自機直前位置、残りはランダム
    //  2回目(5本): 柱が増えて画面が一瞬埋まる
    if (t == 240 || t == 480) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBolt;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = bolt_round++;

        int bolts = (t == 240) ? 3 : 5;
        pEnemyShotSet->param_i[0] = bolts;

        // 1本目は自機の直前位置を狙い、横へ逃げても当たる可能性を残す
        double aimX = player.x;
        if (aimX < 20.0)  aimX = 20.0;
        if (aimX > 460.0) aimX = 460.0;
        pEnemyShotSet->param_d[0] = aimX;
        for (int i = 1; i < bolts; i++) {
            pEnemyShotSet->param_d[i] = 20.0 + GetRand(440);
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}