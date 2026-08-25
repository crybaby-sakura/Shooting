#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <map>
#include <string>

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
//  文字を32pxで描画し、白ピクセルの座標を返す
// ============================================================
static std::vector<std::pair<double, double>> GetTextOffsets(const wchar_t* text)
{
    InitGdiplusOnce();

    const int SIZE = 32;
    const int W = SIZE * 2;
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
//  文字ごとのオフセットリストをキャッシュ
// ============================================================
static const std::vector<std::pair<double, double>>& GetOffsetsForChar(const wchar_t* ch)
{
    static std::map<std::wstring, std::vector<std::pair<double, double>>> cache;
    if (cache.find(ch) == cache.end()) {
        cache[ch] = GetTextOffsets(ch);
    }
    return cache[ch];
}

// ============================================================
//  上句・中句の弾幕：「散る桜」「逃げ場を塞ぐ」
// ============================================================
static void ShotCharSakura(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const wchar_t* chars[] = { L"散", L"る", L"さ", L"く", L"ら" };
        const auto& offsets = GetOffsetsForChar(chars[pSet->param_i[0] % 5]);

        for (auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->param_d[0] + off.first * 2.0;
            shot->y = pSet->param_d[1] + off.second * 2.0;
            shot->muki = 0;
            shot->speed = 0;
            // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            shot->kind = img_enemyShotSmallBall[5]; // マゼンタ
            shot->param_i[0] = 1; // 1: 文字構成弾フラグ

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 上句：「散る桜」扇状に展開
    if (pSet->count < 60) {
        pSet->param_d[0] += pSet->param_d[2] * cos(pSet->param_d[3]);
        pSet->param_d[1] += pSet->param_d[2] * sin(pSet->param_d[3]);
    }
    else if (pSet->count == 60) {
        // 静止して花弁（小弾）を全方位に拡散
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 36; ++i) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->param_d[0];
            shot->y = pSet->param_d[1];
            shot->muki = i * (DX_PI * 2.0 / 36.0);
            shot->speed = 2.0;
            shot->kind = img_enemyShotScale[5]; // マゼンタの鱗弾
            shot->param_i[0] = 0;

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }
    // 中句：「逃げ場を塞ぐ」レーザーによる格子形成
    else if (pSet->count >= 120 && pSet->count < 420) {
        if (pSet->count == 120) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        if (pSet->count % 4 == 0) {
            for (int i = 0; i < 7; ++i) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = pSet->param_d[0];
                shot->y = pSet->param_d[1];
                // 基準の向きから扇状に固定角度で射出
                shot->muki = pSet->param_d[3] + (i - 3) * (DX_PI / 8.0);
                shot->speed = 6.0;
                shot->kind = img_enemyShotLaser[0]; // 赤の短レーザー
                shot->param_i[0] = 0;

                shot->prev = pSet->pEnemyShotHead->prev;
                shot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = shot;
                pSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    // 1サイクル終了時（下句が画面下に消える頃）に文字を消去
    if (pSet->count == 480) {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {
                sEnemyShot* del = p;
                p = p->next;
                del->prev->next = del->next;
                del->next->prev = del->prev;
                delete del;
            }
            else {
                p = p->next;
            }
        }
    }

    // 弾の座標更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 1) {
            if (pSet->count < 60) {
                p->x += pSet->param_d[2] * cos(pSet->param_d[3]);
                p->y += pSet->param_d[2] * sin(pSet->param_d[3]);
            }
        }
        else {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
//  中句の表示：「逃げ場を塞ぐ」（画面上部に一時配置）
// ============================================================
static void ShotCharMid(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const wchar_t* chars[] = { L"逃", L"げ", L"場", L"を", L"塞", L"ぐ" };

        // 6文字を画面上部（Y=80）に横一列で等間隔に配置
        for (int i = 0; i < 6; ++i) {
            const auto& offsets = GetOffsetsForChar(chars[i]);
            double charX = 50.0 + i * 76.0; // X座標を分散
            double charY = 80.0;           // 画面上部の高さ

            for (const auto& off : offsets) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = charX + off.first * 1.8;  // スケール
                shot->y = charY + off.second * 1.8;
                shot->muki = 0;
                shot->speed = 0;
                shot->kind = img_enemyShotSmallBall[0]; // 黄色の小弾

                shot->prev = pSet->pEnemyShotHead->prev;
                shot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = shot;
                pSet->pEnemyShotHead->prev = shot;
            }
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 60フレーム後（phaseTime == 240：「もじのあめ」開始時）に配置した文字弾を消去
    if (pSet->count == 60) {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* del = p;
            p = p->next;
            del->prev->next = del->next;
            del->next->prev = del->prev;
            delete del;
        }
    }
}

// ============================================================
//  下句の弾幕：「文字の雨」
// ============================================================
static void ShotCharRain(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const wchar_t* chars[] = { L"も", L"じ", L"の", L"あ", L"め" };
        const auto& offsets = GetOffsetsForChar(chars[pSet->param_i[0] % 5]);

        for (auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x + off.first * 2.0;
            shot->y = pSet->y + off.second * 2.0;
            shot->muki = DX_PI / 2.0; // 真下
            shot->speed = 3.0;
            shot->kind = img_enemyShotSmallBall[6]; // 白
            shot->margin = 120;

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
//  敵本体のパターン（エントリポイント）
// ============================================================
void EnemyPat_Haiku_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面上部に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    int phaseTime = count % 600; // 600フレーム(10秒)で1サイクル

    if (phaseTime == 60) {
        // 「散る桜」生成
        for (int i = 0; i < 5; ++i) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotCharSakura;

            pSet->param_d[0] = enemy.x;
            pSet->param_d[1] = enemy.y;
            pSet->param_d[2] = 2.5; // 移動速度
            // ボスの位置から扇状（下方向中心）に射出
            pSet->param_d[3] = (DX_PI / 2.0) - (DX_PI / 4.0) + i * (DX_PI / 8.0);

            pSet->param_i[0] = i; // 文字インデックス

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    if (phaseTime == 180) {
        // レーザー開始と同時に画面上部へ「逃げ場を塞ぐ」を配置
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotCharMid;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (phaseTime >= 240 && phaseTime < 480) {
        // 「文字の雨」生成
        if (phaseTime % 10 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotCharRain;

            pSet->x = GetRand(480); // ランダムなX座標
            pSet->y = -40; // 画面上部外
            pSet->param_i[0] = phaseTime / 10 % 5; // "もじのあめ" のいずれか

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