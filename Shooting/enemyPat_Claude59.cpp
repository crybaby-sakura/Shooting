// enemyPat_SanSanShichiByoshi.cpp
// 三三七拍子:発射リズムそのものを「三・三・七」の拍手拍子に合わせ、
// 最後に一本締め(決め弾)で締めくくる弾幕。
// ※難易度上げ版:弾数増・弾速増・拍手が自機側へ寄る・各バーストに自機狙い弾を混在。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕:拍手弾(左右の手が自機寄りの一点へ向かって寄っていく一撃。先頭弾は自機狙い)
static void ShotClap(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        int se = sound_enemyShot_light;
        if (pEnemyShotSet->param_i[1] == 1) se = sound_enemyShot_medium;
        else if (pEnemyShotSet->param_i[1] == 2) se = sound_enemyShot_heavy;

        if (CheckSoundMem(se)) StopSoundMem(se);
        PlaySoundMem(se, DX_PLAYTYPE_BACK);

        int bulletCount = pEnemyShotSet->param_i[0];
        for (int i = 0; i < bulletCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double muki;
            if (i == 0) {
                // 先頭の1発は正確な自機狙い(扇の中に紛れ込ませる)
                muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            }
            else {
                // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意。
                muki = pEnemyShotSet->muki + (GetRand(44) - 22) / 180.0 * DX_PI;
            }
            double speed = pEnemyShotSet->param_d[0] + GetRand(50) / 100.0;

            // formula駆動:発射時の位置・角度・速度をparam_dに保持し、
            // 以後は毎フレーム pEnemyShot->count から直接位置を計算する(速度積分はしない)。
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = muki;
            pEnemyShot->param_d[3] = speed;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->kind = pEnemyShotSet->kind;
           
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * pShot->count * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * pShot->count * sin(pShot->param_d[2]);
        pShot = pShot->next;
    }
}

// 弾幕:一本締め(決め弾。自機狙いの大玉+二層の全方位喝采弾)
static void ShotFinale(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 決めの一発:自機狙いの大玉(紅)
        {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            double speed = 4.2;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = muki;
            pEnemyShot->param_d[3] = speed;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤・大玉
            
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 喝采の散らし弾(全方位・二層の速度差で銃弾リングを二重に)
        const int SPARK_COUNT = 360;
        for (int i = 0; i < SPARK_COUNT; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double muki = DX_PI * 2.0 * i / SPARK_COUNT;
            double speed = (i % 2 == 0) ? (2.6 + GetRand(40) / 100.0) : (4.0 + GetRand(40) / 100.0);

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = muki;
            pEnemyShot->param_d[3] = speed;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤・銃弾
           
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->param_d[3] * pShot->count * cos(pShot->param_d[2]);
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * pShot->count * sin(pShot->param_d[2]);
        pShot = pShot->next;
    }
}

// 敵本体のパターン:三三七拍子(難易度上げ版)
void EnemyPat_337Beat_Claude()
{
    static int muki;

    // 発射タイミング表(パターン開始からの経過フレーム)。テンポを詰めて緊迫感を上げている。
    static const int clapTimes[13] = {
        0, 9, 18,                         // 三(グループ0)
        50, 59, 68,                       // 三(グループ1)
        100, 109, 118, 127, 136, 145, 154 // 七(グループ2)
    };
    static const int CHARGE_TIME = 174; // 締めの予告(溜め)
    static const int FINALE_TIME = 254; // 一本締めの決め弾
    static const int CYCLE_LEN = 314;   // 1サイクルの長さ(以降ループ)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 拍子に合わせて緩やかに左右へ揺れる
        enemy.x += 0.4 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    int localCount = (count - 1) % CYCLE_LEN;

    // 締めの予告音(決め弾を放つ前の"溜め")
    if (localCount == CHARGE_TIME) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 三・三・七の拍手弾
    for (int i = 0; i < 13; i++) {
        if (localCount != clapTimes[i]) continue;

        int group = (i < 3) ? 0 : (i < 6) ? 1 : 2;
        int subIndexInGroup = (group == 0) ? i : (group == 1) ? i - 3 : i - 6;

        // 七拍子(グループ2)は後半に向けて弾数が増え、盛り上がっていく(難易度上げ:基準弾数を増加)
        int bulletCount = 6 + group * 3 + (group == 2 ? subIndexInGroup * 2 : 0);
        double baseSpeed = 2.6 + group * 0.4;          // 難易度上げ:弾速を全体的に増加
        double handOffset = 40.0 + group * 20.0;        // 拍が進むほど手が大きく開く(範囲も拡大)

        int soundLevel;
        if (group == 0) soundLevel = 0;
        else if (group == 1) soundLevel = 1;
        else soundLevel = (subIndexInGroup >= 4) ? 2 : 1;

        // 弾の絵柄:グループが進むほど大きく・締まった印象の弾へ
        int imgHandle;
        if (group == 0) imgHandle = img_enemyShotSmallBall[6];  // 白
        else if (group == 1) imgHandle = img_enemyShotMediumBall[1]; // 黄
        else imgHandle = img_enemyShotDiamond[8]; // 橙

        double originY = enemy.y + 10.0;
        // 難易度上げ:収束先を自機寄りにブレンドし、拍手そのものが自機を追ってくるようにする
        double targetX = enemy.x * 0.3 + player.x * 0.7;
        double targetY = enemy.y + 160.0;

        // 左手・右手の2つの拍手弾セットを同時発射し、中央で交差させる
        for (int side = 0; side < 2; side++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotClap;
            pEnemyShotSet->x = enemy.x + (side == 0 ? -handOffset : handOffset);
            pEnemyShotSet->y = originY;
            pEnemyShotSet->muki = atan2(targetY - pEnemyShotSet->y, targetX - pEnemyShotSet->x);
            pEnemyShotSet->kind = imgHandle;
            pEnemyShotSet->param_i[0] = (side == 0) ? bulletCount / 2 : bulletCount - bulletCount / 2;
            pEnemyShotSet->param_i[1] = soundLevel;
            pEnemyShotSet->param_d[0] = baseSpeed;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // 一本締めの決め弾
    if (localCount == FINALE_TIME) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFinale;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // ShotFinale内で個別に向きを計算するため未使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}