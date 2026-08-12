// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：五芒星の刻印
static void ShotPentagram(sEnemyShotSet* pEnemyShotSet)
{
    // 五芒星の一筆書き順序 (0->2->4->1->3)
    static const int order[5] = { 0, 2, 4, 1, 3 };

    // 五芒星の頂点の基本角度 (ラジアン)
    static const double baseAngles[5] = {
        -DX_PI / 2.0,
        -DX_PI / 2.0 + DX_PI * 2.0 / 5.0,
        -DX_PI / 2.0 + DX_PI * 2.0 / 5.0 * 2,
        -DX_PI / 2.0 + DX_PI * 2.0 / 5.0 * 3,
        -DX_PI / 2.0 + DX_PI * 2.0 / 5.0 * 4
    };

    double R = 200.0; // 五芒星の外接円の半径
    double rot = pEnemyShotSet->param_d[0]; // 全体の回転角度

    // セットの初回フレームでのみ、描き始めの重い音を再生
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 20フレームで1本の線を描く。4フレームに1発撃つため、1本の線につき5発の大玉が生成される。
    int lineIdx = pEnemyShotSet->count / 20;
    int frameInLine = pEnemyShotSet->count % 20;

    if (lineIdx < 5 && frameInLine % 4 == 0) {
        int startIdx = order[lineIdx];
        int endIdx = order[(lineIdx + 1) % 5];

        // 線の始点（頂点）と終点（次の頂点）の座標を計算
        double sx = pEnemyShotSet->x + R * cos(baseAngles[startIdx] + rot);
        double sy = pEnemyShotSet->y + R * sin(baseAngles[startIdx] + rot);

        double ex = pEnemyShotSet->x + R * cos(baseAngles[endIdx] + rot);
        double ey = pEnemyShotSet->y + R * sin(baseAngles[endIdx] + rot);

        double dx = ex - sx;
        double dy = ey - sy;
        double dist = sqrt(dx * dx + dy * dy);

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = sx;
        pShot->y = sy;
        pShot->muki = atan2(dy, dx);
        pShot->speed = 4.0 * 2;
        pShot->kind = img_enemyShotLargeBall[0]; // 赤い大玉
        pShot->margin = 120.0;

        // 交差点までの距離を設定 (五芒星の交差点は線分の長さの 約0.381966 倍の位置)
        pShot->param_d[0] = 0.0;             // 移動距離の累積用
        pShot->param_d[1] = dist * 0.381966; // 炸発させる距離
        pShot->param_i[0] = 0;               // 炸裂済みフラグ (0:未炸裂, 1:炸裂済み)

        // 双方向リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 弾の移動と炸裂処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 交差点での炸裂処理
        pShot->param_d[0] += pShot->speed;
        if (pShot->param_d[0] >= pShot->param_d[1] && pShot->param_i[0] == 0) {
            pShot->param_i[0] = 1; // 炸裂済みにする

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 親弾の進行方向を中心に、扇状に5方向に炸裂
            for (int i = 0; i < 5; i++) {
                sEnemyShot* pBurst = new sEnemyShot;
                pBurst->x = pShot->x;
                pBurst->y = pShot->y;
                pBurst->muki = pShot->muki + (i - 2) * (DX_PI / 6.0); // 中心±30度で5方向
                pBurst->speed = 1.5 * 1.5;
                pBurst->kind = img_enemyShotMediumBall[4]; // 青い中玉
                pBurst->margin = 20.0;
                pBurst->param_i[0] = 1;

                // 双方向リストに追加
                pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pBurst->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
                pEnemyShotSet->pEnemyShotHead->prev = pBurst;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Pentagram_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 100フレーム（約1.7秒）ごとに新しい五芒星のセットを生成
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPentagram;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // セットごとに36度（PI/5）ずつ回転角度をズラす
        pEnemyShotSet->param_d[0] = shot_count * (DX_PI / 5.0);
        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}