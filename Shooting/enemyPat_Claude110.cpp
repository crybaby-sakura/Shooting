// enemyPat_ringToss.cpp
// 弾幕：輪投げ
//
// 使用素材:
//   img_enemyShotMediumBall … 輪本体を構成する弾（輪の外周に並べる）
//   img_enemyShotSmallBall  … 的（ペグ）の柱を構成するビーズ弾／着弾バースト弾
//   専用の「輪」画像は存在しないため、中玉弾を円周上に並べて疑似的に輪の形を表現する。

#include <cmath>

// ============================================================
//  パラメータ
// ============================================================
static const int    PEG_COUNT = 4;                                   // 的の本数
static const double PEG_X[PEG_COUNT] = { 80.0-10, 180.0-10, 300.0+10, 400.0+10 };       // 的のx座標
static const double PEG_BASE_Y = 440.0;                               // 的の根元y座標
static const int    PEG_BEAD_COUNT = 5;                                   // 的1本あたりのビーズ数
static const double PEG_BEAD_SPACING = 12.0;                                // ビーズの間隔

static const int    RING_BULLET_COUNT = 20;                                  // 輪1つを構成する弾数
static const int    RING_SPAWN_INTERVAL = 40;                                  // 輪の投擲間隔（フレーム）
static const int    RING_FLIGHT_FRAMES = 160;                                 // 投げてから的に到達するまでのフレーム数
static const double RING_RADIUS_BASE = 40.0;                                // 輪の半径（基本値）
static const double RING_RADIUS_PULSE = 6.0;                                 // 輪の半径の脈動幅
static const double RING_SPIN_SPEED = 0.05;                                // 輪の自転速度（ラジアン/フレーム）
static const double ARC_HEIGHT = 170.0;                               // 放物線の山の高さ

static const int    BURST_BULLET_COUNT = 24;                                  // 命中バーストの弾数
static const double BURST_SPEED = 3.5;                                 // 命中バーストの基本速度


// ============================================================
//  共通ヘルパー
// ============================================================

// 新しい sEnemyShotSet を生成し、enemyShotSetHead に連結する
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc patternFunc, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    for (int i = 0; i < 16; i++) {
        pEnemyShotSet->param_i[i] = 0;
        pEnemyShotSet->param_d[i] = 0.0;
    }

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// 新しい sEnemyShot を生成し、指定した ShotSet の弾リスト末尾に連結する
static sEnemyShot* AppendShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
    return pShot;
}


// ============================================================
//  弾幕：着弾バースト（輪が的に入った瞬間の演出）
// ============================================================
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    const int N = BURST_BULLET_COUNT;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            double angle = 2.0 * DX_PI * i / N + (GetRand(100) - 50) / 100.0 * (DX_PI / N);
            double speed = BURST_SPEED + GetRand(150) / 100.0;

            pShot->param_d[0] = pEnemyShotSet->x; // 発生点x
            pShot->param_d[1] = pEnemyShotSet->y; // 発生点y
            pShot->param_d[2] = angle;             // 飛翔角度
            pShot->param_d[3] = speed;             // 飛翔速度
            pShot->kind = img_enemyShotSmallBall[pEnemyShotSet->kind % 9];
            pShot->muki = angle;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
        }
    }

    double t = (double)pEnemyShotSet->count;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[2];
        double speed = pShot->param_d[3];
        pShot->x = pShot->param_d[0] + speed * t * cos(angle);
        pShot->y = pShot->param_d[1] + speed * t * sin(angle);
        pShot = pShot->next;
    }
}


// ============================================================
//  弾幕：的（ペグ）の柱 - 動かず明滅するビーズの列
// ============================================================
static void ShotPegPole(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < PEG_BEAD_COUNT; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            pShot->param_i[0] = i;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y - (double)i * PEG_BEAD_SPACING;
            pShot->muki = 0.0;
            pShot->kind = img_enemyShotSmallBall[(pEnemyShotSet->kind + i) % 9];
        }
    }

    // 常時明滅させて視認性を高める（位置は固定のまま）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int color = (pEnemyShotSet->count / 6 + pShot->param_i[0]) % 9;
        pShot->kind = img_enemyShotSmallBall[color];
        pShot = pShot->next;
    }
}


