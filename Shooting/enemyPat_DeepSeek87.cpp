// enemyPat_tmp.cpp
// 弾幕：赤針夕焼（せきしんゆうしょう）
// 俳句「夕焼けの　空を裂きたる　赤き針」をモチーフにした弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <cwchar>

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
//  任意の文字列を32pxで描画し、白ピクセルの中心オフセットを返す
// ============================================================
static std::vector<std::pair<double, double>> GetTextOffsets(const wchar_t* text)
{
    InitGdiplusOnce();

    const int SIZE = 32;
    int textLen = (int)wcslen(text);
    if (textLen == 0) return {};

    int W = SIZE * textLen + 64;
    int H = SIZE * 2;

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
//  文字弾を生成するヘルパー関数
// ============================================================
static void SpawnTextBullets(sEnemyShotSet* pEnemyShotSet, const wchar_t* text, double scale)
{
    auto offsets = GetTextOffsets(text);
    double centerX = pEnemyShotSet->x;
    double centerY = pEnemyShotSet->y;

    for (const auto& off : offsets) {
        sEnemyShot* p = new sEnemyShot;

        p->x = centerX + off.first * scale;
        p->y = centerY + off.second * scale;
        p->muki = 0.0;
        p->speed = 0.0;
        p->kind = img_enemyShotSmallBall[8];   // 橙色の小玉
        p->margin = 240.0;

        p->param_i[0] = 0;                     // 種別：文字弾
        p->param_d[0] = off.first * scale;     // 中心からのオフセットX
        p->param_d[1] = off.second * scale;    // 中心からのオフセットY
        p->param_d[2] = (double)GetRand(3) / 100.0;

        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }
}

// ============================================================
//  弾幕パターン：赤針夕焼
// ============================================================
static void ShotHaikuAkaiHari(sEnemyShotSet* pEnemyShotSet)
{
    const int count = pEnemyShotSet->count;
    const double centerX = pEnemyShotSet->x;
    const double centerY = pEnemyShotSet->y;

    // 開始時にチャージ音
    if (count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ------------------------------------------------------------
    // 第一波：「夕焼けの」（上五） 文字をオレンジの小玉で描画
    // ------------------------------------------------------------
    if (count == 60) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        SpawnTextBullets(pEnemyShotSet, L"夕焼けの", 3.0);
    }

    // ------------------------------------------------------------
    // 第二波：「空を裂きたる」（中七） 文字弾と赤い針弾の放射
    // ------------------------------------------------------------
    if (count == 180) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 文字弾：「空を裂きたる」
        SpawnTextBullets(pEnemyShotSet, L"空を裂きたる", 2.9);

        // 針弾：7方向に赤い銃弾を放射
        double baseAngle = atan2(player.y - centerY, player.x - centerX);
        int needleCount = 7;
        int bulletPerNeedle = 6;
        double spacing = 3.0;
        double speed = 2.5;

        for (int i = 0; i < needleCount; ++i) {
            double angle = baseAngle + (i - 3) * (15.0 / 180.0 * DX_PI);
            for (int j = 0; j < bulletPerNeedle; ++j) {
                sEnemyShot* p = new sEnemyShot;

                p->x = centerX;
                p->y = centerY;
                p->muki = angle;
                p->speed = speed + j * 0.5;
                p->kind = img_enemyShotBullet[0];   // 赤い銃弾型
                p->margin = 240.0;

                p->param_i[0] = 1;                  // 種別：針弾
                p->param_d[0] = angle;
                p->param_d[1] = speed;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // ------------------------------------------------------------
    // 第三波：「赤き針」（下五） 文字弾とプレイヤー狙いの高速針
    // ------------------------------------------------------------
    if (count == 300) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 文字弾：「赤き針」
        SpawnTextBullets(pEnemyShotSet, L"赤き針", 3.1);

        // 高速針：5方向のプレイヤー狙い
        int needleCount = 5;
        int bulletPerNeedle = 8;
        double spacing = 4.0;
        double speed = 4.5;

        for (int i = 0; i < needleCount; ++i) {
            double angle = atan2(player.y - centerY, player.x - centerX);
            angle += (GetRand(20) - 10) / 180.0 * DX_PI;   // 僅かにばらつかせる

            for (int j = 0; j < bulletPerNeedle; ++j) {
                sEnemyShot* p = new sEnemyShot;

                p->x = centerX;
                p->y = centerY;
                p->muki = angle;
                p->speed = speed + 1.0 * j;
                p->kind = (j == 0) ? img_enemyShotLargeBall[0] : img_enemyShotBullet[0];
                p->margin = 240.0;

                p->param_i[0] = 1;
                p->param_d[0] = angle;
                p->param_d[1] = speed;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // ------------------------------------------------------------
    // 全弾の更新
    // ------------------------------------------------------------
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) {
            // 文字弾：ゆらゆら揺れながら降下
            p->x = centerX + p->param_d[0] + sin(p->count * p->param_d[2]) * 12.0;
            p->y = centerY + p->param_d[1] + p->count * 0.8;
        }
        else if (p->param_i[0] == 1) {
            // 針弾：直線運動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Haiku_DeepSeek()
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

    // 一定間隔で俳句弾幕を発射
    if (count % 600 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHaikuAkaiHari;
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