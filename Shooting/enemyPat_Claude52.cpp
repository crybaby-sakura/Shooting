// enemyPat_RokkakuSugomori.cpp
// 六角巣籠 -Hexagonal Nest-
// 陰蜂(蜂)モチーフの4フェーズ無限ループパターン
// 1:営巣(六角の巣が同心リング状に形成) → 2:偵察羽音(警戒セルが決まり羽音弾が漏れる)
// → 3:群飛乱舞(警戒セルから螺旋バースト+自機狙い3way) → 4:毒針一閃(全セル自機狙い針弾→巣が放射状に崩壊)
// 以後フェーズ1へループ。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    LOOP_LEN = 720; // ループ周期(フレーム)

static const int    PH2_START = 150; // 偵察羽音フェーズ開始
static const int    PH3_START = 330; // 群飛乱舞フェーズ開始
static const int    PH4_TELEGRAPH_START = 600; // 毒針予告(全セル赤点滅)開始
static const int    PH4_NEEDLE_FRAME = 630; // 毒針発射フレーム
static const int    PH4_EXPLODE_START = 636; // 巣崩壊(放射状飛散)開始

static const double HEX_SPACING = 34.0; // セル間隔
static const double HEX_VERT_R = 12.0; // 各セル六角形の頂点半径
static const int    HEX_CELL_COUNT = 19; // リング0~2 (1+6+12)
static const int    HEX_VERTS = 6;

static const double NEST_CENTER_X = 240.0;
static const double NEST_CENTER_Y = 190.0;

static const int    MARKED_CELL_COUNT = 6; // 群飛乱舞で暴れるセルの数

// ============================================================
//  巣の六角格子情報(初回のみ計算し使い回す)
// ============================================================
static double gCellOx[HEX_CELL_COUNT];
static double gCellOy[HEX_CELL_COUNT];
static int    gCellRing[HEX_CELL_COUNT];
static bool   gHexInitialized = false;

// 各ループで「次に暴れるセル」として選ばれたかどうか
static bool   gMarkedCell[HEX_CELL_COUNT];

// 軸座標(q,r)からリング番号(中心からの六角距離)を求める
static int HexRing(int q, int r)
{
    int s = -q - r;
    int aq = q < 0 ? -q : q;
    int ar = r < 0 ? -r : r;
    int as = s < 0 ? -s : s;
    int m = aq;
    if (ar > m) m = ar;
    if (as > m) m = as;
    return m;
}

// 六角格子(半径2、19セル)の各セル中心オフセットを一度だけ計算
static void InitHexGrid()
{
    if (gHexInitialized) return;

    int idx = 0;
    for (int q = -2; q <= 2; q++) {
        for (int r = -2; r <= 2; r++) {
            int ring = HexRing(q, r);
            if (ring > 2) continue;
            if (idx >= HEX_CELL_COUNT) continue; // 安全策

            gCellOx[idx] = HEX_SPACING * (sqrt(3.0) * q + sqrt(3.0) / 2.0 * r);
            gCellOy[idx] = HEX_SPACING * (1.5 * r);
            gCellRing[idx] = ring;
            idx++;
        }
    }

    gHexInitialized = true;
}

// 新規sEnemyShotSetを生成しリストへ登録する共通処理
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = NEST_CENTER_X;
    pSet->y = NEST_CENTER_Y;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// リンクリストへ1発追加する共通処理
