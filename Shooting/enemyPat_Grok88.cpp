// enemyPat_Tmp.cpp
// 蚊取り線香をモチーフにした弾幕（螺旋煙）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：蚊取り線香スパイラルスモーク
// 使用素材：
//   線香本体 → 中玉・橙 (img_enemyShotMediumBall[8])
//   煙部分   → 小玉・白 (img_enemyShotSmallBall[6])
// 効果音：sound_enemyShot_medium
static void ShotSpiralSmoke(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 螺旋パラメータ（r = a + b * theta）
        const double a = 18.0;               // 最小半径
        const double b = 7.5;                // 螺旋の広がり
        const int turns = 3 + 1;                 // 巻き数
        const double max_theta = turns * 2.0 * DX_PI;
        const int num_bullets = 72 * 10;          // 弾数（密度調整）
        const double d_theta = max_theta / (num_bullets - 1);

        for (int i = 0; i < num_bullets; i++) {
            pEnemyShot = new sEnemyShot;

            double theta = i * d_theta;
            double r = a + b * theta;

            // 初期位置（螺旋上）
            pEnemyShot->x = pEnemyShotSet->x + r * cos(theta);
            pEnemyShot->y = pEnemyShotSet->y + r * sin(theta);
            pEnemyShot->muki = theta;
            pEnemyShot->speed = 0.0;

            // パラメータ
            // param_d[0] : 元のtheta（燃焼順序判定用・外側ほど大きい）
            // param_d[1] : 半径
            // param_d[2] : 回転オフセット
            // param_i[0] : 0=線香モード / 1=煙モード
            pEnemyShot->param_d[0] = theta;
            pEnemyShot->param_d[1] = r;
            pEnemyShot->param_d[2] = 0.0;
            pEnemyShot->param_i[0] = 0;

            // 線香本体は中玉・橙
            pEnemyShot->kind = img_enemyShotMediumBall[8];
            pEnemyShot->margin = 240;

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // セット側パラメータ
        // param_d[0] : 現在の燃焼theta（外側から減っていく）
        // param_d[1] : 螺旋の回転角速度
        pEnemyShotSet->param_d[0] = max_theta;
        pEnemyShotSet->param_d[1] = 0.018;   // ゆっくり回転
    }

    // --- 毎フレーム更新 ---
    const double center_x = pEnemyShotSet->x;
    const double center_y = pEnemyShotSet->y;
    const double rot_speed = pEnemyShotSet->param_d[1];
    double& burn_theta = pEnemyShotSet->param_d[0];

    // ある程度形が固まってから外側から燃焼開始
    if (pEnemyShotSet->count > 45) {
        burn_theta -= 0.042;
        if (burn_theta < 0.0) burn_theta = 0.0;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 線香モード
            if (pShot->param_d[0] > burn_theta) {
                // 外側から煙に変化
                pShot->param_i[0] = 1;

                // 放射方向へ
                double dx = pShot->x - center_x;
                double dy = pShot->y - center_y;
                pShot->muki = atan2(dy, dx);
                // 少しランダムに散らす
                pShot->muki += (GetRand(24) - 12) / 180.0 * DX_PI;
                pShot->speed = 0.65 + GetRand(50) / 100.0;

                // 煙は小玉・白
                pShot->kind = img_enemyShotSmallBall[6];
            }
            else {
                // 螺旋ごとゆっくり回転
                pShot->param_d[2] += rot_speed;
                double angle = pShot->param_d[0] + pShot->param_d[2];
                double r = pShot->param_d[1];
                pShot->x = center_x + r * cos(angle);
                pShot->y = center_y + r * sin(angle);
            }
        }
        else {
            // 煙モード：外側へゆっくり拡散
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_MosquitoCoil_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で螺旋を発生（長めの間隔で重なりすぎを防止）
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiralSmoke;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
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