// ============================================================
//  弾幕：輪投げ - 輪（リング状に並んだ弾）が放物線を描いて的へ飛ぶ
// ============================================================
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    const int    N = RING_BULLET_COUNT;
    const double startX = pEnemyShotSet->param_d[4];
    const double startY = pEnemyShotSet->param_d[5];
    const double targetX = pEnemyShotSet->param_d[0];
    const double targetY = pEnemyShotSet->param_d[1];
    const double spinDir = pEnemyShotSet->param_d[6]; // +1.0 or -1.0

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < N; i++) {
            sEnemyShot* pShot = AppendShot(pEnemyShotSet);
            pShot->param_d[0] = 2.0 * DX_PI * i / N; // 輪上でのベース角度
            pShot->kind = img_enemyShotMediumBall[pEnemyShotSet->kind % 9];
            pShot->x = startX;
            pShot->y = startY;
            pShot->muki = 0.0;
            pShot->margin = 120;
        }
    }

    double t = (double)pEnemyShotSet->count;
    double frac = t / RING_FLIGHT_FRAMES; // 1.0を超えても止めない（的を通り過ぎて飛んでいく）
    double fracForArc = frac;
    if (fracForArc < 0.0) fracForArc = 0.0;
    if (fracForArc > 1.0) fracForArc = 1.0;

    // 輪全体の中心座標：横は等速、縦は山なりカーブ（的到達時にちょうど頂点の効果が0になる）
    double centerX = startX + (targetX - startX) * frac;
    double centerY = startY + (targetY - startY) * frac - ARC_HEIGHT * sin(DX_PI * fracForArc);

    double radius = RING_RADIUS_BASE + RING_RADIUS_PULSE * sin(t * 0.15); // 輪がわずかに脈動
    double spin = t * RING_SPIN_SPEED * spinDir;                       // 輪の自転

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0] + spin;
        pShot->x = centerX + radius * cos(angle);
        pShot->y = centerY + radius * sin(angle);
        pShot->muki = angle + DX_PI / 2.0 * spinDir; // 回転方向に沿った向き
        pShot = pShot->next;
    }

    // 的への到達判定：一度だけ着弾バーストを発生させる
    if (pEnemyShotSet->param_i[0] == 0 && frac >= 1.0) {
        pEnemyShotSet->param_i[0] = 1;
        CreateShotSet(ShotBurst, targetX, targetY, 0.0, pEnemyShotSet->kind);
    }
}


// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_RingToss_Claude()
{
    static int muki;
    static int ringColorCounter;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        ringColorCounter = 0;

        // 的（ペグ）を画面下部に設置
        for (int i = 0; i < PEG_COUNT; i++) {
            CreateShotSet(ShotPegPole, PEG_X[i], PEG_BASE_Y, 0.0, i);
        }
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 輪を一定間隔で連続投擲する
    if (count % RING_SPAWN_INTERVAL == 1) {
        int targetIndex = GetRand(PEG_COUNT - 1);

        double startX = enemy.x;
        double startY = enemy.y + 10.0;
        double targetX = PEG_X[targetIndex];
        double targetY = PEG_BASE_Y - (double)(PEG_BEAD_COUNT - 1) * PEG_BEAD_SPACING / 2.0;

        sEnemyShotSet* pRing = CreateShotSet(ShotRing, startX, startY, 0.0, ringColorCounter % 9);
        pRing->param_d[0] = targetX;
        pRing->param_d[1] = targetY;
        pRing->param_d[4] = startX;
        pRing->param_d[5] = startY;
        pRing->param_d[6] = (ringColorCounter % 2 == 0) ? 1.0 : -1.0; // 輪ごとに自転方向を交互に

        ringColorCounter++;
    }
}