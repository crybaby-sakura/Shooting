#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//----------------------------------------------------------
// 設定
//----------------------------------------------------------
static const int CRADLE_NUM = 5;
static const int STRING_DOTS = 10;
static const int CRADLE_CYCLE = 240; // 4秒で1往復
static const double CRADLE_PITCH = 36.0;
static const double CRADLE_LENGTH = 160.0;
static const double CRADLE_AMP = 60.0 * DX_PI / 180.0;

//----------------------------------------------------------
// 前方宣言
//----------------------------------------------------------
static void ShotNewtonCradle(sEnemyShotSet* pSet);
static void ShotCradleSpill(sEnemyShotSet* pSet);
static void ShotImpactRing(sEnemyShotSet* pSet);

//----------------------------------------------------------
// 攻撃弾生成ヘルパー
//----------------------------------------------------------
static void CreateSpillSet(double x, double y, double baseMuki)
{
    sEnemyShotSet* pSet = new sEnemyShotSet();
    pSet->count = 0;
    pSet->patternFunc = ShotCradleSpill;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = baseMuki; // プレイヤー方向
    pSet->kind = 0;
    pSet->pEnemyShotHead = new sEnemyShot();
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

static void CreateImpactSet(double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet();
    pSet->count = 0;
    pSet->patternFunc = ShotImpactRing;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;
    pSet->pEnemyShotHead = new sEnemyShot();
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
}

//----------------------------------------------------------
// こぼれ弾：最高点から自機狙い3WAY
//----------------------------------------------------------
static void ShotCradleSpill(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot();
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // -15度, 0度, +15度 + 少しランダム
            // GetRand(x)は0〜xを返すので -10〜+10 は GetRand(20)-10
            double offset = (i - 2) * 10.0 * DX_PI / 180.0 + (GetRand(20) - 10) * DX_PI / 180.0;
            pShot->muki = pEnemyShotSet->muki + offset;
            pShot->speed = 2.2 + GetRand(60) / 100.0; // 2.2〜2.8
            // 赤と橙で交互に
            pShot->kind = (i == 1) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[8];
            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

//----------------------------------------------------------
// 衝突エフェクト：リング
//----------------------------------------------------------
static void ShotImpactRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 12; i++) for (int j = 0; j < 20; j++) {
            sEnemyShot* pShot = new sEnemyShot();
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = i * (360.0 / 12.0) * DX_PI / 180.0 + j * 0.05;
            pShot->speed = 2.0 + GetRand(40) / 100.0 + j * 0.1;
            pShot->kind = img_enemyShotSmallBall[6]; // 白小玉
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

//----------------------------------------------------------
// 本体：ニュートンのゆりかご
//----------------------------------------------------------
static void ShotNewtonCradle(sEnemyShotSet* pEnemyShotSet)
{
    // 初回：おもり2種 + 糸10発を5セット = 60発生成
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = CRADLE_NUM;
        pEnemyShotSet->param_i[1] = STRING_DOTS;
        pEnemyShotSet->param_d[0] = CRADLE_PITCH;
        pEnemyShotSet->param_d[1] = CRADLE_LENGTH;
        pEnemyShotSet->param_d[2] = CRADLE_AMP;

        for (int i = 0; i < CRADLE_NUM; i++) {
            // 大玉：本体
            {
                sEnemyShot* p = new sEnemyShot();
                p->param_i[0] = i; // 振り子番号
                p->param_i[1] = 0; // 0:大玉本体
                p->param_i[2] = 0;
                p->kind = img_enemyShotLargeBall[6]; // 白
                p->speed = 0; p->muki = 0;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            // 中玉：ハイライト
            {
                sEnemyShot* p = new sEnemyShot();
                p->param_i[0] = i;
                p->param_i[1] = 1; // 1:ハイライト
                p->param_i[2] = 0;
                p->kind = img_enemyShotMediumBall[6]; // 白
                p->speed = 0; p->muki = 0;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            // 糸
            for (int j = 0; j < STRING_DOTS; j++) {
                sEnemyShot* p = new sEnemyShot();
                p->param_i[0] = i;
                p->param_i[1] = 2; // 2:糸
                p->param_i[2] = j;
                p->kind = img_enemyShotSmallBall[6]; // 極小白
                p->speed = 0; p->muki = 0;
                p->margin = 100.0; // 糸は画面外判定を甘く
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // --- 角度計算 ---
    int t = pEnemyShotSet->count % CRADLE_CYCLE;
    double theta[CRADLE_NUM] = { 0 };
    double amp = pEnemyShotSet->param_d[2];

    if (t < 60) { // 左が落ちる
        double prog = (double)t / 60.0;
        double cur = amp * cos(prog * DX_PI * 0.5); // amp -> 0
        theta[0] = -cur; theta[1] = -cur;
    }
    else if (t < 120) { // 右が上がる
        double prog = (double)(t - 60) / 60.0;
        double cur = amp * sin(prog * DX_PI * 0.5); // 0 -> amp
        theta[3] = cur; theta[4] = cur;
    }
    else if (t < 180) { // 右が落ちる
        double prog = (double)(t - 120) / 60.0;
        double cur = amp * cos(prog * DX_PI * 0.5); // amp -> 0
        theta[3] = cur; theta[4] = cur;
    }
    else { // 左が上がる
        double prog = (double)(t - 180) / 60.0;
        double cur = amp * sin(prog * DX_PI * 0.5); // 0 -> amp
        theta[0] = -cur; theta[1] = -cur;
    }
    // 中央は常に0

    // 支点とおもり位置を計算
    double pivotX[CRADLE_NUM], ballX[CRADLE_NUM], ballY[CRADLE_NUM];
    double pitch = pEnemyShotSet->param_d[0];
    double len = pEnemyShotSet->param_d[1];
    for (int i = 0; i < CRADLE_NUM; i++) {
        pivotX[i] = pEnemyShotSet->x + (i - 2) * pitch;
        ballX[i] = pivotX[i] + len * sin(theta[i]);
        ballY[i] = pEnemyShotSet->y + len * cos(theta[i]);
    }

    // --- 攻撃発生 ---
    if (t == 0) { // 左最高点
        for (int i = 0; i < 2; i++) {
            double m = atan2(player.y - ballY[i], player.x - ballX[i]);
            CreateSpillSet(ballX[i], ballY[i], m);
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    if (t == 120) { // 右最高点
        for (int i = 3; i < 5; i++) {
            double m = atan2(player.y - ballY[i], player.x - ballX[i]);
            CreateSpillSet(ballX[i], ballY[i], m);
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    if (t == 60 || t == 180) { // 衝突点：中央でリング
        CreateImpactSet(ballX[2], ballY[2]);
    }

    // --- 座標反映 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int idx = pShot->param_i[0];
        int type = pShot->param_i[1];
        int dotNo = pShot->param_i[2];
        if (idx < 0 || idx >= CRADLE_NUM) { pShot = pShot->next; continue; }

        if (type == 0) { // 大玉本体
            pShot->x = ballX[idx];
            pShot->y = ballY[idx];
        }
        else if (type == 1) { // ハイライト：少し左上にずらす
            pShot->x = ballX[idx] - 4.0;
            pShot->y = ballY[idx] - 4.0;
        }
        else if (type == 2) { // 糸ドット：支点とおもりを線形補間
            double rate = (double)(dotNo + 1) / (double)(STRING_DOTS + 1);
            pShot->x = pivotX[idx] + (ballX[idx] - pivotX[idx]) * rate;
            pShot->y = pEnemyShotSet->y + (ballY[idx] - pEnemyShotSet->y) * rate;
        }
        pShot = pShot->next;
    }
}

//----------------------------------------------------------
// 敵本体
//----------------------------------------------------------
void EnemyPat_NewtonsCradle_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        sEnemyShotSet* pSet = new sEnemyShotSet();
        pSet->count = 0;
        pSet->patternFunc = ShotNewtonCradle;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0; // 支点
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot();
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // ゆりかごは固定の方が美しいので移動しない
        // もし揺らしたい場合は enemy.x を少しsin波させる
        // enemy.x = 240.0 + sin(count * DX_PI / 180.0) * 30.0;
        // 作成したセットの支点xも追従させる
        sEnemyShotSet* p = enemyShotSetHead.next;
        while (p != &enemyShotSetHead) {
            if (p->patternFunc == ShotNewtonCradle) {
                p->x = enemy.x;
            }
            p = p->next;
        }
    }
}