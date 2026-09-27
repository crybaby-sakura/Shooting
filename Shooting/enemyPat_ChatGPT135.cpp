#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// バックドラフト：炎が外へ広がった後、空気ごと中心へ吸い込み、
// 一気に下方向へ爆発する「予兆 → 吸引 → 爆発」の三段構成。
static void AddBackdraftShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y,
    double muki, double speed, int kind, int type)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->margin = 80;
    pEnemyShot->param_i[0] = type;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void ShotBackdraft(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 使う素材：橙の小玉・中楕円弾・大玉、赤の小玉、黄の菱形弾、橙の短レーザー
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 炎の輪。最初は外へ膨らみ、途中で中心へ吸い込まれる。
        for (int i = 0; i < 36; i++) {
            double a = DX_PI * 2.0 * (double)i / 36.0;
            double r = 10.0 + (double)(i % 3) * 4.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x + r * cos(a),
                pEnemyShotSet->y + r * sin(a),
                a,
                1.15,
                img_enemyShotSmallBall[8],
                0);
        }

        // 輝く炎の舌を楕円弾で表現。向きは流れに合わせる。
        for (int i = 0; i < 16; i++) {
            double a = DX_PI * 2.0 * (double)i / 16.0 + DX_PI / 16.0;
            double r = 22.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x + r * cos(a),
                pEnemyShotSet->y + r * sin(a),
                a,
                0.72,
                img_enemyShotMediumOval[8],
                1);
        }

        // 炎の中心から漏れる火の粉。こちらは最初から下へ流れ続ける。
        for (int i = 0; i < 9; i++) {
            double a = DX_PI / 2.0 + (double)(i - 4) * DX_PI / 18.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x + (double)(i - 4) * 10.0,
                pEnemyShotSet->y + 4.0,
                a,
                0.72 + 0.08 * (double)(i % 3),
                img_enemyShotSmallBall[0],
                2);
        }
    }

    // 外へ膨らんだ炎が突然、中心へ逆流する。
    if (pEnemyShotSet->count == 48) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0 || pEnemyShot->param_i[0] == 1) {
                // speed は正のまま、向きだけ反転して実際の速度ベクトルを逆転させる。
                pEnemyShot->muki += DX_PI;
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // 吸い込まれた炎が中心で圧縮された直後、大爆発する。
    if (pEnemyShotSet->count == 92) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 0 || pEnemyShot->param_i[0] == 1) {
                pEnemyShot->muki += DX_PI;
                pEnemyShot->speed *= 2.25;
            }
            pEnemyShot = pEnemyShot->next;
        }

        // 爆風の本体。中央から扇状に炎が吹き抜ける。
        for (int i = 0; i < 25; i++) {
            double a = DX_PI / 2.0 + (double)(i - 12) * DX_PI / 34.0;
            double spread = 0.65 + 0.45 * (double)(i % 4) / 3.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x + (double)(i - 12) * 2.0,
                pEnemyShotSet->y + 8.0,
                a,
                2.0 + spread,
                img_enemyShotSmallBall[0],
                3);
        }

        for (int i = 0; i < 7; i++) {
            double a = DX_PI / 2.0 + (double)(i - 3) * DX_PI / 16.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x + (double)(i - 3) * 15.0,
                pEnemyShotSet->y + 14.0,
                a,
                1.7 + 0.2 * (double)(i % 2),
                img_enemyShotLargeBall[8],
                4);
        }

        // 燃え上がる柱を短レーザーで補強。レーザー本体も下方向へ進む。
        for (int i = 0; i < 5; i++) {
            double a = DX_PI / 2.0 + (double)(i - 2) * DX_PI / 14.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x + (double)(i - 2) * 34.0,
                pEnemyShotSet->y + 18.0,
                a,
                1.15,
                img_enemyShotLaser[8],
                5);
        }
    }

    // 爆発後に飛び散る火花。吸引と爆発の境目を分かりやすくする。
    if (pEnemyShotSet->count == 126) {
        for (int i = 0; i < 20; i++) {
            double a = DX_PI * 2.0 * (double)i / 20.0 + DX_PI / 20.0;
            AddBackdraftShot(
                pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                a,
                2.35 + 0.25 * (double)(i % 3),
                img_enemyShotDiamond[1],
                6);
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Backdraft_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.82 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 予告から爆発までを一つの塊として繰り返す。
    if (count % 168 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBackdraft;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
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
