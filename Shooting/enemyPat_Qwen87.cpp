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
//  任意の文字列を32pxで描画し、白ピクセルの相対座標を返す
// ============================================================
static std::vector<std::pair<double, double>> GetTextPixels(const std::wstring& text, double scale = 1.0)
{
    InitGdiplusOnce();

    const int SIZE = 24;
    int charCount = (int)text.length();
    int W = charCount * SIZE + SIZE;
    int H = SIZE * 2;

    Bitmap bitmap(W, H, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    g.Clear(Color(0, 0, 0, 0));    // 透明でクリア

    // アンチエイリアスOFF（単色描画で境界を明確にする）
    g.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

    SolidBrush whiteBrush(Color(255, 255, 255, 255));
    FontFamily fontFamily(L"Yu Gothic UI Light");
    Font font(&fontFamily, (REAL)SIZE, FontStyleRegular, UnitPixel);

    // 中央寄せで描画
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF rect(0, 0, (REAL)W, (REAL)H);
    g.DrawString(text.c_str(), -1, &font, rect, &format, &whiteBrush);

    // ピクセルデータ取得
    BitmapData bmpData;
    Rect lockRect(0, 0, W, H);
    bitmap.LockBits(&lockRect, ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
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

    // バウンディングボックスの中心を原点に変換し、スケールを適用
    std::vector<std::pair<double, double>> result;
    if (!whitePixels.empty()) {
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        result.reserve(whitePixels.size());
        for (const auto& p : whitePixels) {
            result.emplace_back((p.first - cx) * scale, (p.second - cy) * scale);
        }
    }

    return result;
}

// ============================================================
//  文字のオフセットリスト（初回アクセス時に自動生成してキャッシュ）
// ============================================================
static std::vector<std::pair<double, double>> g_pixels_yozakura;
static std::vector<std::pair<double, double>> g_pixels_hanabira;
static std::vector<std::pair<double, double>> g_pixels_maiochinu;
static bool g_textPixelsInitialized = false;

static void InitTextPixels() {
    if (g_textPixelsInitialized) return;
    // スケール2.5倍で画面幅(480)内に収まるサイズに調整
    g_pixels_yozakura = GetTextPixels(L"夜桜の", 3.5);
    g_pixels_hanabira = GetTextPixels(L"花びら文字が", 3.5);
    g_pixels_maiochinu = GetTextPixels(L"舞い落ちぬ", 3.5);
    g_textPixelsInitialized = true;
}

// ============================================================
//  弾幕パターン1：文字を描画する
// ============================================================
static void ShotFormationText(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const std::vector<std::pair<double, double>>* pixels = nullptr;
        if (pEnemyShotSet->kind == 0) pixels = &g_pixels_yozakura;
        else if (pEnemyShotSet->kind == 1) pixels = &g_pixels_hanabira;
        else if (pEnemyShotSet->kind == 2) pixels = &g_pixels_maiochinu;

        if (pixels) {
            for (size_t i = 0; i < pixels->size(); ++i) {
                // 弾密度を調整するため、3つに1つだけ生成（間引き）
                //if (i % 3 != 0) continue;

                sEnemyShot* shot = new sEnemyShot;
                shot->x = pEnemyShotSet->x + (*pixels)[i].first;
                shot->y = pEnemyShotSet->y + (*pixels)[i].second;
                shot->muki = pEnemyShotSet->muki;
                shot->speed = 0.2; // ゆっくりと広がる

                // 鱗弾を使用。kind=1(花びら文字が)のみ白(6)、他はマゼンタ(5)で桜色を表現
                int color = (pEnemyShotSet->kind == 1) ? 6 : 5;
                shot->kind = img_enemyShotScale[color];

                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  弾幕パターン2：文字を崩して花びら弾と墨弾に変換する
// ============================================================
static void ShotFormationPetal(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 既存の文字弾をすべて取得し、花びら弾に変換する
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;

            if (p->param_i[0] != 0) {
                p = next;
                continue;
            }

            // 1つの文字弾を複数の花びら弾に変換 (8〜12個)
            int petalCount = 1;
            for (int i = 0; i < petalCount; ++i) {
                sEnemyShot* petal = new sEnemyShot;
                petal->x = p->x;
                petal->y = p->y;

                // サインカーブ落下用のパラメータ
                petal->param_d[0] = 2.0 + GetRand(30) / 10.0;          // 振幅 (2.0 〜 5.0)
                petal->param_d[1] = GetRand(360) / 180.0 * DX_PI;      // 位相 (0 〜 2π)
                petal->param_d[2] = 0.0;                               // 経過時間用

                petal->speed = 1.5 + GetRand(10) / 10.0;               // 落下速度 (1.5 〜 2.5)

                // 花びら弾の種類：鱗弾または菱形弾の色はマゼンタ(5)か白(6)をランダムに
                int color = (GetRand(1) == 0) ? 6 : 5;
                petal->kind = (GetRand(1) == 0) ? img_enemyShotScale[color] : img_enemyShotDiamond[color];
                petal->param_i[0] = 2; // 花びら弾フラグ
                petal->muki = DX_PI / 2;

                petal->prev = pEnemyShotSet->pEnemyShotHead->prev;
                petal->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = petal;
                pEnemyShotSet->pEnemyShotHead->prev = petal;
            }

            // 墨弾（追尾弾）を1つだけ生成
            sEnemyShot* ink = new sEnemyShot;
            ink->x = p->x;
            ink->y = p->y;
            ink->muki = p->muki;
            ink->speed = 1.2;
            ink->kind = img_enemyShotSmallBall[7]; // 黒の小玉
            ink->param_i[0] = 1; // 墨弾（追尾）フラグ

            ink->prev = pEnemyShotSet->pEnemyShotHead->prev;
            ink->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = ink;
            pEnemyShotSet->pEnemyShotHead->prev = ink;

            // 元の文字弾を削除（プールへ返却）
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;

            p = next;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[0] == 1) {
            // 墨弾の滑らかな追尾処理
            double angle = atan2(player.y - p->y, player.x - p->x);
            double diff = angle - p->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            p->muki += diff * 0.005; // 徐々に向きを変える

            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        else {
            // 花びら弾のサインカーブ落下
            p->param_d[2] += 0.05; // 時間経過
            double wave = p->param_d[0] * sin(p->param_d[2] + p->param_d[1]);

            p->x += wave * 0.1; // 横揺れ
            p->y += p->speed;   // 落下
        }
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン：詩符「夜桜の花びら文字舞い落ちぬ」
// ============================================================
void EnemyPat_Haiku_Qwen()
{
    static int muki = 1;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // テキストピクセルデータの初期化
        InitTextPixels();

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵の左右往復移動
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    const int T = 600;
    int countT = count % T;

    // フェーズ1: 「夜桜の」描画
    if (countT == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFormationText;
        pEnemyShotSet->x = 230;
        pEnemyShotSet->y = 50;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0; // 夜桜の

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    // フェーズ2: 「花びら文字が」描画
    else if (countT == 90) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFormationText;
        pEnemyShotSet->x = 230;
        pEnemyShotSet->y = 150;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 1; // 花びら文字が

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    // フェーズ3: 「舞い落ちぬ」描画
    else if (countT == 150) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFormationText;
        pEnemyShotSet->x = 230;
        pEnemyShotSet->y = 250;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 2; // 舞い落ちぬ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    // フェーズ4: 文字崩壊＆花びら弾幕開始
    else if (countT == 210) {
        // 既存の文字ShotSetをすべて取得し、パターン関数をPetalに変更して再発火させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotFormationText) {
                pSet->patternFunc = ShotFormationPetal;
                pSet->count = 0; // count=0に戻すことで、次フレームで弾の変換処理が走る
            }
            pSet = pSet->next;
        }
    }
}