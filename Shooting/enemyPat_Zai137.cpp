//----------------------------------------------------------------
//  弾幕「終律輪舞 〜汝の過去を射貫け〜」
//  敵本体パターン: void EnemyPat_TenYears_Zai()
//
//  構成(4層同時進行)
//   第1層 千刻螺旋 : 8本の腕が回転しながら3段階の速さの弾を放ち蓄積する
//   第2層 残像十秒 : 10秒前の自機の移動軌跡を正確にトレースする幽霊弾
//   第3層 鏡界     : 円形領域内に入った弾は進行方向が左右反転する(30秒ごとに移動)
//   第4層 未踏の道 : 一度でも自機が通過した8x8マスは地雷化(再進入で炸裂)
//
//  注意:
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//     画面外の弾の消去はメインルーチンで行われる前提。
//   ・GetRand(x) は 0〜x の x+1 種類を返すので +1 に注意して使用。
//   ・リプレイ再現性のため、乱数は GetRand のみを使用。
//----------------------------------------------------------------
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ================= 定数 =================
static const int    GRID_NUM = 8;
static const double CELL_SIZE = 480.0 / GRID_NUM;   // 1マス60px
static const int    HIST_SIZE = 1200;               // 軌跡リングバッファ
static const int    GHOST_LAG = 600;                // 10秒 = 60fps * 10
static const int    MIRROR_NUM = 2;
static const double MIRROR_R = 70.0;
static const int    MIRROR_INTERVAL = 1800;               // 30秒ごとに鏡界が移動

// ================= パターン共通の状態 =================
static double pxHist[HIST_SIZE];        // 自機x座標の履歴
static double pyHist[HIST_SIZE];        // 自機y座標の履歴
static int    histHead = 0;           // 次に書き込むスロット
static int    histFrames = 0;           // 記録済みフレーム数
static bool   cellVisited[GRID_NUM][GRID_NUM]; // 第4層:踏査済みマス
static int    mineCooldown = 0;         // 地雷の連続起爆防止
static double mirrorX[MIRROR_NUM];      // 第3層:鏡界の中心
static double mirrorY[MIRROR_NUM];

// ================= ユーティリティ =================
static int ToCellIndex(double v)
{
    int c = (int)(v / CELL_SIZE);
    if (c < 0) c = 0;
    if (c >= GRID_NUM) c = GRID_NUM - 1;
    return c;
}
static double CellCenter(int c)
{
    return c * CELL_SIZE + CELL_SIZE / 2.0;
}