static sEnemyShot* AppendShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ============================================================
//  フェーズ1: 巣の輪郭(常駐・全フェーズを通して残り続ける)
//  各セルの六角形輪郭を頂点6発x19セル=114発で描く。
//  リングごとに時間差で中心から展開(営巣)し、
//  毒針発射直後(PH4_EXPLODE_START)に中心から放射状に加速崩壊する。
// ============================================================
static void ShotNestOutline(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int c = 0; c < HEX_CELL_COUNT; c++) {
            for (int v = 0; v < HEX_VERTS; v++) {
                double ang = DX_PI / 3.0 * v - DX_PI / 6.0;
                double vx = gCellOx[c] + HEX_VERT_R * cos(ang);
                double vy = gCellOy[c] + HEX_VERT_R * sin(ang);

                sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->speed = 0.0; // 位置はcountからの数式で決めるため未使用

                pEnemyShot->param_d[0] = vx; // 完成位置(中心からのオフセット)X
                pEnemyShot->param_d[1] = vy; // 完成位置(中心からのオフセット)Y
                pEnemyShot->param_i[0] = gCellRing[c];               // リング番号(営巣の遅延用)
                pEnemyShot->param_i[1] = c;                          // 所属セル番号
                pEnemyShot->param_i[2] = (v % 2 == 0) ? 1 : 7;       // 縞模様の基本色(黄/黒)

                pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShot->param_i[2]];
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int    t = pShot->count; // このセットが生成されてからの経過フレーム
        int    ring = pShot->param_i[0];
        int    cellIdx = pShot->param_i[1];
        int    stripeColor = pShot->param_i[2];
        double tx = pShot->param_d[0];
        double ty = pShot->param_d[1];

        int emergeDelay = ring * 25; // リングが外側ほど遅れて展開
        int emergeDur = 20;
        int et = t - emergeDelay;

        double px, py;
        if (et < 0) {
            // まだ中心に折り畳まれた状態
            px = 0.0;
            py = 0.0;
        }
        else if (et < emergeDur) {
            double frac = (double)et / (double)emergeDur;
            frac = 1.0 - (1.0 - frac) * (1.0 - frac); // イーズアウト
            px = tx * frac;
            py = ty * frac;
        }
        else {
            px = tx;
            py = ty;
        }

        // フェーズ4: 毒針発射直後に放射状加速崩壊
        if (t >= PH4_EXPLODE_START) {
            double et2 = (double)(t - PH4_EXPLODE_START);
            double burstDist = 2.5 * et2 + 0.06 * et2 * et2;
            double len = sqrt(tx * tx + ty * ty);
            double dx = (len > 0.001) ? tx / len : 1.0;
            double dy = (len > 0.001) ? ty / len : 0.0;
            px = tx + dx * burstDist;
            py = ty + dy * burstDist;
        }

        pShot->x = NEST_CENTER_X + px;
        pShot->y = NEST_CENTER_Y + py;

        // 色演出: 警戒セルの点滅 / 全体の赤点滅予告
        int colorIdx = stripeColor;
        if (t >= PH4_TELEGRAPH_START && t < PH4_NEEDLE_FRAME) {
            colorIdx = ((t / 4) % 2 == 0) ? 0 : stripeColor; // 赤点滅で総予告
        }
        else if (t >= PH2_START && t < PH4_TELEGRAPH_START && gMarkedCell[cellIdx]) {
            colorIdx = ((t / 8) % 2 == 0) ? 8 : 1; // 橙/黄点滅で警戒セルを強調
        }
        pShot->kind = img_enemyShotSmallBall[colorIdx];

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2: 偵察羽音
//  警戒セル(gMarkedCellがtrueのセル)から小刻みに振動する
//  スカウト弾を一定間隔で漏らす。
// ============================================================
static void ShotReconLeak(sEnemyShotSet* pEnemyShotSet)
{
    const int PHASE_DUR = PH3_START - PH2_START; // このセットの活動期間

    if (pEnemyShotSet->count < PHASE_DUR && pEnemyShotSet->count % 10 == 0) {
        int pick = GetRand(MARKED_CELL_COUNT - 1);
        int idx = -1, cnt = -1;
        for (int c = 0; c < HEX_CELL_COUNT; c++) {
            if (gMarkedCell[c]) {
                cnt++;
                if (cnt == pick) { idx = c; break; }
            }
        }

        if (idx >= 0) {
            double ox = NEST_CENTER_X + gCellOx[idx];
            double oy = NEST_CENTER_Y + gCellOy[idx];
            double ang = atan2(gCellOy[idx], gCellOx[idx]); // 巣の中心から外向き

            sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);
            pEnemyShot->x = ox;
            pEnemyShot->y = oy;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.6;
            pEnemyShot->param_d[0] = ox;
            pEnemyShot->param_d[1] = oy;
            pEnemyShot->param_d[2] = ang;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double ox = pShot->param_d[0];
        double oy = pShot->param_d[1];
        double ang = pShot->param_d[2];

        double dist = 0.6 * t;
        double wob = 6.0 * sin(t * 0.35); // 羽音の微振動
        double perpAng = ang + DX_PI / 2.0;

        pShot->x = ox + cos(ang) * dist + cos(perpAng) * wob;
        pShot->y = oy + sin(ang) * dist + sin(perpAng) * wob;

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ3: 群飛乱舞
//  警戒セルから螺旋バーストを継続的にパルス発射しつつ、
//  一定間隔で自機狙い3wayを織り交ぜる。
//  speed==0を螺旋弾、speed>0を自機狙い弾の判別フラグとして流用。
// ============================================================
static void ShotSwarmBurst(sEnemyShotSet* pEnemyShotSet)
{
    const int PHASE_DUR = PH4_TELEGRAPH_START - PH3_START;

    // 螺旋バースト: 6フレーム毎に警戒セル全てから発射(パルス継続)
    if (pEnemyShotSet->count < PHASE_DUR && pEnemyShotSet->count % 6 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int c = 0; c < HEX_CELL_COUNT; c++) {
            if (!gMarkedCell[c]) continue;

            double dirSign = (c % 2 == 0) ? 1.0 : -1.0; // セルごとに回転方向を互い違いに
            double baseAng = atan2(gCellOy[c], gCellOx[c]);
            double ox = NEST_CENTER_X + gCellOx[c];
            double oy = NEST_CENTER_Y + gCellOy[c];

            for (int i = 0; i < 3; i++) {
                sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);
                pEnemyShot->x = ox;
                pEnemyShot->y = oy;
                pEnemyShot->speed = 0.0; // 螺旋弾フラグ
                pEnemyShot->param_d[0] = ox;
                pEnemyShot->param_d[1] = oy;
                pEnemyShot->param_d[2] = baseAng + i * (2.0 * DX_PI / 3.0);
                pEnemyShot->param_d[3] = dirSign;
                pEnemyShot->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 1 : 7];
            }
        }
    }

    // 自機狙い3way: 40フレーム毎に警戒セルから2箇所選んで発射
    if (pEnemyShotSet->count < PHASE_DUR && pEnemyShotSet->count % 40 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int fired = 0;
        for (int c = 0; c < HEX_CELL_COUNT && fired < 2; c++) {
            if (!gMarkedCell[c]) continue;
            fired++;

            double ox = NEST_CENTER_X + gCellOx[c];
            double oy = NEST_CENTER_Y + gCellOy[c];
            double aimAng = atan2(player.y - oy, player.x - ox);

            for (int w = -1; w <= 1; w++) {
                sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);
                pEnemyShot->x = ox;
                pEnemyShot->y = oy;
                pEnemyShot->speed = 2.6; // 自機狙い弾フラグ(>0)
                pEnemyShot->param_d[0] = ox;
                pEnemyShot->param_d[1] = oy;
                pEnemyShot->param_d[2] = aimAng + w * (DX_PI / 10.0);
                pEnemyShot->kind = img_enemyShotBullet[0]; // 赤
                pEnemyShot->muki = aimAng;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double ox = pShot->param_d[0];
        double oy = pShot->param_d[1];
        double ang = pShot->param_d[2];

        if (pShot->speed > 0.0) {
            // 自機狙い3way: 直進
            pShot->x = ox + cos(ang) * pShot->speed * t;
            pShot->y = oy + sin(ang) * pShot->speed * t;
        }
        else {
            // 螺旋バースト: 回転しながら外側へ加速
            double dirSign = pShot->param_d[3];
            double curAng = ang + dirSign * t * 0.05;
            double dist = 0.015 * t * t + 0.3 * t;
            pShot->x = ox + cos(curAng) * dist;
            pShot->y = oy + sin(curAng) * dist;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ4: 毒針一閃
//  全19セルから同時に自機狙いの高速針弾を放つ(一度きり)。
// ============================================================
static void ShotNeedle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int c = 0; c < HEX_CELL_COUNT; c++) {
            double ox = NEST_CENTER_X + gCellOx[c];
            double oy = NEST_CENTER_Y + gCellOy[c];
            double ang = atan2(player.y - oy, player.x - ox);

            sEnemyShot* pEnemyShot = AppendShot(pEnemyShotSet);
            pEnemyShot->x = ox;
            pEnemyShot->y = oy;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 6.5;
            pEnemyShot->param_d[0] = ox;
            pEnemyShot->param_d[1] = oy;
            pEnemyShot->param_d[2] = ang;
            pEnemyShot->kind = img_enemyShotBullet[7]; // 黒い毒針
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double ox = pShot->param_d[0];
        double oy = pShot->param_d[1];
        double ang = pShot->param_d[2];

        pShot->x = ox + cos(ang) * pShot->speed * t;
        pShot->y = oy + sin(ang) * pShot->speed * t;

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_Bee_Claude()
{
    if (count == 1) {
        InitHexGrid();
        enemy.x = NEST_CENTER_X;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        for (int c = 0; c < HEX_CELL_COUNT; c++) gMarkedCell[c] = false;
    }
    else {
        enemy.x = NEST_CENTER_X + 14.0 * sin(count * 0.02);
        enemy.y = 45.0 + 4.0 * sin(count * 0.05);
    }

    int local = (count - 1) % LOOP_LEN;

    // フェーズ1開始: 巣の輪郭セットを生成(ループの先頭で1回だけ)
    if (local == 0) {
        CreateShotSet(ShotNestOutline);
    }

    // フェーズ2開始: 警戒セルを決定し、偵察羽音セットを生成
    if (local == PH2_START) {
        for (int c = 0; c < HEX_CELL_COUNT; c++) gMarkedCell[c] = false;
        int marked = 0;
        while (marked < MARKED_CELL_COUNT) {
            int c = GetRand(HEX_CELL_COUNT - 1);
            if (!gMarkedCell[c]) { gMarkedCell[c] = true; marked++; }
        }
        CreateShotSet(ShotReconLeak);
    }

    // フェーズ3開始: 群飛乱舞セットを生成
    if (local == PH3_START) {
        CreateShotSet(ShotSwarmBurst);
    }

    // フェーズ4予告: 全セル赤点滅のための合図音のみ(点滅描画はShotNestOutline側で処理)
    if (local == PH4_TELEGRAPH_START) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズ4: 毒針発射セットを生成(この直後、ShotNestOutline側が自動で崩壊演出に入る)
    if (local == PH4_NEEDLE_FRAME) {
        CreateShotSet(ShotNeedle);
    }
}