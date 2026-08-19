// enemyPat_Tmp.cpp
// 布団吹雪（ふとんふぶき）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾追加用ヘルパー
static void AddBullet(sEnemyShotSet* pSet, double x, double y, double muki, double speed,
    int kind, int role, int p1, int p2, double baseX, double baseY)
{
    sEnemyShot* shot = new sEnemyShot;
    shot->x = x;
    shot->y = y;
    shot->muki = muki;
    shot->speed = speed;
    shot->kind = kind;
    shot->param_i[0] = role;
    shot->param_i[1] = p1;
    shot->param_i[2] = p2;
    shot->param_d[0] = baseX;
    shot->param_d[1] = baseY;

    shot->prev = pSet->pEnemyShotHead->prev;
    shot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = shot;
    pSet->pEnemyShotHead->prev = shot;
}

// 布団吹雪パターン
static void FutonBlizzard(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 布団本体のグリッド設定
        const double spacingX = 24.0;
        const double spacingY = 24.0;
        const int cols = 7 + 2;
        const int rows = 9 + 3;
        double originX = pSet->x;
        double originY = pSet->y;
        double startX = originX - (cols - 1) / 2.0 * spacingX;
        double startY = originY - (rows - 1) / 2.0 * spacingY;

        // 布団本体：白大玉
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                double bx = startX + c * spacingX;
                double by = startY + r * spacingY;
                AddBullet(pSet, bx, by, 0.0, 0.0,
                    img_enemyShotLargeBall[6], 0, c, r,
                    bx - originX, by - originY);
            }
        }

        // 布団の縁：赤小玉
        double minX = startX - spacingX / 2.0;
        double maxX = startX + (cols - 1) * spacingX + spacingX / 2.0;
        double minY = startY - spacingY / 2.0;
        double maxY = startY + (rows - 1) * spacingY + spacingY / 2.0;

        // 上下
        for (int c = 0; c < cols; c++) {
            double bx = startX + c * spacingX;
            AddBullet(pSet, bx, minY, 0.0, 0.0,
                img_enemyShotSmallBall[0], 1, c, 0,
                bx - originX, minY - originY);
            AddBullet(pSet, bx, maxY, 0.0, 0.0,
                img_enemyShotSmallBall[0], 1, c, 1,
                bx - originX, maxY - originY);
        }

        // 左右（四隅は重複するが許容）
        for (int r = 0; r < rows; r++) {
            double by = startY + r * spacingY;
            AddBullet(pSet, minX, by, 0.0, 0.0,
                img_enemyShotSmallBall[0], 1, -1, r,
                minX - originX, by - originY);
            AddBullet(pSet, maxX, by, 0.0, 0.0,
                img_enemyShotSmallBall[0], 1, cols, r,
                maxX - originX, by - originY);
        }

        // 縫い目：青銃弾（水平・垂直のステッチ）
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols - 1; c++) {
                double bx = startX + (c + 0.5) * spacingX;
                double by = startY + r * spacingY;
                AddBullet(pSet, bx, by, 0.0, 0.0,
                    img_enemyShotBullet[4], 2, c, r,
                    bx - originX, by - originY);
            }
        }
        for (int c = 0; c < cols; c++) {
            for (int r = 0; r < rows - 1; r++) {
                double bx = startX + c * spacingX;
                double by = startY + (r + 0.5) * spacingY;
                AddBullet(pSet, bx, by, 0.0, 0.0,
                    img_enemyShotBullet[4], 2, c, r,
                    bx - originX, by - originY);
            }
        }
    }
    else if (pSet->count < 60) {
        // 展開フェーズ：布団を敷いた状態を維持
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] != 3) {
                p->x = pSet->x + p->param_d[0];
                p->y = pSet->y + p->param_d[1];
            }
            p = p->next;
        }
    }
    else if (pSet->count < 180) {
        // はためきフェーズ：風で揺れる
        double t = pSet->count;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] != 3) {
                double baseX = pSet->x + p->param_d[0];
                double baseY = pSet->y + p->param_d[1];
                double amp = 0.0;
                double freqX = 0.0;
                double freqY = 0.0;
                double phase = 0.0;

                if (p->param_i[0] == 0) { // 本体
                    int col = p->param_i[1];
                    int row = p->param_i[2];
                    amp = 6.0;
                    freqX = 0.15;
                    freqY = 0.12;
                    phase = (col + row) * 0.8;
                }
                else if (p->param_i[0] == 1) { // 縁
                    amp = 10.0;
                    freqX = 0.20;
                    freqY = 0.17;
                    phase = (p->param_i[1] + p->param_i[2]) * 1.3;
                }
                else if (p->param_i[0] == 2) { // 縫い目
                    amp = 4.0;
                    freqX = 0.18;
                    freqY = 0.14;
                    phase = (p->param_i[1] + p->param_i[2]) * 1.1;
                }

                p->x = baseX + amp * sin(t * freqX + phase);
                p->y = baseY + amp * sin(t * freqY + phase + 1.0);
            }
            p = p->next;
        }
    }
    else {
        // 吹っ飛びフェーズ
        if (pSet->count == 180) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 既存弾に外向きの速度を与える
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                if (p->param_i[0] != 3) {
                    double dx = p->x - pSet->x;
                    double dy = p->y - pSet->y;
                    double dist = sqrt(dx * dx + dy * dy);
                    if (dist < 0.01) dist = 0.01;

                    double speed = 0.0;
                    if (p->param_i[0] == 0) {
                        speed = (120.0 + GetRand(100)) / 100.0; // 1.2～2.2
                    }
                    else if (p->param_i[0] == 1) {
                        speed = (180.0 + GetRand(120)) / 100.0; // 1.8～3.0
                    }
                    else if (p->param_i[0] == 2) {
                        speed = (220.0 + GetRand(150)) / 100.0; // 2.2～3.7
                    }
                    p->muki = atan2(dy, dx);
                    p->speed = speed;
                }
                p = p->next;
            }

            // 綿ぼこり：黄小玉を追加
            for (int i = 0; i < 16; i++) {
                double angle = (double)i / 16.0 * 2.0 * DX_PI + (GetRand(100) - 50) / 100.0;
                double dist = 10.0 + GetRand(30);
                double bx = pSet->x + cos(angle) * dist;
                double by = pSet->y + sin(angle) * dist;
                double speed = (80.0 + GetRand(120)) / 100.0;
                AddBullet(pSet, bx, by, angle + (GetRand(40) - 20) / 180.0 * DX_PI,
                    speed, img_enemyShotSmallBall[1], 3, 0, 0,
                    bx - pSet->x, by - pSet->y);
            }
        }
        else {
            // 速度に従って移動
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
                p = p->next;
            }
        }
    }
}

// 敵本体パターン
void EnemyPat_FutonFlewAway_DeepSeek()
{
    static int muki = 1;
    static int nextSpawn = 60;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        nextSpawn = 60;
    }
    else {
        // 左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) muki *= -1;
        enemy.y = 140.0 + 20.0 * sin(count / 80.0);
    }

    // 一定間隔で布団吹雪を発射
    if (count == nextSpawn) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = FutonBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        nextSpawn += 240;
    }
}