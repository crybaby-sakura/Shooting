// ============================================================
// 「青天の霹靂」モチーフ弾幕
//
// 平穏な青空(疎らなシアンの小玉)を延々と漂わせておき、
// 予告らしい予告もほぼ無いまま(ごく短いフラッシュのみ)、
// 画面全体を貫く稲妻(菱形弾+中楕円弾のジグザグ連結)が
// 一瞬静止したのち一気に降り注ぎ、着弾点で雷鳴の衝撃波
// (大玉のリング+短レーザーの放電)が炸裂する。
//
// 専用素材は使わず、既存の弾種(小玉・中玉・大玉・銃弾・鱗弾・
// 菱形弾・中楕円弾・短レーザー)の組み合わせのみで表現する。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 共通ヘルパー(sample の定型処理をまとめただけで、ロジックは同一)
// ------------------------------------------------------------
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
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

static sEnemyShot* SpawnShot(sEnemyShotSet* pSet,
    double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ------------------------------------------------------------
// フェーズ①: 晴天 ―― 疎らなシアンの小玉を緩やかに漂わせる
// ------------------------------------------------------------
static void ShotClearSky(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int shotNum = 5 + GetRand(3); // 5〜8発、疎らに(密にしない)
        for (int i = 0; i < shotNum; i++) {
            double muki = pEnemyShotSet->muki + (GetRand(200) - 100) / 100.0 * (DX_PI / 3.0);
            double speed = 0.6 + GetRand(60) / 100.0; // ゆったりした速度
            // muki は移動方向そのままでよい(小玉は向きに意味を持たない形状)
            SpawnShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                muki, speed, img_enemyShotSmallBall[3]); // シアン
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
// フェーズ②: 予兆 ―― 気づけないほど短い白フラッシュ(1発・静止)
// ------------------------------------------------------------
static void ShotLightningFlash(sEnemyShotSet* pEnemyShotSet)
{
    const int FLASH_LIFE = 5; // ごく短時間で消す

    if (pEnemyShotSet->count == 0) {
        SpawnShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
            DX_PI / 2.0, 0.0, img_enemyShotMediumOval[6]); // 白・静止
    }
    if (pEnemyShotSet->count == FLASH_LIFE) {
        // 画面内に留まる弾なので、自前でリンクを外して消去する
        // (画面外弾の自動消去とは別枠の、時間経過による自己消去)
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = pNext;
        }
    }
}

