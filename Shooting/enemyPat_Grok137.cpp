#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  永遠の迷宮（Eien no Meikyū）
//  超高密度・自己相似的多層弾幕
//  - 多腕スパイラルが異なる角速度で重なる
//  - 回転する狭いギャップ付きリングが層を成す
//  - 弾に微細な角速度を持たせ、安全地帯を絶えず変形させる
//  - 小玉主体で密度を稼ぎ、視認性のために色を層ごとに分離
// ============================================================

// 共通：弾を生成してリストに繋ぐヘルパー
static sEnemyShot* CreateShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    // param_d[0] : 角速度（rad/frame）
    // param_d[1] : 加速度（任意）
    p->param_d[0] = 0.0;
    p->param_d[1] = 0.0;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ------------------------------------------------------------
// 層1：多腕スパイラル（主に赤・橙系小玉）
// 腕数が徐々に増え、角速度が時間とともに変化する
// ------------------------------------------------------------
static void ShotFractalSpiral(sEnemyShotSet* pSet)
{
    // 生成タイミング
    if (pSet->count % 3 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        if (pSet->count % 15 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        const int arms = 7 + (pSet->count / 600) % 5;          // 7〜11腕
        const double baseAngle = pSet->count * 0.017;          // 全体回転
        const double speed = 1.35 + 0.15 * sin(pSet->count * 0.01);

        for (int i = 0; i < arms; ++i) {
            double ang = baseAngle + i * (DX_PI * 2.0 / arms);
            // 二重螺旋にするため少し位相をずらした弾も出す
            for (int k = 0; k < 2; ++k) {
                double a = ang + k * 0.12;
                sEnemyShot* p = CreateShot(pSet, pSet->x, pSet->y, a, speed, img_enemyShotSmallBall[k == 0 ? 0 : 8]); // 赤 / 橙
                // 角速度を与えて軌道を徐々に曲げる（安全地帯を変形させる）
                p->param_d[0] = 0.0045 * (k == 0 ? 1.0 : -1.0) * (1.0 + 0.3 * sin(pSet->count * 0.005));
            }
        }
    }

    // 弾の更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 層2：回転ギャップ付き高密度リング（シアン・青・白）
// ギャップがゆっくり回転し、複数リングが重なって迷路状になる
// ------------------------------------------------------------
static void ShotRotatingGapRings(sEnemyShotSet* pSet)
{
    // 一定間隔でリングを放出
    if (pSet->count % 28 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int bulletsPerRing = 48;
        const double gapWidth = 0.38;          // ギャップの角度幅（狭い）
        const double ringSpeed = 1.1 + 0.25 * (pSet->count % 3);
        const double gapCenter = pSet->count * 0.009;  // ギャップの回転

        for (int i = 0; i < bulletsPerRing; ++i) {
            double ang = i * (DX_PI * 2.0 / bulletsPerRing);
            // ギャップ部分は弾を出さない
            double diff = fmod(ang - gapCenter + DX_PI * 3.0, DX_PI * 2.0) - DX_PI;
            if (fabs(diff) < gapWidth * 0.5) continue;

            int color;
            if (i % 3 == 0)      color = 3; // シアン
            else if (i % 3 == 1) color = 4; // 青
            else                 color = 6; // 白

            sEnemyShot* p = CreateShot(pSet, pSet->x, pSet->y, ang, ringSpeed, img_enemyShotSmallBall[color]);
            // わずかな角速度でギャップがさらに歪む
            p->param_d[0] = 0.0018 * sin(ang * 3.0 + pSet->count * 0.002);
        }
    }

    // 弾の更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 層3：低速大玉＋中玉の「壁」層（視認性の高い障害物）
// プレイヤーの移動を強制し、狭い隙間を通らざるを得なくする
// ------------------------------------------------------------
static void ShotSlowWalls(sEnemyShotSet* pSet)
{
    if (pSet->count % 45 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 大玉を放射状に少数
        const int largeNum = 9;
        double base = pSet->count * 0.013;
        for (int i = 0; i < largeNum; ++i) {
            double ang = base + i * (DX_PI * 2.0 / largeNum);
            sEnemyShot* p = CreateShot(pSet, pSet->x, pSet->y, ang, 0.75, img_enemyShotLargeBall[5]); // マゼンタ大玉
            p->param_d[0] = 0.0022 * ((i % 2) ? 1.0 : -1.0);
        }

        // 中玉で隙間を埋める
        const int midNum = 18;
        for (int i = 0; i < midNum; ++i) {
            double ang = base + 0.17 + i * (DX_PI * 2.0 / midNum);
            CreateShot(pSet, pSet->x, pSet->y, ang, 0.95, img_enemyShotMediumBall[2]); // 緑中玉
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 層4：プレイヤー方向への微細拡散弾（常時圧力）
// 完全に逃げ道を塞がないよう、ごく狭い扇形
// ------------------------------------------------------------
static void ShotMicroAimed(sEnemyShotSet* pSet)
{
    if (pSet->count % 8 == 0) {
        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        const int ways = 5;
        const double spread = 0.22;

        for (int i = 0; i < ways; ++i) {
            double ang = aim + (i - (ways - 1) / 2.0) * spread;
            sEnemyShot* p = CreateShot(pSet, pSet->x, pSet->y, ang, 2.4, img_enemyShotSmallBall[6]); // 白小玉
            // わずかに外側へ開く角速度
            p->param_d[0] = 0.0035 * (i - (ways - 1) / 2.0);
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_TenYears_Grok()
{
    static int phase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止
        phase = 0;
    }
    else {
        // ゆっくり左右に揺れる（逃げ道を作らない程度）
        enemy.x = 240.0 + 55.0 * sin(count * 0.008);
        enemy.y = 60.0 + 12.0 * sin(count * 0.013);
    }

    // 各層を独立したショットセットとして常時展開
    // 開始タイミングをずらして密度の立ち上がりを調整

    // 層1：多腕スパイラル（最序盤から）
    if (count == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFractalSpiral;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 層2：回転ギャップリング
    if (count == 90) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRotatingGapRings;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 1;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 層3：低速壁
    if (count == 150) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSlowWalls;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 2;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 層4：微細狙い弾（中盤以降の圧力）
    if (count == 240) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMicroAimed;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 3;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ショットセットの発生位置を敵に追従させる
    // （既に生成済みのセットの x/y を毎フレーム更新）
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet = pSet->next;
    }
}