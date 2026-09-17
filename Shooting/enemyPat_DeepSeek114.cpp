// ============================================================
//  enemyPat_tenkawaSeira.cpp
//
//  東方星舞譚 6面ボス 天川星羅
//  流符「夜空のカーペット」
//
//  夜空に広がる星々の絨毯が、ゆっくりと流れ落ちる弾幕。
//  層ごとに色と弾種を変え、互い違いに隙間を配置することで、
//  「織り目」のような美しい流れを作り出す。
//  さらに自機狙いの星弾を混ぜ、流れを読む力を要求する。
// ============================================================

#include "gv.h"
#include <cmath>

// ------------------------------------------------------------
//  ヘルパー関数 前方宣言
// ------------------------------------------------------------
static void ShotNightCarpet(sEnemyShotSet* pEnemyShotSet);

// ------------------------------------------------------------
//  弾幕：夜空のカーペット
//
//  sEnemyShotSet の param に設定値を格納しておき、
//  この関数が毎フレーム呼ばれることで弾幕を形成する。
//
//  param_i[0] : 層の数
//  param_i[1] : 1層あたりの弾数
//  param_i[2] : 弾の間隔(px)
//  param_d[0] : 基準速度
//  param_d[1] : 横揺れ振幅
//  param_d[2] : 横揺れ周波数
// ------------------------------------------------------------
static void ShotNightCarpet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int    LAYERS = pEnemyShotSet->param_i[0];
    const int    PER_ROW = pEnemyShotSet->param_i[1];
    const int    SPACING = pEnemyShotSet->param_i[2];
    const double FLOW_SPEED = pEnemyShotSet->param_d[0];
    const double WAVE_AMP = pEnemyShotSet->param_d[1];
    const double WAVE_FRQ = pEnemyShotSet->param_d[2];
    const int    LAYER_DELAY = 14; // 層を出す間隔（フレーム）

    // --------------------------------------------------------
    //  カーペットの生成（層ごとに時間差で横一列を敷く）
    // --------------------------------------------------------
    if (pEnemyShotSet->count < LAYERS * LAYER_DELAY
        && pEnemyShotSet->count % LAYER_DELAY == 0)
    {
        const int layer = pEnemyShotSet->count / LAYER_DELAY;

        // 効果音：最初の層は中音、以降は軽音
        if (layer == 0) {
            if (CheckSoundMem(sound_enemyShot_medium))
                StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_light))
                StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 層ごとの色（夜空を彩る青・白・シアン・マゼンタ）
        static const int layerColor[4] = { 4, 6, 3, 5 };
        const int color = layerColor[layer % 4];

        // 層ごとの弾種（星屑を思わせる小玉・鱗弾・菱形弾）
        const int kindIdx = layer % 3;

        // 層ごとの横オフセット（互い違いの織り目）
        const double offsetX = (layer % 2) ? SPACING * 0.5 : 0.0;

        // 隙間の位置（波ごとにローテーションして流れを作る）
        const int holeBase = (pEnemyShotSet->kind * 3) % PER_ROW;
        const int hole1 = holeBase;
        const int hole2 = (holeBase + PER_ROW / 3) % PER_ROW;

        // 弾を横一列に配置
        for (int i = 0; i < PER_ROW; i++) {
            // 隙間をあけて、プレイヤーが通れる安全地帯を作る
            if (i == hole1 || i == hole2) continue;

            pEnemyShot = new sEnemyShot;

            // 位置：画面幅480を覆うように、ボス位置を中心に展開
            const double baseX = (i - PER_ROW / 2.0) * SPACING + offsetX;

            pEnemyShot->x = pEnemyShotSet->x + baseX
                + (GetRand(10) - 5) * 0.5;
            pEnemyShot->y = pEnemyShotSet->y + layer * 7.0
                + (GetRand(10) - 5) * 0.5;

            // 向き：下向きを基本に、わずかに左右へ散らす
            pEnemyShot->muki = DX_PI / 2.0
                + (GetRand(20) - 10) / 180.0 * DX_PI;

            // 速度：層が下がるほど少し速く（絨毯が流れ落ちる感）
            pEnemyShot->speed = FLOW_SPEED + layer * 0.06
                + GetRand(40) / 100.0;

            // 弾の種類：星屑
            switch (kindIdx) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[color]; break;
            case 1: pEnemyShot->kind = img_enemyShotScale[color];     break;
            case 2: pEnemyShot->kind = img_enemyShotDiamond[color];   break;
            }

            // 横揺れの位相を弾ごとにずらす（流れのうねり）
            pEnemyShot->param_d[0] = (double)GetRand(360) / 180.0 * DX_PI;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    //  自機狙いの星弾（絨毯に混ぜて緊張感を出す）
    // --------------------------------------------------------
    if (pEnemyShotSet->count % 55 == 40
        && pEnemyShotSet->count < LAYERS * LAYER_DELAY + 100)
    {
        pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y + 10.0;
        pEnemyShot->muki = atan2(player.y - pEnemyShot->y,
            player.x - pEnemyShot->x);
        pEnemyShot->speed = 3.0 + GetRand(60) / 100.0;

        // 白い中玉で流れ弾と区別
        pEnemyShot->kind = img_enemyShotMediumBall[6];

        pEnemyShot->param_d[0] = 0.0; // 横揺れなし

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --------------------------------------------------------
    //  弾の移動（流れ）
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 横揺れ：サイン波でゆらゆらと流れる
        if (WAVE_AMP != 0.0) {
            const double phase = pShot->param_d[0]
                + pShot->count * WAVE_FRQ;
            pShot->x += sin(phase) * WAVE_AMP;
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン：天川星羅
// ------------------------------------------------------------
void EnemyPat_NightCarpet_DeepSeek()
{
    static int muki;        // 移動方向（+1: 右, -1: 左）
    static int wave_count;  // カーペットの波数

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        wave_count = 0;
    }
    else {
        // 左右にゆったりと移動（夜空を舞う星の神）
        enemy.x += 1.0 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // カーペットの波を一定間隔で生成
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        // セットの初期化
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightCarpet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き
        pEnemyShotSet->kind = wave_count++;

        // パターン設定を param に格納
        pEnemyShotSet->param_i[0] = 3;    // 層の数
        pEnemyShotSet->param_i[1] = 21;   // 1層あたりの弾数
        pEnemyShotSet->param_i[2] = 26;   // 弾の間隔(px)
        pEnemyShotSet->param_d[0] = 1.2;  // 基準速度
        pEnemyShotSet->param_d[1] = 0.4;  // 横揺れ振幅
        pEnemyShotSet->param_d[2] = 0.12; // 横揺れ周波数

        // 波ごとに少しずつ変化をつける
        if (wave_count % 3 == 0) {
            // 3波ごとに層を増やして密度を上げる
            pEnemyShotSet->param_i[0] = 4;
        }
        if (wave_count % 5 == 0) {
            // 5波ごとに流れを速くする
            pEnemyShotSet->param_d[0] = 1.6;
        }

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}