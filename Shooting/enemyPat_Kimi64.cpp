// enemyPat_dangoRondo.cpp
// 三色団子モチーフ弾幕「花見団子の輪舞曲（ロンド）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン：三色団子の輪舞曲
// ============================================================

// 串（3つの弾のセット）の構造
//  1つの sEnemyShotSet が1本の串を表し、3つの sEnemyShot を持つ
//  param_i[0] : 串の種類 0=左(時計回り), 1=中央(回転なし), 2=右(反時計回り)
//  param_i[1] : 分離フラグ 0=未分離, 1=分離済み
//  param_d[0] : 回転角度（ラジアン）
//  param_d[1] : 回転速度
//  param_d[2] : 串の中心X（分離後も参照用）
//  param_d[3] : 串の中心Y

static void ShotDangoRondo(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int skewerType = pEnemyShotSet->param_i[0];   // 0:左, 1:中央, 2:右
    int separated = pEnemyShotSet->param_i[1];   // 0:未分離, 1:分離済み
    double& rotAngle = pEnemyShotSet->param_d[0];
    double& rotSpeed = pEnemyShotSet->param_d[1];
    double& centerX = pEnemyShotSet->param_d[2];
    double& centerY = pEnemyShotSet->param_d[3];

    // --- 初回生成：3つの弾（桃・白・緑）を串に刺す ---
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 回転速度の設定（串ごとに異なる）
        switch (skewerType) {
        case 0:  rotSpeed = 0.03;  break;  // 左：時計回り、ゆっくり
        case 1:  rotSpeed = 0.0;   break;  // 中央：回転なし
        case 2:  rotSpeed = -0.06; break;  // 右：反時計回り、速め
        }

        // 串の中心位置を記録
        centerX = pEnemyShotSet->x;
        centerY = pEnemyShotSet->y;

        // 3つの弾を生成（桃=赤(0)、白=白(6)、緑=緑(2)）
        // 弾の種類：小玉(img_enemyShotSmallBall)を使用
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            // 初期位置は串の中心（後で回転計算でずらす）
            pEnemyShot->x = centerX;
            pEnemyShot->y = centerY;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // 色の割り当て
            // i=0:桃(赤=0), i=1:白(白=6), i=2:緑(緑=2)
            int colorIdx;
            switch (i) {
            case 0: colorIdx = 0; break;  // 桃 → 赤
            case 1: colorIdx = 6; break;  // 白 → 白
            case 2: colorIdx = 2; break;  // 緑 → 緑
            }
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];

            // 弾の種類を識別するため param_i[0] に色種別を保存
            // 0=桃(誘導), 1=白(直進), 2=緑(加速)
            pEnemyShot->param_i[0] = i;

            // 串内での相対角度（120度間隔）
            pEnemyShot->param_d[0] = i * (DX_PI * 2.0 / 3.0);

            // 串からの距離（半径）
            pEnemyShot->param_d[1] = 16.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 毎フレームの更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    // 分離前：串として回転降下
    if (separated == 0) {
        // 串全体がゆっくり降下
        centerY += 1.2;

        // 回転角度を更新
        rotAngle += rotSpeed;

        // 分離判定：画面中央付近（Y=200）を超えたら分離
        if (centerY >= 200.0) {
            pEnemyShotSet->param_i[1] = 1;  // 分離フラグON

            // 分離時の効果音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 各弾に分離後の挙動を設定
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                int dangoType = p->param_i[0];  // 0=桃, 1=白, 2=緑

                // 分離時の方向：串の回転角度 + 弾の相対角度
                double sepAngle = rotAngle + p->param_d[0];

                switch (dangoType) {
                case 0: // 桃（誘導弾）：プレイヤー方向へ緩やかに曲がる
                    p->muki = atan2(player.y - p->y, player.x - p->x);
                    p->speed = 1.8;
                    p->param_d[2] = 0.015;  // 誘導強度
                    break;

                case 1: // 白（通常弾）：分離時の勢いのまま直進
                    p->muki = sepAngle;
                    p->speed = 2.0;
                    p->param_d[2] = 0.0;    // 誘導なし
                    break;

                case 2: // 緑（加速弾）：分離後加速して拡散
                    p->muki = sepAngle;
                    p->speed = 1.5;
                    p->param_d[2] = 0.02;   // 毎フレーム加速量
                    break;
                }

                p = p->next;
            }
        }

        // 分離前：串の回転位置を各弾に反映
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double relAngle = rotAngle + pShot->param_d[0];
            double radius = pShot->param_d[1];
            pShot->x = centerX + radius * cos(relAngle);
            pShot->y = centerY + radius * sin(relAngle);
            pShot = pShot->next;
        }
    }
    // 分離後：各弾が独立して動く
    else {
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int dangoType = pShot->param_i[0];

            switch (dangoType) {
            case 0: { // 桃：プレイヤー方向へ緩やかに誘導
                double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
                // 角度を近い方へ補間
                double diff = targetMuki - pShot->muki;
                while (diff > DX_PI) diff -= DX_PI * 2.0;
                while (diff < -DX_PI) diff += DX_PI * 2.0;
                pShot->muki += diff * pShot->param_d[2];
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                break;
            }

            case 1: // 白：直進
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                break;

            case 2: // 緑：加速
                pShot->speed += pShot->param_d[2];
                if (pShot->speed > 4.5) pShot->speed = 4.5;  // 最高速度制限
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                break;
            }

            pShot = pShot->next;
        }
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================

void EnemyPat_TricolorDango_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵を左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で3本の串（左・中央・右）を同時生成
    if (count % 45 == 1) {
        // 3本の串を生成
        for (int s = 0; s < 3; s++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotDangoRondo;
            pEnemyShotSet->x = enemy.x + (s - 1) * 60.0;  // 左:-60, 中央:0, 右:+60
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = shot_count++;

            // 串の種類を設定
            pEnemyShotSet->param_i[0] = s;  // 0=左, 1=中央, 2=右
            pEnemyShotSet->param_i[1] = 0;  // 分離フラグOFF

            // 回転パラメータ初期化
            pEnemyShotSet->param_d[0] = 0.0;  // 回転角度
            pEnemyShotSet->param_d[1] = 0.0;  // 回転速度（ShotDangoRondo内で設定）
            pEnemyShotSet->param_d[2] = 0.0;  // 中心X
            pEnemyShotSet->param_d[3] = 0.0;  // 中心Y

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}