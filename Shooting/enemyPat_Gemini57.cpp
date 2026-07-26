// enemyPat_CellularCascade.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：カオスと秩序のセル・カスケード（ルール30）
static void ShotRule30(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0] に現在の世代のビット配列（31列分）を保持
    int state = pEnemyShotSet->param_i[0];

    // 定期的にバタフライ効果（ビット反転）ギミックを発動
    // 180フレームごとに、敵が上部に向けて予告レーザーを撃つ
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 90 == 0) {
        // カオスを誘発するため、中央付近（5～25列目）をランダムに狙う
        // GetRand(20)は0～20を返すため、5 + 0～20 = 5～25 となる
        int targetCol = 5 + GetRand(20);
        pEnemyShotSet->param_i[2] = targetCol;
        pEnemyShotSet->param_i[3] = 30; // 30フレーム後に画面上部に着弾して反転発動

        // 予告効果音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 上へ向かう予告レーザーの発射
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = targetCol * 16.0; // 狙った列の真下（敵のX座標付近）から発射
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = -DX_PI / 2.0;  // 真上
        pEnemyShot->speed = pEnemyShotSet->y / 30.0; // 30フレームでY=0に到達する速度
        pEnemyShot->kind = img_enemyShotLaser[5];    // マゼンタ色の短レーザー
        pEnemyShot->margin = 50.0;

        // リストへの追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // レーザー着弾によるビット反転処理
    if (pEnemyShotSet->param_i[2] != -1) {
        pEnemyShotSet->param_i[3]--;
        if (pEnemyShotSet->param_i[3] <= 0) {
            int targetCol = pEnemyShotSet->param_i[2];
            state ^= (1 << targetCol);         // 指定列のビットを反転(0↔1)
            pEnemyShotSet->param_i[0] = state; // 即座に状態へ反映
            pEnemyShotSet->param_i[2] = -1;    // 予約解除

            // 反転時の効果音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }

    // 10フレームごとに1行（1世代）生成
    if (pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 現在の世代の弾を配置
        for (int i = 0; i <= 30; i++) {
            if ((state >> i) & 1) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = i * 16.0;         // 16ピクセル間隔で配置
                pEnemyShot->y = 0.0;              // 画面最上部から降ってくる
                pEnemyShot->muki = DX_PI / 2.0;   // 真下
                pEnemyShot->speed = 1.6;          // 10フレームで16px進む速度（縦横のグリッドが綺麗に揃う）

                // 弾の視認性を高めるための色分け（左右のセルの状態を確認）
                int left = (i == 0) ? 0 : ((state >> (i - 1)) & 1);
                int right = (i == 30) ? 0 : ((state >> (i + 1)) & 1);

                if (left == 1 || right == 1) {
                    // 連続している壁は「赤玉」
                    pEnemyShot->kind = img_enemyShotDiamond[0];
                }
                else {
                    // 単体で孤立している弾は「青玉」
                    pEnemyShot->kind = img_enemyShotMediumBall[4];
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 次の世代の状態（ルール30）を計算
        int next_state = 0;
        for (int i = 0; i <= 30; i++) {
            // 左、自分、右のビットを取得（画面外は0扱い）
            int left = (i == 0) ? 0 : ((state >> (i - 1)) & 1);
            int self = ((state >> i) & 1);
            int right = (i == 30) ? 0 : ((state >> (i + 1)) & 1);

            // 3つの状態を3ビットの整数(0～7)にまとめる
            int pattern = (left << 2) | (self << 1) | right;

            // ルール30 (0x1E = 00011110) のビットマスクから次の状態を判定
            if ((0x1E >> pattern) & 1) {
                next_state |= (1 << i);
            }
        }
        // 次世代の状態を保存
        pEnemyShotSet->param_i[0] = next_state;
    }

    // 生成された弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Rule30_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480 を想定
        enemy.x = 240.0;
        enemy.y = 180.0; // 上部から降る弾幕を見せるため、やや下に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 弾幕セットの初期化と登録（一度だけ実行）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRule30;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // param_i[0]: セルの初期状態 (中央である15番目の列のみ1にする)
        pEnemyShotSet->param_i[0] = (1 << 15);
        // param_i[2]: ビット反転を予約する列（-1で予約なし）
        pEnemyShotSet->param_i[2] = -1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵はゆっくりと左右に揺れる
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }
}