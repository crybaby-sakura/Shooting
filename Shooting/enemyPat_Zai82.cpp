// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：布団が吹っ飛んだ
static void ShotFuton(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 予告音と発射音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 布団の中心座標・回転角度・速度などのパラメータを初期化
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 布団中心X
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 布団中心Y
        pEnemyShotSet->param_d[2] = 0.0;              // Z軸回転角（布団のくるくる回転）
        pEnemyShotSet->param_d[3] = 0.0;              // X軸回転角（布団のバサバサ揺れ）
        pEnemyShotSet->param_d[4] = 1.5 + GetRand(10) / 10.0; // 上昇速度
        pEnemyShotSet->param_d[5] = GetRand(60);             // 揺れの位相オフセット

        // 布団本体の弾を生成（横5個 × 縦7個 の長方形）
        // 素材選定: 中玉(7.0x7.0)・白(6)
        // 理由: 小玉だと隙間が多すぎて布団に見えず、大玉だと重すぎるため。
        //       白色で布団の綿のフワッとした質感を表現。
        int futonW = 5;
        int futonH = 7;
        double spaceX = 10.0; // 弾の配置間隔
        double spaceY = 10.0;

        for (int iy = 0; iy < futonH; iy++) {
            for (int ix = 0; ix < futonW; ix++) {
                pEnemyShot = new sEnemyShot;

                // 布団の中心を(0,0)とした相対座標を記録
                pEnemyShot->param_d[0] = (ix - (futonW - 1) / 2.0) * spaceX;
                pEnemyShot->param_d[1] = (iy - (futonH - 1) / 2.0) * spaceY;

                // 初期座標をセット
                pEnemyShot->x = pEnemyShotSet->param_d[0] + pEnemyShot->param_d[0];
                pEnemyShot->y = pEnemyShotSet->param_d[1] + pEnemyShot->param_d[1];

                // 座標を直接制御するため速度は0にする
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 中玉・白

                // 双方向リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // === 布団の移動・回転処理 ===
    // 上方向へ飛ばしつつ、サイン波で風に揺られるように左右に動かす
    pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[4];
    pEnemyShotSet->param_d[0] += sin((pEnemyShotSet->count + pEnemyShotSet->param_d[5]) * 0.05) * 1.5;

    // 回転角度を更新
    pEnemyShotSet->param_d[2] += 0.015; // ゆっくりくるくる回転
    pEnemyShotSet->param_d[3] += 0.035; // バサバサ感を出すためのやや速い回転

    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];
    double rotZ = pEnemyShotSet->param_d[2];
    double rotX = pEnemyShotSet->param_d[3];
    double scaleX = cos(rotX); // 布団が横に見えたり縦に見えたりする擬似3D表現用


    // === 綿ぼこりの追加生成 ===
    // 素材選定: 小玉(2.5x2.5)・白(6)
    // 理由: 布団からこぼれ落ちる軽い綿ぼこりを表現するため。
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 15 == 0) {
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + GetRand(40) - 20;
            pEnemyShot->y = cy + GetRand(60) - 30;
            // GetRand(628) は 0〜628 の 629 通り。628/100.0 で約 2π になる
            pEnemyShot->muki = (GetRand(628) - 314) / 100.0;
            pEnemyShot->speed = (50 + GetRand(100)) / 100.0; // 0.5 〜 1.5 の遅さ
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 小玉・白
            pEnemyShot->param_d[0] = 0.0; // 布団弾ではない印として0を入れておく（速度>0で判別するため実質不要）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // === 突風の追加生成 ===
    // 素材選定: 短レーザー(64.0x4.0)・シアン(3)
    // 理由: 布団が通った後に遅れて吹き抜ける風の軌跡を線で表現するため。
    if (pEnemyShotSet->count > 10 && pEnemyShotSet->count % 30 == 15) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = cx;
        pEnemyShot->y = cy;
        // 上方向(-π/2)を中心に ±30度ばらつかせる
        pEnemyShot->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
        pEnemyShot->speed = 8.0;
        pEnemyShot->kind = img_enemyShotLaser[3]; // 短レーザー・シアン
        pEnemyShot->param_d[0] = 0.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }


    // === 全弾の座標更新 ===
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->speed == 0.0) {
            // 布団を構成する弾：回転を適用して座標を直接代入
            double lx = pShot->param_d[0];
            double ly = pShot->param_d[1];

            // Z軸回転（平面での回転）
            double rx = lx * cos(rotZ) - ly * sin(rotZ);
            double ry = lx * sin(rotZ) + ly * cos(rotZ);

            // X軸回転（横方向への圧縮・拡大で布団の反転を表現）
            rx *= scaleX;

            pShot->x = cx + rx;
            pShot->y = cy + ry;
        }
        else {
            // 綿ぼこり・突風：通常のベクトル移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_FutonFlewAway_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 300フレームごとに布団を発射
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFuton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0; // 今回のパターンでは使用しない
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