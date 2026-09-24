// enemyPat_rose.cpp
//
// バラ曲線をモチーフにした弾幕パターン
// 薔薇符「五瓣輪舞 -Pentagram Rose-」
//
// r = A * cos(5θ) で表されるバラ曲線に沿って弾を配置し、
// 時間差で位相をずらしながら多重に描くことで、
// 回転しながら開いていく薔薇の花を表現する。
//
// 避け方：花弁と花弁の間の「谷」を縫って移動する。
// 花弁が回転しているため、回転方向に合わせて円を描くように避ける。
//
// ※ img_enemyShot* と sound_* は既存のヘッダ等で宣言されている前提。

#include "gv.h"
#include <cmath>

// ------------------------------------------------------------
// バラ曲線の弾幕パターン
// ------------------------------------------------------------
static void ShotRoseCurve(sEnemyShotSet* pEnemyShotSet)
{
    const double PI = DX_PI;
    const double A = 150.0;          // 花弁の長さ
    const int    K = 5;              // 花弁の数（奇数）
    const int    N = 72;             // 1波あたりのサンプル数
    const int    MAX_WAVES = 8*3;      // 波の数
    const int    WAVE_INTERVAL = 6;  // 波を打つ間隔（フレーム）

    int wave = pEnemyShotSet->count / WAVE_INTERVAL;

    // ---- 新しい波を発生 ----
    if (pEnemyShotSet->count % WAVE_INTERVAL == 0 && wave < MAX_WAVES) {
        // 予告音（最初の波のみ）
        if (wave == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 回転方向：セットごとに交互に
        double rotDir = (pEnemyShotSet->param_i[0] == 0) ? 1.0 : -1.0;
        // 波ごとに少しずつ位相をずらす
        double phase = rotDir * wave * PI / 48.0;

        for (int i = 0; i < N; i++) {
            double theta = 2.0 * PI * i / N;
            double r = A * cos(K * theta);

            // 中心付近は弾を出さない（理不尽な密集を避ける）
            if (fabs(r) < 10.0) continue;

            double ang = theta + phase;
            double px = r * cos(ang);
            double py = r * sin(ang);

            double wx = pEnemyShotSet->x + px;
            double wy = pEnemyShotSet->y + py;

            // 画面外ならスキップ
            if (wx < -30.0 || wx > 510.0 || wy < -30.0 || wy > 510.0) continue;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = wx;
            pShot->y = wy;

            // 中心から見た外向き方向
            double outDir = atan2(py, px);

            // 花弁の先端ほど速く
            double speed = 1.5 + 2.8 * fabs(r) / A;

            // 接線方向の速度（花弁が回転しながら開く）
            double tangSpeed = 0.7 * (r / A) * rotDir;
            double tangDir = outDir + PI / 2.0;

            double vx = speed * cos(outDir) + tangSpeed * cos(tangDir);
            double vy = speed * sin(outDir) + tangSpeed * sin(tangDir);

            pShot->muki = atan2(vy, vx);
            pShot->speed = sqrt(vx * vx + vy * vy);

            // 中心からの距離で色を変える
            double ratio = fabs(r) / A;
            int color;
            if (ratio > 0.75)      color = 0;  // 赤
            else if (ratio > 0.45) color = 5;  // マゼンタ
            else                   color = 6;  // 白

            // 花弁の先端は大きめの弾、根本は鱗弾で花芯を表現
            if (ratio > 0.85) {
                pShot->kind = img_enemyShotMediumBall[color];
            }
            else if (ratio > 0.5) {
                pShot->kind = img_enemyShotSmallBall[color];
            }
            else {
                pShot->kind = img_enemyShotScale[color];
            }

            // リストに挿入
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 波ごとの効果音
        if (wave > 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // ---- 弾の移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_RoseCurve_DeepSeek()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;             // 画面中央に配置
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }

    // 3秒ごとに新しい薔薇を発生
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRoseCurve;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->param_i[0] = shot_count % 2;  // 回転方向
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