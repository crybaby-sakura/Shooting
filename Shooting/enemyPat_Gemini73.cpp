// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 残影として配置されたShotSetの制御関数
static void ShotConvergenceAndDiffusion(sEnemyShotSet* pSet)
{
    int waitTime = pSet->param_i[0]; // 発射開始までの待機時間
    int id = pSet->param_i[1];       // 残影のID (0～7)

    // 配置された瞬間に赤の大玉（マーカー）を設置
    if (pSet->count == 0) {
        sEnemyShot* pMarker = new sEnemyShot;
        pMarker->x = pSet->x;
        pMarker->y = pSet->y;
        pMarker->kind = img_enemyShotLargeBall[0]; // 赤の大玉
        pMarker->param_i[0] = 99; // 状態フラグ：99=マーカー

        // リストに追加
        pMarker->prev = pSet->pEnemyShotHead->prev;
        pMarker->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pMarker;
        pSet->pEnemyShotHead->prev = pMarker;
    }

    // 発射タイミング（回数と頻度を2倍：7フレーム間隔で6ウェーブ）
    if (pSet->count >= waitTime && pSet->count <= waitTime + 35 && (pSet->count - waitTime) % 7 == 0) {

        if (id == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        double dx = enemy.x - pSet->x;
        double dy = enemy.y - pSet->y;
        double dist = sqrt(dx * dx + dy * dy);

        double muki_to_boss = atan2(dy, dx);
        double speed_to_boss = dist / 60.0;

        for (int j = 0; j < 9; j++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = muki_to_boss;
            pShot->speed = speed_to_boss;
            pShot->kind = img_enemyShotScale[3]; // シアンの鱗弾

            pShot->param_i[0] = 0;  // 0=収束中, 1=拡散中
            pShot->param_i[1] = 60; // 到達残りフレーム

            if (id == 0 && j == 0) pShot->param_i[2] = 1; // 音再生フラグ

            double base_angle = id * 5.0 * DX_PI / 180.0 + count * 2.5 * DX_PI / 180.0 / 7;
            pShot->param_d[2] = base_angle + j * 40.0 * DX_PI / 180.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の更新処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 99) {
            // マーカー（赤の大玉）：最後の鱗弾発射後（waitTime + 35）に消去
            if (pSet->count > waitTime + 135) {
                pShot->y = -9999.0; // 画面外へ飛ばして消去
            }
        }
        else if (pShot->param_i[0] == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->param_i[0] = 1;
                pShot->muki = pShot->param_d[2];
                pShot->speed = 3.5;
                pShot->kind = img_enemyShotDiamond[3]; // 菱形弾へ
                if (pShot->param_i[2] == 1) {
                    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
                }
            }
        }
        else if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン：残影収束陣
void EnemyPat_Warp_Gemini()
{
    int t = (count - 60) % 300;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -50.0;
        enemy.maxHp = enemy.hp = 200;
    }

    if (count < 60) enemy.y += 170.0 / 60.0;

    // 高速テレポートと残影配置：10フレーム間隔で8回
    if (t >= 10 && t <= 80 && t % 10 == 0) {
        int id = (t - 10) / 10;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 半径200の円周上からランダム配置
        double angle = GetRand(359) * DX_PI / 180.0;
        enemy.x = 240.0 + 220.0 * cos(angle);
        enemy.y = 240.0 + 220.0 * sin(angle);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotConvergenceAndDiffusion;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->param_i[0] = 140 - t; // 発射待機時間
        pSet->param_i[1] = id;      // 残影ID

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 最終位置
    if (t == 100) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}