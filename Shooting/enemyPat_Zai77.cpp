// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：大車輪（アラウンド・ザ・ワールド）
static void ShotAroundTheWorld(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // ヨーヨー発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // ヨーヨー本体：橙の中玉
        pEnemyShot->kind = img_enemyShotMediumBall[8];
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->margin = 240;

        // param_i[0] : 状態 (0:往路, 1:停止, 2:復路, 3:消滅待ち)
        pEnemyShot->param_i[0] = 0;
        pEnemyShotSet->param_i[0] = 0;
        // param_i[1] : タイマー・糸生成用カウンタ
        pEnemyShot->param_i[1] = 0;

        // param_d[0], [1] : 発射元の座標（巻き取りの目標点）
        pEnemyShot->param_d[0] = pEnemyShotSet->x;
        pEnemyShot->param_d[1] = pEnemyShotSet->y;
        // param_d[2] : 発射時のベース角度
        pEnemyShot->param_d[2] = pEnemyShotSet->muki;
        // param_d[3] : 累積角度変化（弧を描くために使用）
        pEnemyShot->param_d[3] = 0.0;
        // param_d[4] : 進んだ距離
        pEnemyShot->param_d[4] = 0.0;
        // param_d[5] : 糸の最大長さ
        pEnemyShot->param_d[5] = 350.0;
        // param_d[6] : 往路の速度
        pEnemyShot->param_d[6] = 2.0;
        // param_d[7] : 復路の速度（往路の3倍の猛スピード）
        pEnemyShot->param_d[7] = 6.0;

        // リストの末尾に追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {
            // --- 状態0：往路（ゆっくり弧を描きながら飛ぶ） ---
            pShot->param_d[3] += 0.01; // 弧を描くための角度加算
            double currentMuki = pShot->param_d[2] + pShot->param_d[3];
            pShot->x += pShot->param_d[6] * cos(currentMuki);
            pShot->y += pShot->param_d[6] * sin(currentMuki);
            pShot->param_d[4] += pShot->param_d[6];

            // 4フレームに1回、糸（シアンの小玉）を生成して配置する
            pShot->param_i[1]++;
            if (pShot->param_i[1] % 4 == 0) {
                sEnemyShot* pString = new sEnemyShot;
                pString->x = pShot->x;
                pString->y = pShot->y;
                pString->muki = 0.0;
                pString->speed = 0.0; // 停止弾としてその場に留まる
                pString->kind = img_enemyShotSmallBall[3]; // シアンの小玉
                pString->param_i[0] = 4;
                pString->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pString->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pString;
                pEnemyShotSet->pEnemyShotHead->prev = pString;
            }

            // 最大距離（糸の長さ）に達したら停止状態へ
            if (pShot->param_d[4] >= pShot->param_d[5]) {
                pShot->param_i[0] = 1; // 停止状態へ遷移
                pEnemyShotSet->param_i[0] = 1;
                pShot->param_i[1] = 0;

                // ヨーヨーの回転による火花（黄の小玉）を撒く
                for (int i = 0; i < 8 * 2; i++) {
                    sEnemyShot* pSpark = new sEnemyShot;
                    pSpark->x = pShot->x;
                    pSpark->y = pShot->y;
                    pSpark->muki = DX_PI * 2.0 / 8.0 / 2 * i;
                    pSpark->speed = 2.0;
                    pSpark->kind = img_enemyShotSmallBall[1]; // 黄の小玉
                    pSpark->param_i[0] = 3;
                    pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSpark->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                    pEnemyShotSet->pEnemyShotHead->prev = pSpark;
                }
            }
        }
        else if (pShot->param_i[0] == 1) {
            // --- 状態1：停止（スリープ） ---
            pShot->param_i[1]++;
            if (pShot->param_i[1] > 15) { // 15フレーム停止したら復路へ
                pShot->param_i[0] = 2;
                pEnemyShotSet->param_i[0] = 2;
                // 発射元（敵の初期位置）に向かう角度を再計算
                pShot->muki = atan2(pShot->param_d[1] - pShot->y, pShot->param_d[0] - pShot->x);
            }
        }
        else if (pShot->param_i[0] == 2) {
            // --- 状態2：復路（猛スピードで巻き取られる） ---
            pShot->x += pShot->param_d[7] * cos(pShot->muki);
            pShot->y += pShot->param_d[7] * sin(pShot->muki);

            // 3フレームに1回、帰り道の糸（白の小玉）を生成
            pShot->param_i[1]++;
            if (pShot->param_i[1] % 3 == 0) {
                sEnemyShot* pString = new sEnemyShot;
                pString->x = pShot->x;
                pString->y = pShot->y;
                pString->muki = 0.0;
                pString->speed = 0.0; // 停止弾
                pString->kind = img_enemyShotSmallBall[6]; // 白の小玉
                pString->param_i[0] = 5;
                pString->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pString->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pString;
                pEnemyShotSet->pEnemyShotHead->prev = pString;
            }

            // 発射元（敵の位置）に近づいたら、自動消去させるために画面外へ飛ばす
            double dx = pShot->param_d[0] - pShot->x;
            double dy = pShot->param_d[1] - pShot->y;
            if (dx * dx + dy * dy < 100.0) { // 距離10以内に接近
                pShot->param_i[0] = 3; // 消滅待ち状態へ
                pEnemyShotSet->param_i[0] = 3;
                pShot->muki = -DX_PI / 2.0; // 真上へ
                pShot->speed = 9999.0; // 猛スピード
            }
        }
        else if (pShot->param_i[0] == 3) {
            // --- 状態3：消滅待ち（画面外へ飛び去る） ---
            // メインルーチンの画面外消去処理に任せるため、普通に移動させるのみ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 4) {
            if (pEnemyShotSet->param_i[0] == 1) pShot->margin = -9999;
        }
        else if (pShot->param_i[0] == 5) {
            if (pEnemyShotSet->param_i[0] == 3) pShot->margin = -9999;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Yoyo_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = -5;

        // 弾幕の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 15フレームごとにヨーヨーを発射するセットを生成
    if (count % 15 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAroundTheWorld;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // プレイヤーへのベース角度を取得
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        // 発射ごとに30度(PI/6)ずつズラして、大車輪のように網目を形成する
        double offset = (shot_count) * (DX_PI / 6.0);
        pEnemyShotSet->muki = baseMuki + offset;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }
}