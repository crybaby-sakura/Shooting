// enemyPat_AunSoushi.cpp
// 阿吽双獅（あうんそうし）
// 狛犬（阿形・吽形）をモチーフにした双ボス4フェーズパターン。
// 　フェーズ1 対峙        : 両者が中央へは撃たず、外向きファンで壁を築く（中央通路は常に安全）
// 　フェーズ2 阿の咆哮    : 阿形が自機狙いで攻め、吽形は白黒交互の石畳カーテンで守る
// 　フェーズ3 吽の逆襲    : フェーズ2の役割を交代
// 　フェーズ4 阿吽同時解放: 両者が半スロットずらして噛み合う同心リング＋自機狙いを同時発射（以後ループ）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 色定数（阿形=暖色 / 吽形=寒色 / 守りの壁=白黒の石畳）
// ============================================================
enum {
    COL_RED = 0, COL_YELLOW = 1, COL_GREEN = 2, COL_CYAN = 3,
    COL_BLUE = 4, COL_MAGENTA = 5, COL_WHITE = 6, COL_BLACK = 7, COL_ORANGE = 8
};

// フェーズ長（フレーム数）
static const int LEN_TAIJI = 150; // フェーズ1: 対峙
static const int LEN_AGYOU = 180; // フェーズ2: 阿の咆哮
static const int LEN_UNGYOU = 180; // フェーズ3: 吽の逆襲
static const int LEN_KOKYUU = 140; // フェーズ4: 阿吽同時解放
static const int CYCLE_LEN = LEN_TAIJI + LEN_AGYOU + LEN_UNGYOU + LEN_KOKYUU; // 650フレームで1周し以後ループ

// ============================================================
// 効果音再生共通処理（鳴りっぱなしを止めてから鳴らす）
// ============================================================
static void PlaySoundReset(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// ============================================================
// 弾生成＋更新（formula-driven: 発射原点と速度をparam_dへ保存し、
// 座標は毎フレーム pShot->count から再計算する。速度の積分は行わない）
// ============================================================
static void SpawnLinearShot(sEnemyShotSet* pSet, double ox, double oy, double angle, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = ox;
    pShot->y = oy;
    pShot->muki = angle;
    pShot->kind = kind;
    pShot->param_d[0] = ox;                  // 発射原点X
    pShot->param_d[1] = oy;                  // 発射原点Y
    pShot->param_d[2] = speed * cos(angle);  // X方向速度
    pShot->param_d[3] = speed * sin(angle);  // Y方向速度

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static void UpdateLinearShots(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t;
        pShot = pShot->next;
    }
}

static sEnemyShotSet* CreateShotSet(double x, double y, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->patternFunc = func;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ============================================================
// フェーズ1: 対峙 - 外向きファン
// param_i[0] = 0:阿形（左） / 1:吽形（右）
// 中央へ向かう角度成分を一切含まないため、両者の間の通路は常に安全。
// ============================================================
static void ShotOutwardFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundReset(sound_enemyShot_medium);

        bool isUngyou = (pSet->param_i[0] == 1);
        const int N = 13;
        double angleStart = isUngyou ? 0.0 : DX_PI * 0.5;
        double angleEnd = isUngyou ? DX_PI * 0.5 : DX_PI * 1.0;
        int color = isUngyou ? COL_CYAN : COL_RED;

        for (int i = 0; i < N; i++) {
            double a = angleStart + (angleEnd - angleStart) * i / (N - 1);
            double speed = 1.6 + 0.05 * i;
            SpawnLinearShot(pSet, pSet->x, pSet->y, a, speed, img_enemyShotSmallBall[color]);
        }
    }
    UpdateLinearShots(pSet);
}

// ============================================================
// フェーズ2/3共通: 攻め側の自機狙い5way
// param_i[0] = 0:阿形が撃つ / 1:吽形が撃つ
// ============================================================
static void ShotAimedBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundReset(sound_enemyShot_heavy);

        bool isUngyou = (pSet->param_i[0] == 1);
        int color = isUngyou ? COL_CYAN : COL_ORANGE;
        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);
        const int WAY = 5;
        const double SPREAD = DX_PI * 0.18;

        for (int i = 0; i < WAY; i++) {
            double a = baseAngle + SPREAD * (i - (WAY - 1) / 2.0) / (WAY - 1);
            SpawnLinearShot(pSet, pSet->x, pSet->y, a, 3.3, img_enemyShotDiamond[color]);
        }
    }
    UpdateLinearShots(pSet);
}

