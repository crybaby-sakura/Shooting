#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <unordered_map>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  GDI+ 初期化
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
//  指定文字の白ピクセル座標を取得（2ピクセル間引きで軽量化）
// ============================================================
static std::vector<std::pair<double, double>> SampleTextPixels(wchar_t ch)
{
    InitGdiplusOnce();
    const int SIZE = 32;          // 24pxに縮小して弾数を抑制
    const int W = SIZE * 2;
    const int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);

    WCHAR str[2] = { ch, L'\0' };
    g.DrawString(str, -1, &font, PointF(0, 0), &whiteBrush);

    BitmapData bmpData;
    Rect rect(0, 0, W, H);
    bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
    BYTE* argb = (BYTE*)bmpData.Scan0;

    std::vector<std::pair<int, int>> whitePixels;
    int minX = W, maxX = -1, minY = H, maxY = -1;

    for (int y = 0; y < H; y += 1) {      // 2ピクセル間引きで軽量化
        for (int x = 0; x < W; x += 1) {
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
//  文字キャッシュ（初回アクセス時に全文字を生成）
// ============================================================
static std::vector<std::pair<double, double>>& GetOffsetsForChar(wchar_t ch)
{
    static std::unordered_map<wchar_t, std::vector<std::pair<double, double>>> cache;
    static bool initialized = false;
    if (!initialized) {
        const wchar_t chars[] = L"蝉時雨落ちる陽炎夏の暮";
        for (wchar_t c : chars) {
            if (c != L'\0' && cache.find(c) == cache.end()) {
                cache[c] = SampleTextPixels(c);
            }
        }
        initialized = true;
    }
    return cache[ch];
}

// ============================================================
//  第一幕「蝉時雨」：青白い小弾が不規則に降る
// ============================================================
static void ShotPattern_SemiShigure(sEnemyShotSet* pSet)
{
    const wchar_t chars[] = L"蝉時雨";
    const int colors[] = { 3, 6, 4 }; // シアン、白、青

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int charIdx = pSet->kind % 3;
        wchar_t ch = chars[charIdx];
        const auto& offsets = GetOffsetsForChar(ch);

        constexpr double SCALE = 3.0;
        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + off.first * SCALE;
            shot->y = pSet->y + off.second * SCALE;
            shot->muki = DX_PI / 2.0; // 下向き
            shot->speed = 1.2 + GetRand(8) / 100.0; // 1.2 〜 2.0
            shot->kind = img_enemyShotSmallBall[colors[charIdx]];

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    // 追加の不規則な雨弾（密度が増減する）
    if (pSet->count < 90 && pSet->count % (12 - pSet->kind * 2) == 0) {
        for (int i = 0; i < 2 + pSet->kind; i++) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + GetRand(160) - 80;
            shot->y = pSet->y + GetRand(30) - 15;
            shot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            shot->speed = 1.5 + GetRand(120) / 100.0;
            shot->kind = img_enemyShotSmallBall[6]; // 白

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  第二幕「落ちる」：重力加速で文字が急降下
// ============================================================
static void ShotPattern_Ochiru(sEnemyShotSet* pSet)
{
    const wchar_t chars[] = L"落ちる陽炎";
    const int colors[] = { 1, 8, 1, 8, 1 }; // 黄、橙、黄

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int charIdx = pSet->kind % 5;
        wchar_t ch = chars[charIdx];
        const auto& offsets = GetOffsetsForChar(ch);

        constexpr double SCALE = 3.0;
        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + off.first * SCALE;
            shot->y = pSet->y + off.second * SCALE;
            shot->muki = DX_PI / 2.0;
            shot->speed = 0.5;
            shot->kind = img_enemyShotSmallBall[colors[charIdx]];
            shot->param_d[0] = 0.06; // 重力加速度
            shot->param_d[1] = shot->speed; // 現在の垂直速度

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->param_d[1] += p->param_d[0];
        p->speed = p->param_d[1];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  第二幕「陽炎」：正弦波を描く中楕円弾
// ============================================================
static void ShotPattern_Kagerou(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (pSet->count % 15 == 0 && pSet->count <= 60) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = 50 + i * 95 + (GetRand(60) - 30);
            shot->y = 520;
            shot->muki = -DX_PI / 2.0; // 上向き
            shot->speed = 1.2 + GetRand(80) / 100.0; // 1.2 〜 2.0
            shot->kind = img_enemyShotMediumOval[8]; // 橙
            shot->param_i[0] = i;       // 位相用インデックス
            shot->param_d[0] = shot->x; // 基準X座標
            shot->margin = 50;

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double wave = sin(pSet->count * 0.04 + p->param_i[0] * 1.3) * 3.0;
        p->x = p->param_d[0] + wave;
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  第三幕「夏の暮」：文字＋中心大玉＋放射状拡散
// ============================================================
static void ShotPattern_NatsuNoKure(sEnemyShotSet* pSet)
{
    const wchar_t chars[] = L"夏の暮";
    const int colors[] = { 0, 8, 0 }; // 赤、橙、赤

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int charIdx = pSet->kind % 3;
        wchar_t ch = chars[charIdx];
        const auto& offsets = GetOffsetsForChar(ch);

        constexpr double SCALE = 2.5;
        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + off.first * SCALE;
            shot->y = pSet->y + off.second * SCALE;
            shot->muki = 0;
            shot->speed = 0; // 静止
            shot->kind = img_enemyShotSmallBall[colors[charIdx]];

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }

        // 中央に大玉（kind==0のセットのみ生成）
        if (pSet->kind == 1) {
            sEnemyShot* center = new sEnemyShot;
            center->x = pSet->x;
            center->y = pSet->y;
            center->muki = DX_PI / 2.0;
            center->speed = 0.3; // ゆっくり下がる
            center->kind = img_enemyShotLargeBall[0]; // 赤

            center->prev = pSet->pEnemyShotHead->prev;
            center->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = center;
            pSet->pEnemyShotHead->prev = center;
        }
    }

    // 60フレーム後に放射状拡散（夕日から最後の光）
    if (pSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24 * 3; i++) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x;
            shot->y = pSet->y;
            shot->muki = (DX_PI * 2.0 / 24.0 / 3) * i;
            shot->speed = 2.0 + GetRand(100) / 100.0; // 2.0 〜 3.0
            shot->kind = img_enemyShotSmallBall[8]; // 橙

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->speed > 0) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン：夏の終わりの三幕
// ============================================================
void EnemyPat_Haiku_Kimi()
{
    static int muki = 1;
    static int prevPhase = -1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        prevPhase = -1;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 約10秒ごとに三幕を循環（600フレーム周期）
    int currentPhase = (count / 200) % 3;

    if (currentPhase != prevPhase) {
        prevPhase = currentPhase;

        if (currentPhase == 0) {
            // ===== 第一幕「蝉時雨」 =====
            for (int i = 0; i < 3; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotPattern_SemiShigure;
                pSet->x = enemy.x + (i - 1) * 100;
                pSet->y = enemy.y + 15;
                pSet->muki = DX_PI / 2.0;
                pSet->kind = i; // 0=蝉、1=時、2=雨

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else if (currentPhase == 1) {
            // ===== 第二幕「落ちる陽炎」 =====
            // 文字「落ちる」
            for (int i = 0; i < 5; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotPattern_Ochiru;
                pSet->x = enemy.x + (i - 2) * 80;
                pSet->y = enemy.y + 15;
                pSet->muki = DX_PI / 2.0;
                pSet->kind = i; // 0=落、1=ち、2=る

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            // 陽炎（画面下から這い上がる中楕円弾）
            sEnemyShotSet* pSetK = new sEnemyShotSet;
            pSetK->count = 0;
            pSetK->patternFunc = ShotPattern_Kagerou;
            pSetK->x = 240.0;
            pSetK->y = 520.0;
            pSetK->muki = -DX_PI / 2.0;
            pSetK->kind = 0;

            pSetK->pEnemyShotHead = new sEnemyShot;
            pSetK->pEnemyShotHead->prev = pSetK->pEnemyShotHead;
            pSetK->pEnemyShotHead->next = pSetK->pEnemyShotHead;

            pSetK->prev = enemyShotSetHead.prev;
            pSetK->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSetK;
            enemyShotSetHead.prev = pSetK;
        }
        else if (currentPhase == 2) {
            // ===== 第三幕「夏の暮」 =====
            for (int i = 0; i < 3; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotPattern_NatsuNoKure;
                pSet->x = enemy.x + (i - 1) * 80;
                pSet->y = enemy.y + 20;
                pSet->muki = 0;
                pSet->kind = i; // 0=夏、1=の、2=暮

                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
    }
}