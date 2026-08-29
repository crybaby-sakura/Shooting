// EnemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 分割する小正方形の一辺のピクセル数 (K = 1)
const int K = 1;

// ---------------------------------------------------------
// RGB値から最も近い指定色のインデックスを返す
// ---------------------------------------------------------
static int GetClosestColorIndex(int r, int g, int b) {
    struct ColorRGB {
        int r, g, b, idx;
    };
    // 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
    ColorRGB palette[9] = {
        {255,   0,   0, 0},
        {255, 255,   0, 1},
        {  0, 255,   0, 2},
        {  0, 255, 255, 3},
        {  0,   0, 255, 4},
        {255,   0, 255, 5},
        {255, 255, 255, 6},
        {  0,   0,   0, 7},
        {255, 128,   0, 8} // 橙
    };

    int min_dist = 99999999;
    int min_idx = 0;

    for (int i = 0; i < 9; ++i) {
        int dr = r - palette[i].r;
        int dg = g - palette[i].g;
        int db = b - palette[i].b;
        int dist = dr * dr + dg * dg + db * db;
        if (dist < min_dist) {
            min_dist = dist;
            min_idx = palette[i].idx;
        }
    }
    return min_idx;
}

// ---------------------------------------------------------
// 弾幕：画像を読み込み、形を保ったまま自機狙いで射出＆常時ジェット噴射
// ---------------------------------------------------------
static void ShotPlayerShape(sEnemyShotSet* pEnemyShotSet)
{
    // 画像の読み込み（初回のみ実行しキャッシュ）
    static int softImg = -1;
    if (softImg == -1) {
        softImg = LoadSoftImage("assets/images/player.png");
    }

    if (softImg == -1) return;

    int w, h;
    GetSoftImageSize(softImg, &w, &h);

    // ピクセル同士の配置間隔
    const double interval = 8.0;

    // 画像の上方向を自機の方向(muki)に合わせるための回転角
    double base_angle = pEnemyShotSet->muki + DX_PI / 2.0;

    // 1. 生成フレーム (count == 0) のみ実行される機体形状（小玉）の配置
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int cols = w / K;
        int rows = h / K;

        for (int y = 0; y + K <= h; y += K) {
            for (int x = 0; x + K <= w; x += K) {

                int r_sum = 0, g_sum = 0, b_sum = 0, a_sum = 0;

                for (int dy = 0; dy < K; ++dy) {
                    for (int dx = 0; dx < K; ++dx) {
                        int pr, pg, pb, pa;
                        GetPixelSoftImage(softImg, x + dx, y + dy, &pr, &pg, &pb, &pa);
                        r_sum += pr;
                        g_sum += pg;
                        b_sum += pb;
                        a_sum += pa;
                    }
                }

                int area = K * K;
                int a = a_sum / area;

                // 透明部分には何も割り当てない
                if (a > 128) {
                    int r = r_sum / area;
                    int g = g_sum / area;
                    int b = b_sum / area;

                    int color_idx = GetClosestColorIndex(r, g, b);

                    sEnemyShot* pEnemyShot = new sEnemyShot;

                    // 画像中心を原点とした配置座標
                    double cell_x = (x / (double)K) - cols / 2.0 + 0.5;
                    double cell_y = (y / (double)K) - rows / 2.0 + 0.5;

                    // 実座標でのオフセット
                    double ox = cell_x * interval;
                    double oy = cell_y * interval;

                    // 自機方向への回転を適用
                    double rot_x = ox * cos(base_angle) - oy * sin(base_angle);
                    double rot_y = ox * sin(base_angle) + oy * cos(base_angle);

                    // 弾のパラメータ設定
                    pEnemyShot->x = pEnemyShotSet->x + rot_x;
                    pEnemyShot->y = pEnemyShotSet->y + rot_y;
                    pEnemyShot->muki = pEnemyShotSet->muki; // 全ての弾が同じ自機方向を向く
                    pEnemyShot->speed = 3.0; // 形を保つため全弾同じ速度
                    pEnemyShot->kind = img_enemyShotSmallBall[color_idx];
                    pEnemyShot->margin = 240; // マージン設定

                    // リストへ追加
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // 2. 常時ジェット噴射処理 (毎フレーム生成)
    // 元画像 (17.5, 42.0) のノズル座標から後方へ噴射
    double jet_px = 17.5;
    double jet_py = 40.0;

    // 画像中心からのオフセット計算
    double jet_ox = (jet_px - w / 2.0) * interval;
    double jet_oy = (jet_py - h / 2.0) * interval;

    // 自機方向への回転適用
    double jet_rot_x = jet_ox * cos(base_angle) - jet_oy * sin(base_angle);
    double jet_rot_y = jet_ox * sin(base_angle) + jet_oy * cos(base_angle);

    // 現在の機体中心位置 (速度 3.0 で進行中)
    double current_center_x = pEnemyShotSet->x + pEnemyShotSet->count * 3.0 * cos(pEnemyShotSet->muki);
    double current_center_y = pEnemyShotSet->y + pEnemyShotSet->count * 3.0 * sin(pEnemyShotSet->muki);

    double jet_base_x = current_center_x + jet_rot_x;
    double jet_base_y = current_center_y + jet_rot_y;

    // 毎フレーム2発ずつ噴射して連続感を出す
    for (int i = 0; i < 1; ++i) {
        sEnemyShot* pJetShot = new sEnemyShot;

        // 噴射口周辺のわずかなブレ
        double offset_r = (GetRand(20) - 10) / 10.0 * 10.0; // -2.0 ～ +2.0
        double offset_ang = GetRand(360) / 180.0 * DX_PI;
        pJetShot->x = jet_base_x + offset_r * cos(offset_ang);
        pJetShot->y = jet_base_y + offset_r * sin(offset_ang);

        // 進行方向の逆向き (muki + DX_PI) から ±20度の範囲でバラ撒く
        double spread = (GetRand(120) - 60) / 180.0 * DX_PI;
        pJetShot->muki = pEnemyShotSet->muki + DX_PI + spread;

        // 速度に幅を持たせる (2.0 ～ 4.5)
        pJetShot->speed = 2.0 + GetRand(250) / 100.0;

        // シアン色の中玉 (インデックス 3)
        pJetShot->kind = img_enemyShotMediumBall[3];
        pJetShot->margin = 20; // マージン設定

        // リストへ追加
        pJetShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pJetShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pJetShot;
        pEnemyShotSet->pEnemyShotHead->prev = pJetShot;
    }

    // 3. 各弾の進行処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン
// ---------------------------------------------------------
void EnemyPat_PlayerIMG()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.0 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 150フレーム周期で射出
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPlayerShape;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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