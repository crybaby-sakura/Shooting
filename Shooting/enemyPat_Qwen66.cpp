// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// プロトタイプ宣言
static void ShotGiantMoonfall(sEnemyShotSet* pSet);

// 敵本体のパターン
void EnemyPat_HugeBullet_Qwen()
{
    static int state = 0;         // 0:落下, 1:攻撃, 2:上昇退去
    static int timer = 0;
    static sEnemyShotSet* pGiantSet = nullptr;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -150.0;
        enemy.maxHp = enemy.hp = 200; // 200に固定
        state = 0;
        timer = 0;
        pGiantSet = nullptr;
    }

    // 状態遷移と移動
    if (state == 0) {
        if (timer == 0) {
            // 超巨大弾セットの生成
            pGiantSet = new sEnemyShotSet;
            pGiantSet->count = 0;
            pGiantSet->patternFunc = ShotGiantMoonfall;
            pGiantSet->x = enemy.x;
            pGiantSet->y = enemy.y;
            pGiantSet->param_d[0] = 0.0; // 回転オフセット
            pGiantSet->param_i[0] = 0;   // 状態: 0=落下中

            // 双方向リンクリスト初期化
            pGiantSet->pEnemyShotHead = new sEnemyShot;
            pGiantSet->pEnemyShotHead->prev = pGiantSet->pEnemyShotHead;
            pGiantSet->pEnemyShotHead->next = pGiantSet->pEnemyShotHead;

            // グローバルリストへ接続
            pGiantSet->prev = enemyShotSetHead.prev;
            pGiantSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pGiantSet;
            enemyShotSetHead.prev = pGiantSet;

            // リング構成要素（黒大玉）の生成
            int numSegments = 48;
            double radius = 90.0;
            for (int i = 0; i < numSegments; i++) {
                // 隙間の作成 (インデックス 10-13, 34-37 をスキップ)
                if ((i >= 10 && i <= 13) || (i >= 34 && i <= 37)) continue;

                sEnemyShot* pShot = new sEnemyShot;
                pShot->param_i[0] = 1;  // タイプID: 1=リング構成要素
                pShot->param_d[0] = i * (DX_PI * 2.0 / numSegments);
                pShot->param_d[1] = radius;
                pShot->margin = 500.0;  // 画面外判定を無効化するため大きめに設定

                // 素材選択: 黒(7)の大玉を使用
                pShot->kind = img_enemyShotLargeBall[7];

                // リストへ追加
                pShot->prev = pGiantSet->pEnemyShotHead->prev;
                pShot->next = pGiantSet->pEnemyShotHead;
                pGiantSet->pEnemyShotHead->prev->next = pShot;
                pGiantSet->pEnemyShotHead->prev = pShot;
            }
        }

        // 落下移動 (180フレームかけて y=100 へ)
        double startY = -150.0;
        double targetY = 240.0;
        int duration = 180;

        if (timer < duration) {
            enemy.y = startY + (targetY - startY) * ((double)timer / duration);
        }
        else {
            enemy.y = targetY;
            state = 1;
            timer = 0;
            if (pGiantSet) pGiantSet->param_i[0] = 1; // 状態: 1=攻撃中
        }

        if (pGiantSet) {
            pGiantSet->y = enemy.y;
            pGiantSet->x = enemy.x;
        }
        timer++;
    }
    else if (state == 1) {
        // 攻撃フェーズ (600フレーム = 10秒)
        if (timer > 600) {
            state = 2;
            timer = 0;
            if (pGiantSet) pGiantSet->param_i[0] = 2; // 状態: 2=上昇中
        }
        timer++;
    }
    else if (state == 2) {
        // 上昇・退去フェーズ (180フレームかけて画面外へ)
        double startY = 240.0;
        double targetY = -150.0;
        int duration = 180;

        if (timer < duration) {
            enemy.y = startY + (targetY - startY) * ((double)timer / duration);
        }
        else {
            // ループ: 状態をリセットして最初から
            state = 0;
            timer = -1;

            // pGiantSet の安全な解放とリセット
            if (pGiantSet) {
                // グローバルリストから外す
                pGiantSet->prev->next = pGiantSet->next;
                pGiantSet->next->prev = pGiantSet->prev;

                // 保持している sEnemyShot を全て解放
                sEnemyShot* pShot = pGiantSet->pEnemyShotHead->next;
                while (pShot != pGiantSet->pEnemyShotHead) {
                    sEnemyShot* pNext = pShot->next;
                    delete pShot;
                    pShot = pNext;
                }
                delete pGiantSet->pEnemyShotHead;
                delete pGiantSet;
                pGiantSet = nullptr;
            }
        }

        if (pGiantSet) {
            pGiantSet->y = enemy.y;
            pGiantSet->x = enemy.x;
        }
        timer++;
    }
}

// 超巨大弾の挙動関数
static void ShotGiantMoonfall(sEnemyShotSet* pSet)
{
    // 1. 回転処理
    pSet->param_d[0] += 0.015;

    // 2. 構成要素の座標更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // リング構成要素: 親に合わせて強制移動
            double angle = pShot->param_d[0] + pSet->param_d[0];
            pShot->x = pSet->x + cos(angle) * pShot->param_d[1];
            pShot->y = pSet->y + sin(angle) * pShot->param_d[1];
        }
        else if (pShot->param_i[0] == 2) {
            // 発射された弾: 物理法則に従い移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 3. 弾幕の生成 (攻撃フェーズ中のみ)
    if (pSet->param_i[0] == 1) {
        // A. リングの隙間からの放射弾
        if (pSet->count % 3 == 0) {
            double unitAngle = DX_PI * 2.0 / 48.0;
            double gaps[2] = {
                (11.5 * unitAngle) + pSet->param_d[0],
                (35.5 * unitAngle) + pSet->param_d[0]
            };

            for (int i = 0; i < 2; i++) {
                sEnemyShot* pNewShot = new sEnemyShot;
                pNewShot->param_i[0] = 2; // タイプID: 2=通常弾
                pNewShot->x = pSet->x + cos(gaps[i]) * 90.0;
                pNewShot->y = pSet->y + sin(gaps[i]) * 90.0;
                pNewShot->muki = gaps[i];
                pNewShot->speed = 3.5;
                pNewShot->kind = img_enemyShotMediumBall[0];

                pNewShot->prev = pSet->pEnemyShotHead->prev;
                pNewShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pNewShot;
                pSet->pEnemyShotHead->prev = pNewShot;
            }
        }

        // B. 中心からの螺旋弾
        if (pSet->count % 4 == 0) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->param_i[0] = 2;
            pNewShot->x = pSet->x;
            pNewShot->y = pSet->y;
            pNewShot->muki = pSet->param_d[0] * 5.0;
            pNewShot->speed = 2.8;
            pNewShot->kind = img_enemyShotScale[3];

            pNewShot->prev = pSet->pEnemyShotHead->prev;
            pNewShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pNewShot;
            pSet->pEnemyShotHead->prev = pNewShot;
        }
    }
}