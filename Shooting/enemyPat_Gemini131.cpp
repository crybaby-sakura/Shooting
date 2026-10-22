#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：配列符「バブルスワップ・グラデーション」
static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 13; // 1配列あたりの弾の数

    if (pEnemyShotSet->count == 0) {
        // --- Set側のパラメータ初期化 ---
        pEnemyShotSet->param_i[0] = 0; // バブルソート外側ループカウンタ (i)
        pEnemyShotSet->param_i[1] = 0; // バブルソート内側ループカウンタ (j)
        pEnemyShotSet->param_i[2] = 0; // 状態遷移タイマー（0:比較, >0:アニメ待機）
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 配列の基準X座標
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 配列の基準Y座標

        // 配列の生成
        for (int k = 0; k < N; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->param_i[0] = k;           // 現在の論理インデックス(0～N-1)
            pEnemyShot->param_i[1] = GetRand(2);  // サイズのソートキー (0:小, 1:中, 2:大)
            pEnemyShot->param_i[2] = 0;           // スワップアニメーション残りフレーム数

            // ソートキーに応じた画像と判定を設定
            if (pEnemyShot->param_i[1] == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[3];  // 3:シアン(小)
            }
            else if (pEnemyShot->param_i[1] == 1) {
                pEnemyShot->kind = img_enemyShotMediumBall[1]; // 1:黄(中)
            }
            else {
                pEnemyShot->kind = img_enemyShotLargeBall[5];  // 5:マゼンタ(大)
            }

            // 初期位置の設定（等間隔に配置）
            pEnemyShot->x = pEnemyShotSet->x + (k - (N - 1) / 2.0) * 35.0;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->margin = 120;

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 配列全体を徐々に画面下部へ降下させる
    pEnemyShotSet->param_d[1] += 1.2;

    // --- バブルソートのアルゴリズム制御 ---
    int i = pEnemyShotSet->param_i[0];
    int j = pEnemyShotSet->param_i[1];
    int state = pEnemyShotSet->param_i[2];

    if (state == 0 && i < N - 1) {
        // リストの中から現在の比較対象（j と j+1）を探す
        sEnemyShot* shotJ = nullptr;
        sEnemyShot* shotJ1 = nullptr;
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;

        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == j) shotJ = p;
            if (p->param_i[0] == j + 1) shotJ1 = p;
            p = p->next;
        }

        if (shotJ && shotJ1) {
            // 左(j) が 右(j+1) よりも大きい場合、スワップ発生
            if (shotJ->param_i[1] > shotJ1->param_i[1]) {
                // 論理インデックスを入れ替え
                shotJ->param_i[0] = j + 1;
                shotJ1->param_i[0] = j;

                int animFrames = 8; // スワップに要するフレーム数

                // 左の弾（右へ移動するアニメーション設定）
                shotJ->param_i[2] = animFrames;
                shotJ->param_i[3] = 1; // 円弧の膨らむ方向
                shotJ->param_d[0] = pEnemyShotSet->param_d[0] + (j - (N - 1) / 2.0) * 35.0;     // 移動元X
                shotJ->param_d[1] = pEnemyShotSet->param_d[0] + (j + 1 - (N - 1) / 2.0) * 35.0; // 移動先X

                // 右の弾（左へ移動するアニメーション設定）
                shotJ1->param_i[2] = animFrames;
                shotJ1->param_i[3] = -1; // 円弧の膨らむ方向
                shotJ1->param_d[0] = pEnemyShotSet->param_d[0] + (j + 1 - (N - 1) / 2.0) * 35.0; // 移動元X
                shotJ1->param_d[1] = pEnemyShotSet->param_d[0] + (j - (N - 1) / 2.0) * 35.0;     // 移動先X

                pEnemyShotSet->param_i[2] = animFrames; // Set全体もアニメーション完了まで待機

                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            else {
                // スワップ不要な場合でも、視覚的な間（テンポ）を作るため2フレーム待機
                pEnemyShotSet->param_i[2] = 2;
            }
        }
    }
    else if (state > 0) {
        // 待機（アニメーション実行）中
        pEnemyShotSet->param_i[2]--;
        if (pEnemyShotSet->param_i[2] == 0) {
            // アニメが完了したら、次のインデックスの比較へ
            j++;
            if (j >= N - 1 - i) {
                j = 0;
                i++;
            }
            pEnemyShotSet->param_i[0] = i;
            pEnemyShotSet->param_i[1] = j;
            pEnemyShotSet->param_i[2] = 0; // 再び比較フェーズへ
        }
    }

    // --- 弾の物理座標の更新 ---
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        double targetY = pEnemyShotSet->param_d[1];

        if (p->param_i[2] > 0) {
            // スワップアニメーション中の座標計算
            int animFrames = 8;
            int f = animFrames - p->param_i[2] + 1; // 1～8
            double rate = (double)f / animFrames;   // 0.125 ～ 1.0

            // X座標は線形補間、Y座標はサイン波で円弧（交差）を描く
            p->x = p->param_d[0] * (1.0 - rate) + p->param_d[1] * rate;
            p->y = targetY + sin(rate * DX_PI) * 15.0 * p->param_i[3];

            p->param_i[2]--;
        }
        else {
            // アニメーションしていない弾は、自身の論理インデックスが示すX座標へ追従
            p->x = pEnemyShotSet->param_d[0] + (p->param_i[0] - (N - 1) / 2.0) * 35.0;
            p->y = targetY;
        }

        p = p->next;
    }
}


// 敵本体のパターン
void EnemyPat_BubbleSort_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボスは上部をゆっくりと左右に移動
        enemy.x += 0.5 * (double)muki;
        if (enemy.x > 360.0) muki = -1;
        if (enemy.x < 120.0) muki = 1;
    }

    // 75フレーム毎に新しいバブルソート配列を生成して放つ
    if (count % 75 == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleSort;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 以下のパラメータは今回の弾幕では直接移動計算に使わないが初期化
        pEnemyShotSet->muki = 0;
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