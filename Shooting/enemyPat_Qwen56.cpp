// enemyPat_Ebbinghaus.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：錯弾「エビングハウスの庭」
static void ShotEbbinghaus(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音再生（重厚な音で錯覚への警戒感を煽る）
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int isLeft = pEnemyShotSet->kind; // 0: 左(狭い隙間/広く見える錯覚), 1: 右(広い隙間/狭く見える錯覚)

        // 敵の現在位置を基準に、左右にオフセットを掛けた位置を円環の中心とする
        double cx = isLeft ? (enemy.x - 100.0) : (enemy.x + 100.0);
        double cy = enemy.y;

        // ---------------------------------------------------------
        // 1. 中核弾の生成 (左右で完全に同一のサイズ・色)
        // ---------------------------------------------------------
        sEnemyShot* pCore = new sEnemyShot;
        pCore->x = cx;
        pCore->y = cy;
        pCore->muki = 0.0;
        pCore->speed = 0.0;
        pCore->kind = img_enemyShotMediumBall[6]; // 中玉(7.0x7.0), 6:白

        // param_d[4] を 0.0 にすることで、更新処理で「中核弾」であると識別させる
        pCore->param_d[0] = 0.0;      // 角度（使用しない）
        pCore->param_d[1] = 0.0;      // 回転速度（使用しない）
        pCore->param_d[2] = cx;       // 目標中心X
        pCore->param_d[3] = cy;       // 目標中心Y
        pCore->param_d[4] = 0.0;      // 半径（0.0 = 中核弾フラグ）
        pCore->margin = 100;

        pCore->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCore->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCore;
        pEnemyShotSet->pEnemyShotHead->prev = pCore;

        // ---------------------------------------------------------
        // 2. 従属弾の生成 (錯視を引き起こす周囲の弾)
        // ---------------------------------------------------------
        // 左: 半径50, 弾数10, 大玉 -> 隙間は約11.4 (狭い)
        // 右: 半径70, 弾数6, 小玉  -> 隙間は約70.8 (広い)
        int numShots = isLeft ? 10 : 6;
        double radius = isLeft ? 50.0 : 70.0;
        double rotSpeed = isLeft ? 0.04 : -0.04; // 左は時計回り、右は反時計回りで視覚的対比を強調

        // 初期角度をランダムにずらす (GetRand(360)は0~360を返す)
        double startAngle = (GetRand(360) / 360.0) * (DX_PI * 2.0);

        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = startAngle + (DX_PI * 2.0 / numShots) * i;

            pShot->x = cx + radius * cos(angle);
            pShot->y = cy + radius * sin(angle);
            pShot->muki = angle + DX_PI / 2.0; // 進行方向（見た目の向き）
            pShot->speed = 0.0; // 速度は角度計算で制御するため0

            if (isLeft) {
                pShot->kind = img_enemyShotLargeBall[8]; // 大玉(20.0x20.0), 8:橙
            }
            else {
                pShot->kind = img_enemyShotSmallBall[3]; // 小玉(2.5x2.5), 3:シアン
            }

            pShot->param_d[0] = angle;    // 現在の角度
            pShot->param_d[1] = rotSpeed; // 回転速度
            pShot->param_d[2] = cx;       // 中心X
            pShot->param_d[3] = cy;       // 中心Y
            pShot->param_d[4] = radius;   // 半径（>0.0 = 従属弾フラグ）
            pShot->margin = 100;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pEnemyShotSet->y += 1.3;

    // 弾の座標更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[4] == 0.0) {
            // 【中核弾】敵の現在位置にゆっくり追従させる
            int isLeft = pEnemyShotSet->kind;
            double targetCx = isLeft ? (enemy.x - 100.0) : (enemy.x + 100.0);
            double targetCy = enemy.y;

            // 線形補間による滑らかな追従
            pShot->param_d[2] += (targetCx - pShot->param_d[2]) * 0.1;
            pShot->param_d[3] += (targetCy - pShot->param_d[3]) * 0.1;

            pShot->x = pShot->param_d[2];
            pShot->y = pShot->param_d[3] + pEnemyShotSet->y;
        }
        else {
            // 【従属弾】中心座標を基準に回転運動
            pShot->param_d[0] += pShot->param_d[1];
            pShot->x = pShot->param_d[2] + pShot->param_d[4] * cos(pShot->param_d[0]);
            pShot->y = pShot->param_d[3] + pShot->param_d[4] * sin(pShot->param_d[0]) + pEnemyShotSet->y;
            pShot->muki = pShot->param_d[0] + DX_PI / 2.0;
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Ebbinghaus_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // HPを少し上げて耐久パターンに
        muki = 1;
    }
    else {
        // 敵は画面左右（120.0 〜 360.0）を往復移動する
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 360.0 || enemy.x < 120.0) {
            muki *= -1;
        }
    }

    // 180フレーム(約3秒)ごとに展開
    if (count % 110 == 1) {
        // 展開予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count % 110 == 60) {
        // ---------------------------------------------------------
        // 左側の円環生成 (kind = 0)
        // ---------------------------------------------------------
        sEnemyShotSet* pSetLeft = new sEnemyShotSet;
        pSetLeft->count = 0;
        pSetLeft->patternFunc = ShotEbbinghaus;
        pSetLeft->kind = 0; // 0: 左側設定を使用

        pSetLeft->pEnemyShotHead = new sEnemyShot;
        pSetLeft->pEnemyShotHead->prev = pSetLeft->pEnemyShotHead;
        pSetLeft->pEnemyShotHead->next = pSetLeft->pEnemyShotHead;

        pSetLeft->prev = enemyShotSetHead.prev;
        pSetLeft->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetLeft;
        enemyShotSetHead.prev = pSetLeft;

        // ---------------------------------------------------------
        // 右側の円環生成 (kind = 1)
        // ---------------------------------------------------------
        sEnemyShotSet* pSetRight = new sEnemyShotSet;
        pSetRight->count = 0;
        pSetRight->patternFunc = ShotEbbinghaus;
        pSetRight->kind = 1; // 1: 右側設定を使用

        pSetRight->pEnemyShotHead = new sEnemyShot;
        pSetRight->pEnemyShotHead->prev = pSetRight->pEnemyShotHead;
        pSetRight->pEnemyShotHead->next = pSetRight->pEnemyShotHead;

        pSetRight->prev = enemyShotSetHead.prev;
        pSetRight->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetRight;
        enemyShotSetHead.prev = pSetRight;
    }
}