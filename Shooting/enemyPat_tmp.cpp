// enemyPat_Tmp_SignPole.cpp
// サインポール（理容店の赤白青螺旋ポール）をモチーフにした弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：サインポール螺旋
// 赤・白・青の小玉が円周上に並び、回転＋半径拡大＋上方向（画面上方）への流れで
// ポールの螺旋ストライプが回転しながら流れる様子を再現する。
// param_i[0] == 1 のときは新規弾生成を停止する（古いショットセット用）
static void ShotSignPole(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 停止フラグが立っていない場合のみ、一定間隔でリング状に弾を生成
    if (pEnemyShotSet->param_i[0] == 0 && pEnemyShotSet->count % 7 == 0) {
        // 使える効果音: sound_enemyShot_light / medium / heavy / extreme / sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int num = 14;                          // 1リングの弾数
        const double baseRadius = 28.0;              // 初期半径
        // 自機狙いの向き（muki）を初期位相に加え、時間とともに位相を進めて螺旋感を出す
        double phase = pEnemyShotSet->muki + pEnemyShotSet->count * 0.085;

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

            // 色：赤(0)・白(6)・青(4) を順番に配置。位相を加味して螺旋縞にする
            // 弾の種類一覧から小玉を選択（密集させても見やすい）
            static const int colors[3] = { 0, 6, 4 }; // 赤・白・青
            int color = colors[(i + (pEnemyShotSet->count / 7)) % 3];
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
    // 停止後も既存弾は動き続ける
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
// 新しく作成する場合の名前は void EnemyPat_Tmp() にすること
void EnemyPat_Tmp()
{
    static sEnemyShotSet* pSignPoleSet = nullptr;  // 現在アクティブなショットセット
    static int lastFireCount = 0;                  // 前回発射フレーム

    // 一定間隔で自機狙いに撃ち直す間隔（フレーム）
    const int fireInterval = 150;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 400.0;
        enemy.maxHp = enemy.hp = 200;               // 200で固定
        lastFireCount = 0;
        pSignPoleSet = nullptr;
        player.y = 240;
    }
    else {
        // 敵本体を緩やかに左右へ揺らす（ポールが少し揺れている感じ）
        enemy.x = 240.0 + 18.0 * sin(count * 0.018);
        enemy.y = 400.0 + 6.0 * sin(count * 0.027);
    }

    // 一定時間ごとに古いショットセットの弾生成を停止し、自機狙いで新しいセットを生成
    if (count == 1 || (count - lastFireCount) >= fireInterval) {
        // 古いセットがあれば弾生成をストップ
        if (pSignPoleSet != nullptr) {
            pSignPoleSet->param_i[0] = 1;            // 生成停止フラグ
        }

        // 新しいショットセットを生成（自機狙い）
        pSignPoleSet = new sEnemyShotSet;
        pSignPoleSet->count = 0;
        pSignPoleSet->patternFunc = ShotSignPole;
        pSignPoleSet->x = enemy.x;
        pSignPoleSet->y = enemy.y;
        // 自機への角度を記録（パターンの初期位相に使用）
        pSignPoleSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pSignPoleSet->kind = 0;
        pSignPoleSet->param_i[0] = 0;                // 生成許可
        pSignPoleSet->pEnemyShotHead = new sEnemyShot;
        pSignPoleSet->pEnemyShotHead->prev = pSignPoleSet->pEnemyShotHead;
        pSignPoleSet->pEnemyShotHead->next = pSignPoleSet->pEnemyShotHead;

        // リストに追加
        pSignPoleSet->prev = enemyShotSetHead.prev;
        pSignPoleSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSignPoleSet;
        enemyShotSetHead.prev = pSignPoleSet;

        lastFireCount = count;
    }

    // 現在アクティブなセットの中心を敵に追従させる（停止済みの古いセットは追従させない）
    if (pSignPoleSet != nullptr && pSignPoleSet->param_i[0] == 0) {
        pSignPoleSet->x = enemy.x;
        pSignPoleSet->y = enemy.y;
    }
}