// ============================================================
// フェーズ2/3共通: 守り側の石畳カーテン（白黒交互）
// param_i[0] = 0:阿形側（左半分）に展開 / 1:吽形側（右半分）に展開
// ============================================================
static void ShotCurtainWall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundReset(sound_enemyShot_light);

        bool rightSide = (pSet->param_i[0] == 1);
        const int N = 10;
        double xStart = rightSide ? 250.0 : 0.0;

        for (int i = 0; i < N; i++) {
            double x = xStart + i * 23.0;
            int color = (i % 2 == 0) ? COL_WHITE : COL_BLACK;
            SpawnLinearShot(pSet, x, pSet->y, DX_PI * 0.5, 0.9, img_enemyShotScale[color]);
        }
    }
    UpdateLinearShots(pSet);
}

// ============================================================
// フェーズ4: 阿吽同時解放 - 半スロットずらして噛み合う同心リング＋自機狙い3way
// param_i[0] = 0:阿形 / 1:吽形　　param_i[1] = 波番号（0=1波目 / 1=2波目）
// ============================================================
static void ShotFinaleRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundReset(sound_enemyShot_extreme);

        bool isUngyou = (pSet->param_i[0] == 1);
        int wave = pSet->param_i[1];
        int colorMain = isUngyou ? COL_BLUE : COL_RED;
        int colorSub = isUngyou ? COL_CYAN : COL_ORANGE;
        const int N = 22 + wave * 8;
        double speed = 2.0 + wave * 0.8;
        double rotOffset = isUngyou ? (DX_PI / N) : 0.0; // 半スロットずらして両者のリングを噛み合わせる

        for (int i = 0; i < N; i++) {
            double a = rotOffset + DX_PI * 2.0 * i / N;
            int color = (i % 2 == 0) ? colorMain : colorSub;
            SpawnLinearShot(pSet, pSet->x, pSet->y, a, speed, img_enemyShotLargeBall[color]);
        }

        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = -1; i <= 1; i++) {
            double a = baseAngle + i * DX_PI * 0.08;
            SpawnLinearShot(pSet, pSet->x, pSet->y, a, 4.2, img_enemyShotDiamond[colorMain]);
        }
    }
    UpdateLinearShots(pSet);
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_TwoBoss_Claude()
{
    if (count == 1) {
        enemy.x = 120.0;  enemy.y = 70.0;  // 阿形（左）
        enemy.x2 = 360.0; enemy.y2 = 70.0; // 吽形（右）
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    int localCount = ((count - 1) % CYCLE_LEN) + 1;

    if (localCount <= LEN_TAIJI) {
        // ---- フェーズ1: 対峙 ----
        if (localCount % 30 == 1 && localCount <= LEN_TAIJI - 10) {
            sEnemyShotSet* pA = CreateShotSet(enemy.x, enemy.y, ShotOutwardFan);
            pA->param_i[0] = 0;
            sEnemyShotSet* pU = CreateShotSet(enemy.x2, enemy.y2, ShotOutwardFan);
            pU->param_i[0] = 1;
        }
    }
    else if (localCount <= LEN_TAIJI + LEN_AGYOU) {
        // ---- フェーズ2: 阿の咆哮（阿形が攻め、吽形が守る） ----
        int p = localCount - LEN_TAIJI;
        if (p == 1) PlaySoundReset(sound_enemyCharge);

        if (p % 22 == 1) {
            sEnemyShotSet* pAtk = CreateShotSet(enemy.x, enemy.y, ShotAimedBurst);
            pAtk->param_i[0] = 0;
        }
        if (p % 12 == 1) {
            sEnemyShotSet* pDef = CreateShotSet(enemy.x2, enemy.y2, ShotCurtainWall);
            pDef->param_i[0] = 1;
        }
    }
    else if (localCount <= LEN_TAIJI + LEN_AGYOU + LEN_UNGYOU) {
        // ---- フェーズ3: 吽の逆襲（役割交代） ----
        int p = localCount - LEN_TAIJI - LEN_AGYOU;
        if (p == 1) PlaySoundReset(sound_enemyCharge);

        if (p % 22 == 1) {
            sEnemyShotSet* pAtk = CreateShotSet(enemy.x2, enemy.y2, ShotAimedBurst);
            pAtk->param_i[0] = 1;
        }
        if (p % 12 == 1) {
            sEnemyShotSet* pDef = CreateShotSet(enemy.x, enemy.y, ShotCurtainWall);
            pDef->param_i[0] = 0;
        }
    }
    else {
        // ---- フェーズ4: 阿吽同時解放 ----
        int p = localCount - LEN_TAIJI - LEN_AGYOU - LEN_UNGYOU;
        if (p == 1) PlaySoundReset(sound_enemyCharge);

        if (p == 60 || p == 100) {
            int wave = (p == 60) ? 0 : 1;
            sEnemyShotSet* pA = CreateShotSet(enemy.x, enemy.y, ShotFinaleRing);
            pA->param_i[0] = 0; pA->param_i[1] = wave;
            sEnemyShotSet* pU = CreateShotSet(enemy.x2, enemy.y2, ShotFinaleRing);
            pU->param_i[0] = 1; pU->param_i[1] = wave;
        }
    }
}