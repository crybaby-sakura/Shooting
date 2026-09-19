#include "gv.h"

// ============================================================
//  弾幕:竹やぶ焼けた 〜夜更けの竹林火災〜
//
//  フェーズ1「竹」  :緑レーザーを竹に見立て、画面下から時間差で伸びる
//  フェーズ2「延焼」:火の粉(小玉)が竹を根元から駆け上がり、
//                      頂上に到達する頃に竹が赤く燃え上がり散り火を撒く
//  フェーズ3「鎮火」:燃え殻が落ちた後、敵本体から中玉のリング
// ============================================================

// 弾の状態 (sEnemyShot::param_i[0] に格納)
static constexpr int ST_BAMBOO_RISE = 0;  // 竹:成長中(上昇)
static constexpr int ST_BAMBOO_STAND = 1;  // 竹:成長完了(発火待ち)
static constexpr int ST_BAMBOO_BURN = 2;  // 竹:燃焼中
static constexpr int ST_BAMBOO_ASH = 3;  // 竹:燃え殻(落下)
static constexpr int ST_SPARK = 4;  // 火の粉:竹を駆け上がる
static constexpr int ST_EMBER = 5;  // 散り火:減速して漂う小弾
static constexpr int ST_RING = 6;  // 中玉リング:等速直線

static constexpr int    BAMBOO_NUM = 12-4;    // 竹の本数
static constexpr int    STAND_TIME = 20;    // 竹が止まってから燃えるまでの時間
static constexpr int    BURN_TIME = 30;    // 燃焼時間
static constexpr double SPARK_SPEED = 2.7;   // 火の粉が竹を上る速さ

// 弾を生成してリストに繋げる補助関数
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int state)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = state;
    p->margin = 240;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾幕本体:竹林火災
static void ShotBambooFire(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int c = pEnemyShotSet->count;

    // ---------------- 生成 ----------------

    // 開始時:延焼の予告音
    if (c == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズ1:竹を1フレームに1本ずつ生やす(時間差で植わる)
    if (c < BAMBOO_NUM) {
        pEnemyShot = AddShot(pEnemyShotSet,
            20.0 + GetRand(440),   // x: 20〜460
            520.0,                 // 画面下の見えない位置から
            -DX_PI / 2.0,          // 上向き
            3.0,                   // 伸びる速さ
            img_enemyShotLaser[2], // 緑レーザー = 竹
            ST_BAMBOO_RISE);
        // param_d[0]:竹の伸びる上限Y(130〜380)
        pEnemyShot->param_d[0] = 130.0 + GetRand(250);
    }

    // フェーズ3:燃え尽きのリング(中玉、3連発・少しずつ回転)
    if (c == 170 || c == 185 || c == 200) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        constexpr int n = 22;
        double base = c * 0.045;
        for (int i = 0; i < n; i++) {
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                base + DX_PI * 2.0 * i / n, 2.2,
                img_enemyShotMediumBall[(i % 2) ? 0 : 8], // 赤/橙
                ST_RING);
        }
    }

    // ---------------- 更新 ----------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        switch (pShot->param_i[0]) {

        case ST_BAMBOO_RISE: // 竹が下から上へ伸びる
            pShot->y -= pShot->speed;
            if (pShot->y <= pShot->param_d[0]) {
                pShot->y = pShot->param_d[0];
                pShot->param_i[0] = ST_BAMBOO_STAND;
                pShot->param_i[1] = c + STAND_TIME; // 発火フレームを予約

                // 根元から火の粉を発生させる
                sEnemyShot* pSpark = AddShot(pEnemyShotSet,
                    pShot->x, pShot->y + 28.0,
                    -DX_PI / 2.0, SPARK_SPEED,
                    img_enemyShotSmallBall[0], ST_SPARK);
                // param_d[0]:ここまで上がったら着火(=竹のてっぺん付近)
                pSpark->param_d[0] = pShot->y - 26.0;
            }
            break;

        case ST_BAMBOO_STAND: // 火の粉が頂上に届く頃に燃え上がる
            if (c >= pShot->param_i[1]) {
                pShot->param_i[0] = ST_BAMBOO_BURN;
                pShot->param_i[2] = c + BURN_TIME;
                pShot->kind = img_enemyShotLaser[0]; // 緑→赤(燃焼色)

                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                // 散り火:7方向の小弾(赤/橙)
                for (int i = 0; i < 7; i++) {
                    double muki = DX_PI * 2.0 * i / 7.0
                        + (GetRand(30) - 15) / 180.0 * DX_PI;
                    AddShot(pEnemyShotSet, pShot->x, pShot->y, muki,
                        1.5 + GetRand(100) / 100.0,
                        img_enemyShotSmallBall[(i % 2) ? 8 : 0],
                        ST_EMBER);
                }
            }
            break;

        case ST_BAMBOO_BURN: // 燃焼中:時々上に小さな火の粉
            if (pShot->count % 8 == 0) {
                AddShot(pEnemyShotSet,
                    pShot->x + GetRand(20) - 10.0,
                    pShot->y + GetRand(40) - 20.0,
                    -DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI,
                    1.0 + GetRand(50) / 100.0,
                    img_enemyShotSmallBall[8], ST_EMBER);
            }
            if (c >= pShot->param_i[2]) { // 燃え尽き→燃え殻が落ちる
                pShot->param_i[0] = ST_BAMBOO_ASH;
                pShot->muki = DX_PI / 2.0;  // 下向き
                pShot->speed = 5.0;
            }
            break;

        case ST_BAMBOO_ASH: // 燃え殻が落ちて画面外へ(消去はメインルーチン)
            pShot->y += pShot->speed;
            break;

        case ST_SPARK: // 火の粉が竹を駆け上がる
            pShot->y -= pShot->speed;
            if (pShot->y <= pShot->param_d[0]) {
                // 竹のてっぺんで小さく爆ぜる(3方向)
                if (pEnemyShotSet->count % 10 == 0) {
                    for (int i = 0; i < 3; i++) {
                        double muki = -DX_PI / 2.0 + (i - 1) * 0.7;
                        AddShot(pEnemyShotSet, pShot->x, pShot->y, muki, 2.0,
                            img_enemyShotSmallBall[8], ST_EMBER);
                    }
                }
                // 火の粉自身はそのまま飛び去り画面外で消える
            }
            break;

        case ST_EMBER: // 散り火:減速→やがて火の粉が降り注ぐ
            pShot->speed *= 0.985;
            if (pShot->speed < 0.8) { // 静かに燃えさしとして降る
                pShot->muki = DX_PI / 2.0;
                pShot->speed = 1.5;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case ST_RING:
        default: // 等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_TakeyabuYaketa_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // ゆっくり左右に揺れる(リング弾の発射位置を変えるため)
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 100.0) { enemy.x = 100.0; muki = 1; }
        if (enemy.x > 380.0) { enemy.x = 380.0; muki = -1; }
    }

    // 240フレームごとに「竹林火災」を起こす
    // (1ウェーブ約230フレーム:竹の成長→延焼→燃え殻→リング、でループ)
    if (count % 360 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBambooFire;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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