// enemyPat_tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ファランクス・プッシュ
static void PhalanxMove(sEnemyShotSet* pEnemyShotSet)
{
    const int COLS = 10 * 2;
    const int ROWS = 6 * 2;
    const double GRID_SPACING_X = 34.0 / 2;
    const double GRID_SPACING_Y = 32.0 / 2;
    const double COMPRESS_SPACING_X = 24.0 / 2;
    const double ADVANCE_SPEED = 0.75 * 2;          // 前進速度(px/frame)
    const double SWAY_AMPLITUDE = 50.0;         // 蛇行振幅
    const double SWAY_PERIOD = 330.0;           // 蛇行周期(frame)
    const int    FIRE_INTERVAL = 12 / 2;            // 槍発射間隔(frame)
    const double SPEAR_SPEED = 4.0 * 2;             // 槍の弾速

    // 初回のみ隊列を生成
    if (pEnemyShotSet->count == 0) {
        double offsetXTotal = (COLS - 1) * GRID_SPACING_X;
        double offsetYTotal = (ROWS - 1) * GRID_SPACING_Y;

        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLS; col++) {
                sEnemyShot* pShot = new sEnemyShot;

                pShot->x = pEnemyShotSet->x + col * GRID_SPACING_X - offsetXTotal / 2.0;
                pShot->y = pEnemyShotSet->y + row * GRID_SPACING_Y - offsetYTotal / 2.0;
                pShot->muki = 0.0;
                pShot->speed = 0.0;

                // 前列（row=5）は赤、次列は橙、他は青
                int color;
                if (row == ROWS - 1)      color = 0; // 赤
                else if (row == ROWS - 2) color = 8; // 橙
                else                      color = 4; // 青
                pShot->kind = img_enemyShotSmallBall[color];

                // param_i[0]=列, param_i[1]=行, param_i[2]=種別(0:盾, 1:槍/散開後)
                pShot->param_i[0] = col;
                pShot->param_i[1] = row;
                pShot->param_i[2] = 0;

                // 中心からの相対オフセット
                pShot->param_d[0] = col * GRID_SPACING_X - offsetXTotal / 2.0;
                pShot->param_d[1] = row * GRID_SPACING_Y - offsetYTotal / 2.0;

                // 連結リストへ追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // フェーズ管理用
        pEnemyShotSet->param_i[0] = 0;                 // 0:前進, 1:圧縮射撃, 2:散開
        pEnemyShotSet->param_i[1] = 0;                 // 発射行インデックス
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;  // 蛇行の基準X
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;  // 現在Y
    }

    // フェーズ別の制御
    int phase = pEnemyShotSet->param_i[0];

    if (phase == 0) {
        // 蛇行しながら前進
        double time = (double)pEnemyShotSet->count;
        double baseX = pEnemyShotSet->param_d[0];
        double sway = SWAY_AMPLITUDE * sin(time / SWAY_PERIOD * 2.0 * DX_PI);

        pEnemyShotSet->x = baseX + sway;
        pEnemyShotSet->y += ADVANCE_SPEED;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;

        // 画面高さの60%付近まで来たら圧縮フェーズへ
        if (pEnemyShotSet->y >= 280.0) {
            pEnemyShotSet->param_i[0] = 1;
            pEnemyShotSet->param_i[1] = 0; // 前列から発射開始

            // 横間隔を圧縮
            double offsetXTotalC = (COLS - 1) * COMPRESS_SPACING_X;
            double offsetYTotalC = (ROWS - 1) * GRID_SPACING_Y;

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[2] == 0) {
                    pShot->param_d[0] = pShot->param_i[0] * COMPRESS_SPACING_X - offsetXTotalC / 2.0;
                    pShot->param_d[1] = pShot->param_i[1] * GRID_SPACING_Y - offsetYTotalC / 2.0;
                }
                pShot = pShot->next;
            }
        }
    }
    else if (phase == 1) {
        // 一定間隔で行ごとに槍を発射
        if (pEnemyShotSet->count % FIRE_INTERVAL == 0) {
            int rowToFire = pEnemyShotSet->param_i[1];

            if (rowToFire >= 0 && rowToFire < ROWS) {
                // 効果音（あれば）
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
                while (pShot != pEnemyShotSet->pEnemyShotHead) {
                    if (pShot->param_i[2] == 0 && pShot->param_i[1] == rowToFire) {
                        // 槍（高速自機狙い）を生成
                        sEnemyShot* pSpear = new sEnemyShot;
                        pSpear->x = pShot->x;
                        pSpear->y = pShot->y;
                        pSpear->muki = atan2(player.y - pSpear->y, player.x - pSpear->x);
                        pSpear->speed = SPEAR_SPEED;
                        pSpear->kind = img_enemyShotBullet[6]; // 白色の銃弾（細長い）

                        pSpear->param_i[0] = -1;
                        pSpear->param_i[1] = -1;
                        pSpear->param_i[2] = 1; // 独立弾

                        pSpear->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pSpear->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pSpear;
                        pEnemyShotSet->pEnemyShotHead->prev = pSpear;
                    }
                    pShot = pShot->next;
                }

                pEnemyShotSet->param_i[1]++;

                // 全行発射後は散開フェーズへ
                if (pEnemyShotSet->param_i[1] >= ROWS) {
                    pEnemyShotSet->param_i[0] = 2;
                }
            }
        }
    }
    else if (phase == 2) {
        // 散開（一度だけ実行）
        if (pEnemyShotSet->param_i[1] == ROWS) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[2] == 0) {
                    // 隊列中心から外側へ飛ばす
                    double angle = atan2(pShot->y - pEnemyShotSet->y,
                        pShot->x - pEnemyShotSet->x);
                    angle += (GetRand(60) - 30) / 180.0 * DX_PI; // ランダム幅
                    pShot->muki = angle;
                    pShot->speed = 1.5 + GetRand(50) / 100.0; // 1.5～2.0
                    pShot->param_i[2] = 1; // 独立弾へ変更
                }
                pShot = pShot->next;
            }
            pEnemyShotSet->param_i[1] = -1; // 再実行防止
        }
    }

    // 全弾の座標更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[2] == 0) {
            // 盾弾：隊列中心＋オフセットに固定
            pShot->x = pEnemyShotSet->x + pShot->param_d[0];
            pShot->y = pEnemyShotSet->y + pShot->param_d[1];
        }
        else {
            // 槍・散開後の盾弾：速度と向きで移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Phalanx_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    if (count % 150 == 1) {
        // ファランクス弾幕セットを1つだけ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = PhalanxMove;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 隊列中心の初期位置
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッダを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルな弾幕セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 敵本体自体は動かさない（弾幕のみで攻撃）
}