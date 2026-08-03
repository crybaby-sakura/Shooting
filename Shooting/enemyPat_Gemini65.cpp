// enemyPat_hydra.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：不滅の九頭連鎖（ハイドラ・リジェネシス）
static void ShotHydraRegenesis(sEnemyShotSet* pEnemyShotSet)
{
    const double HIT_DIST = 25.0; // 自機ショットとの当たり判定距離
    const int MAX_GEN = 4;        // 最大分裂世代（0から始まり3まで）
    const int HEAD = 0;           // 弾の役割：首の先端（頭）
    const int BODY = 1;           // 弾の役割：残留する毒（胴体）
    const int BURST = 2;          // 弾の役割：破裂した小弾

    // 最初のフレームで3本の「頭」を生成
    if (pEnemyShotSet->count == 0) {
        // 予告音・発射音を鳴らす
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pHead = new sEnemyShot;
            pHead->x = pEnemyShotSet->x;
            pHead->y = pEnemyShotSet->y;
            // 真下を中心に3方向に扇状に発射（-0.5, 0, 0.5 rad）
            pHead->muki = pEnemyShotSet->muki + (i - 1) * 0.5;
            pHead->speed = 1.2;
            pHead->kind = img_enemyShotLargeBall[5]; // マゼンタ色（猛毒イメージ）の大玉
            pHead->param_i[0] = HEAD;
            pHead->param_i[1] = 0; // 世代0
            pHead->param_i[2] = 0;
            pHead->margin = 480;

            pHead->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pHead;
            pEnemyShotSet->pEnemyShotHead->prev = pHead;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == HEAD) {
            // --- 首の先端（頭）の処理 ---
            bool splitted = false;

            // 1. 緩やかなホーミング
            double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = targetAngle - pShot->muki;
            while (diff > DX_PI) diff -= DX_PI * 2;
            while (diff < -DX_PI) diff += DX_PI * 2;
            pShot->muki += diff * 0.008; // 緩やかに自機へ曲がる

            // 2. 移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 3. 軌跡として残留弾（胴体）を生成
            // 少しずつ間隔をあけて置く
            if (pShot->count % 6 == 0) {
                sEnemyShot* pBody = new sEnemyShot;
                pBody->x = pShot->x;
                pBody->y = pShot->y;
                pBody->muki = 0;
                pBody->speed = 0;
                pBody->kind = img_enemyShotMediumBall[5]; // マゼンタ中玉
                pBody->param_i[0] = BODY;
                pBody->param_i[2] = 240 * 2; // 寿命（4秒程度で消滅）

                pBody->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pBody->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pBody;
                pEnemyShotSet->pEnemyShotHead->prev = pBody;
            }

            // 4. 自機ショットとの当たり判定（被弾・分裂ギミック）
            sPlayerShot* pPShot = playerShotHead.next;
            while (pPShot != &playerShotHead) {
                double dx = pPShot->x - pShot->x;
                double dy = pPShot->y - pShot->y;

                if (dx * dx + dy * dy < HIT_DIST * HIT_DIST) {
                    // 自機ショットが命中！ -> 自機ショットを消去して無効化
                    pPShot->prev->next = pPShot->next;
                    pPShot->next->prev = pPShot->prev;
                    sPlayerShot* tempP = pPShot;
                    pPShot = pPShot->next; // ループ続行のため次へ
                    delete tempP;

                    // 頭の分裂処理
                    if (pShot->param_i[1] < MAX_GEN) {
                        // 分裂可能なら2本に増える
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                        for (int i = 0; i < 2; i++) {
                            sEnemyShot* pNewHead = new sEnemyShot;
                            pNewHead->x = pShot->x;
                            pNewHead->y = pShot->y;
                            // 進行方向から左右に分岐して広がる
                            pNewHead->muki = pShot->muki + (i == 0 ? 0.6 : -0.6);
                            pNewHead->speed = pShot->speed * 1.15; // 少し加速してプレイヤーを追い詰める
                            pNewHead->kind = img_enemyShotLargeBall[5];
                            pNewHead->param_i[0] = HEAD;
                            pNewHead->param_i[1] = pShot->param_i[1] + 1; // 世代を進める
                            pNewHead->param_i[2] = 0;
                            pNewHead->margin = 480;

                            pNewHead->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pNewHead->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pNewHead;
                            pEnemyShotSet->pEnemyShotHead->prev = pNewHead;
                        }
                    }
                    else {
                        // 最大世代到達時は破裂して小弾をばら撒く
                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                        for (int i = 0; i < 8; i++) {
                            sEnemyShot* pBurst = new sEnemyShot;
                            pBurst->x = pShot->x;
                            pBurst->y = pShot->y;
                            pBurst->muki = i * DX_PI / 4.0;
                            pBurst->speed = 2.5;
                            pBurst->kind = img_enemyShotSmallBall[5]; // マゼンタ小玉
                            pBurst->param_i[0] = BURST;

                            pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pBurst->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
                            pEnemyShotSet->pEnemyShotHead->prev = pBurst;
                        }
                    }
                    splitted = true;
                    break; // 1回被弾したら自身は消滅するので判定を抜ける
                }
                else {
                    pPShot = pPShot->next;
                }
            }

            // 分裂した、または一定時間経過（寿命）で自身を消去
            if (splitted || pShot->count > 450) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                sEnemyShot* temp = pShot;
                pShot = pShot->next;
                delete temp;
                continue;
            }

            pShot = pShot->next;
        }
        else if (pShot->param_i[0] == BODY) {
            // --- 残留毒（胴体）の処理 ---
            // 動かずに寿命を減らす
            pShot->param_i[2]--;
            if (pShot->param_i[2] <= 0) {
                // 寿命で消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                sEnemyShot* temp = pShot;
                pShot = pShot->next;
                delete temp;
                continue;
            }
            pShot = pShot->next;
        }
        else if (pShot->param_i[0] == BURST) {
            // --- 破裂した小弾の処理 ---
            // ただ直進するだけ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
        else {
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Hydra_Gemini() // 課題の指定通り
{
    if (count == 1) {
        // ゲーム画面は 480x480 を想定
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面上部に待機
        enemy.maxHp = enemy.hp = 200; // ボスなのでHP多め
    }
    else {
        // 左右にゆっくり揺れる
        enemy.x = 240.0 + 80.0 * sin(count * DX_PI / 180.0);
    }

    // 定期的にハイドラ弾幕を放つ（例: 6秒ごと）
    if (count % 360 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHydraRegenesis;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        // 真下を基準方向とする
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