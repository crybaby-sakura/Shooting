// enemyPat_pitchDrop.cpp
// 弾幕:「千年の一滴」(ピッチドロップ実験モチーフ)
//
// 構成(約27秒で1ループ):
//   Phase 0 蓄積(900f): ボスの頭上に大玉が超スローでしだれ、中玉がへばりついて膨らむ。
//                       同時に左右交互の小玉の雨と、プレイヤー狙いの中玉を散射。
//   Phase 1 切り際(180f): 滴が完全停止。プレイヤーのX座標に黄色レーザーの予兆線が降下。
//   Phase 2 落下(260f):  滴が加速して落下。弱ホーミング。
//   Phase 3 静寂(300f):  着弾→白い飛沫が放射(1回だけバウンド&分裂)後、静寂。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 共通ヘルパー
// ============================================================

// 弾を1発生成して弾セットのリスト末尾に繋ぐ
static sEnemyShot* NewShot(sEnemyShotSet* pSet, int kind, double x, double y, double muki, double speed)
{
    sEnemyShot* p = new sEnemyShot;
    p->kind = kind;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->margin = 120;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾セットを生成して全体リストに繋ぐ
static sEnemyShotSet* CreateSet(void (*func)(sEnemyShotSet*), double x, double y, double muki, int kind)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->kind = kind;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// 全弾をそれぞれの向き・速度で直線移動
static void MoveShotsLinear(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 蓄積中の背景弾幕: 左右交互の斜めの雨(小玉)
// ============================================================
static void ShotRain(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int sideFlag = pSet->kind; // 0:左から 1:右から
        for (int i = 0; i < 4; i++) {
            double x = (sideFlag == 0) ? -15.0 : 495.0;
            double y = (double)GetRand(100);
            double muki = (sideFlag == 0) ? DX_PI / 2 - 0.4 : DX_PI / 2 + 0.4;
            muki += (GetRand(40) - 20) / 180.0 * DX_PI;
            double speed = 2.0 + GetRand(150) / 100.0;
            NewShot(pSet, img_enemyShotSmallBall[4], x, y, muki, speed); // 青の小玉
        }
    }
    MoveShotsLinear(pSet);
}

// ============================================================
// 蓄積中の背景弾幕: プレイヤー狙いの中玉(1発)
// ============================================================
static void ShotAimed(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        NewShot(pSet, img_enemyShotMediumBall[3], pSet->x, pSet->y, pSet->muki, 3.2); // シアンの中玉
    }
    MoveShotsLinear(pSet);
}

// ============================================================
// 予兆線: プレイヤーのX座標を上から下へ走る黄色レーザー(縦)
// ============================================================
static void ShotWarning(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 短レーザー(64x4)を muki=PI/2 で縦にして降下させる
        NewShot(pSet, img_enemyShotLaser[1], pSet->x, 0.0, DX_PI / 2, 2.6); // 黄
    }
    MoveShotsLinear(pSet);
}

