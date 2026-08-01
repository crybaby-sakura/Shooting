// enemyPat_Tmp_SignPole.cpp
// サインポール（理容店の赤白螺旋ポール）をモチーフにした弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：サインポール螺旋
// 赤と白の小玉が円周上に並び、回転＋半径拡大＋上方向（画面上方）への流れで
// ポールの螺旋ストライプが回転しながら流れる様子を再現する。
static void ShotSignPole(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定間隔でリング状に弾を生成（位相をずらし続けることで螺旋感を出す）
    if (pEnemyShotSet->count % 7 == 0 && pEnemyShotSet->count % 300 < 270) {
        // 使える効果音: sound_enemyShot_light / medium / heavy / extreme / sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int num = 14;                          // 1リングの弾数
        const double baseRadius = 28.0;              // 初期半径
        // 時間とともに位相を進めて縞がねじれて見えるようにする
        double phase = pEnemyShotSet->count * 0.085;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            double ang = phase + i * (2.0 * DX_PI / num);

            // 生成位置（中心からの相対）
            pEnemyShot->x = pEnemyShotSet->x + baseRadius * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + baseRadius * sin(ang);

            // param_d をカスタム移動用に使用
            // [0] 現在角度
            // [1] 現在半径
            // [2] 角速度 (rad/frame)
            // [3] 半径拡大速度
            // [4] 垂直速度（負で画面上方へ）
            pEnemyShot->param_d[0] = ang;
            pEnemyShot->param_d[1] = baseRadius;
            pEnemyShot->param_d[2] = 0.038;          // 回転速度
            pEnemyShot->param_d[3] = 0.32;           // 徐々に広がる
            pEnemyShot->param_d[4] = -0.75;          // 上方向へ流れる

            // 色：赤(0) と 白(6) を交互配置。位相を加味して螺旋縞にする
            // 弾の種類一覧から小玉を選択（密集させても見やすい）
            int color = ((i + (pEnemyShotSet->count / 7)) % 2 == 0) ? 0 : 6;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // 線形移動は使わないのでダミー
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の位置を毎フレーム更新（回転・拡大・上昇）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 角度と半径を進める
        pShot->param_d[0] += pShot->param_d[2];
        pShot->param_d[1] += pShot->param_d[3];

        // 中心（ショットセット位置）を基準に極座標 → 直交座標へ
        // さらに経過フレーム分だけ垂直方向へオフセット
        pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y
            + pShot->param_d[1] * sin(pShot->param_d[0])
            + pShot->param_d[4] * pShot->count;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
// 新しく作成する場合の名前は void EnemyPat_SignPole_Grok() にすること
void EnemyPat_SignPole_Grok()
{
    static sEnemyShotSet* pSignPoleSet = nullptr;  // このパターン専用のショットセット

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 440.0;
        enemy.maxHp = enemy.hp = 60 * 20;               // 200で固定

        // ショットセットを1つだけ生成し、以降ずっとこのパターンを動かす
        pSignPoleSet = new sEnemyShotSet;
        pSignPoleSet->count = 0;
        pSignPoleSet->patternFunc = ShotSignPole;
        pSignPoleSet->x = enemy.x;
        pSignPoleSet->y = enemy.y;
        pSignPoleSet->muki = 0.0;
        pSignPoleSet->kind = 0;
        pSignPoleSet->pEnemyShotHead = new sEnemyShot;
        pSignPoleSet->pEnemyShotHead->prev = pSignPoleSet->pEnemyShotHead;
        pSignPoleSet->pEnemyShotHead->next = pSignPoleSet->pEnemyShotHead;

        // リストに追加
        pSignPoleSet->prev = enemyShotSetHead.prev;
        pSignPoleSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSignPoleSet;
        enemyShotSetHead.prev = pSignPoleSet;

        player.y = 240;
    }
    else {
        // 敵本体を緩やかに左右へ揺らす（ポールが少し揺れている感じ）
        enemy.x = 240.0 + 18.0 * sin(count * 0.018);
        enemy.y = 440.0 + 6.0 * sin(count * 0.027);

        // ショットセットの中心も敵に追従させる
        if (pSignPoleSet != nullptr) {
            pSignPoleSet->x = enemy.x;
            pSignPoleSet->y = enemy.y;
        }
    }

    enemy.hp--;
}