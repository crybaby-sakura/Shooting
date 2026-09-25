#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：ピッチドロップ（粘性の雫）
// ============================================================
static void ShotPitchDrop(sEnemyShotSet* pEnemyShotSet)
{
    int setCount = pEnemyShotSet->count;

    // --- 初期生成 (count == 0) ---
    if (setCount == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1. 親弾（ノズル）の生成
        sEnemyShot* pParent = new sEnemyShot;
        pParent->x = pEnemyShotSet->x;
        pParent->y = 40.0+50;
        pParent->muki = 0.0;
        pParent->speed = 0.0;
        pParent->kind = img_enemyShotLargeBall[7]; // 黒い大玉（ピッチの質感）
        pParent->param_i[0] = 1; // 種別: 親弾

        pParent->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pParent->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pParent;
        pEnemyShotSet->pEnemyShotHead->prev = pParent;

        // 2. 子弾（滴）の生成
        sEnemyShot* pChild = new sEnemyShot;
        pChild->x = pEnemyShotSet->x;
        pChild->y = 55.0+50;
        pChild->muki = DX_PI / 2.0;
        pChild->speed = 0.16; // 超ゆっくりな落下
        pChild->kind = img_enemyShotMediumBall[8]; // 橙の中玉
        pChild->param_i[0] = 2; // 種別: 子弾
        pChild->param_i[1] = 0; // 状態: 0=伸長中
        pChild->margin = 480;

        pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pChild->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
        pEnemyShotSet->pEnemyShotHead->prev = pChild;

        // 3. レーザー（粘性の糸）の生成
        sEnemyShot* pLaser = new sEnemyShot;
        pLaser->x = pEnemyShotSet->x;
        pLaser->y = 47.5+50;
        pLaser->muki = DX_PI / 2.0;
        pLaser->speed = 0.0;
        pLaser->kind = img_enemyShotLaser[8]; // 橙の短レーザー
        pLaser->param_i[0] = 3; // 種別: レーザー

        pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pLaser->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
        pEnemyShotSet->pEnemyShotHead->prev = pLaser;
    }

    // --- 弾の挙動更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];
        int state = pShot->param_i[1];

        if (type == 2) { // 子弾
            if (state == 0) {
                // [伸長中] ゆっくり落下
                pShot->y += pShot->speed;

                // 画面中央付近で限界を迎えて断裂
                if (pShot->y >= 220.0) {
                    pShot->param_i[1] = 1; // 状態: 1=跳ね返り中
                    pShot->speed = -5.5;   // 上方へ跳ね返る

                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }
            }
            else if (state == 1) {
                // [跳ね返り中] 重力で減速し、やがて下向きになる
                pShot->y += pShot->speed;
                pShot->speed += 0.15;

                // 20フレーム経過、または速度が正（下向き）になったら急落下へ移行
                if (pShot->count >= 20 || pShot->speed > 0.0) {
                    pShot->param_i[1] = 2; // 状態: 2=急落下中
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 5.0+3; // 急加速
                    pShot->kind = img_enemyShotMediumBall[0]; // 赤の中玉に変化（危険を視覚的に警告）

                    //if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    //PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
            }
            else if (state == 2) {
                // [急落下中] 自機に向かって一直線
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 画面下部到達で着弾・波紋生成
                if (pShot->y >= 460.0) {
                    pShot->param_i[1] = 3; // 状態: 3=波紋生成済

                    // 波紋の生成（12方向に小玉をばら撒く）
                    for (int i = 0; i < 12*2; i++) {
                        sEnemyShot* pRipple = new sEnemyShot;
                        pRipple->x = pShot->x;
                        pRipple->y = pShot->y;
                        pRipple->muki = (DX_PI * 2.0 / 12.0/2) * i;
                        pRipple->speed = 2.5;
                        pRipple->kind = img_enemyShotSmallBall[6]; // 白の小玉
                        pRipple->param_i[0] = 4; // 種別: 波紋弾
                        pRipple->param_i[1] = 0;
                        pRipple->margin = 240;

                        pRipple->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pRipple->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pRipple;
                        pEnemyShotSet->pEnemyShotHead->prev = pRipple;
                    }
                    // 本体は画面外へ退避させ、メインルーチンに消去を委ねる
                    pShot->y = 600.0;
                    pShot->speed = 0.0;
                }
            }
        }
        else if (type == 4) {
            // [波紋弾] 広がっていく演出のため徐々に減速
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.992;
        }

        pShot = pShot->next;
    }

    // --- レーザー（糸）の位置・状態更新 ---
    sEnemyShot* pLaser = nullptr;
    sEnemyShot* pChild = nullptr;
    sEnemyShot* pParent = nullptr;

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) pParent = pShot;
        else if (pShot->param_i[0] == 2) pChild = pShot;
        else if (pShot->param_i[0] == 3) pLaser = pShot;

        pShot = pShot->next;
    }

    if (pLaser && pParent && pChild) {
        if (pChild->param_i[1] <= 1) {
            // 伸長中〜跳ね返り直後まではレーザーで繋ぐ
            double dx = pChild->x - pParent->x;
            double dy = pChild->y - pParent->y;
            pLaser->x = pParent->x + dx / 2.0;
            pLaser->y = pParent->y + dy / 2.0;
            pLaser->muki = atan2(dy, dx);

            // 伸びるにつれて色を赤く変化させ、千切れそうな緊張感を演出
            if (dy > 120.0) {
                pLaser->kind = img_enemyShotLaser[0]; // 赤の短レーザー
            }
        }
        else {
            // 断裂後はレーザーを画面外へ飛ばして実質的に非表示化
            pLaser->y = -100.0;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_PitchDrop_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右のゆっくりした移動（画面端で反転）
        enemy.x += 0.8 * (double)muki;
        if (enemy.x >= 440.0 || enemy.x <= 40.0) {
            muki *= -1;
        }
    }

    // 90フレームごとに新しい滴を生成
    // GetRand(40) は 0〜40 を返すため、-20 することで敵の現在位置から前後20pxの範囲でばらつきを出す
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPitchDrop;

        pEnemyShotSet->x = enemy.x + (GetRand(40) - 20);
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0;
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