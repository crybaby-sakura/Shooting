// enemyPat_LissajousWeaver.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：位相織機『リサジュー・ウィーバー』
static void ShotLissajousWeaver(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 波の生成時に効果音を鳴らす
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int bulletNum = 120;  // 1つの網目を構成する弾数
        double a = 3.0;       // X軸方向の周波数
        double b = 4.0;       // Y軸方向の周波数

        // 発射ウェーブ(kind)ごとに色と初期位相を変更する
        // 3:シアン、5:マゼンタ を交互に発射して視覚的なコントラストを付ける
        int colorIndex = (pEnemyShotSet->kind % 2 == 0) ? 3 : 5;
        int shotImg = img_enemyShotMediumBall[colorIndex];

        // 波ごとに少しずつ初期位相をずらすことで、複数の網目が重なったときの美しさを出す
        double base_delta = pEnemyShotSet->kind * (DX_PI / 6.0);

        for (int i = 0; i < bulletNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double theta = (DX_PI * 2.0 / bulletNum) * i;

            // 弾の固有パラメータとして初期情報を記録
            pEnemyShot->param_d[0] = pEnemyShotSet->x;       // 発射位置X (cx)
            pEnemyShot->param_d[1] = pEnemyShotSet->y;       // 発射位置Y (cy)
            pEnemyShot->param_d[2] = theta;                  // リサジュー曲線の媒介変数(θ)
            pEnemyShot->param_d[3] = base_delta;             // 初期位相

            pEnemyShot->kind = shotImg;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 登録された弾の座標を更新（ぐにゃぐにゃと変形しながら拡大する）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double cx = pShot->param_d[0];
        double cy = pShot->param_d[1];
        double theta = pShot->param_d[2];
        double delta0 = pShot->param_d[3];
        double t = pShot->count;

        double a = 3.0;
        double b = 4.0;
        double omega = 0.015; // 位相が変化するスピード（ぐにゃぐにゃ感の強さ）

        // 時間経過で位相を変化させる
        double current_delta = delta0 + omega * t;

        // 拡大半径 (徐々に加速しながら広がるようにする)
        double R = 1.0 * t + 0.003 * t * t;

        // リサジュー曲線の座標計算
        // Y方向への広がりを少し強くして、プレイヤー側へ迫る圧迫感を強調
        pShot->x = cx + R * 0.9 * sin(a * theta);
        pShot->y = cy + R * 1.2 * sin(b * theta + current_delta);

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Lissajous_Gemini()
{
    static int muki;
    static int shot_count;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 長く弾幕を見せるためにHPを高めに設定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 画面上部でゆるやかに左右へ揺れ動く
        enemy.x += 0.8 * (double)muki;
        if (enemy.x > 320.0) muki = -1;
        if (enemy.x < 160.0) muki = 1;
    }

    // 一定フレーム間隔でリサジュー・ウィーバーのウェーブを発射
    if (count > 60 && count % 35 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLissajousWeaver;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}