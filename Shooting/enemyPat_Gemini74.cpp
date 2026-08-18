// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：プルチーノ・アルデンテ（明太子スパゲッティの狂気）
static void ShotTarakoSpaghetti(sEnemyShotSet* pSet)
{
    // pSet->param_d[0] : 基準となる回転角度
    if (pSet->count == 0) {
        pSet->param_d[0] = pSet->muki;
    }

    // フェーズ1：フォーク・ワインディング（麺の展開）
    // 30〜180フレームの間、渦を巻くように麺を放出
    if (pSet->count >= 30 && pSet->count < 180) {
        if (pSet->count % 2 == 0) {
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;

                // 4方向に渦を巻くように角度を変化
                double baseAngle = pSet->param_d[0] + (DX_PI * 2.0 / 4.0) * i + (pSet->count * 0.02);
                pShot->muki = baseAngle;
                pShot->speed = 4.0;

                // 麺（中楕円弾・黄色）
                pShot->kind = img_enemyShotMediumOval[1];

                pShot->param_i[0] = 0; // 役割 0:麺
                pShot->param_d[0] = baseAngle; // 基本進行方向
                pShot->param_d[1] = pSet->count * 0.1; // サイン波の初期位相

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
            if (pSet->count % 8 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }

    // フェーズ3：アルデンテ・フィニッシュ（海苔の裁断とたらこの解放）
    // 240フレーム目で一斉発射のトリガー＆海苔落下開始
    if (pSet->count == 240 + 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 240〜270の間、海苔（刻み海苔）を降らせる
    if (pSet->count >= 240 + 60 && pSet->count <= 270 + 60) {
        if (pSet->count % 2 == 0) {
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = GetRand(480);
                pShot->y = -20 - GetRand(40);
                pShot->muki = DX_PI / 2.0; // 真下へ
                pShot->speed = 3.0 + GetRand(300) / 100.0;
                pShot->margin = 100;

                // 刻み海苔（短レーザー・黒）
                pShot->kind = img_enemyShotLaser[7];
                pShot->param_i[0] = 2; // 役割 2:海苔

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // --- 弾の更新ループ ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int role = pShot->param_i[0];

        if (role == 0) {
            // 【麺の動き】うねうねと波打ちながら進む
            pShot->param_d[1] += 0.15; // 波の位相を進める
            double wave = sin(pShot->param_d[1]) * 0.6; // 振幅
            pShot->muki = pShot->param_d[0] + wave;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 【プチプチ・バースト】麺の軌跡から確率でたらこ（マゼンタ小玉）を生成
            // プール枯渇を防ぐため生成確率と頻度を調整
            if (pSet->count < 200 && pShot->count % 8 == 0 && GetRand(100) < 10) {
                sEnemyShot* pTarako = new sEnemyShot;
                pTarako->x = pShot->x + GetRand(16) - 8;
                pTarako->y = pShot->y + GetRand(16) - 8;
                pTarako->muki = GetRand(359) * DX_PI / 180.0; // ランダムな方向へ
                pTarako->speed = GetRand(100) / 100.0; // 0.0 ~ 1.0 の低速でポロッと出る
                pTarako->margin = 10;

                // たらこ（小玉・マゼンタ）
                pTarako->kind = img_enemyShotSmallBall[5];
                pTarako->param_i[0] = 1; // 役割 1:たらこ
                pTarako->param_i[1] = 0; // 状態 0:停滞, 1:発射

                // リストへの追加 (末尾に追加)
                pTarako->prev = pSet->pEnemyShotHead->prev;
                pTarako->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pTarako;
                pSet->pEnemyShotHead->prev = pTarako;
            }
        }
        else if (role == 1) {
            // 【たらこの動き】
            if (pShot->param_i[1] == 0) {
                // 停滞・漂いフェーズ
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot->speed *= 0.92; // 徐々に減速して停滞

                // 240フレーム目で一斉に自機へ向かって発射
                if (pSet->count == 240 + 60) {
                    pShot->param_i[1] = 1; // 状態を「発射」へ移行
                    double aimAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                    aimAngle += (GetRand(30) - 15) / 180.0 * DX_PI; // 少し自機外し（ばらつき）
                    pShot->muki = aimAngle;
                    pShot->speed = 1.0 + GetRand(200) / 100.0; // 1.0 ~ 3.0 の速度で一斉襲撃
                }
            }
            else {
                // 発射後の直線移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (role == 2) {
            // 【海苔の動き】ひたすら真下へ降る
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TarakoSpaghetti_Gemini()
{
    // 敵の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 中央付近でゆっくり漂う（8の字軌道）
    enemy.x = 240.0 + sin(count * 0.02) * 60.0;
    enemy.y = 80.0 + cos(count * 0.015) * 20.0;

    // 400フレーム周期で弾幕を展開
    int period = 400 + 60 + 30;
    if (count % period == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTarakoSpaghetti;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 初期の狙いは自機方向
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}