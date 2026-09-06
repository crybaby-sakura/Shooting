// enemyPat_phalanx.cpp
//
// 弾幕「ファランクスの槍壁」
//  ・盾(壁)   : 短レーザー(白)   … ゆっくり進軍する3行の壁(2列だけ隙間がある)
//  ・槍(突撃) : 中玉(赤)         … 予告音の後、壁の隙間を貫通して飛ぶ高速弾
//  ・翼(包囲) : 中楕円弾(緑)     … 画面左右から中央へ斜め進軍しV字で挟み込む
//  ・散兵     : 小玉(黄)         … V字完成と同時に扇状へ一斉投擲
//
// 仕様メモ:
//  - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントは
//    メインルーチン側で行うため、このファイル内では加算しない。
//  - 画面外へ出た弾の消去もメインルーチン側で行われる。
//  - GetRand(x) は 0〜x の「x+1種類」を返す点に注意して使用している。
//  - 弾の移動はサンプルと同様、各パターン関数内で行う。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
namespace {
    // ---- 盾の壁 ----
    const int    WALL_SEG_NUM = 8;     // 1行あたりのレーザー本数
    const double WALL_PITCH = 70.0;  // レーザーの横間隔
    const double WALL_X0 = 0.0;  // 左端レーザーの中心x
    const double WALL_Y0 = 90.0;  // 壁の出現y
    const double WALL_SPEED = 1.5;   // 壁の進軍速度(下向き)
    const int    WALL_ROW_STEP = 30;    // 行の発射間隔(フレーム)
    const int    WALL_ROW_NUM = 3;     // 行数
    const double WALL_LINE_GAP = 12.0;  // 1行内の上下2本の間隔

    // ---- 槍 ----
    const int    SPEAR_DELAY1 = 50;    // 第1波(隙間貫通)までの猶予
    const int    SPEAR_DELAY2 = 110;   // 第2波(狙い撃ち)までの猶予
    const double SPEAR_SPEED1 = 5.5;   // 隙間貫通の速度
    const double SPEAR_SPEED2 = 4.5;   // 狙い撃ちの速度

    // ---- 両翼 ----
    const int    WING_NUM = 8 * 3;     // 片翼の兵数
    const double WING_SPEED = 2.2;
    const double WING_TX = 240.0; // 収束点(画面中央やや下)
    const double WING_TY = 230.0;

    // ---- 散兵 ----
    const int    SKIRMISH_NUM = 9 * 3;

    // ---- 全体サイクル ----
    const int    CYCLE = 720;   // 1サイクル(フレーム)
}

// ============================================================
//  共通ヘルパ
// ============================================================

// 弾をセットの双方向リストへ登録
static void AddShot(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->margin = 120;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// muki方向へ直進させる(消去はメインルーチン任せ)
static void MoveShots(sEnemyShotSet* pSet)
{
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ショットセットを新規作成してリストへ登録
static sEnemyShotSet* CreateShotSet(void (*func)(sEnemyShotSet*), double x, double y, double muki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;

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
//  フェーズ1：盾の壁
//  param_i[0], param_i[1] : 隙間となる列(0〜WALL_SEG_NUM-1)
// ============================================================
static void ShotPhalanxWall(sEnemyShotSet* pSet)
{
    // count 0, 30, 60 フレーム目に1行ずつ発射
    if (pSet->count % WALL_ROW_STEP == 0 && pSet->count / WALL_ROW_STEP < WALL_ROW_NUM) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int col = 0; col < WALL_SEG_NUM; col++) {
            if (col == pSet->param_i[0] || col == pSet->param_i[1]) continue; // 隙間列は空ける
            for (int line = 0; line < 2; line++) { // 1行を上下2本のレーザーで分厚く
                sEnemyShot* p = new sEnemyShot;
                p->x = WALL_X0 + WALL_PITCH * col;
                p->y = WALL_Y0 + WALL_LINE_GAP * line;
                p->muki = 0.0;                   // 横長のまま=「盾」の見た目
                p->speed = WALL_SPEED;           // 進軍方向は下(独自移動)
                p->kind = img_enemyShotLaser[6]; // 白
                AddShot(pSet, p);
            }
        }
    }

    // 盾はまっすぐ下へ進軍(mukiを使わない独自移動)
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->y += p->speed;
        p = p->next;
    }
}

