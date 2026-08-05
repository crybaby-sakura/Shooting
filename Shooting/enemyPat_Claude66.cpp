// enemyPat_OodamaKorogashi.cpp
//
// 「大玉転がし」パターン
// 超巨大弾専用の素材が無いため、大玉(20x20)を同心リング状(1+6+12+18=37発)に
// 並べた編隊として"1つの超巨大弾"を表現する。
//   フェーズ1：予告レーン(小玉の壁)が転がる方向を示す
//   フェーズ2：大玉本体が自転しながら加速ジグザグで転がり降りてくる(数式駆動)
//   フェーズ3：ゴール到達で編隊メンバーがそのまま放射状の破裂片として解放され、紙吹雪バーストが重なる
//   フェーズ4：反対側から強化版の2個目が登場し、破裂と同時に自機狙い3wayを追加する

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// enemyShotSetHead へ新しい弾セットを作成・連結する
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// 弾セットに1発の弾を追加する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = 240;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;

    return pShot;
}

// 弾幕：紙吹雪(ゴール激突時の演出用、放射状バラ撒き)
static void ShotScatterConfetti(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 16; i++) {
            double m = 2.0 * DX_PI * i / 16.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            double sp = (150 + GetRand(150)) / 100.0;
            int color = i % 8;
            int kind;
            switch (i % 3) {
            case 0: kind = img_enemyShotDiamond[color]; break;
            case 1: kind = img_enemyShotScale[color]; break;
            default: kind = img_enemyShotSmallBall[color]; break;
            }
            AddShot(pSet, pSet->x, pSet->y, m, sp, kind);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ゴール地点に紙吹雪バーストを1つ生成する
static void CreateConfettiBurst(double x, double y)
{
    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    CreateShotSet(x, y, 0.0, 0, ShotScatterConfetti);
}

// 弾幕：自機狙い3way(2個目の大玉の破裂に追加する仕上げの一撃)
static void ShotAimed3Way(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = -1; i <= 1; i++) {
            double m = pSet->muki + i * 0.18;
            AddShot(pSet, pSet->x, pSet->y, m, 2.6, img_enemyShotBullet[0]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：予告レーン(小玉の壁で転がる方向を示し、合図とともに左右へ弾け去る)
static void ShotLaneGuide(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 18; i++) {
            double y = 40.0 + i * 13.0;
            AddShot(pSet, pSet->x - 90.0, y, DX_PI, 0.0, img_enemyShotSmallBall[6]);
            AddShot(pSet, pSet->x + 90.0, y, 0.0, 0.0, img_enemyShotSmallBall[6]);
        }
    }

    bool release = (pSet->count >= 40);

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (release) {
            pShot->speed = 3.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 弾幕：大玉本体(超巨大弾)
//   大玉(20x20)を同心リング状(1+6+12+18=37発)に並べ、自転させながら1つの巨大な弾として転がす。
//   ゴール到達(param_i[0]フレーム)で、編隊メンバーはそのまま放射状の破裂片として解放される。
static void ShotGiantBall(sEnemyShotSet* pSet)
{
    int t = pSet->count;

    // 編隊の中心座標(tのみから決まる数式：加速するジグザグをしながら下降)
    double amp = pSet->param_d[3] + pSet->param_d[4] * t;
    if (amp > pSet->param_d[5]) amp = pSet->param_d[5];
    double phase = pSet->param_d[6] * t + 0.5 * pSet->param_d[7] * t * t;
    double groupX = pSet->param_d[0] + amp * sin(phase);
    double groupY = pSet->param_d[1] + pSet->param_d[2] * t;

    // 初回：大玉本体を同心リング状に生成
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        static const int    ringCount[4] = { 1, 6, 12, 18 };
        static const double ringRadius[4] = { 0.0, 22.0, 44.0, 66.0 };
        int colorBase = pSet->param_i[3];
        int colorAlt = pSet->param_i[4];

        for (int r = 0; r < 4; r++) {
            for (int j = 0; j < ringCount[r]; j++) {
                double baseAngle = (ringCount[r] == 1)
                    ? 0.0
                    : (2.0 * DX_PI * j / ringCount[r]) + (r % 2) * (DX_PI / ringCount[r]);
                int color = (r == 3 && (j % 2 == 0)) ? colorAlt : colorBase; // 一番外側は市松模様で継ぎ目を表現

                sEnemyShot* pShot = AddShot(pSet,
                    groupX + ringRadius[r] * cos(baseAngle),
                    groupY + ringRadius[r] * sin(baseAngle),
                    0.0, 0.0, img_enemyShotLargeBall[color]);
                pShot->param_i[0] = 0;          // 0=編隊追従中
                pShot->param_d[0] = baseAngle;  // 自転の基準角
                pShot->param_d[1] = ringRadius[r];
            }
        }
    }

    bool burst = (pSet->param_i[1] == 1);

    // ゴール到達：破裂トリガー(一度だけ)
    if (!burst && t >= pSet->param_i[0]) {
        pSet->param_i[1] = 1;
        burst = true;

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                double dx = pShot->x - groupX;
                double dy = pShot->y - groupY;
                pShot->muki = atan2(dy, dx);
                pShot->speed = 2.0 + GetRand(150) / 100.0; // 2.0～3.5で放射
                pShot->param_i[0] = 1; // 1=解放済み
            }
            pShot = pShot->next;
        }

        CreateConfettiBurst(groupX, groupY);

        // 2個目の大玉だけ、破裂と同時に自機狙い3wayを追加
        if (pSet->param_i[2] == 1) {
            double aim = atan2(player.y - groupY, player.x - groupX);
            CreateShotSet(groupX, groupY, aim, 0, ShotAimed3Way);
        }
    }

    // 転がり中：一定間隔で土煙を残す
    if (!burst && t > 0 && t % 8 == 0) {
        for (int i = 0; i < 3; i++) {
            double m = DX_PI / 2.0 + (GetRand(160) - 80) / 180.0 * DX_PI; // 概ね下方向へ拡散
            sEnemyShot* pShot = AddShot(pSet,
                groupX + GetRand(100) - 50, groupY - 55 + GetRand(20) - 10,
                m, (20 + GetRand(40)) / 100.0, img_enemyShotSmallBall[6]);
            pShot->param_i[0] = 2; // 2=土煙(単純加算移動)
        }
    }

    // 全弾の座標更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 編隊追従中：中心＋自転オフセットの数式のみで座標決定(速度積分はしない)
            double angle = pShot->param_d[0] + t * 0.04;
            pShot->x = groupX + pShot->param_d[1] * cos(angle);
            pShot->y = groupY + pShot->param_d[1] * sin(angle);
        }
        else {
            // 解放後(破裂片・土煙)：通常通り速度を加算
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン：大玉転がし
void EnemyPat_HugeBullet_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        enemy.x = 240.0 + 10.0 * sin(count / 45.0); // 控えめな左右の揺れ
    }

    // 1個目：予告レーン→大玉(赤)
    const int T = 680;
    if (count % T == 1) {
        CreateShotSet(160.0, 0.0, 0.0, 0, ShotLaneGuide);
    }
    if (count % T == 41) {
        sEnemyShotSet* pSet = CreateShotSet(160.0, 40.0, 0.0, 0, ShotGiantBall);
        pSet->param_d[0] = 160.0;  // startX
        pSet->param_d[1] = 40.0;   // startY
        pSet->param_d[2] = 1.7;    // fallSpeed
        pSet->param_d[3] = 15.0;   // amp0
        pSet->param_d[4] = 0.35;   // ampRate
        pSet->param_d[5] = 130.0;  // maxAmp
        pSet->param_d[6] = 0.05;   // w0
        pSet->param_d[7] = 0.0004; // wRate(加速チャープ)
        pSet->param_i[0] = 220;    // burstFrame
        pSet->param_i[1] = 0;      // burstTriggered
        pSet->param_i[2] = 0;      // isSecondBall
        pSet->param_i[3] = 0;      // colorBase = 赤
        pSet->param_i[4] = 6;      // colorAlt  = 白
    }

    // 2個目：予告レーン(反対側)→大玉(青、強化版)
    if (count % T == 341) {
        CreateShotSet(320.0, 0.0, 0.0, 0, ShotLaneGuide);
    }
    if (count % T == 381) {
        sEnemyShotSet* pSet = CreateShotSet(320.0, 40.0, 0.0, 0, ShotGiantBall);
        pSet->param_d[0] = 320.0;
        pSet->param_d[1] = 40.0;
        pSet->param_d[2] = 2.0;
        pSet->param_d[3] = 20.0;
        pSet->param_d[4] = 0.55;
        pSet->param_d[5] = 150.0;
        pSet->param_d[6] = 0.06;
        pSet->param_d[7] = 0.0009;
        pSet->param_i[0] = 200;
        pSet->param_i[1] = 0;
        pSet->param_i[2] = 1; // 自機狙い3way付き
        pSet->param_i[3] = 4; // colorBase = 青
        pSet->param_i[4] = 6; // colorAlt  = 白
    }
}