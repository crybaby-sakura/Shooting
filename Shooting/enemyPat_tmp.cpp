// enemyPat_Tmp.cpp
// 弾幕「噴水 -カルーセル・スプラッシュ-」
// ・噴水口から水柱(中玉シアン)を放物線で打ち上げ、頂点で飛沫(小玉白)に分裂
// ・着水した水柱は波紋(小玉青)に変化して左右へ流れる
// ・カウント・画面外消去はメインルーチン側の仕様に委ねる

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 噴水弾幕の調整用定数 ----
static const double FOUNTAIN_GRAVITY = 0.12;   // 重力加速度(大きいほど水柱が低い)
static const double FOUNTAIN_GROUND_Y = 440.0;  // 着水ライン(波紋が走る高さ)
static const int    FOUNTAIN_CYCLE = 150;    // 噴水の1周期(フレーム)
static const int    FOUNTAIN_SPRAY_END = 60;     // 吹き上げを止めるフレーム(周期の前半のみ噴射)

// 弾の状態 param_i[0] の意味
//  0:水柱(頂点で分裂し、着水で波紋になる)
//  1:根元の飛沫(分裂しない。画面外でメインルーチンが消去)
//  2:頂点で分裂した飛沫(重力で落下し、画面外で消去)
//  3:着水後の波紋(水面に沿って水平移動)
static const int STATE_JET = 0;
static const int STATE_MIST = 1;
static const int STATE_SPLASH = 2;
static const int STATE_RIPPLE = 3;

// 弾を1発生成してセットの弾リストに繋げる
static sEnemyShot* SpawnFountainShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y,
    double angle, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = angle;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_d[0] = speed * cos(angle); // vx
    pEnemyShot->param_d[1] = speed * sin(angle); // vy(上向きは負)
    pEnemyShot->param_i[0] = STATE_JET;
    pEnemyShot->param_i[1] = 0;                  // 頂点分裂済みフラグ
    // リストの末尾に追加
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 噴水の弾幕パターン(セットごとに1周期分を自動運転)
static void ShotFountain(sEnemyShotSet* pEnemyShotSet)
{
    int cycle = pEnemyShotSet->count ;

    // 効果音:周期の先頭で発射音、次の噴射の20フレーム前に予告音
    if (cycle == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (cycle == FOUNTAIN_CYCLE - 20) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 吹き上げ(周期の前半のみ) ----
    if (cycle < FOUNTAIN_SPRAY_END) {
        // 噴水口が左右に揺れる(0.35rad ≒ 20°)
        double sway = sin(pEnemyShotSet->count * 0.03) * 0.35;

        // 水柱:中玉シアンを2フレームに1発
        if (cycle % 2 == 0) {
            double angle = -DX_PI / 2.0 + sway + (GetRand(22) - 11) / 180.0 * DX_PI;
            SpawnFountainShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                angle, 4.0 + GetRand(60) / 100.0,
                img_enemyShotMediumBall[3]); // シアン
        }
        // 根元の飛沫:小玉白を4フレームに2発
        if (cycle % 4 == 0) {
            for (int j = 0; j < 2; j++) {
                double angle = -DX_PI / 2.0 + (GetRand(80) - 40) / 180.0 * DX_PI;
                sEnemyShot* pMist = SpawnFountainShot(pEnemyShotSet,
                    pEnemyShotSet->x, pEnemyShotSet->y,
                    angle, 2.0 + GetRand(150) / 100.0,
                    img_enemyShotSmallBall[6]); // 白
                pMist->param_i[0] = STATE_MIST; // 分裂しない飛沫
            }
        }
    }

    // ---- 既存弾の更新(放物線運動・分裂・着水) ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // ループ中に新弾を追加するため先に保存

        if (pShot->param_i[0] == STATE_RIPPLE) {
            // 波紋:着水ラインに沿って左右へ流れる
            pShot->x += pShot->speed * cos(pShot->muki);
        }
        else {
            // 重力を加えて放物線移動
            pShot->param_d[1] += FOUNTAIN_GRAVITY;
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
            pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0]
                + pShot->param_d[1] * pShot->param_d[1]);

            // 頂点で分裂(水柱のみ・1回だけ)
            if (pShot->param_i[0] == STATE_JET
                && pShot->param_i[1] == 0
                && pShot->param_d[1] >= 0.0) {
                pShot->param_i[1] = 1;
                for (int j = 0; j < 2; j++) {
                    // 下方向中心にランダム散開する飛沫(小玉白)
                    sEnemyShot* pSplash = SpawnFountainShot(pEnemyShotSet,
                        pShot->x, pShot->y,
                        DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI,
                        1.5 + GetRand(100) / 100.0,
                        img_enemyShotSmallBall[6]); // 白
                    pSplash->param_i[0] = STATE_SPLASH;
                }
            }

            // 着水したら波紋に変化(水柱のみ)
            if (pShot->param_i[0] == STATE_JET
                && pShot->y >= FOUNTAIN_GROUND_Y) {
                pShot->y = FOUNTAIN_GROUND_Y;
                pShot->kind = img_enemyShotSmallBall[4]; // 青
                pShot->muki = (pShot->x < pEnemyShotSet->x) ? 0.0 : DX_PI; // 噴水口から遠ざかる方向
                pShot->speed = 1.8 + GetRand(80) / 100.0;
                pShot->param_i[0] = STATE_RIPPLE;
            }
        }
        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // 噴水本体は左右に動かず、ゆっくり上下に浮遊する
        enemy.y = 90.0 + sin(count * 0.03) * 8.0;
    }

    // 周期の先頭で噴水セットを1つ生成(以降はセット側で自動運転)
    if (count % FOUNTAIN_CYCLE == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFountain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 噴水口の位置
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