// 弾をセットの弾リスト末尾に追加
static void AddBullet(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = 480;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕セットを生成してリストに接続
static sEnemyShotSet* CreateShotSet(double x, double y, double muki,
    void (*func)(sEnemyShotSet*), int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;
    pSet->alive = 9999;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// 鏡界の2つの円を離して配置(起爆音などの乱数消費を避けるため専用にしない)
static void PlaceMirrors()
{
    for (int i = 0; i < MIRROR_NUM; i++) {
        do {
            // GetRand(240) は 0〜240 を返すので +120 で 120〜360 の範囲
            mirrorX[i] = 120.0 + GetRand(240);
            mirrorY[i] = 120.0 + GetRand(240);
        } while (i > 0 &&
            (mirrorX[i] - mirrorX[0]) * (mirrorX[i] - mirrorX[0]) +
            (mirrorY[i] - mirrorY[0]) * (mirrorY[i] - mirrorY[0]) < 160.0 * 160.0);
    }
}

// 第3層:鏡界 ― 円内に入った瞬間に進行方向を左右反転する
static void MirrorBullet(sEnemyShot* pShot)
{
    for (int i = 0; i < MIRROR_NUM; i++) {
        double dx = pShot->x - mirrorX[i];
        double dy = pShot->y - mirrorY[i];
        bool inside = (dx * dx + dy * dy) <= MIRROR_R * MIRROR_R;
        if (inside && pShot->param_i[0] == 0) {
            pShot->muki = DX_PI - pShot->muki;   // 左右反転
            pShot->param_i[0] = 1;               // 円内フラグ(重複反転防止)
        }
        else if (!inside && pShot->param_i[0] == 1) {
            pShot->param_i[0] = 0;               // 円外に出たらリセット
        }
    }
}

//----------------------------------------------------------------
// 弾幕第1層:千刻螺旋
//  8本の腕がゆっくり回転しながら、速さ3段階の弾を放つ。
//  遅い弾(シアン)ほど画面に残り、時間とともに密度が蓄積する。
//----------------------------------------------------------------
static void ShotSenkokuSpiral(sEnemyShotSet* pSet)
{
    // 敵本体に追従
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count >= 60 && pSet->count % 30 == 0) {   // 開幕60フレームは猶予
        if (pSet->count % 90 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        pSet->param_d[0] += 0.021;   // 腕の回転角を進める
        for (int i = 0; i < 8; i++) {
            double m = pSet->param_d[0] + i * DX_PI / 4.0;
            double sp = 1.6 + (i % 3) * 0.8;            // 1.6 / 2.4 / 3.2 の3段階
            int    col = (i % 3 == 0) ? 3 : 0;           // 遅弾はシアン、他は赤
            AddBullet(pSet, pSet->x, pSet->y, m, sp, img_enemyShotSmallBall[col]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        MirrorBullet(pShot);   // 第3層:鏡界で反射する
        pShot = pShot->next;
    }
}

//----------------------------------------------------------------
// 弾幕第2層:残像十秒
//  10秒前の自機の移動軌跡を正確にトレースする幽霊弾。
//  軌跡を辿り切ったら最後の移動方向へ突進して画面外へ抜ける。
//----------------------------------------------------------------
static void ShotZanzouGhost(sEnemyShotSet* pSet)
{
    if (pSet->count >= 120 && pSet->count % 45 == 0) {   // 記録が溜まってから出現
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int lag = GHOST_LAG;
        if (histFrames < lag) lag = histFrames;   // 記録が未達の分は最古の地点から
        int start = (histHead - lag + HIST_SIZE) % HIST_SIZE;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pxHist[start];
        pShot->y = pyHist[start];
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotDiamond[6];   // 白い菱形弾
        pShot->param_i[0] = start;               // 軌跡の起点
        pShot->param_i[1] = lag;                 // 辿る長さ
        pShot->param_i[2] = 0;                   // 0:トレース中 1:突進中
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[2] == 0) {
            if (pShot->count < pShot->param_i[1]) {
                // 10秒前の自分の移動をそのまま再現(位置を直接代入)
                int idx = (pShot->param_i[0] + pShot->count) % HIST_SIZE;
                pShot->x = pxHist[idx];
                pShot->y = pyHist[idx];
            }
            else {
                // 軌跡を辿り切ったら最後の移動方向へ突進
                int idx = (pShot->param_i[0] + pShot->param_i[1] - 1 + HIST_SIZE) % HIST_SIZE;
                int pre = (idx - 1 + HIST_SIZE) % HIST_SIZE;
                double dx = pxHist[idx] - pxHist[pre];
                double dy = pyHist[idx] - pyHist[pre];
                pShot->muki = (dx * dx + dy * dy > 0.0001) ? atan2(dy, dx) : DX_PI / 2.0;
                pShot->speed = 6.0;
                pShot->param_i[2] = 1;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

//----------------------------------------------------------------
// 弾幕第4層:未踏の道(地雷炸裂)
//  踏査済みマスに再び入ると30フレーム後に2重の環状弾が炸裂する。
//----------------------------------------------------------------
static void ShotMichiMine(sEnemyShotSet* pSet)
{
    if (pSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        // 内周:中玉(橙)・やや不規則
        for (int i = 0; i < 24; i++) {
            double m = i * DX_PI / 12.0 + (GetRand(20) - 10) * DX_PI / 360.0;
            AddBullet(pSet, pSet->x, pSet->y, m, 2.8, img_enemyShotMediumBall[8]);
        }
        // 外周:小玉(黄)・速い
        for (int i = 0; i < 24; i++) {
            double m = i * DX_PI / 12.0 + DX_PI / 24.0 + (GetRand(20) - 10) * DX_PI / 360.0;
            AddBullet(pSet, pSet->x, pSet->y, m, 4.2, img_enemyShotSmallBall[1]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//----------------------------------------------------------------
// 敵本体のパターン
//----------------------------------------------------------------
void EnemyPat_TenYears_Zai()
{
    // ---- 初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと

        // 状態のリセット
        for (int i = 0; i < HIST_SIZE; i++) { pxHist[i] = 240.0; pyHist[i] = 460.0; }
        histHead = 0;
        histFrames = 0;
        mineCooldown = 0;
        for (int gy = 0; gy < GRID_NUM; gy++)
            for (int gx = 0; gx < GRID_NUM; gx++)
                cellVisited[gy][gx] = false;

        // 第3層:鏡界の初期配置
        PlaceMirrors();

        // 各層の弾幕セットを生成(1回だけ)
        CreateShotSet(enemy.x, enemy.y + 10.0, 0.0, ShotSenkokuSpiral, 0); // 第1層
        CreateShotSet(enemy.x, enemy.y + 10.0, 0.0, ShotZanzouGhost, 1);  // 第2層

        // 開幕の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }

    // ---- 敵本体の動き(ゆっくり左右に揺れる) ----
    enemy.x = 240.0 + sin(count * 0.008) * 90.0;

    // ---- 自機の軌跡を記録(第2層・第4層の共通データ) ----
    pxHist[histHead] = player.x;
    pyHist[histHead] = player.y;
    histHead = (histHead + 1) % HIST_SIZE;
    if (histFrames < HIST_SIZE) histFrames++;

    // ---- 第3層:鏡界(30秒ごとに円が移動) ----
    if (count % MIRROR_INTERVAL == 0) {
        PlaceMirrors();
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);   // 移動の予告音
    }

    // ---- 第4層:未踏の道(8x8マスの踏査判定) ----
    if (mineCooldown > 0) mineCooldown--;
    int cx = ToCellIndex(player.x);
    int cy = ToCellIndex(player.y);
    if (!cellVisited[cy][cx]) {
        cellVisited[cy][cx] = true;   // 初通過マスは足跡が付くだけ
    }
    else if (mineCooldown == 0) {
        // 既に通過したマス = 地雷。30フレーム後に炸裂するセットを生成
        mineCooldown = 90;
        CreateShotSet(CellCenter(cx), CellCenter(cy), 0.0, ShotMichiMine, 2);
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}