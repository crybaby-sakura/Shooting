// enemyPat_YoYo.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ヨーヨー・リターン
static void ShotYoYo(sEnemyShotSet* pEnemyShotSet)
{
    // パラメータ定義 (pEnemyShotSet->param_d)
    // [0]: 現在の距離
    // [1]: 最大到達距離
    // [2]: 往路角度
    // [3]: 復路角度
    // [4]: ステート (0:往路, 1:停止, 2:復路, 3:消去待ち)
    // [5]: 停止カウンタ
    // [6]: 回転角度

    if (pEnemyShotSet->count == 0) {
        // 初期化フェーズ
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_d[1] = 380.0;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->muki;
        // GetRand(30) は 0..30 を返すので、-15して -15..+15度のランダムなずれを作る
        pEnemyShotSet->param_d[3] = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
        pEnemyShotSet->param_d[4] = 0.0;
        pEnemyShotSet->param_d[5] = 0.0;
        pEnemyShotSet->param_d[6] = 0.0;

        // 1. ヨーヨー本体 (大玉)
        sEnemyShot* pBody = new sEnemyShot;
        pBody->x = pEnemyShotSet->x; pBody->y = pEnemyShotSet->y;
        pBody->speed = 0.0; // メインルーチンの移動は無効化
        pBody->kind = img_enemyShotLargeBall[2]; // 緑色の大玉
        pBody->param_i[0] = 0; // ID: 本体
        pBody->margin = 480;

        pBody->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBody->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBody;
        pEnemyShotSet->pEnemyShotHead->prev = pBody;

        // 2. 回転弾 (菱形弾 x 3)
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pSpin = new sEnemyShot;
            pSpin->x = pBody->x; pSpin->y = pBody->y;
            pSpin->speed = 0.0;
            pSpin->kind = img_enemyShotDiamond[i % 8];
            pSpin->param_i[0] = 1; // ID: 回転弾
            pSpin->param_i[1] = i; // インデックス
            pSpin->margin = 480;

            pSpin->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pSpin->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pSpin;
            pEnemyShotSet->pEnemyShotHead->prev = pSpin;
        }

        // 3. 糸弾 (小玉 x 12)
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pString = new sEnemyShot;
            pString->x = pBody->x; pString->y = pBody->y;
            pString->speed = 0.0;
            pString->kind = img_enemyShotSmallBall[6]; // 白色の小玉
            pString->param_i[0] = 2; // ID: 糸弾
            pString->param_i[1] = i; // インデックス
            pString->margin = 480;

            pString->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pString->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pString;
            pEnemyShotSet->pEnemyShotHead->prev = pString;
        }
    }

    int state = (int)pEnemyShotSet->param_d[4];

    // 消去待ち状態の場合は、メインルーチンの移動・消去処理に任せるため早期リターン
    //if (state == 3) return;

    // --- ステート更新 ---
    if (state == 0) {
        // 往路 (減速しながら進む)
        double spd = 6.0 - (pEnemyShotSet->param_d[0] / pEnemyShotSet->param_d[1]) * 4.0;
        pEnemyShotSet->param_d[0] += spd;
        if (pEnemyShotSet->param_d[0] >= pEnemyShotSet->param_d[1]) {
            pEnemyShotSet->param_d[0] = pEnemyShotSet->param_d[1];
            pEnemyShotSet->param_d[4] = 1.0;
        }
    }
    else if (state == 1) {
        // 停止・回転
        pEnemyShotSet->param_d[5] += 1.0;
        pEnemyShotSet->param_d[6] += 10.0 * DX_PI / 180.0; // 回転
        if (pEnemyShotSet->param_d[5] >= 60.0) { // 60フレーム停止
            pEnemyShotSet->param_d[4] = 2.0;
        }
    }
    else if (state == 2) {
        // 復路 (加速しながら戻る)
        double spd = 4.0 + (pEnemyShotSet->param_d[1] - pEnemyShotSet->param_d[0]) / pEnemyShotSet->param_d[1] * 6.0;
        pEnemyShotSet->param_d[0] -= spd;
        if (pEnemyShotSet->param_d[0] <= 0.0) {
            pEnemyShotSet->param_d[0] = 0.0;
            pEnemyShotSet->param_d[4] = 3.0; // 終了

            // 全弾を画面外（真上）へ飛ばして自動消去を促す
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] != 99) {
                    pShot->param_i[0] = 99;
                    pShot->speed = 20.0;
                    pShot->muki = -DX_PI / 2.0;
                }
                pShot = pShot->next;
            }
            return;
        }
    }

    // --- 各弾の位置計算と適用 ---
    double dist = pEnemyShotSet->param_d[0];
    double baseX = pEnemyShotSet->x;
    double baseY = pEnemyShotSet->y;
    double angle = 0.0;
    if (state == 0) {
        angle = pEnemyShotSet->param_d[2];
    }
    else if (state == 1) {
        angle = pEnemyShotSet->param_d[2]
            + pEnemyShotSet->param_d[5] / 60.0
            * (pEnemyShotSet->param_d[3] - pEnemyShotSet->param_d[2]);
    }
    else if (state == 2) {
       angle = pEnemyShotSet->param_d[3];
    }

    // ヨーヨー本体の現在位置
    double bodyX = baseX + dist * cos(angle);
    double bodyY = baseY + dist * sin(angle);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];

        if (role == 0) {
            // 本体
            pShot->x = bodyX;
            pShot->y = bodyY;
        }
        else if (role == 1) {
            // 回転弾
            int idx = pShot->param_i[1];
            double spinAngle = pEnemyShotSet->param_d[6] + (idx * 2.0 * DX_PI / 3.0);
            pShot->x = bodyX + 28.0 * cos(spinAngle);
            pShot->y = bodyY + 28.0 * sin(spinAngle);
            pShot->muki = spinAngle;

            if (state == 1) {
                sEnemyShot* pSub = new sEnemyShot;
                pSub->x = pShot->x; pSub->y = pShot->y;
                pSub->speed = 2.0;
                pSub->muki = spinAngle;
                pSub->kind = pShot->kind;
                pSub->param_i[0] = 99; // ID: 回転弾
                pSub->margin = 480;

                pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSub->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                pEnemyShotSet->pEnemyShotHead->prev = pSub;
            }
        }
        else if (role == 2) {
            // 糸弾 (ボスと本体の間に等間隔で配置)
            int idx = pShot->param_i[1];
            double t = (idx + 1) / 13.0;
            pShot->x = baseX + (bodyX - baseX) * t;
            pShot->y = baseY + (bodyY - baseY) * t;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Yoyo_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 180フレームごとにヨーヨーを発射
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotYoYo;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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