// ------------------------------------------------------------
// フェーズ③: 落雷本体 ―― ジグザグの稲妻(菱形弾+中楕円弾)が
//   画面全体に一瞬で形成され、一拍静止したのち一気に降り注ぐ。
//   本線から2本の枝分かれも伸ばす。
//   ※ muki は稲妻の折れ線に沿った"見た目の向き"、
//     実際の移動方向は param_d[0] に保持する。
// ------------------------------------------------------------
static void ShotLightningBolt(sEnemyShotSet* pEnemyShotSet)
{
    const int    HOLD_FRAMES = 4;    // この間は静止して稲妻の形を見せる
    const double SURGE_SPEED = 13.0; // 静止後、一気に走る速さ
    const int    SEGMENTS = 12;   // ジグザグの折れ数(画面上端〜下端)

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseX = pEnemyShotSet->x;
        double vx[SEGMENTS + 1];
        double vy[SEGMENTS + 1];
        for (int i = 0; i <= SEGMENTS; i++) {
            double amp = (i % 2 == 0) ? -42.0 : 42.0;
            vx[i] = baseX + amp;
            vy[i] = 480.0 * i / SEGMENTS;
        }

        // 本線:頂点間を密な菱形弾でつなぎ、頂点に光の節(中楕円弾)を置く
        for (int i = 0; i <= SEGMENTS; i++) {
            double segMuki = (i < SEGMENTS)
                ? atan2(vy[i + 1] - vy[i], vx[i + 1] - vx[i])
                : atan2(vy[i] - vy[i - 1], vx[i] - vx[i - 1]);

            sEnemyShot* pNode = SpawnShot(pEnemyShotSet, vx[i], vy[i],
                segMuki, 0.0, img_enemyShotMediumOval[1]); // 黄:頂点の節
            pNode->param_d[0] = DX_PI / 2.0; // 発動後は画面下方向へ

            if (i < SEGMENTS) {
                double dx = vx[i + 1] - vx[i];
                double dy = vy[i + 1] - vy[i];
                double len = sqrt(dx * dx + dy * dy);
                int div = (int)(len / 7.0); // 密な間隔で並べる
                for (int j = 1; j < div; j++) {
                    double t = (double)j / div;
                    double x = vx[i] + dx * t;
                    double y = vy[i] + dy * t;
                    sEnemyShot* pSeg = SpawnShot(pEnemyShotSet, x, y,
                        segMuki, 0.0, img_enemyShotDiamond[6]); // 白:本線
                    pSeg->param_d[0] = DX_PI / 2.0;
                }
            }
        }

        // 枝分かれ:中間の頂点から斜めに短い稲妻を3本伸ばす
        int    branchOrigin[3] = { SEGMENTS / 4, SEGMENTS / 2, (SEGMENTS * 3) / 4 };
        double branchDir[3] = { -1.0, 1.0, -1.0 };
        for (int b = 0; b < 3; b++) {
            int oi = branchOrigin[b];
            double ox = vx[oi], oy = vy[oi];
            double angle = DX_PI / 2.0 + branchDir[b] * (DX_PI / 4.0 + GetRand(20) / 100.0);
            const int BRANCH_LEN = 14;
            for (int k = 1; k <= BRANCH_LEN; k++) {
                double bx = ox + cos(angle) * k * 7.0;
                double by = oy + sin(angle) * k * 7.0;
                sEnemyShot* pSeg = SpawnShot(pEnemyShotSet, bx, by,
                    angle, 0.0, img_enemyShotDiamond[1]); // 黄:枝分かれ
                pSeg->param_d[0] = angle; // 枝はその角度のまま放たれる
            }
        }
    }

    if (pEnemyShotSet->count == HOLD_FRAMES) {
        // 一瞬の静止のあと、一斉に走り出す
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->speed = SURGE_SPEED;
            pShot = pShot->next;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 見た目の向き(muki)と実移動方向(param_d[0])をあえて分離
        pShot->x += pShot->speed * cos(pShot->param_d[0]);
        pShot->y += pShot->speed * sin(pShot->param_d[0]);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ④: 雷鳴 ―― 着弾点で大玉のリング+短レーザーの放電
// ------------------------------------------------------------
static void ShotThunderClap(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 衝撃波:大玉を全方位に高密度でばら撒く
        const int RING_NUM = 40;
        for (int i = 0; i < RING_NUM; i++) {
            double muki = DX_PI * 2.0 * i / RING_NUM;
            double speed = 2.6 + GetRand(120) / 100.0;
            SpawnShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                muki, speed, img_enemyShotLargeBall[8]); // 橙
        }
        // 余韻のバチバチとした放電:短レーザーを不規則な角度で
        const int LASER_NUM = 16;
        for (int i = 0; i < LASER_NUM; i++) {
            double muki = DX_PI * 2.0 * i / LASER_NUM + (GetRand(40) - 20) / 100.0;
            double speed = 6.0 + GetRand(200) / 100.0;
            // 短レーザーは向きが見た目に直結するので、移動方向=mukiのままでよい
            SpawnShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                muki, speed, img_enemyShotLaser[6]); // 白
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
// 敵本体のパターン:「青天の霹靂」
// ------------------------------------------------------------
void EnemyPat_SeitenNoHekireki_Claude()
{
    static int enemyMuki;
    static int strikeTimer;      // -1:平常時 / 0以上:落雷シーケンス進行中の経過フレーム
    static int strikeX;
    static int nextStrikeCount;

    // ShotLightningBolt 内の HOLD_FRAMES(=4) と揃えること
    const int FLASH_DELAY = 0;             // 予兆フラッシュ
    const int BOLT_DELAY = 6;             // 稲妻本体の出現
    const int CLAP_DELAY = 6 + 4 + 2;     // 静止(4F)を経てすぐ着弾→雷鳴
    const int SEQUENCE_END = CLAP_DELAY + 6;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100;
        enemyMuki = 1;
        strikeTimer = -1;
        nextStrikeCount = count + 90 + GetRand(120); // 最初の落雷までの間隔もランダム
    }
    else {
        enemy.x += 0.6 * (double)enemyMuki;
        if (count % 160 == 80) enemyMuki *= -1;
    }

    // フェーズ①:平常時は絶えず青空の小玉を疎らに散らす
    // (落雷シーケンス中は新規生成を止め、既存の弾はそのまま漂わせる)
    if (strikeTimer < 0 && count % 28 == 1) {
        SpawnShotSet(ShotClearSky, enemy.x, enemy.y + 10.0, DX_PI / 2.0, 0);
    }

    // 落雷の発生判定(タイミング・位置ともにランダムで唐突さを演出)
    if (strikeTimer < 0 && count == nextStrikeCount) {
        strikeTimer = 0;
        strikeX = (int)(player.x + GetRand(160) - 80); // 自機付近だが完全な狙い撃ちにはしない
        if (strikeX < 70)  strikeX = 70;
        if (strikeX > 410) strikeX = 410;
    }

    if (strikeTimer >= 0) {
        if (strikeTimer == FLASH_DELAY) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            SpawnShotSet(ShotLightningFlash, (double)strikeX, 0.0, 0.0, 0);
        }
        if (strikeTimer == BOLT_DELAY) {
            SpawnShotSet(ShotLightningBolt, (double)strikeX, 0.0, 0.0, 0);
        }
        if (strikeTimer == CLAP_DELAY) {
            SpawnShotSet(ShotThunderClap, (double)strikeX, 460.0, 0.0, 0);
        }
        strikeTimer++;
        if (strikeTimer >= SEQUENCE_END) {
            strikeTimer = -1;
            nextStrikeCount = count + 150 + GetRand(150); // 次の落雷までまたランダムな間隔
        }
    }
}