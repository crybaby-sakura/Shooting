// EnemyPat_Phalanx_MetaAI.cpp
// モチーフ：ファランクス「方陣 ファランクス・ウォール」
// 素材選定：
//  盾 = 中楕円弾(10.5x7.0) 青 ... 横幅があり盾に見える、判定も大きめで壁になる
//  槍 = 短レーザー(64.0x4.0) 赤 ... 細長く槍の穂先～柄を表現、向きで伸縮が分かりやすい
//  合図 = 小玉(2.5x2.5) 白 ... 自機狙いの1発で「突け！」の掛け声を可視化
// 効果音：突撃前に sound_enemyCharge、突き出し時に heavy、引っ込める時に light

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotPhalanx(sEnemyShotSet* pEnemyShotSet)
{
    const int SHIELD_NUM = 16 * 2;
    const int GAP_A = 5 * 2;
    const int GAP_B = 10 * 2;
    const int CYCLE_LEN = 360 / 3; // 1サイクル 6秒
    const int MAX_CYCLE = 3;   // 3回繰り返して崩壊

    // ----------------------------------------
    // 生成時 count == 0
    // ----------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 0) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SHIELD_NUM; i++) {
            if (i == GAP_A || i == GAP_B) continue; // 意図的な隙間＝安全地帯

            // 盾
            {
                sEnemyShot* pShot = new sEnemyShot;
                // 480幅に30px間隔で配置 中央寄せ -225～+225
                double baseX = pEnemyShotSet->x - 225.0 + i * 30.0 / 2 + (GetRand(10) - 5) / 2.0; // 少しランダムで有機的に
                pShot->x = baseX;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = DX_PI / 2.0; // 真下
                pShot->speed = 0.8;
                pShot->kind = img_enemyShotMediumOval[4]; // 青
                pShot->margin = 30.0;
                pShot->param_i[0] = 0; // 0:盾
                pShot->param_i[1] = i; // 元の列番号
                pShot->param_d[0] = baseX;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
            // 槍（盾の20px後ろに待機）
            {
                sEnemyShot* pShot = new sEnemyShot;
                double baseX = pEnemyShotSet->x - 225.0 + i * 30.0 / 2;
                pShot->x = baseX;
                pShot->y = pEnemyShotSet->y - 20.0;
                pShot->muki = DX_PI / 2.0;
                pShot->speed = 0.8 * 3;
                pShot->kind = img_enemyShotLaser[0]; // 赤レーザー = 槍
                pShot->margin = 40.0;
                pShot->param_i[0] = 1; // 1:槍
                pShot->param_i[1] = 0; // 状態 0:随伴 1:突撃中 2:保持 3:退却中
                pShot->param_i[2] = i;
                pShot->param_d[0] = baseX;
                pShot->param_d[1] = 0.0; // 突撃開始Y
                pShot->margin = 480;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        pEnemyShotSet->param_i[0] = 0; // 経過サイクル数保存用
    }

    int total = pEnemyShotSet->count;
    int cycle = total / CYCLE_LEN;
    int t = total % CYCLE_LEN;

    // 合図弾生成：各サイクルの突撃直前 t==150
    if (cycle < MAX_CYCLE && t == 150 / 3) {
        if (CheckSoundMem(sound_enemyCharge) == 0) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = -5; i <= 5; i++) {
            sEnemyShot* pSignal = new sEnemyShot;
            pSignal->x = pEnemyShotSet->x;
            pSignal->y = pEnemyShotSet->y;
            pSignal->muki = atan2(player.y - pSignal->y, player.x - pSignal->x) + i * 0.2;
            pSignal->speed = 3.0;
            pSignal->kind = img_enemyShotSmallBall[6]; // 白
            pSignal->param_i[0] = 2; // 2:合図
            pSignal->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pSignal->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pSignal;
            pEnemyShotSet->pEnemyShotHead->prev = pSignal;
        }

        if (CheckSoundMem(sound_enemyShot_heavy) == 0) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (cycle < MAX_CYCLE && t == 250 / 3) {
        if (CheckSoundMem(sound_enemyShot_light) == 0) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ----------------------------------------
    // 弾の更新
    // ----------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 崩壊フェーズ：3サイクル終了後
        if (cycle >= MAX_CYCLE) {
            if (total == MAX_CYCLE * CYCLE_LEN) {
                // 一度だけ自機狙いに切り替え
                if (pShot->param_i[0] != 2) { // 合図弾以外
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pShot->speed = 1.5 + GetRand(100) / 100.0;
                }
            }
            // その後は等速で落下
        }
        else {
            // 通常ファランクス
            if (pShot->param_i[0] == 0) { // 盾
                if (t < 120 / 3) {
                    pShot->muki = DX_PI / 2.0;
                    pShot->speed = 0.8;
                }
                else if (t < 290 / 3) {
                    pShot->speed = 0.0;
                }
                else { // 290-359 帰陣
                    pShot->muki = -DX_PI / 2.0;
                    pShot->speed = 1.5;
                }
            }
            else if (pShot->param_i[0] == 1) { // 槍
                if (t < 120 / 3) { // 盾に随伴前進
                    pShot->muki = DX_PI / 2.0;
                    pShot->speed = 0.8;
                    pShot->param_i[1] = 0;
                }
                else if (t < 150 / 3) { // 停止・構え
                    pShot->speed = 0.0;
                }
                else if (t < 190 / 3) { // 突撃
                    if (pShot->param_i[1] == 0) {
                        pShot->param_i[1] = 1;
                        pShot->param_d[1] = pShot->y; // 突撃開始Yを記録
                    }
                    if (pShot->param_i[1] == 1) {
                        pShot->muki = DX_PI / 2.0;
                        pShot->speed = 7.0 * 3;
                        // 280px伸びたら停止
                        if (pShot->y >= pShot->param_d[1] + 280.0) {
                            pShot->speed = 0.0;
                            pShot->param_i[1] = 2;
                        }
                    }
                }
                else if (t < 250 / 3) { // 保持
                    pShot->speed = 0.0;
                    if (pShot->param_i[1] == 1) pShot->param_i[1] = 2;
                }
                else if (t < 290 / 3) { // 退却
                    if (pShot->param_i[1] == 2) pShot->param_i[1] = 3;
                    pShot->muki = -DX_PI / 2.0;
                    pShot->speed = 4.0 * 3;
                }
                else { // 帰陣
                    pShot->muki = -DX_PI / 2.0;
                    pShot->speed = 1.5;
                }
            }
            else { // 合図弾は自由落下（自機狙いのまま）
                // 何もしない
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_Phalanx_MetaAI()
{
    static int shot_interval;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        shot_interval = 0;
    }
    else {
        // ファランクスは陣形を組むので左右に動かず中央で構える
        // 少しだけ揺らぎを入れる
        enemy.x = 240.0 + sin(count / 60.0) * 10.0;
    }

    // 1200フレームごとに1セット生成。長く生きるパターンなので重ねない
    if (count % 400 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhalanx;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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