// ============================================================
// 本体: 滴(大玉+中玉)の蓄積→停止→落下→着弾飛沫
// param_i[0](セット): 着弾フラグ 0:未着弾 1:着弾済み
// param_i[0](弾):     0:大玉 1:滴の中玉 2:飛沫(バウンドあり) 3:飛沫(バウンドなし)
// param_d[0,1]: 大玉の現在位置  param_d[2]: 落下速度  param_d[3]: 落下方向
// ============================================================
static void ShotDrop(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    int c = pSet->count;

    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pSet->param_i[0] = 0;
        pSet->param_d[0] = pSet->x;
        pSet->param_d[1] = pSet->y;
        pSet->param_d[2] = 0.0;
        pSet->param_d[3] = DX_PI / 2;

        // 滴の芯になる大玉(マゼンタ)
        sEnemyShot* pLarge = NewShot(pSet, img_enemyShotLargeBall[5], pSet->x, pSet->y, DX_PI / 2, 0.0);
        pLarge->param_i[0] = 0;
    }

    // --- 蓄積中: 大玉の下に中玉をへばりつかせて膨らませる ---
    if (c < 900) {
        if (c > 0 && c % 35 == 0) {
            sEnemyShot* m = NewShot(pSet, img_enemyShotMediumBall[5],
                pSet->param_d[0] + GetRand(16) - 8.0,
                pSet->param_d[1] + 24.0 + GetRand(8) - 4.0,
                DX_PI / 2, 0.0);
            m->param_i[0] = 1;
            m->param_d[6] = GetRand(628) / 100.0; // ゆらぎの位相
        }
    }
    // --- 切り際(900〜1079): 完全停止、何もしない ---
    // --- 落下(1080〜): 加速+弱ホーミング ---
    else if (c >= 1080 && pSet->param_i[0] == 0) {
        pSet->param_d[2] += 0.05;
        if (pSet->param_d[2] > 6.0) pSet->param_d[2] = 6.0;

        // プレイヤー方向へ弱く曲がる(真下から±0.35rad以内に制限)
        double lo = DX_PI / 2 - 0.35, hi = DX_PI / 2 + 0.35;
        double target = atan2(player.y - pSet->param_d[1], player.x - pSet->param_d[0]);
        if (target < lo) target = lo;
        if (target > hi) target = hi;
        double diff = target - pSet->param_d[3];
        if (diff > 0.02) diff = 0.02;
        if (diff < -0.02) diff = -0.02;
        pSet->param_d[3] += diff;

        pSet->param_d[0] += pSet->param_d[2] * cos(pSet->param_d[3]);
        pSet->param_d[1] += pSet->param_d[2] * sin(pSet->param_d[3]);

        // --- 着弾: 飛沫を上方放射 ---
        if (pSet->param_d[1] > 468.0) {
            pSet->param_i[0] = 1;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 14*5; i++) {
                double muki = DX_PI + (i + 0.5) / 14.0/5 * DX_PI; // 上半円へ放射
                sEnemyShot* s = NewShot(pSet, img_enemyShotSmallBall[6],
                    pSet->param_d[0], 466.0, muki, 3.0 + GetRand(150) / 100.0);
                s->param_i[0] = 2; // バウンドあり飛沫(白)
            }
            // 滴本体は画面外へ(メインルーチンに消去させる)
            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->param_i[0] <= 1) pShot->y = 1000.0;
                pShot = pShot->next;
            }
        }
    }

    // --- 各弾の移動 ---
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        if (pSet->param_i[0] == 1) {
            // 着弾後: 飛沫は個別移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (c < 900) {
            if (type == 0) {
                // 大玉: 超スローでしだれる
                pShot->y += 0.08;
                pSet->param_d[0] = pShot->x;
                pSet->param_d[1] = pShot->y;
            }
            else if (type == 1) {
                // 中玉: 大玉の下端でゆらゆら揺れる
                pShot->x = pSet->param_d[0] + cos(pShot->param_d[6] + c * 0.03) * 7.0;
                pShot->y = pSet->param_d[1] + 24.0 + sin(pShot->param_d[6] + c * 0.03) * 4.0;
            }
        }
        else if (c < 1080) {
            // 切り際: 停止
        }
        else {
            // 落下: 全滴が共通速度で移動(配置を保ったまま落ちる)
            pShot->x += pSet->param_d[2] * cos(pSet->param_d[3]);
            pShot->y += pSet->param_d[2] * sin(pSet->param_d[3]);
        }

        // 飛沫の1回限りのバウンド&分裂
        if (type == 2 && pShot->y > 466.0 && sin(pShot->muki) > 0.0) {
            pShot->param_i[0] = 3;
            pShot->y = 466.0;
            pShot->muki = -pShot->muki; // 跳ね返る
            pShot->speed *= 0.4;
            for (int i = 0; i < 2; i++) {
                sEnemyShot* s = NewShot(pSet, img_enemyShotSmallBall[6],
                    pShot->x, pShot->y,
                    -DX_PI / 2 + (GetRand(60) - 30) / 180.0 * DX_PI,
                    1.0 + GetRand(100) / 100.0);
                s->param_i[0] = 3; // 分裂弾はバウンドなし
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    static int phase;      // 0:蓄積 1:切り際 2:落下 3:静寂
    static int phaseTimer;
    static int side;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
        side = 0;

        // 1周目の滴を生成
        CreateSet(ShotDrop, enemy.x, enemy.y + 28.0, 0.0, 0);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        return;
    }

    // ボスはゆっくり揺れる(切り際は震える)
    enemy.x += sin(count * 0.01) * 0.3;
    if (phase == 1) enemy.x += (GetRand(4) - 2) * 0.5;
    if (enemy.x < 60.0) enemy.x = 60.0;
    if (enemy.x > 420.0) enemy.x = 420.0;

    switch (phase) {
    case 0: // 蓄積
        // 左右交互の斜めの雨
        if (phaseTimer % 15 == 0) {
            CreateSet(ShotRain, 0.0, 0.0, 0.0, side);
            side ^= 1;
        }
        // プレイヤー狙いの中玉
        if (phaseTimer % 50 == 25) {
            double mx = enemy.x, my = enemy.y + 10.0;
            CreateSet(ShotAimed, mx, my,
                atan2(player.y - my, player.x - mx), 0);
        }
        if (phaseTimer >= 900) {
            // 切り際へ: 予告音+予兆線
            phase = 1;
            phaseTimer = 0;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            CreateSet(ShotWarning, player.x, 0.0, 0.0, 0);
        }
        break;

    case 1: // 切り際(滴停止・予兆線降下中)
        if (phaseTimer >= 180) {
            phase = 2;
            phaseTimer = 0;
        }
        break;

    case 2: // 落下(着弾と飛沫はShotDrop内で処理)
        if (phaseTimer >= 260) {
            phase = 3;
            phaseTimer = 0;
        }
        break;

    case 3: // 静寂
        if (phaseTimer >= 60) {
            // 次のサイクル: 新しい滴の蓄積を開始
            phase = 0;
            phaseTimer = 0;
            CreateSet(ShotDrop, enemy.x, enemy.y + 28.0, 0.0, 0);
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        break;
    }

    phaseTimer++;
}