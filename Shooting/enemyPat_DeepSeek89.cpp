// EnemyPat_JumpScare_DeepSeek.cpp
// ジャンプスケア風弾幕「虚ろなる眼球（ホロウ・アイ）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾追加ヘルパー
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ジャンプスケア弾幕パターン
static void EyeJumpScare(sEnemyShotSet* pSet)
{
    const double CX = pSet->x;   // 眼球中心X
    const double CY = pSet->y;   // 眼球中心Y
    const double PI = DX_PI;

    // 既存弾の移動
    for (sEnemyShot* p = pSet->pEnemyShotHead->next; p != pSet->pEnemyShotHead; p = p->next) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
    }

    int t = pSet->count; // メインルーチンが毎フレーム+1する

    // ---------- フェーズ0：まぶたの輪郭形成（0〜149） ----------
    if (t < 150) {
        // 2フレームごとに楕円の輪郭を上下同時に1点ずつ追加
        if (t % 2 == 0) {
            int idx = t / 2;                 // 0〜74
            double theta = idx * (PI / 75.0); // 0〜PI

            const double RX = 150.0; // 横半径
            const double RY = 80.0;  // 縦半径

            // 上まぶた
            double x1 = CX + RX * cos(theta);
            double y1 = CY - RY * sin(theta);
            AddShot(pSet, x1, y1, 0.0, 0.0, img_enemyShotSmallBall[0]); // 赤

            // 下まぶた
            double x2 = CX + RX * cos(theta);
            double y2 = CY + RY * sin(theta);
            AddShot(pSet, x2, y2, 0.0, 0.0, img_enemyShotSmallBall[0]); // 赤
        }
    }

    // ---------- フェーズ1：開眼＆ジャンプスケア（t=150） ----------
    if (t == 150) {
        // 重い効果音で恐怖を演出
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 白目のリング（中玉・白）
        const int SCLERA_N = 48;
        const double SCLERA_RX = 125.0;
        const double SCLERA_RY = 60.0;
        for (int i = 0; i < SCLERA_N; i++) {
            double a = (2.0 * PI * i) / SCLERA_N;
            double x = CX + SCLERA_RX * cos(a);
            double y = CY + SCLERA_RY * sin(a);
            AddShot(pSet, x, y, 0.0, 0.0, img_enemyShotMediumBall[6]); // 白
        }

        // 瞳孔：中心に大玉・黒＋周囲に小玉・黒
        AddShot(pSet, CX, CY + 1e-5, 0.0, 0.0, img_enemyShotLargeBall[7]); // 黒大玉
        const int PUPIL_N = 12;
        for (int i = 0; i < PUPIL_N; i++) {
            double a = (2.0 * PI * i) / PUPIL_N;
            double r = 14.0;
            double x = CX + r * cos(a);
            double y = CY + r * sin(a);
            AddShot(pSet, x, y, 0.0, 0.0, img_enemyShotSmallBall[7]); // 黒小玉
        }

        // 閃光：外側へ飛び散る白い小玉
        for (int i = 0; i < 12; i++) {
            double a = (2.0 * PI * i) / 12.0;
            AddShot(pSet, CX, CY, a, 3.5, img_enemyShotSmallBall[6]); // 白
        }
    }

    // ---------- フェーズ2：凝視追尾（180〜299） ----------
    if (t >= 180 && t < 300) {
        // 6フレームごとに自機狙いの黒い視線弾を2発
        if (t % 3 == 0) {
            double base = atan2(player.y - CY, player.x - CX);
            AddShot(pSet, CX, CY, base, 7.8, img_enemyShotSmallBall[7]);       // 黒
            AddShot(pSet, CX, CY, base + 0.08, 7.6, img_enemyShotSmallBall[7]);
        }
    }

    // ---------- フェーズ3：眼球破裂（t=300） ----------
    if (t == 300) {
        // 極大音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 静止していた弾（まぶた・白目・瞳孔）を外側へ爆散させる
        for (sEnemyShot* p = pSet->pEnemyShotHead->next; p != pSet->pEnemyShotHead; p = p->next) {
            if (p->speed == 0.0) {
                p->speed = 2.0 + (GetRand(100) / 100.0); // 2.0〜3.0
                p->muki = atan2(p->y - CY, p->x - CX);
            }
        }

        // 中心からの放射状破裂弾
        for (int i = 0; i < 36 * 3; i++) {
            double a = (i * 10.0 / 3) * PI / 180.0;
            int color;
            switch (i % 3) {
            case 0: color = 0; break; // 赤
            case 1: color = 6; break; // 白
            default: color = 7; break; // 黒
            }
            AddShot(pSet, CX, CY, a, 3.0 + (GetRand(100) / 100.0), img_enemyShotSmallBall[color]);
        }
    }
}

// 敵本体パターン
void EnemyPat_JumpScare_DeepSeek()
{
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 固定
        shot_count = 0;
    }
    else {
        // 敵本体はゆっくり左右に動くだけ
        static int muki = 1;
        enemy.x += 0.5 * muki;
        if (count % 180 == 90) muki *= -1;
    }

    // ジャンプスケア用ショットセットを1回だけ生成
    if (count % 360 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = EyeJumpScare;
        pEnemyShotSet->x = 240.0;   // 眼球中心X
        pEnemyShotSet->y = 200.0;   // 眼球中心Y
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