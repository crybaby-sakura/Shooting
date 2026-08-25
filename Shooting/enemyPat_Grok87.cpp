#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <string>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  GDI+ 初期化（一度だけ）
// ============================================================
static void InitGdiplusOnce()
{
    static bool initialized = false;
    if (!initialized) {
        GdiplusStartupInput gdiInput;
        ULONG_PTR token;
        GdiplusStartup(&token, &gdiInput, nullptr);
        initialized = true;
    }
}

// ============================================================
//  指定文字列を32pxで描画し、白ピクセルの相対座標を返す
// ============================================================
static std::vector<std::pair<double, double>> SampleTextPixels(const wchar_t* text)
{
    InitGdiplusOnce();
    const int SIZE = 28;
    // 文字列長に応じて幅を確保（最大8文字想定）
    const int W = SIZE * 10;
    const int H = SIZE * 2;
    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);
    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);
    g.DrawString(text, -1, &font, PointF(0, 0), &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    BYTE* argb = (BYTE*)bmpData.Scan0;
    std::vector<std::pair<int, int>> whitePixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * bmpData.Stride + x * 4;
            BYTE a = argb[idx + 3];
            if (a == 255) {
                whitePixels.emplace_back(x, y);
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    bitmap.UnlockBits(&bmpData);

    std::vector<std::pair<double, double>> result;
    if (!whitePixels.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        result.reserve(whitePixels.size());
        for (const auto& p : whitePixels) {
            result.emplace_back(p.first - cx, p.second - cy);
        }
    }
    return result;
}

// ============================================================
//  各行のオフセット（初回アクセス時に生成）
// ============================================================
static const std::vector<std::pair<double, double>>& GetOffsetsLine1()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool init = false;
    if (!init) {
        offsets = SampleTextPixels(L"闇を裂く");
        init = true;
    }
    return offsets;
}

static const std::vector<std::pair<double, double>>& GetOffsetsLine2()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool init = false;
    if (!init) {
        offsets = SampleTextPixels(L"光の矢雨");
        init = true;
    }
    return offsets;
}

static const std::vector<std::pair<double, double>>& GetOffsetsLine3()
{
    static std::vector<std::pair<double, double>> offsets;
    static bool init = false;
    if (!init) {
        offsets = SampleTextPixels(L"魂震う");
        init = true;
    }
    return offsets;
}

// ============================================================
//  第1行「闇を裂く」：黒弾で文字形成 → 左右に裂ける
// ============================================================
static void ShotHaikuLine1(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 4.5;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const auto& offsets = GetOffsetsLine1();
        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;
            // 中心より左は左へ、右は右へ裂ける
            double dx = offsets[i].first;
            shot->muki = (dx < 0.0) ? DX_PI : 0.0; // 左 or 右
            shot->speed = 0.0; // 最初は静止
            shot->kind = img_enemyShotSmallBall[7]; // 黒
            shot->param_d[0] = 1.8 + (GetRand(40) / 100.0); // 最終速度
            shot->param_i[0] = 40 + GetRand(20); // 裂ける開始フレーム
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 動き
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->count == p->param_i[0]) {
            // 裂ける瞬間に音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            p->speed = p->param_d[0];
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  第2行「光の矢雨」：金色の矢型弾が扇状に降り注ぐ
// ============================================================
static void ShotHaikuLine2(sEnemyShotSet* pEnemyShotSet)
{
    // count == 0 で予告、以降数フレームごとに矢を降らす
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 10フレームごとに矢を生成（計約8波）
    if (pEnemyShotSet->count >= 20 && pEnemyShotSet->count <= 90 && pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 文字の形を薄く残しつつ、矢雨を追加
        if (pEnemyShotSet->count == 20) {
            const auto& offsets = GetOffsetsLine2();
            constexpr double SCALE = 4.0;
            for (size_t i = 0; i < offsets.size(); i += 1) { // 密度を落とす
                sEnemyShot* shot = new sEnemyShot;
                shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
                shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;
                shot->muki = DX_PI / 2.0; // 下向き
                shot->speed = 1.2;
                shot->kind = img_enemyShotDiamond[1]; // 黄の菱形
                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }

        // 本命の矢雨（扇状）
        int num = 7 + GetRand(4);
        double baseAngle = DX_PI / 2.0; // 下
        for (int i = 0; i < num; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + (GetRand(200) - 100);
            shot->y = pEnemyShotSet->y + (GetRand(30) - 15);
            // 扇状に少し角度を付ける
            double spread = (i - num / 2.0) * 0.12;
            shot->muki = baseAngle + spread + (GetRand(20) - 10) / 180.0 * DX_PI;
            shot->speed = 2.0 + GetRand(80) / 100.0;
            // 黄 or 橙の鱗弾・銃弾を混ぜる
            if (GetRand(1) == 0)
                shot->kind = img_enemyShotScale[1]; // 黄
            else
                shot->kind = img_enemyShotBullet[8]; // 橙
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 全弾移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  第3行「魂震う」：白弾で文字形成 → 円形に広がり多段爆発
// ============================================================
static void ShotHaikuLine3(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 4.2;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const auto& offsets = GetOffsetsLine3();
        for (size_t i = 0; i < offsets.size(); ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
            shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;
            shot->muki = atan2(offsets[i].second, offsets[i].first); // 外側向き
            shot->speed = 0.0;
            shot->kind = img_enemyShotSmallBall[6]; // 白
            shot->param_d[0] = 1.5 + GetRand(50) / 100.0; // 拡散速度
            shot->param_i[0] = 50; // 拡散開始
            shot->param_i[1] = 0;  // 爆発済みフラグ
            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    // 動きと爆発処理
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        // 拡散開始
        if (p->count == p->param_i[0] && p->param_i[1] == 0) {
            p->speed = p->param_d[0];
        }

        // 拡散後しばらくして小さな弾を8方向に撒く（震え表現）
        if (p->count == p->param_i[0] + 35 && p->param_i[1] == 0) {
            p->param_i[1] = 1; // フラグ
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int k = 0; k < 8/2; ++k) {
                sEnemyShot* child = new sEnemyShot;
                child->x = p->x;
                child->y = p->y;
                child->muki = p->muki + k * (DX_PI * 2.0 / 8.0*2);
                child->speed = 1.8 + GetRand(40) / 100.0;
                child->kind = img_enemyShotSmallBall[6]; // 白
                child->param_i[1] = 1;
                child->prev = pEnemyShotSet->pEnemyShotHead->prev;
                child->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = child;
                pEnemyShotSet->pEnemyShotHead->prev = child;
            }
            // 親弾は少し減速
            p->speed *= 0.4;
        }

        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン：俳句弾幕「詠唱弾幕・三行の裁き」
// ============================================================
void EnemyPat_Haiku_Grok()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 緩やかな左右揺れ
        enemy.x = 240.0 + 60.0 * sin(count * 0.015);
    }

    const int T = 600;
    int countT = count % T;

    // --- 第1行「闇を裂く」 ---
    if (countT == 90) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHaikuLine1;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- 第2行「光の矢雨」 ---
    if (countT == 220) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHaikuLine2;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = 1;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --- 第3行「魂震う」 ---
    if (countT == 360) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHaikuLine3;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 25.0;
        pSet->muki = 0.0;
        pSet->kind = 2;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}