// EnemyPat_Tmp.cpp
// 「布団が吹っ飛んだ」をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：布団が吹っ飛んだ
static void ShotFuton(sEnemyShotSet* pEnemyShotSet)
{
    int state = pEnemyShotSet->param_i[0];

    if (state == 0) {
        // ==========================================
        // 状態0: 布団の生成 (初回のみ実行)
        // ==========================================
        int cols = 8;
        int rows = 12;
        // 中心を pEnemyShotSet->x, y に合わせるための開始座標計算
        double startX = pEnemyShotSet->x - (cols * 10.0) / 2.0;
        double startY = pEnemyShotSet->y - (rows * 10.0) / 2.0;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = startX + c * 10.0;
                pShot->y = startY + r * 10.0;

                // 縁取りは外周のみシアン(3)、中は白(6)で綿を表現
                bool isEdge = (r == 0 || r == rows - 1 || c == 0 || c == cols - 1);
                if (isEdge) {
                    pShot->kind = img_enemyShotSmallBall[3]; // 3:シアン (布団カバー)
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[6]; // 6:白 (綿)
                }

                pShot->speed = 0.0; // 生成時は移動速度0（状態1でまとめて移動）
                pShot->muki = pEnemyShotSet->muki;
                pShot->margin = 240;

                // 連結リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 1; // 状態1へ遷移
    }
    else if (state == 1) {
        // ==========================================
        // 状態1: 布団が漂う (count < 90)
        // ==========================================
        if (pEnemyShotSet->count < 90) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                // 全体をゆっくり指定方向へ移動
                pShot->x += 1.5 * cos(pEnemyShotSet->muki);
                pShot->y += 1.5 * sin(pEnemyShotSet->muki);

                // 空中を漂うようなふわふわしたゆらぎを加える
                pShot->x += sin(pEnemyShotSet->count * 0.1 + pShot->y * 0.1) * 0.3;
                pShot->y += cos(pEnemyShotSet->count * 0.1 + pShot->x * 0.1) * 0.3;

                pShot = pShot->next;
            }
        }
        else {
            // ==========================================
            // 状態2: 吹っ飛び！ (count == 90 で移行)
            // ==========================================
            pEnemyShotSet->param_i[0] = 2;

            // 突風のSE
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                double dx = pShot->x - pEnemyShotSet->x;

                // 風で巻き上げられるように、基本は真上(-PI/2)へ飛ばす
                double base_muki = DX_PI / 2.0;
                // GetRand(240)は0〜240を返すので、-120して -120〜120 の範囲にする
                // これを180.0で割ることで、約 -66度 〜 +66度 の拡散角度を作る
                double spread = (GetRand(240) - 120) / 180.0 * DX_PI;
                pShot->muki = base_muki + spread;

                bool isEdge = (pShot->kind == img_enemyShotSmallBall[3]);
                if (isEdge) {
                    // 縁取り(カバー)は軽く、外側に広がりやすい
                    // GetRand(200)は0〜200を返すので、/100.0 で 0.0〜2.0
                    pShot->speed = 3.0 + GetRand(200) / 100.0;
                    // 左右の外側へさらに押し広げる補正
                    pShot->muki += (dx > 0 ? 1.0 : -1.0) * 0.3;
                }
                else {
                    // 中身(綿)は少し重めで速度を抑えめにする
                    pShot->speed = 2.0 + GetRand(150) / 100.0;
                }

                pShot = pShot->next;
            }
        }
    }
    else if (state == 2) {
        // ==========================================
        // 状態2: 散乱中 (ヒラヒラと舞い散る)
        // ==========================================
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 進行方向の移動に加えて、正弦波でヒラヒラさせる
            // pShot->y * 0.05 で位置ごとに位相をずらし、渦を巻くような美しさを出す
            pShot->x += pShot->speed * cos(pShot->muki) + sin(pEnemyShotSet->count * 0.2 + pShot->y * 0.05) * 1.5;
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_FutonFlewAway_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // 少し下から開始
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵本体は左右にゆっくり揺れ動きながら、少し上下もする
        enemy.x = 240.0 + sin(count * 0.02) * 150.0;
        enemy.y = 80.0 + sin(count * 0.03) * 20.0;
    }

    // 布団は1回だけ発射（例：count == 60 のタイミング）
    if (count % 120 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFuton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向を向きつつ、少し上方向に投げるように角度を補正
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) - 0.5;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0; // 状態0: 生成開始

        // 弾リストのヘッダ初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}