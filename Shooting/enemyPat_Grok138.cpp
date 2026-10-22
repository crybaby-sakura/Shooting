#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ニュートンのゆりかご弾幕
// おもり = 大玉、糸 = 小型弾の列で表現
static void ShotNewtonCradle(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM_BALLS = 5;
    const int NUM_SEG = 12;          // 糸の分割数
    const double LENGTH = 90.0;      // 糸の長さ
    const double SPACING = 42.0;     // おもり間隔
    static double ATTACH_Y;    // 固定点のY
    const double CENTER_X = 240.0;

    sEnemyShot* pEnemyShot;

    // ---------- 初期化（count == 0） ----------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 状態初期化
        pEnemyShotSet->param_i[0] = 0;          // phase
        pEnemyShotSet->param_i[1] = 0;          // サイクル回数
        for (int i = 0; i < NUM_BALLS; i++) {
            pEnemyShotSet->param_d[i] = 0.0;            // angle[i]
            pEnemyShotSet->param_d[5 + i] = 0.0;        // angVel[i]
        }
        pEnemyShotSet->param_d[10] = 0.0;       // 予備
        ATTACH_Y = 55.0;

        // 5個のおもり（大玉）を生成
        for (int i = 0; i < NUM_BALLS; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = CENTER_X + (i - 2) * SPACING;
            pEnemyShot->y = ATTACH_Y + LENGTH;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[3]; // シアン大玉
            pEnemyShot->param_i[0] = 1;          // type: ball
            pEnemyShot->param_i[1] = i;          // ball index
            pEnemyShot->param_i[2] = 0;
            pEnemyShot->margin = 240;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 糸（小型弾）を生成
        for (int i = 0; i < NUM_BALLS; i++) {
            for (int s = 0; s < NUM_SEG; s++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = CENTER_X + (i - 2) * SPACING;
                pEnemyShot->y = ATTACH_Y + LENGTH * (s + 1) / (NUM_SEG + 1);
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白小玉
                pEnemyShot->param_i[0] = 2;      // type: string
                pEnemyShot->param_i[1] = i;      // ball index
                pEnemyShot->param_i[2] = s;      // segment
                pEnemyShot->margin = 240;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---------- 位相管理 ----------
    int phase = pEnemyShotSet->param_i[0];
    int cycle = pEnemyShotSet->param_i[1];
    double* angle = &pEnemyShotSet->param_d[0];
    double* angVel = &pEnemyShotSet->param_d[5];

    // phase 0: 待機（少し待つ）
    if (phase == 0) {
        if (pEnemyShotSet->count >= 40) {
            pEnemyShotSet->param_i[0] = 1; // 次へ
            // 左端を引き上げ開始
            angVel[0] = 0.0;
        }
    }
    // phase 1: 左端おもりをゆっくり引き上げ
    else if (phase == 1) {
        angle[0] -= 0.018; // 左へ（負の角度）
        if (angle[0] <= -0.85) { // 約 -49度
            angle[0] = -0.85;
            pEnemyShotSet->param_i[0] = 2; // 保持へ
        }
    }
    // phase 2: 少し保持
    else if (phase == 2) {
        if (pEnemyShotSet->count % 30 == 0 && pEnemyShotSet->count > 100) {
            pEnemyShotSet->param_i[0] = 3; // 解放
            angVel[0] = 0.045; // 振り下ろし初速
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    // phase 3: 左端が振り下ろし → 衝突 → 右端が飛び出す
    else if (phase == 3) {
        // 左端の運動（簡易）
        angle[0] += angVel[0];
        angVel[0] += 0.0012; // 重力っぽい加速（下向き）

        if (angle[0] >= 0.0) {
            // 衝突発生
            angle[0] = 0.0;
            angVel[0] = 0.0;

            // 右端に速度を渡す
            angVel[NUM_BALLS - 1] = 0.055;
            angle[NUM_BALLS - 1] = 0.0;

            // 衝撃波（短い放射弾）
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            double impactX = CENTER_X + (NUM_BALLS - 1 - 2) * SPACING;
            double impactY = ATTACH_Y + LENGTH;
            for (int k = 0; k < 12*3; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = impactX;
                pEnemyShot->y = impactY;
                pEnemyShot->muki = k * (DX_PI * 2.0 / 12.0/3);
                pEnemyShot->speed = 1.8 + GetRand(80) / 100.0;
                pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄中玉
                pEnemyShot->param_i[0] = 0; // free
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            pEnemyShotSet->param_i[0] = 4; // 右端飛翔へ
        }
    }
    // phase 4: 右端が飛び出す
    else if (phase == 4) {
        angle[NUM_BALLS - 1] += angVel[NUM_BALLS - 1];
        angVel[NUM_BALLS - 1] -= 0.0015; // 減速

        // 最高点付近で弾を放出
        if (angle[NUM_BALLS - 1] >= 0.9 && angVel[NUM_BALLS - 1] < 0.01) {
            double bx = CENTER_X + ((NUM_BALLS - 1) - 2) * SPACING + LENGTH * sin(angle[NUM_BALLS - 1]);
            double by = ATTACH_Y + LENGTH * cos(angle[NUM_BALLS - 1]);

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // 扇状弾幕
            for (int k = 0; k < 15*3; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                pEnemyShot->muki = -0.6 + k * 0.085;
                pEnemyShot->speed = 2.4 + GetRand(60) / 100.0;
                pEnemyShot->kind = img_enemyShotScale[0 + (k % 3)]; // 赤～緑系鱗弾
                pEnemyShot->param_i[0] = 0; // free
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            // 追加で小玉ばらまき
            for (int k = 0; k < 8*3; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                pEnemyShot->muki = GetRand(628) / 100.0; // ほぼ全方向
                pEnemyShot->speed = 1.5 + GetRand(120) / 100.0;
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青小玉
                pEnemyShot->param_i[0] = 0;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 次のサイクル準備（左右反転）
            pEnemyShotSet->param_i[1]++;
            if (cycle >= 4) {
                pEnemyShotSet->param_i[0] = 5; // 終了フェーズ
            }
            else {
                // 左右入れ替えて再開
                // 簡易的に右端を戻して左から再スタート
                angle[NUM_BALLS - 1] = 0.0;
                angVel[NUM_BALLS - 1] = 0.0;
                pEnemyShotSet->param_i[0] = 1;
                // 次は右端を引き上げるように角度符号を反転させるため、ここでは左を再使用
            }
        }
    }
    // phase 5: 終了（おもりをゆっくり消す感じで下へ落とす）
    else if (phase == 5) {
        for (int i = 0; i < NUM_BALLS; i++) {
            angle[i] += 0.008 * (i - 2); // 少し散らす
        }
        if (pEnemyShotSet->count > 700) {
            // 以降は自由弾のみ残す
        }
        ATTACH_Y += 0.5;
    }

    // ---------- 全弾の位置更新 ----------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];
        int idx = pShot->param_i[1];
        int seg = pShot->param_i[2];

        if (type == 1) { // おもり（大玉）
            double ax = CENTER_X + (idx - 2) * SPACING;
            pShot->x = ax + LENGTH * sin(angle[idx]);
            pShot->y = ATTACH_Y + LENGTH * cos(angle[idx]);
            pShot->speed = 0.0;
        }
        else if (type == 2) { // 糸セグメント
            double ax = CENTER_X + (idx - 2) * SPACING;
            double bx = ax + LENGTH * sin(angle[idx]);
            double by = ATTACH_Y + LENGTH * cos(angle[idx]);
            double t = (seg + 1.0) / (NUM_SEG + 1.0);
            pShot->x = ax + (bx - ax) * t;
            pShot->y = ATTACH_Y + (by - ATTACH_Y) * t;
            pShot->speed = 0.0;
        }
        else { // 自由弾（type == 0）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NewtonsCradle_Grok()
{
    static int shot_flag;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_flag = 0;
    }

    // 敵はほぼ静止（わずかに揺れる程度）
    enemy.x = 240.0 + 8.0 * sin(count * 0.03);
    enemy.y = 40.0;

    // ゆりかご弾幕を1回だけセット
    if (count == 30 && shot_flag == 0) {
        shot_flag = 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNewtonCradle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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