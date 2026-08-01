// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：カントリーサイン・フォール（道路標識の倒壊）
static void ShotCountrySignFall(sEnemyShotSet* pEnemyShotSet)
{
    const double ROOT_Y = 480.0;
    const int POLE_LENGTH = 25;       // ポールを構成する弾の数
    const double POLE_SPACE = 15.0;   // 弾の間隔
    const int POLE_NUM = 5;           // ポールの本数
    const double SWAY_AMP = 0.2;      // 揺れの振幅（ラジアン）
    const double FALL_SPEED = 0.015 / 3;  // 倒壊の角速度

    if (pEnemyShotSet->count == 0) {
        // ポール出現時の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ポールのX座標のベース（画面内にバランスよく配置）
        int baseX[POLE_NUM] = { 20, 130, 240, 350, 460 };

        for (int i = 0; i < POLE_NUM; i++) {
            // GetRand(x) は 0〜x までの x+1 種類の整数を返すので、
            // GetRand(30) - 15 で -15〜+15 のブレにする
            int rootX = baseX[i] + GetRand(30) - 15;

            // 倒れる方向をランダムに決定 (1.0 または -1.0)
            double dir = (GetRand(1) == 0) ? 1.0 : -1.0;

            for (int j = 0; j <= POLE_LENGTH; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // 初期配置（真上に伸びた状態）
                pEnemyShot->x = (double)rootX;
                pEnemyShot->y = ROOT_Y - (double)j * POLE_SPACE;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->margin = 200;

                // 一番先端の弾を「標識」、それ以外を「ポール」とする
                if (j == POLE_LENGTH) {
                    // 標識：黄色(1)の大玉
                    pEnemyShot->kind = img_enemyShotLargeBall[1];
                    pEnemyShot->param_d[4] = 1.0; // 標識フラグ                   
                }
                else {
                    // ポール：白(6)の中玉（棒状に見えるように少し大きめの玉を使用）
                    pEnemyShot->kind = img_enemyShotMediumBall[6];
                    pEnemyShot->param_d[4] = 0.0; // ポールフラグ
                }

                // 回転計算用のパラメータを保存
                pEnemyShot->param_d[0] = (double)rootX;          // 根元X座標
                pEnemyShot->param_d[1] = ROOT_Y;                 // 根元Y座標
                pEnemyShot->param_d[2] = (double)j * POLE_SPACE; // 根元からの距離
                pEnemyShot->param_d[3] = dir;                    // 倒壊方向
                pEnemyShot->param_d[5] = 0.0;                    // 標識の飛散済みフラグ

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        return;
    }

    // 倒壊開始時の重い音（1回のみ鳴らす）
    if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 標識飛散時の音（1回のみ鳴らす）
    if (pEnemyShotSet->count == 180 && pEnemyShotSet->param_i[0] == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_i[0] = 1; // 鳴らしたフラグを立てる
    }

    // 現在の角度のオフセットを計算
    double theta_offset = 0.0;
    if (pEnemyShotSet->count < 90) {
        // 揺らぎフェーズ (30フレームで1周期 × 3回 = 90フレームでピタッと真っ直ぐに止まる)
        theta_offset = SWAY_AMP * sin(DX_PI * 2.0 / 30.0 * pEnemyShotSet->count);
    }
    else {
        // 倒壊フェーズ (90フレーム以降、継続的に倒れていく)
        theta_offset = (pEnemyShotSet->count - 90) * FALL_SPEED;
    }

    const double BASE_THETA = -DX_PI / 2.0; // 基準角度（真上）

    // 全弾の座標を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double px = pShot->x;
        double py = pShot->y;

        // 既に飛び出した標識は通常の直線移動を行う
        if (pShot->param_d[5] > 0.5) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
            continue;
        }

        // 根元を中心とした回転座標の計算
        double dir = pShot->param_d[3];
        double dist = pShot->param_d[2];
        double rootX = pShot->param_d[0];
        double rootY = pShot->param_d[1];

        double theta = BASE_THETA + theta_offset * dir;

        pShot->x = rootX + dist * cos(theta);
        pShot->y = rootY + dist * sin(theta);

        // 標識の飛散処理 (倒壊開始から90フレーム後、大きく傾いたタイミングで発射)
        if (pShot->param_d[4] > 0.5 && pEnemyShotSet->count == 180) {
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 4.0;
            pShot->param_d[5] = 1.0; // 飛散済みにする
        }
        else if (pShot->param_d[4] < 0.5 && pEnemyShotSet->count == 180) {
            pShot->muki = atan2(pShot->y - py, pShot->x - px) + (GetRand(100) - 50) / 100.0;
            pShot->speed = 0.5 + GetRand(100) / 200.0;
            pShot->param_d[5] = 1.0;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SignPole_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 240フレームごとにサインポール弾幕を生成
    if (count % 360 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCountrySignFall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}