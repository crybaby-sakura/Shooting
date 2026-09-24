// enemyPat_Tmp.cpp
// スペルカード：「雨述符 『雷鳴恵みの豪雨』」
// 雨下(小玉 青/白のS字落下) ＋ かみなり(中玉の縦列予兆 → 時差落雷 → 放射飛散)
//
// ※ img_enemyShot*** / sound_enemy*** / player / enemy / enemyShotSetHead / count
//   は enemyPat_sampleForAI.cpp と同じく外部で宣言されているものを使用します。

#include <math.h>
#include "DxLib.h"
#include "gv.h"

// ============================================================
//  定数
// ============================================================
static const double PI2 = DX_PI * 2.0;

// --- 雨 ---
static const int    RAIN_INTERVAL = 240;   // 雨セットを生成する間隔(フレーム)
static const int    RAIN_LIFE = 240;   // 各雨セットが雨を降らせるフレーム数

// --- 雷 ---
static const int    LIGHTNING_COLS = 10;    // 予兆の縦列を構成する弾数
static const double COL_SPACING = 22.0;  // 縦列の弾の間隔
static const double BEAM_SPEED = 9.0;   // 落雷(時差発射)の速度
static const int    BURST_NUM = 8;     // 着雷時の放射弾数

// ============================================================
//  雨：小玉(青・白)がS字に揺れながら降り続く
// ============================================================
static void ShotRain(sEnemyShotSet* pEnemyShotSet)
{
    // 降らせ期間中は毎フレーム雨粒を生成
    if (pEnemyShotSet->count % 6 == 0 && pEnemyShotSet->count <= RAIN_LIFE) {
        // HP半分以下(雷鳴ラッシュ期)は雨も強める
        int num = (enemy.hp <= enemy.maxHp / 2) ? 2 : 1;

        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = (double)(GetRand(500) - 10); // -10 ～ 490
            pEnemyShot->y = -10.0;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0 - 0.5;
            pEnemyShot->param_d[0] = GetRand(628) / 100.0; // 揺れの位相 0～6.28
            pEnemyShot->param_d[1] = pEnemyShot->speed;    // 基本速度を保持

            // 青(4)と白(6)の小玉をランダムに混ぜる
            pEnemyShot->kind = (GetRand(2) == 0) ? img_enemyShotSmallBall[4]
                : img_enemyShotSmallBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // S字に揺れながら落下(雨粒らしいふらつき)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki = DX_PI / 2.0 + 0.30 * sin(pShot->count * 0.08 + pShot->param_d[0]);
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  かみなり：中玉の縦列予兆 → 時差落雷 → 着雷点から放射
// ============================================================
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 予兆：目標地点の真上に中玉(白)を縦一列に静止表示
        for (int i = 0; i < LIGHTNING_COLS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = 10.0 + i * COL_SPACING;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;          // 予兆中は静止
            pEnemyShot->param_i[0] = i * 2;   // 発射ディレイ(上から順に遅延)
            pEnemyShot->param_i[1] = 0;       // 0:未発射 1:発射済み
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
            pEnemyShot->margin = 120;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 一番下の弾(18フレーム後に発射)が目標地点に届くフレームを計算
        int burst = (int)(18 + (pEnemyShotSet->y
            - (10.0 + (LIGHTNING_COLS - 1) * COL_SPACING)) / BEAM_SPEED);
        if (burst < 30) burst = 30;
        pEnemyShotSet->param_i[0] = burst;    // 着雷フレームを保存
    }
    else if (pEnemyShotSet->count == pEnemyShotSet->param_i[0]) {
        // 落雷音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 着雷：地面に当たって飛び散る放射弾(中玉 黄)
        for (int i = 0; i < BURST_NUM; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = PI2 / BURST_NUM * i
                + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄
            pEnemyShot->param_i[1] = 2;
            pEnemyShot->margin = 120;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新(予兆 → 時差発射 → 落下)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 発射タイミングになったら真下へ向けて発射
        if (pShot->param_i[1] == 0 && pShot->count >= pShot->param_i[0]) {
            pShot->param_i[1] = 1;
            pShot->muki = DX_PI / 2.0;
            pShot->speed = BEAM_SPEED;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_ThunderInRain_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ---- 雨(常時)：一定間隔で雨セットを生成し、切れ目なく降らし続ける ----
    if (count % RAIN_INTERVAL == 2) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRain;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;
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

    // ---- 落雷：プレイヤーの現在地(少しブレて)へ予兆→落雷 ----
    // HP半分以下は「雷鳴ラッシュ」：間隔を詰めて連続落雷
    int interval = (enemy.hp <= enemy.maxHp / 2) ? 120 : 160;
    if (count > 60 && count % interval == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightning;

        // 狙いはプレイヤー位置 + ±40px のブレ
        pEnemyShotSet->x = player.x + (GetRand(81) - 40);
        if (pEnemyShotSet->x < 20.0)  pEnemyShotSet->x = 20.0;
        if (pEnemyShotSet->x > 460.0) pEnemyShotSet->x = 460.0;
        pEnemyShotSet->y = player.y;   // 目標(着雷)地点の高さ
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