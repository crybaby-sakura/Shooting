// enemyPat_rotatingBar.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：回転棒弾（ローテーティング・バー）
static void ShotRotatingBar(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->param_d[0] : 棒の現在中心X
    // pEnemyShotSet->param_d[1] : 棒の現在中心Y
    // pEnemyShotSet->param_d[2] : 現在の回転角度 theta
    // pEnemyShotSet->param_d[3] : 回転角速度 (フレームあたりの変化量)
    // pEnemyShotSet->param_d[4] : 直進速度

    if (pEnemyShotSet->count == 0) {
        // 発射音の再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 初期パラメータの設定
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = (GetRand(360) / 180.0) * DX_PI; // 初期ランダム角度

        // GetRand(1) は 0 または 1 を返すため、時計回り/反時計回りをランダム選択
        double rotSpeed = 0.035;
        pEnemyShotSet->param_d[3] = (GetRand(1) == 0) ? rotSpeed : -rotSpeed;
        pEnemyShotSet->param_d[4] = 1.5; // 前進速度

        // --- 1. 中心核の作成（赤色の中玉） ---
        sEnemyShot* pCentralShot = new sEnemyShot;
        pCentralShot->kind = img_enemyShotMediumBall[0]; // 0:赤
        pCentralShot->param_d[0] = 0.0;                 // 中心からの距離 r = 0
        pCentralShot->param_d[1] = 0.0;                 // 角度オフセット = 0
        pCentralShot->x = pEnemyShotSet->param_d[0];
        pCentralShot->y = pEnemyShotSet->param_d[1];

        // 双方向リストへ挿入
        pCentralShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCentralShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCentralShot;
        pEnemyShotSet->pEnemyShotHead->prev = pCentralShot;

        // --- 2. アーム（棒）部分の作成（青色の小玉） ---
        const int armLength = 6;      // 片側6個（両側で計12個）
        const double interval = 12.0; // 弾同士の間隔(px)

        for (int dir = 0; dir < 2; dir++) { // 0度方向 と 180度(DX_PI)方向
            double angleOffset = dir * DX_PI;

            for (int i = 1; i <= armLength; i++) {
                sEnemyShot* pArmShot = new sEnemyShot;
                pArmShot->kind = img_enemyShotSmallBall[4]; // 4:青
                pArmShot->param_d[0] = i * interval;        // 個別の距離 r を保持
                pArmShot->param_d[1] = angleOffset;         // 個別の角度オフセットを保持

                // 初期位置計算
                double totalAngle = pEnemyShotSet->param_d[2] + angleOffset;
                pArmShot->x = pEnemyShotSet->param_d[0] + pArmShot->param_d[0] * cos(totalAngle);
                pArmShot->y = pEnemyShotSet->param_d[1] + pArmShot->param_d[0] * sin(totalAngle);

                // 双方向リストへ挿入
                pArmShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pArmShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pArmShot;
                pEnemyShotSet->pEnemyShotHead->prev = pArmShot;
            }
        }
    }

    // --- 毎フレームの軌道・回転更新処理 ---

    // 1. 棒全体（中心座標）の前進移動
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[4] * cos(pEnemyShotSet->muki);
    pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[4] * sin(pEnemyShotSet->muki);

    // 2. 棒全体の回転角度の加算
    pEnemyShotSet->param_d[2] += pEnemyShotSet->param_d[3];

    double centerX = pEnemyShotSet->param_d[0];
    double centerY = pEnemyShotSet->param_d[1];
    double currentRotAngle = pEnemyShotSet->param_d[2];

    // 3. 各構成弾の座標を再計算
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->param_d[0];
        double offsetAngle = pShot->param_d[1];

        pShot->x = centerX + r * cos(currentRotAngle + offsetAngle);
        pShot->y = centerY + r * sin(currentRotAngle + offsetAngle);

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_KuruKuruKururin_Gemini()
{
    static int muki;

    if (count == 1) {
        // 初期位置とパラメータ設定
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵本体の左右往復運動
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) muki *= -1;
    }

    // 90フレームごとに回転棒を1本生成して自機方向へ発射
    if (count % 90 == 1) {
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRotatingBar;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;

            // 自機を狙う角度を設定
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + i * 0.3;
            pEnemyShotSet->kind = 0;

            // 子弾リストのダミーヘッド生成
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 全体リストに追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}