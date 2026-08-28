// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：熱ゆらぎのコロイド（ブラウン運動）
static void ShotColloidalDrift(sEnemyShotSet* pEnemyShotSet)
{
    // ----------------------------------------------------
    // 1. 生成処理（pEnemyShotSet->count == 0 の時のみ実行）
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // コロイド粒子となる「大玉」を3個生成
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 初期進行方向と速さをランダムに設定
            double angle = (GetRand(360) / 180.0) * DX_PI;
            double speed = 1.0 + (GetRand(100) / 100.0) * 2; // 1.0 ～ 2.0

            pEnemyShot->muki = angle;
            pEnemyShot->speed = speed;

            // 弾種の設定（赤色の大玉）
            // 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒
            pEnemyShot->kind = img_enemyShotLargeBall[0];

            // 独自パラメータの格納
            pEnemyShot->param_i[0] = 0;                   // 役割フラグ (0: 大玉 / 1: 小弾)
            pEnemyShot->param_d[0] = speed * cos(angle);  // 速度 vx
            pEnemyShot->param_d[1] = speed * sin(angle);  // 速度 vy

            // 双方向連結リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----------------------------------------------------
    // 2. 毎フレームの運動更新処理
    // ----------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 次のノードをあらかじめ保持（ループ内で弾を新規追加するため）
        sEnemyShot* pNext = pShot->next;

        // ------------------------------------------------
        // 【大玉（コロイド粒子）の処理】
        // ------------------------------------------------
        if (pShot->param_i[0] == 0) {
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1];

            // ① 水分子とのランダム衝突による微小な加速度（ゆらぎ）を加算
            // GetRand(200) - 100 => -100 ～ +100
            double ax = (GetRand(200) - 100) / 400.0; // -0.25 ～ +0.25
            double ay = (GetRand(200) - 100) / 400.0;

            vx += ax;
            vy += ay;

            // ② 粘性抵抗（ブレーキ）と速度制限
            vx *= 0.97;
            vy *= 0.97;

            double currentSpeed = sqrt(vx * vx + vy * vy);

            // 止まりすぎないように突発的なゆらぎ補正
            if (currentSpeed < 0.5 * 2) {
                double boostAngle = (GetRand(360) / 180.0) * DX_PI;
                vx += 0.8 * cos(boostAngle) * 2;
                vy += 0.8 * sin(boostAngle) * 2;
            }
            // 早くなりすぎないように上限制限
            else if (currentSpeed > 2.5 * 2) {
                vx = (vx / currentSpeed) * 2.5 * 2;
                vy = (vy / currentSpeed) * 2.5 * 2;
            }

            // ③ 画面壁付近での跳ね返り（画面外に流れていかないように制御）
            if ((pShot->x < 0.0 && vx < 0.0) || (pShot->x > 480.0 && vx > 0.0)) vx = -vx;
            if ((pShot->y < 0.0 && vy < 0.0) || (pShot->y > 480.0 && vy > 0.0)) vy = -vy;

            // ④ 衝突反作用の可視化：定期的に加速度の反対方向へ「小弾（水分子）」を撃ち出す
            if (pEnemyShotSet->count % 8 == 0) {
                sEnemyShot* pSubShot = new sEnemyShot;

                pSubShot->x = pShot->x;
                pSubShot->y = pShot->y;

                // 加速度（または運動方向）の逆方向 + わずかな散乱角
                double recoilAngle = atan2(-ay, -ax) + ((GetRand(60) - 30) / 180.0) * DX_PI;
                double subSpeed = 0.4 + (GetRand(60) / 100.0); // 0.4 ～ 1.0

                pSubShot->muki = recoilAngle;
                pSubShot->speed = subSpeed;

                // シアン色の小玉（水分子をイマージ）
                pSubShot->kind = img_enemyShotSmallBall[3];

                pSubShot->param_i[0] = 1;                           // 役割フラグ (1: 小弾)
                pSubShot->param_d[0] = subSpeed * cos(recoilAngle); // 速度 vx
                pSubShot->param_d[1] = subSpeed * sin(recoilAngle); // 速度 vy

                // リスト末尾に追加
                pSubShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSubShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSubShot;
                pEnemyShotSet->pEnemyShotHead->prev = pSubShot;

                // 射出音（控えめな効果音）
                if (GetRand(10) == 0) {
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
            }

            // 位置と状態パラメータの更新
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;
            pShot->muki = atan2(vy, vx);
            pShot->speed = sqrt(vx * vx + vy * vy);
        }
        // ------------------------------------------------
        // 【小弾（溶媒分子）の処理】
        // ------------------------------------------------
        else if (pShot->param_i[0] == 1) {
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1];

            // 極小のゆらぎを加えつつ、減速して漂う
            vx += (GetRand(40) - 20) / 400.0;
            vy += (GetRand(40) - 20) / 400.0;

            vx *= 0.96;
            vy *= 0.96;

            // 位置と状態パラメータの更新
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;
            pShot->muki = atan2(vy, vx);
            pShot->speed = sqrt(vx * vx + vy * vy);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_BrownianMotion_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面: 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵本体のゆったりとした左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 160フレーム周期でブラウン運動弾幕セットを生成
    if (count % 160 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotColloidalDrift;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;

        // ダミーヘッドノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリスト（enemyShotSetHead）に追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}