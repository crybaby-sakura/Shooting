// enemyPat_katori.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：蚊取り線香「渦巻の燻煙」
// ============================================================
// 【使用素材】
//   小玉(2.5x2.5)・黒色[7] … 渦巻き本体＋灰
//   中玉(7.0x7.0)・橙色[8] … 火の先端＋終息時の火花
//   大玉(20.0x20.0)・白色[6] … 立ち上る煙
//   効果音: sound_enemyCharge(予告) / sound_enemyShot_light(灰落下) / sound_enemyShot_medium(終息)
// ============================================================

static void ShotKatori(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ----- 初期化（count==0 のみ） -----
    if (pEnemyShotSet->count == 0) {
        // 渦巻きの中心を画面中央付近に固定
        pEnemyShotSet->param_d[0] = 240.0;   // 中心X
        pEnemyShotSet->param_d[1] = 240.0;   // 中心Y
        pEnemyShotSet->param_d[2] = 0.0;     // 現在角度（ラジアン）
        pEnemyShotSet->param_d[3] = 140.0;   // 開始半径
        pEnemyShotSet->param_d[4] = 0.12;    // 角度増分（約6.9度/フレーム）
        pEnemyShotSet->param_d[5] = 0.35;    // 半径減分
        pEnemyShotSet->param_i[0] = 0;       // フェーズ 0=形成中, 1=終息

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];
    double angle = pEnemyShotSet->param_d[2];
    double radius = pEnemyShotSet->param_d[3];

    // ========================================================
    //  【フェーズ1】渦巻き形成（半径が縮むまで）
    // ========================================================
    if (pEnemyShotSet->param_i[0] == 0) {

        // --- 渦巻き本体：小玉(黒) ---
        // 毎フレーム、現在の渦巻き座標に弾を配置。射出後減速して静止し、
        // 「線香の軌跡」として残る。
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = cx + radius * cos(angle);
        pEnemyShot->y = cy + radius * sin(angle);
        pEnemyShot->muki = angle + DX_PI / 2.0;   // 接線方向
        pEnemyShot->speed = 0.6;
        pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒色
        pEnemyShot->param_i[0] = 0;               // type: 渦巻き本体
        pEnemyShot->param_d[0] = 0.998;            // 減衰率

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // --- 火の先端：中玉(橙) ---
        // 3フレームに1回、先端に大きめの弾を置くことで「燃えている部分」を強調
        if (pEnemyShotSet->count % 3 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + radius * cos(angle);
            pEnemyShot->y = cy + radius * sin(angle);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;              // 静止
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙色
            pEnemyShot->param_i[0] = 1;           // type: 火の先端

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 煙：大玉(白) ---
        // 渦巻きの外周から上方へゆっくり浮遊。サイン波で左右に揺れる。
        if (pEnemyShotSet->count % 8 == 0) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + (radius + 12.0) * cos(angle);
            pEnemyShot->y = cy + (radius + 12.0) * sin(angle);
            // 上方向(-PI/2) ±15度のばらつき
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = (25 + GetRand(35)) / 100.0; // 0.25〜0.60
            pEnemyShot->kind = img_enemyShotLargeBall[6];   // 白色
            pEnemyShot->param_i[0] = 2;           // type: 煙
            pEnemyShot->param_d[0] = GetRand(360) / 180.0 * DX_PI; // 位相
            pEnemyShot->param_d[1] = 0.997;       // 減速率

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 灰：小玉(黒) ---
        // 渦巻きの内側（中心寄り）から真下へ直線落下。
        if (pEnemyShotSet->count % 15 == 0) {
            pEnemyShot = new sEnemyShot;
            double ashAngle = GetRand(360) / 180.0 * DX_PI;
            double ashRadius = GetRand((int)(radius * 0.6));
            pEnemyShot->x = cx + ashRadius * cos(ashAngle);
            pEnemyShot->y = cy + ashRadius * sin(ashAngle);
            pEnemyShot->muki = DX_PI / 2.0;       // 真下
            pEnemyShot->speed = 1.0 + GetRand(15) / 10.0; // 1.0〜2.5
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒色
            pEnemyShot->param_i[0] = 3;           // type: 灰

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 灰が落ちる軽い音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // --- 渦巻きパラメータ更新 ---
        pEnemyShotSet->param_d[2] += pEnemyShotSet->param_d[4];
        pEnemyShotSet->param_d[3] -= pEnemyShotSet->param_d[5];

        // 半径が小さくなったら終息フェーズへ移行
        if (pEnemyShotSet->param_d[3] < 10.0) {
            pEnemyShotSet->param_i[0] = 1;
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    // ========================================================
    //  【フェーズ2】終息（火が消える演出）
    // ========================================================
    else if (pEnemyShotSet->param_i[0] == 1) {
        // 中心から橙色の火花が散る（count=450まで）
        if (pEnemyShotSet->count < 450 && pEnemyShotSet->count % 4 == 0) {
            for (int i = 0; i < 3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
                pEnemyShot->speed = (80 + GetRand(120)) / 100.0; // 0.8〜2.0
                pEnemyShot->kind = img_enemyShotMediumBall[8];   // 橙色
                pEnemyShot->param_i[0] = 4;       // type: 散り際の火花
                pEnemyShot->param_d[0] = 0.997;    // 減衰率

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ========================================================
    //  弾の毎フレーム更新
    // ========================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        switch (type) {
        case 0: // 渦巻き本体：減速して静止
            pShot->speed *= pShot->param_d[0];
            if (pShot->speed < 0.03) pShot->speed = 0.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case 1: // 火の先端：そのまま静止
            break;

        case 2: // 煙：上方向へゆっくり＋サイン波で左右揺れ
            pShot->speed *= pShot->param_d[1];
            pShot->x += pShot->speed * cos(pShot->muki)
                + sin(pShot->count * 0.035 + pShot->param_d[0]) * 0.35;
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case 3: // 灰：真下へ直線落下
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case 4: // 散り際の火花：拡散して減速
            pShot->speed *= pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_MosquitoCoil_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // count=60 で蚊取り線香弾幕を1回だけ発射（画面中央に展開）
    if (count % 600 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKatori;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = 0.0;
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