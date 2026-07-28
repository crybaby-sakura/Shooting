// enemyPat_Tmp.cpp
// アリジゴクモチーフ弾幕「漏斗陥穽（ろうとかんせい）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// フェーズ1：螺旋斜面（誘導フェーズ）
// 外縁から中心へ向かう螺旋弾で漏斗の壁を形成
// ------------------------------------------------------------
static void ShotSpiralWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 48;
        const double radius = 300.0;
        const double cx = 240.0;
        const double cy = 240.0;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI * i) / num;
            pEnemyShot->x = cx + radius * cos(angle);
            pEnemyShot->y = cy + radius * sin(angle);

            double toCenter = atan2(cy - pEnemyShot->y, cx - pEnemyShot->x);
            pEnemyShot->muki = toCenter;
            pEnemyShot->speed = 1.0;

            // 中玉・橙：土・砂をイメージ
            pEnemyShot->kind = img_enemyShotMediumBall[8];

            pEnemyShot->param_d[0] = cx;      // 中心X
            pEnemyShot->param_d[1] = cy;      // 中心Y
            pEnemyShot->param_d[2] = 0.6;     // 螺旋強度（接線速度）

            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double cx = pShot->param_d[0];
        double cy = pShot->param_d[1];
        double dx = cx - pShot->x;
        double dy = cy - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);
        double toCenter = atan2(dy, dx);
        double spiral = pShot->param_d[2] * (dist / 200.0 + 0.3);

        // 中心方向 + 接線方向（時計回り）
        double vx = pShot->speed * cos(toCenter) - spiral * sin(toCenter);
        double vy = pShot->speed * sin(toCenter) + spiral * cos(toCenter);

        pShot->x += vx;
        pShot->y += vy;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ2：砂嵐（視界妨害・心理誘導）
