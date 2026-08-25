#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
// GDI+ 初期化
// ============================================================
static void InitGdiplusOnce_Haiku()
{
    static bool initialized = false;
    if (!initialized) {
        static GdiplusStartupInput gdiInput;
        static ULONG_PTR token = 0;
        GdiplusStartup(&token, &gdiInput, nullptr);
        initialized = true;
    }
}

// ============================================================
// 文字列を白ピクセルの座標列へ変換
// ============================================================
static std::vector<std::pair<double, double>> SampleText_Haiku(const wchar_t* text)
{
    InitGdiplusOnce_Haiku();

    const int SIZE = 28;
    const int W = 360;
    const int H = 60;

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
            if (argb[idx + 3] == 255) {
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
        const double cx = (minX + maxX) / 2.0;
        const double cy = (minY + maxY) / 2.0;
        result.reserve(whitePixels.size());
        for (const auto& p : whitePixels)
            result.emplace_back(p.first - cx, p.second - cy);
    }
    return result;
}

static const std::vector<std::pair<double, double>>& GetHaikuOffsets(int index)
{
    static std::vector<std::pair<double, double>> offsets[3];
    static bool initialized = false;

    if (!initialized) {
        offsets[0] = SampleText_Haiku(L"月しずく");
        offsets[1] = SampleText_Haiku(L"風鈴がひとつ");
        offsets[2] = SampleText_Haiku(L"夜を裂く");
        initialized = true;
    }
    return offsets[index];
}

// ============================================================
// 俳句の一行を弾で形成する
// ============================================================
static void CreateHaikuLine(sEnemyShotSet* pSet, int line)
{
    const auto& offsets = GetHaikuOffsets(line);
    const double scale = 3.2;
    const int kinds[3] = { img_enemyShotSmallBall[3], img_enemyShotSmallBall[4], img_enemyShotSmallBall[5] };

    for (size_t i = 0; i < offsets.size(); ++i) {
        sEnemyShot* shot = new sEnemyShot;
        shot->x = pSet->x + offsets[i].first * scale;
        shot->y = pSet->y + offsets[i].second * scale + line * 80;
        shot->muki = 0.0;
        shot->speed = 0.0;
        shot->kind = kinds[line];
        shot->margin = 240.0;

        // 句ごとの展開制御
        shot->param_i[0] = line;
        shot->param_i[1] = (int)i;
        shot->param_i[2] = 0;

        shot->prev = pSet->pEnemyShotHead->prev;
        shot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = shot;
        pSet->pEnemyShotHead->prev = shot;
    }
}

// ============================================================
// 俳句弾幕
// 「月しずく」
// 「風鈴がひとつ」
// 「夜を裂く」
// ============================================================
static void ShotHaiku(sEnemyShotSet* pSet)
{
    // 句を順番に出現させる
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        CreateHaikuLine(pSet, 0);
    }
    else if (pSet->count == 55) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        CreateHaikuLine(pSet, 1);
    }
    else if (pSet->count == 110) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        CreateHaikuLine(pSet, 2);
    }

    // 最後の句が出たあと、一斉に文字を崩して弾幕化
    const int releaseFrame = 165;
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        if (pSet->count < releaseFrame) {
            shot->speed = 0.0;
        }
        else {
            const int line = shot->param_i[0];
            const int index = shot->param_i[1];

            // 句ごとに異なる動きで「五・七・五」を崩す
            if (line == 0) {
                // 月しずく：上から下へ静かに落ちる
                shot->muki = DX_PI / 2.0 + 0.16 * sin(index * 0.37);
                shot->speed = 1.55;
            }
            else if (line == 1) {
                // 風鈴がひとつ：左右へ揺れる波
                const double base = (index % 2 == 0) ? 1.0 : -1.0;
                shot->muki = DX_PI / 2.0 + base * 0.55 * sin((pSet->count - releaseFrame) * 0.075 + index * 0.11);
                shot->speed = 1.85;
            }
            else {
                // 夜を裂く：上方から左右へ鋭く開く
                const double center = 0.0;
                double dx = shot->x - pSet->x;
                if (fabs(dx) < 1.0) dx = (index & 1) ? 1.0 : -1.0;
                shot->muki = (dx < 0.0) ? DX_PI * 0.82 : DX_PI * 0.18;
                shot->speed = 2.15;
            }
        }

        // 文字形成中も、解放後もここだけで座標を進める
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Haiku_ChatGPT()
{
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 90.0 || enemy.x > 390.0)
            muki *= -1;
    }

    // 一句ずつ現れ、三句そろったところで一斉に飛び散る
    if (count % 210 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHaiku;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 20.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}