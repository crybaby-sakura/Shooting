// enemyPat_GravityCore.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h" // 既存の画像・音声ハンドルが宣言されていると想定
#include "player.h"
#include <math.h>

// 弾幕：グラビティ・コアと破砕衝撃波
static void ShotGravityCore(sEnemyShotSet* pSet)
{
    int c = pSet->count;

    // pSet->param_d[0] : コアの中心 X座標
    // pSet->param_d[1] : コアの中心 Y座標
    // pSet->param_d[2] : コアの半径
    // pSet->param_d[3] : コア全体の回転角

    // ---------------------------------------------------------
    // 発生（圧迫）：超巨大弾のガワ（外殻）を生成
    // ---------------------------------------------------------
    if (c == 0) {
        pSet->param_d[0] = pSet->x;
        pSet->param_d[1] = pSet->y;
        pSet->param_d[2] = 0.0;
        pSet->param_d[3] = 0.0;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 巨大弾のガワを大玉を密集させて表現する（3層構造）
        for (int r = 0; r < 3; r++) {
            int num = 30 + r * 8; // 外周ほど弾数を増やす (30, 45, 60個)
            for (int i = 0; i < num; i++) {
                sEnemyShot* pShot = new sEnemyShot;

                pShot->kind = img_enemyShotLargeBall[7]; // 黒の大玉で重力球を表現（色は適宜変更可）

                pShot->param_i[0] = 1; // 1: 巨大弾のガワとして追従する弾
                pShot->param_d[0] = r; // レイヤー番号 (0, 1, 2)
                pShot->param_d[1] = (DX_PI * 2.0 / num) * i; // 初期角度

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ---------------------------------------------------------
    // フェーズ1: 発生・膨張 (0 ～ 59フレーム)
    // ---------------------------------------------------------
    if (c < 60) {
        // コアを画面中央(240, 200)へ徐々に移動
        pSet->param_d[0] += (240.0 - pSet->param_d[0]) * 0.05;
        pSet->param_d[1] += (120.0 - pSet->param_d[1]) * 0.05;
        // 半径を広げて巨大弾を形成 (最大 140)
        pSet->param_d[2] += (140.0 - pSet->param_d[2]) * 0.05;
    }
    // ---------------------------------------------------------
    // フェーズ2: 圧縮・吸引 (60 ～ 239フレーム)
    // ---------------------------------------------------------
    else if (c < 240) {
        if (c == 60) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 半径を徐々に縮小 (最小 25まで)
        pSet->param_d[2] += (25.0 - pSet->param_d[2]) * 0.015;

        // 吸引処理: プレイヤーをコア中心へ強制的に引き寄せる
        double dx = pSet->param_d[0] - player.x;
        double dy = pSet->param_d[1] - player.y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist > 0.1) {
            // 近づくほど強く吸い寄せるが、理不尽にならないよう最大速度(3.0px)を設ける
            double pull = 800.0 / (dist + 50.0);
            if (pull > 3.0) pull = 3.0;
            player.x += (dx / dist) * pull;
            player.y += (dy / dist) * pull;
            spawnForceParticles(player.x, player.y, (dx / dist) * pull * 3, (dy / dist) * pull * 3);
        }

        // 吸引中、コアから全方位に針弾（銃弾）をポロポロと発射
        if (c % 1 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotBullet[3]; // シアンの銃弾
            pShot->x = pSet->param_d[0];
            pShot->y = pSet->param_d[1];
            pShot->muki = (GetRand(359) / 180.0) * DX_PI; // 0～359度のランダム
            pShot->speed = 1.5 + (GetRand(100) / 100.0);  // 1.5 ～ 2.5のランダム速度
            pShot->param_i[0] = 0; // 0: 通常の直進弾

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
    // ---------------------------------------------------------
    // フェーズ3: 解放・爆発 (240フレーム)
    // ---------------------------------------------------------
    else if (c == 240) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 1. ガワ（外殻）を構成していた大玉の拘束を解除し、一斉に外側へ吹き飛ばす
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                pShot->param_i[0] = 0; // 直進弾へ移行
                double angle = atan2(pShot->y - pSet->param_d[1], pShot->x - pSet->param_d[0]);
                pShot->muki = angle;
                // レイヤー（内側・外側）に応じて速度差をつける
                pShot->speed = 3.0 + pShot->param_d[0] * 1.5;
            }
            pShot = pShot->next;
        }

        // 2. 極太レーザーに見立てた「短レーザーの連なり」を8方向に発射
        double baseAngle = (GetRand(359) / 180.0) * DX_PI;
        for (int i = 0; i < 8; i++) {
            double angle = baseAngle + (DX_PI * 2.0 / 8.0) * i;
            for (int j = 0; j < 10; j++) {
                sEnemyShot* pL = new sEnemyShot;
                pL->kind = img_enemyShotLaser[6]; // 白の短レーザー
                // 間隔を空けて生成することで一本の長いレーザーのように見せる
                pL->x = pSet->param_d[0] + cos(angle) * (j * 25);
                pL->y = pSet->param_d[1] + sin(angle) * (j * 25);
                pL->muki = angle;
                pL->speed = 6.0;
                pL->param_i[0] = 0;

                pL->prev = pSet->pEnemyShotHead->prev;
                pL->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pL;
                pSet->pEnemyShotHead->prev = pL;
            }
        }

        // 3. 隙間を埋める高速リング弾（中楕円弾）
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pR = new sEnemyShot;
            pR->kind = img_enemyShotMediumOval[5]; // マゼンタの中楕円弾
            double angle = (DX_PI * 2.0 / 36.0) * i;
            pR->x = pSet->param_d[0];
            pR->y = pSet->param_d[1];
            pR->muki = angle;
            pR->speed = 2.0;
            pR->param_i[0] = 0;

            pR->prev = pSet->pEnemyShotHead->prev;
            pR->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pR;
            pSet->pEnemyShotHead->prev = pR;
        }
    }

    // ---------------------------------------------------------
    // 全フェーズ共通の弾移動処理
    // ---------------------------------------------------------

    // コア全体の回転を更新
    pSet->param_d[3] += 0.015;
    if (c >= 60 && c < 240) {
        // 圧縮中は徐々に回転を速くして切迫感を出す
        pSet->param_d[3] += 0.03 * ((c - 60) / 180.0);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // --- 巨大弾のガワ（コアに追従・回転） ---
            double r = pSet->param_d[2];
            // レイヤーごとに半径を調整 (0.4倍, 0.7倍, 1.0倍)
            double layer_r = r * (0.4 + pShot->param_d[0] * 0.3);

            // レイヤー1(真ん中)だけ逆回転させて禍々しさを出す
            double rotDir = (pShot->param_d[0] == 1) ? -1.0 : 1.0;
            double angle = pShot->param_d[1] + pSet->param_d[3] * rotDir;

            pShot->x = pSet->param_d[0] + layer_r * cos(angle);
            pShot->y = pSet->param_d[1] + layer_r * sin(angle);

            // 弾の向きは円周の接線方向に向ける
            pShot->muki = angle + DX_PI / 2.0;
        }
        else if (pShot->param_i[0] == 0) {
            // --- 通常弾（等速直線運動） ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン（メインから呼ばれる関数）
void EnemyPat_HugeBullet_Gemini()
{
    static int muki;

    // 登場時・初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0; // 画面上部に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右に揺れ動く
        enemy.x += 0.3 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 360フレーム（約6秒）を1サイクルとして、定期的にグラビティ・コアを発動
    if (count % 360 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGravityCore;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}