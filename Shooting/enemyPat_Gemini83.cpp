// enemyPat_prismSpectrum.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：プリズム・スペクトルアーチ
static void ShotPrismSpectrumArch(sEnemyShotSet* pEnemyShotSet)
{
    // 0フレーム目：予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 30フレーム目：虹のアーチ発射
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 虹の7色（外側から：赤、橙、黄、緑、シアン、青、マゼンタ）
        int colors[7] = { 0, 8, 1, 2, 3, 4, 5 };

        for (int i = 0; i < 7; i++) {
            // 外側の色ほど初速を速くして、減速時に綺麗な同心円になるようにする
            double initial_speed = 10.0 - i * 1.2 + 10;

            // 扇状に各色40発ずつ配置
            for (int j = 0; j <= 40; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 真下(PI/2)を中心に、広角に扇状発射
                pEnemyShot->muki = DX_PI / 2.0 - 1.2 + (2.4 / 40.0) * j;
                pEnemyShot->speed = initial_speed;
                pEnemyShot->kind = img_enemyShotSmallBall[colors[i]];

                // 状態管理用のパラメータ設定
                pEnemyShot->param_i[0] = 0; // State: 0=広がる, 1=停止, 2=固有運動, 3=派生弾
                pEnemyShot->param_i[1] = colors[i]; // 自分の色を記憶

                // 弾リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の更新ループ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ループ内で派生弾が追加されても安全に回せるよう、先に次のポインタを保持
        sEnemyShot* pNext = pShot->next;

        int state = pShot->param_i[0];
        int color = pShot->param_i[1];

        if (state == 0) {
            // 【State 0: 減速しながら広がる】
            pShot->speed *= 0.92; // 徐々に減速
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 90フレーム目で完全に停止（虹の架橋完成）
            if (pEnemyShotSet->count == 90) {
                pShot->param_i[0] = 1;
                pShot->speed = 0.0;
            }
        }
        else if (state == 1) {
            // 【State 1: 停止（タメ）】
            // 150フレーム目（約1秒後）に固有運動へ移行
            if (pEnemyShotSet->count == 150) {
                pShot->param_i[0] = 2;
                pShot->count = 0; // 各弾のカウンタをリセットして同期させる

                // 動き出しのSE（先頭の弾が処理された時だけ1回鳴らす）
                if (pShot == pEnemyShotSet->pEnemyShotHead->next) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                // --- 色ごとの固有運動の初期化 ---
                if (color == 0 || color == 8) {
                    // 赤・橙：自機狙いへ角度を変更
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 4.0 + (GetRand(100) / 100.0); // わずかに速度をばらつかせる
                }
                else if (color == 1 || color == 2) {
                    // 黄・緑：S字降下用のベース座標を保存
                    pShot->param_d[0] = pShot->x; // ベースX
                    pShot->param_d[1] = pShot->y; // ベースY
                    pShot->param_d[2] = DX_PI / 2.0; // 基本進行方向（真下）
                    pShot->speed = 2.0;
                }
                else if (color == 3 || color == 4 || color == 5) {
                    // 水・青・紫：停滞して派生弾を撃つ準備
                    pShot->speed = 0.4;
                    // 派生弾の発射タイミングが全弾同時にならないよう、ランダムなオフセット値を仕込む
                    pShot->param_i[2] = GetRand(90);
                }
            }
        }
        else if (state == 2) {
            // 【State 2: 色別の固有運動】
            if (color == 0 || color == 8) {
                // 赤・橙：一直線に高速移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (color == 1 || color == 2) {
                // 黄・緑：S字を描きながら降下
                // ベース座標を進行方向へ進める
                pShot->param_d[0] += pShot->speed * cos(pShot->param_d[2]);
                pShot->param_d[1] += pShot->speed * sin(pShot->param_d[2]);

                // サイン波で横揺れ幅を計算
                double wave = sin(pShot->count * 0.05) * 40.0;

                // 進行方向に対して直角(PI/2)に wave 分だけ座標をずらす
                pShot->x = pShot->param_d[0] + wave * cos(pShot->param_d[2] + DX_PI / 2.0);
                pShot->y = pShot->param_d[1] + wave * sin(pShot->param_d[2] + DX_PI / 2.0);
            }
            else if (color == 3 || color == 4 || color == 5) {
                // 水・青・紫：ゆっくり旋回しながら停滞
                pShot->muki += 0.03;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 一定間隔で小さな派生弾（菱形弾）をばら撒く
                if (pShot->count > 0 && (pShot->count + pShot->param_i[2]) % 90 == 0) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pShot->x;
                    pNewShot->y = pShot->y;
                    // 回転している向きをベースに、ランダムに拡散
                    pNewShot->muki = pShot->muki + (GetRand(60) - 30) * DX_PI / 180.0;
                    pNewShot->speed = 1.5;
                    pNewShot->kind = img_enemyShotDiamond[color]; // 菱形弾
                    pNewShot->param_i[0] = 3; // State: 派生弾

                    // 弾リストへ追加
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
            }
            if (pShot->count >= 360) pShot->margin = -9999;
        }
        else if (state == 3) {
            // 【State 3: 派生弾（水・青・紫から放出された弾）】
            // 単純に直進して画面外へ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}


// 敵本体のパターン
void EnemyPat_Rainbow_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;  // 虹の広がりを魅せるため少し下げる
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボス本体はゆっくりと左右に揺れる
        enemy.x += 0.4 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 周期的に弾幕を展開 (ここでは360フレーム = 約6秒周期)
    if (count % 360 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPrismSpectrumArch;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // ヘッダノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}