// ============================================================
//  フェーズ2：槍の突撃
//  param_i[0], param_i[1] : 貫通する隙間列
// ============================================================
static void ShotPhalanxSpear(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 予告音(チャージ)。ここから数フレーム後に槍が飛ぶ
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pSet->count == SPEAR_DELAY1) {
        // 第1波：壁の隙間(回廊)を貫通する槍
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 2; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = WALL_X0 + WALL_PITCH * pSet->param_i[i]; // 隙間の列
            p->y = pSet->y;
            p->muki = DX_PI / 2.0;                 // 下向き
            p->speed = SPEAR_SPEED1;               // 壁より速く、隙間を突き抜ける
            p->kind = img_enemyShotMediumBall[0];  // 赤
            AddShot(pSet, p);
        }
    }
    else if (pSet->count == SPEAR_DELAY2) {
        // 第2波：隙間2本 + プレイヤー狙い1本(回廊滞留への牽制)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 2; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = WALL_X0 + WALL_PITCH * pSet->param_i[i];
            p->y = pSet->y;
            p->muki = DX_PI / 2.0;
            p->speed = SPEAR_SPEED2;
            p->kind = img_enemyShotMediumBall[0];
            AddShot(pSet, p);
        }

        sEnemyShot* p = new sEnemyShot;
        p->x = player.x;
        p->y = pSet->y;
        p->muki = atan2(player.y - p->y, player.x - p->x);
        p->speed = SPEAR_SPEED2 * 2;
        p->kind = img_enemyShotMediumBall[0];
        AddShot(pSet, p);
    }

    MoveShots(pSet);
}

// ============================================================
//  フェーズ3：両翼包囲
//  左右の縦隊が中央の収束点へ向かい、V字でプレイヤーを挟む
// ============================================================
static void ShotPhalanxWing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < WING_NUM; i++) {
            double y = 60.0 + 26.0 * i - 60;

            // 左翼
            sEnemyShot* pL = new sEnemyShot;
            pL->x = -10.0;
            pL->y = y;
            pL->muki = atan2(WING_TY - y, WING_TX - pL->x); // 収束点へ向く
            pL->speed = WING_SPEED;
            pL->kind = img_enemyShotMediumOval[2];          // 緑
            AddShot(pSet, pL);

            // 右翼
            sEnemyShot* pR = new sEnemyShot;
            pR->x = 490.0;
            pR->y = y;
            pR->muki = atan2(WING_TY - y, WING_TX - pR->x);
            pR->speed = WING_SPEED;
            pR->kind = img_enemyShotMediumOval[2];
            AddShot(pSet, pR);
        }
    }

    MoveShots(pSet);
}

// ============================================================
//  散兵の一斉投擲(V字収束に合わせた扇状弾)
// ============================================================
static void ShotPhalanxSkirmish(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - pSet->y, player.x - pSet->x);

        for (int i = 0; i < SKIRMISH_NUM; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = base + (i - (SKIRMISH_NUM - 1) / 2.0) * (10.0 / 180.0 * DX_PI) / 2; // ±40度
            p->speed = 2.4 + 0.5 * (i % 2);                                           // 2速交互で波を作る
            p->kind = img_enemyShotSmallBall[1];                                      // 黄
            AddShot(pSet, p);
        }
    }

    MoveShots(pSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Phalanx_Zai()
{
    static int muki;
    static int gap1, gap2; // 現在の盾の壁の隙間列(槍と共有)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        gap1 = gap2 = 0;
    }
    else {
        // ファランクスの行軍: ゆっくり左右に歩み寄る
        enemy.x += 0.7 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    int c = count % CYCLE;

    // ---- 盾の壁 ×3 + 槍 ×3 (フェーズ1&2) ----
    if (c == 60 || c == 210 || c == 360) {
        // 隙間列を決定(毎回異なる回廊が開く)
        // GetRand(WALL_SEG_NUM-1) は 0〜7 の8種類
        gap1 = GetRand(WALL_SEG_NUM - 1);
        // 2以上離れた別の列を必ず選ぶ
        gap2 = (gap1 + 2 + GetRand(WALL_SEG_NUM - 4)) % WALL_SEG_NUM;

        sEnemyShotSet* pSet = CreateShotSet(ShotPhalanxWall, 240.0, WALL_Y0, 0.0);
        pSet->param_i[0] = gap1;
        pSet->param_i[1] = gap2;
    }
    if (c == 100 || c == 250 || c == 400) {
        // 盾の40フレーム後に槍(チャージ音→隙間貫通→狙い撃ち)
        sEnemyShotSet* pSet = CreateShotSet(ShotPhalanxSpear, enemy.x, enemy.y + 10.0, DX_PI / 2.0);
        pSet->param_i[0] = gap1;
        pSet->param_i[1] = gap2;
    }

    // ---- 両翼包囲 (フェーズ3) ----
    if (c == 540) {
        CreateShotSet(ShotPhalanxWing, 240.0, 40.0, 0.0);
    }

    // ---- 散兵の一斉投擲(翼の収束タイミングに合わせる) ----
    if (c == 660) {
        CreateShotSet(ShotPhalanxSkirmish, enemy.x, enemy.y + 10.0, 0.0);
    }
}