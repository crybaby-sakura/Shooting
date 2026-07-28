// enemyPat_Tmp.cpp
// アリジゴクをモチーフにした弾幕「砂坑の顎」
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 砂坑の顎：メインパターン関数
// pEnemyShotSet->count でフェーズ管理
// param_d[0] : 坑の中心X
// param_d[1] : 坑の底Y
// param_d[2] : 坑の開き幅（片側）
// ------------------------------------------------------------
static void ShotAntlionPit(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double pitCenterX = pEnemyShotSet->param_d[0];
    const double pitBottomY = pEnemyShotSet->param_d[1];
    const double pitHalfWidth = pEnemyShotSet->param_d[2];

    // ========== フェーズ1: 巣穴形成（count 0〜90） ==========
    // 砂粒で円錐状の坑壁を徐々に形成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count <= 90 && pEnemyShotSet->count % 3 == 0) {
        // 左右の壁に沿って砂弾を配置（固定位置寄り）
        double progress = pEnemyShotSet->count / 90.0; // 0→1
        double currentHalfW = pitHalfWidth * (0.3 + 0.7 * progress);
        double currentHeight = (pitBottomY - 80.0) * progress;

        for (int side = -1; side <= 1; side += 2) {
            // 壁の上から下へ数点
            for (int i = 0; i < 5; i++) {
                double t = i / 4.0;
                double wx = pitCenterX + side * currentHalfW * (1.0 - t * 0.85);
                double wy = 80.0 + currentHeight * t;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = wx + (GetRand(6) - 3);
                pEnemyShot->y = wy + (GetRand(6) - 3);
                pEnemyShot->muki = (side > 0) ? 0.0 : DX_PI; // とりあえず
                pEnemyShot->speed = 0.0; // ほぼ固定（後で微動）
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄の小玉 = 砂
                pEnemyShot->param_i[0] = 1; // 壁弾フラグ
                pEnemyShot->param_d[0] = wx; // 目標X
                pEnemyShot->param_d[1] = wy; // 目標Y
                pEnemyShot->param_d[2] = side; // 左右

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        if (pEnemyShotSet->count % 9 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // ========== フェーズ2: 砂飛ばし誘導（count 90〜210） ==========
    if (pEnemyShotSet->count >= 90 && pEnemyShotSet->count <= 210) {
        if (pEnemyShotSet->count == 90) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 一定間隔で坑縁から内側・下向きの砂波を発射
        if ((pEnemyShotSet->count - 90) % 12 == 0) {
            for (int side = -1; side <= 1; side += 2) {
                for (int k = 0; k < 4; k++) {
                    pEnemyShot = new sEnemyShot;
                    double edgeX = pitCenterX + side * (pitHalfWidth * 0.95);
                    double edgeY = 100.0 + GetRand(30);

                    pEnemyShot->x = edgeX;
                    pEnemyShot->y = edgeY;
                    // 内側やや下向き
                    double aimX = pitCenterX + side * (GetRand(40) - 20);
                    double aimY = pitBottomY - 40.0 + GetRand(60);
                    pEnemyShot->muki = atan2(aimY - edgeY, aimX - edgeX);
                    pEnemyShot->speed = 1.8 + GetRand(12) / 10.0;
                    pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄砂
                    pEnemyShot->param_i[0] = 2; // 砂波フラグ

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 底から細い砂煙（上向きの牽制）
        if ((pEnemyShotSet->count - 90) % 18 == 0) {
            for (int i = 0; i < 3; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pitCenterX + (GetRand(50) - 25);
                pEnemyShot->y = pitBottomY - 10.0;
                pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.2 + GetRand(8) / 10.0;
                pEnemyShot->kind = img_enemyShotScale[1]; // 鱗弾で砂煙感
                pEnemyShot->param_i[0] = 3;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ========== フェーズ3: 顎の挟撃（count 210〜300） ==========
    if (pEnemyShotSet->count == 210) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count >= 210 && pEnemyShotSet->count <= 300) {
        double jawProgress = (pEnemyShotSet->count - 210) / 90.0; // 0→1
        // 加速閉じ
        double closeT = jawProgress * jawProgress; // ease-in

        // 左右の顎をレーザー帯で表現（閉じる）
        if (pEnemyShotSet->count % 2 == 0) {
            for (int side = -1; side <= 1; side += 2) {
                // 顎の根本（底の少し外側）から先端へ
                double baseX = pitCenterX + side * (30.0 + 40.0 * (1.0 - closeT));
                double baseY = pitBottomY - 20.0;
                double tipX = pitCenterX + side * (8.0 + 10.0 * (1.0 - closeT));
                double tipY = pitBottomY - 180.0 - 40.0 * (1.0 - closeT);

                // 顎の線上に複数弾を配置（短レーザーで太く）
                for (int j = 0; j < 7; j++) {
                    double t = j / 6.0;
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = baseX + (tipX - baseX) * t;
                    pEnemyShot->y = baseY + (tipY - baseY) * t;
                    pEnemyShot->muki = atan2(tipY - baseY, tipX - baseX);
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->kind = img_enemyShotLaser[8]; // 橙の短レーザー
                    pEnemyShot->param_i[0] = 4; // 顎弾フラグ
                    pEnemyShot->param_d[0] = closeT;
                    pEnemyShot->count = 0; // 寿命管理用にリセット気味

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 閉じきる直前〜直後に爆散
        if (pEnemyShotSet->count >= 280 && pEnemyShotSet->count <= 295 && pEnemyShotSet->count % 3 == 0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 32; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pitCenterX + (GetRand(30) - 15);
                pEnemyShot->y = pitBottomY - 60.0 + (GetRand(40) - 20);
                pEnemyShot->muki = (i / 16.0) * 2.0 * DX_PI + (GetRand(20) - 10) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.5 + GetRand(20) / 10.0;
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤中玉
                pEnemyShot->param_i[0] = 5;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ========== 全弾の移動・寿命処理 ==========
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除の可能性を考慮

        int type = pShot->param_i[0];

        if (type == 1) {
            // 壁弾：目標位置に軽く吸着＋微振動
            pShot->x += (pShot->param_d[0] - pShot->x) * 0.08;
            pShot->y += (pShot->param_d[1] - pShot->y) * 0.08;
            pShot->x += (GetRand(3) - 1) * 0.15;
            pShot->y += (GetRand(3) - 1) * 0.15;
            // 長く残す
            if (pEnemyShotSet->count > 320) {
                // フェードアウト的に外側へ
                pShot->x += pShot->param_d[2] * 1.5;
                pShot->y -= 0.8;
            }
        }
        else if (type == 2 || type == 3 || type == 5) {
            // 通常移動弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 4) {
            // 顎弾：そのフレームの位置に固定（毎フレーム再生成しているので短命）
            // 少し内側に引き寄せる演出
            double cx = pitCenterX;
            pShot->x += (cx - pShot->x) * 0.02 * pShot->param_d[0];
            // 寿命を短くするため、countが進んだら消す方向
            if (pShot->count > 8) {
                // 画面外扱いになるよう大きく動かす（メイン側で消される）
                pShot->y = -1000.0;
            }
        }

        pShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Antlion_Grok()
{
    static int phaseStartCount = 0;
    static int wave = 0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        phaseStartCount = 0;
        wave = 0;
    }

    // 敵はゆっくり左右に揺れる（アリジゴクが潜む感じ）
    enemy.x = 240.0 + 40.0 * sin(count / 90.0);
    enemy.y = 50.0 + 8.0 * sin(count / 60.0);

    // パターン発動間隔（最初は早め、以降は周期的）
    int interval = (wave == 0) ? 30 : 360;
    if (count >= phaseStartCount + interval) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAntlionPit;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = wave;

        // 坑のパラメータ設定
        pEnemyShotSet->param_d[0] = 240.0;               // 坑中心X
        pEnemyShotSet->param_d[1] = 380.0;               // 坑底Y
        pEnemyShotSet->param_d[2] = 160.0 + GetRand(40); // 片側開き幅

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        phaseStartCount = count;
        wave++;
    }
}