// 画面全体に低速の細かい弾（砂粒）を降らせる
// ------------------------------------------------------------
static void ShotSandstorm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 150; i++) {
            pEnemyShot = new sEnemyShot;
            while (true) {
                pEnemyShot->x = GetRand(520) - 20;
                pEnemyShot->y = GetRand(520) - 20;
                if (hypot(pEnemyShot->x - player.x, pEnemyShot->y - player.y) > 20) break;
            }
            // 下方向中心に±30度のばらつき
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = (30 + GetRand(100)) / 100.0; // 0.3〜1.3

            // 小玉・黄または橙
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ3：滑落加速（収束フェーズ）
// 螺旋弾を高速化。上方向に1箇所だけ出口（隙間）を開ける
// ------------------------------------------------------------
static void ShotSpiralWallFast(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int num = 40;
        const double radius = 300.0;
        const double cx = 240.0;
        const double cy = 240.0;
        // 出口：上方向（-PI/2）に±20度の隙間
        double gapCenter = -DX_PI / 2.0;
        double gapHalf = DX_PI / 9.0;

        for (int i = 0; i < num; i++) {
            double angle = (2.0 * DX_PI * i) / num;
            double diff = angle - gapCenter;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            if (fabs(diff) < gapHalf) continue;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx + radius * cos(angle);
            pEnemyShot->y = cy + radius * sin(angle);

            double toCenter = atan2(cy - pEnemyShot->y, cx - pEnemyShot->x);
            pEnemyShot->muki = toCenter;
            pEnemyShot->speed = 2.2;

            // 中玉・赤：警告色
            pEnemyShot->kind = img_enemyShotMediumBall[0];

            pEnemyShot->param_d[0] = cx;
            pEnemyShot->param_d[1] = cy;
            pEnemyShot->param_d[2] = 1.2;

            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double cx = pShot->param_d[0];
        double cy = pShot->param_d[1];
        double dx = cx - pShot->x;
        double dy = cy - pShot->y;
        double dist = sqrt(dx * dx + dy * dy);
        double toCenter = atan2(dy, dx);
        double spiral = pShot->param_d[2] * (dist / 200.0 + 0.3);

        double vx = pShot->speed * cos(toCenter) - spiral * sin(toCenter);
        double vy = pShot->speed * sin(toCenter) + spiral * cos(toCenter);

        pShot->x += vx;
        pShot->y += vy;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ4：底部の顎（捕獲フェーズ・挟撃）
// 2つの大玉が自機を狙って左右から挟む
// ------------------------------------------------------------
static void ShotJaw(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = -1; i <= 1; i++) {
            // 左顎
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = -20.0;
            pEnemyShot->y = player.y + i * 40;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 右顎
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 500.0;
            pEnemyShot->y = player.y + i * 40;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ4続き：砂噴射（捕獲フェーズ・放射）
// 中心から全方位へ砂を噴き上げる
// ------------------------------------------------------------
static void ShotSandBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int num = 128;
        const double cx = 240.0;
        const double cy = 240.0;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = (2.0 * DX_PI * i) / num + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = (120 + GetRand(200)) / 100.0; // 1.2〜3.2

            // 鱗弾・黄、菱形弾・橙を交互に
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotScale[1];
            }
            else {
                pEnemyShot->kind = img_enemyShotDiamond[8];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体：アリジゴクモチーフ「漏斗陥穽」
// ------------------------------------------------------------
void EnemyPat_Antlion_Kimi()
{
    static int phase;
    static int phase_count;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 250;
        phase = 0;
        phase_count = 0;
        shot_count = 0;
    }

    // アリジゴクの待機：微かな揺れ
    enemy.x = 240.0 + sin(count / 60.0) * 4.0;
    enemy.y = 80.0 + cos(count / 45.0) * 3.0;

    // フェーズ遷移
    phase_count++;

    // フェーズ0: 螺旋斜面（誘導）
    if (phase == 0 && phase_count == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSpiralWall;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        phase = 1;
        phase_count = 0;
    }
    // フェーズ1: 砂嵐（視界妨害）
    else if (phase == 1 && phase_count == 180) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSandstorm;
        pSet->x = 0.0;
        pSet->y = 0.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        phase = 2;
        phase_count = 0;
    }
    // フェーズ2: 滑落加速（収束・出口あり）
    else if (phase == 2 && phase_count == 180) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSpiralWallFast;
        pSet->x = 240.0;
        pSet->y = 240.0;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        phase = 3;
        phase_count = 0;
    }
    // フェーズ3: 底部の顎＋砂噴射（捕獲）
    else if (phase == 3 && phase_count == 120) {
        // 顎
        sEnemyShotSet* pSet1 = new sEnemyShotSet;
        pSet1->count = 0;
        pSet1->patternFunc = ShotJaw;
        pSet1->x = 240.0;
        pSet1->y = 240.0;
        pSet1->muki = 0.0;
        pSet1->kind = shot_count++;

        pSet1->pEnemyShotHead = new sEnemyShot;
        pSet1->pEnemyShotHead->prev = pSet1->pEnemyShotHead;
        pSet1->pEnemyShotHead->next = pSet1->pEnemyShotHead;

        pSet1->prev = enemyShotSetHead.prev;
        pSet1->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet1;
        enemyShotSetHead.prev = pSet1;

        // 砂噴射
        sEnemyShotSet* pSet2 = new sEnemyShotSet;
        pSet2->count = 0;
        pSet2->patternFunc = ShotSandBurst;
        pSet2->x = 240.0;
        pSet2->y = 240.0;
        pSet2->muki = 0.0;
        pSet2->kind = shot_count++;

        pSet2->pEnemyShotHead = new sEnemyShot;
        pSet2->pEnemyShotHead->prev = pSet2->pEnemyShotHead;
        pSet2->pEnemyShotHead->next = pSet2->pEnemyShotHead;

        pSet2->prev = enemyShotSetHead.prev;
        pSet2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet2;
        enemyShotSetHead.prev = pSet2;

        phase = 4;
        phase_count = 0;
    }
    // フェーズ4: 待機後、次のサイクルへ
    else if (phase == 4 && phase_count == 180) {
        phase = 0;
        phase_count = 0;
    }
}