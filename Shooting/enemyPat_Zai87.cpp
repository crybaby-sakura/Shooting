#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// リストから弾を外してプールに戻すマクロ
#define REMOVE_AND_DELETE_SHOT(shot) \
    shot->prev->next = shot->next;   \
    shot->next->prev = shot->prev;   \
    delete shot;

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
//  1文字の白ピクセル座標を取得（バウンディングボックス中心を原点）
// ============================================================
static std::vector<std::pair<double, double>> SampleCharPixels(wchar_t c)
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
    g.DrawString(&c, 1, &font, PointF(0, 0), &whiteBrush);

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
//  俳句の全文字オフセットをキャッシュ
// ============================================================
static const std::vector<std::vector<std::pair<double, double>>>& GetHaikuOffsets()
{
    static std::vector<std::vector<std::pair<double, double>>> offsets;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const wchar_t* haiku = L"春風に舞い散る桜白き罠";
        for (int i = 0; haiku[i] != L'\0'; ++i) {
            offsets.push_back(SampleCharPixels(haiku[i]));
        }
    }
    return offsets;
}

// ============================================================
//  弾幕パターン：文字を撃つ俳句弾幕
// ============================================================
static void ShotHaiku_Characters(sEnemyShotSet* pEnemyShotSet)
{
    constexpr double SCALE = 5.0;
    const auto& haikuOffsets = GetHaikuOffsets();
    const int totalChars = (int)haikuOffsets.size(); // 11文字

    // 60フレームごとに1文字撃つ
    if (pEnemyShotSet->count % 60 == 0) {
        int charIndex = pEnemyShotSet->count / 60;
        if (charIndex < totalChars) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            const auto& offsets = haikuOffsets[charIndex];

            // 「白き罠」の場合、ワープ先のグリッド座標をシャッフルして用意
            std::vector<std::pair<double, double>> gridCenters;
            if (charIndex >= 8) {
                for (int y = 10; y < 480; y += 20) {
                    for (int x = 10; x < 480; x += 20) {
                        gridCenters.emplace_back(x, y);
                    }
                }
                // Fisher-Yates シャッフル
                for (int i = (int)gridCenters.size() - 1; i > 0; --i) {
                    int j = GetRand(i);
                    std::swap(gridCenters[i], gridCenters[j]);
                }
            }

            for (size_t i = 0; i < offsets.size(); ++i) {
                sEnemyShot* shot = new sEnemyShot;
                shot->x = pEnemyShotSet->x + offsets[i].first * SCALE;
                shot->y = pEnemyShotSet->y + offsets[i].second * SCALE;
                shot->muki = pEnemyShotSet->muki; // 下方向
                shot->speed = 2.0;
                shot->kind = img_enemyShotSmallBall[6]; // 白い小玉

                // どの文字かを記録 (0~2:春風に, 3~7:舞い散る桜, 8~10:白き罠, -1:破裂弾)
                shot->param_i[0] = charIndex;

                // --- 文字ごとの初期パラメータ設定 ---
                if (charIndex <= 2) {
                    // 【春風に】うねうね用の角速度
                    shot->param_d[0] = 0.03 + (GetRand(20) - 10) / 1000.0;
                }
                else if (charIndex <= 7) {
                    // 【舞い散る桜】散らばる方向と速度
                    shot->param_d[0] = GetRand(628) / 100.0;
                    shot->param_d[1] = 1.5 + GetRand(20) / 10.0;
                }
                else {
                    // 【白き罠】ワープ先の座標
                    if (i < gridCenters.size()) {
                        shot->param_d[0] = gridCenters[i].first;
                        shot->param_d[1] = gridCenters[i].second;
                    }
                    else {
                        shot->param_d[0] = 20.0 + GetRand(440);
                        shot->param_d[1] = 20.0 + GetRand(440);
                    }
                }

                shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                shot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = shot;
                pEnemyShotSet->pEnemyShotHead->prev = shot;
            }
        }
    }

    // --- 弾の移動・変化・消去処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 次のノードを事前に保持
        sEnemyShot* pNext = pShot->next;
        int charIdx = pShot->param_i[0];

        if (charIdx <= 2) {
            // 【春風に】うねうねと螺旋を描く
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (charIdx <= 7) {
            // 【舞い散る桜】
            if (pShot->count == 60) {
                // 破裂して鱗弾を撒く
                for (int i = 0; i < 2; i++) {
                    sEnemyShot* burst = new sEnemyShot;
                    burst->x = pShot->x;
                    burst->y = pShot->y;
                    burst->muki = DX_PI * 2.0 / 2.0 * i + GetRand(60) / 60.0 * (DX_PI * 2.0 / 2.0);
                    burst->speed = 2.0 + GetRand(10) / 10.0;
                    burst->kind = img_enemyShotScale[5];
                    burst->param_i[0] = -1; // 通常直進弾のフラグ

                    burst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    burst->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = burst;
                    pEnemyShotSet->pEnemyShotHead->prev = burst;
                }
                REMOVE_AND_DELETE_SHOT(pShot);
                pShot = pNext;
                continue;
            }
            else if (pShot->count < 30) {
                // 最初は文字の形を保って直進
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // その後、個別の方向に舞い散る
                pShot->x += pShot->param_d[1] * cos(pShot->param_d[0]);
                pShot->y += pShot->param_d[1] * sin(pShot->param_d[0]) + 0.5; // 少し重力気味
            }
        }
        else if (charIdx >= 8) {
            // 【白き罠】
            if (pShot->count > 180) {
                // 収束後、一定時間で消去
                REMOVE_AND_DELETE_SHOT(pShot);
                pShot = pNext;
                continue;
            }

            if (pShot->count == 30) {
                // 画面全体のグリッド座標にワープ（散らばって敷き詰める演出）
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->speed = 0.0;
                pShot->muki = 0.0;
            }
            else if (pShot->count >= 90) {
                // ボスに向かって収束
                pShot->muki = atan2(pEnemyShotSet->y - pShot->y, pEnemyShotSet->x - pShot->x);
                pShot->speed += 0.15;
                if (pShot->speed > 8.0) pShot->speed = 8.0;
            }

            if (pShot->count != 30) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else {
            // 桜の破裂弾（param_i[0] == -1）などの通常直進弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Haiku_Zai()
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

    // 900フレーム（15秒）ごとに発動
    if (count % 900 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHaiku_Characters;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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