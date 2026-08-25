// enemyPat_HaikuRanbu.cpp
// 俳句弾幕「弾幕を　抜けて涼しき　夏の月」
//
// ボスが自作の俳句(5-7-5)を、文字形状の弾幕として詠み上げていくパターン。
// 実装上は「モーラ数」ではなく実際の文字(グリフ)数で区切っている。
//   上五「弾幕を」   … 3文字
//   中七「抜けて涼しき」… 6文字
//   下五「夏の月」   … 3文字
//
// 全体は4フェーズ・900フレーム周期でループする。
//   フェーズ1(count 60-210)  : 上五3文字を毛筆風に収束させ、収束毎に自機狙い3wayで反撃
//   フェーズ2(count 260-520) : 中七6文字を2段組で収束させながら、隙間が正弦波で移動する降下弾壁を展開
//   フェーズ3(count 560-710) : 下五3文字を収束させ、収束毎に拡散リングバーストを連鎖
//   フェーズ4(count 760-800) : 満月が出現し点滅予告 → count800で全17…ではなく全12グリフの弾が
//                               各文字の中心から放射状に加速飛散、同時に満月から自機狙い5way+大型リングを解放
//
// 文字の弾配置は GDI+ でグリフをラスタライズし、白ピクセル座標をオフセットとして
// 弾の目標位置に使う(enemyPat_sampleForAI.cpp の SampleTextPixelsStr 系の手法を流用・汎用化)。
//
// アーキテクチャ規約:
//   ・弾の位置は毎フレーム pShot->count のみから絶対位置を再計算する(formula駆動、+=による蓄積更新はしない)
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと画面外弾の消去はメインルーチン側の仕様

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <map>
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// ============================================================
//  GDI+ 初期化(enemyPat_sampleForAI.cpp と同じ手法)
// ============================================================
static void HaikuInitGdiplusOnce()
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
//  1文字を描画し、白ピクセル座標をバウンディングボックス中心基準で返す。
//  stride間引きで弾数を抑える(全ピクセルを使うと弾プール(4096発)を圧迫するため)。
// ============================================================
static std::vector<std::pair<double, double>> HaikuSampleGlyphPixels(wchar_t ch, int stride)
{
    HaikuInitGdiplusOnce();

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

    wchar_t buf[2] = { ch, L'\0' };
    g.DrawString(buf, -1, &font, PointF(0, 0), &whiteBrush);

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
        int idx = 0;
        for (const auto& p : whitePixels) {
            if (idx % stride == 0) {
                result.emplace_back(p.first - cx, p.second - cy);
            }
            idx++;
        }
    }
    return result;
}

// 文字ごとのオフセットをキャッシュ(初回アクセス時にのみGDI+で生成)
static const std::vector<std::pair<double, double>>& HaikuGetGlyphOffsets(wchar_t ch)
{
    static std::map<wchar_t, std::vector<std::pair<double, double>>> cache;
    auto it = cache.find(ch);
    if (it != cache.end()) return it->second;

    // stride=3 … 1文字あたり概ね数十〜百発程度に収める狙い
    cache[ch] = HaikuSampleGlyphPixels(ch, 1);
    return cache[ch];
}

// ============================================================
//  弾幕パターン1: 文字形状収束(KanjiConverge)
//
//  ・formationの中心(pEnemyShotSet->x, y)を基準に、文字ピクセル位置へ
//    ランダムな方向から吸い込まれるように収束する(イーズアウト)。
//  ・収束後は目標位置で静止保持。
//  ・pEnemyShotSet->param_i[0] に「生成からの解放トリガーまでのローカル経過フレーム」を
//    入れておくと、そのフレームを過ぎた弾は文字中心から放射状(2次関数加速)に飛散する。
//  ・pEnemyShotSet->param_i[1] に対象文字(wchar_t)を int キャストして渡す。
//  ・完全 formula 駆動: 毎フレーム pShot->count のみから絶対座標を再計算する(+=蓄積はしない)。
// ============================================================
static constexpr int    HAIKU_CONVERGE_FRAMES = 30;
static constexpr double HAIKU_RELEASE_ACCEL = 0.0018;

static void ShotKanjiConverge(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        wchar_t ch = (wchar_t)pEnemyShotSet->param_i[1];
        int     colorIdx = pEnemyShotSet->kind;
        int     releaseLocal = pEnemyShotSet->param_i[0];

        const auto& offsets = HaikuGetGlyphOffsets(ch);
        constexpr double SCALE = 3.6;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (const auto& off : offsets) {
            sEnemyShot* shot = new sEnemyShot;

            double targetX = off.first * SCALE;
            double targetY = off.second * SCALE;

            // 吸い込み始点: 文字外周のさらに外側、ランダムな方向・距離
            double inAngle = (GetRand(3599) / 3599.0) * 2.0 * DX_PI;
            double inDist = 160.0 + GetRand(80);

            shot->x = pEnemyShotSet->x + targetX;
            shot->y = pEnemyShotSet->y + targetY;

            shot->param_d[0] = targetX;
            shot->param_d[1] = targetY;
            shot->param_d[2] = cos(inAngle) * inDist;
            shot->param_d[3] = sin(inAngle) * inDist;
            
            // 解放方向: 文字中心から見た自ピクセルの方向(中心付近はランダム方向で代用)
            double dirAngle;
            if (fabs(targetX) < 0.01 && fabs(targetY) < 0.01) {
                dirAngle = (GetRand(3599) / 3599.0) * 2.0 * DX_PI;
            }
            else {
                dirAngle = atan2(targetY, targetX);
            }
            shot->param_d[4] = dirAngle;
            shot->param_i[0] = releaseLocal;

            shot->muki = dirAngle; // 弾の向き=解放時に飛んでいく方向
            shot->speed = 0.0;      // 未使用(位置はformula駆動)
            shot->kind = img_enemyShotSmallBall[colorIdx];
            shot->margin = 120;

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        int    releaseLocal = p->param_i[0];
        double targetX = p->param_d[0];
        double targetY = p->param_d[1];

        if (releaseLocal >= 0 && p->count >= releaseLocal) {
            // 解放フェーズ: 文字中心から放射状に2次関数加速で飛散
            double re = (double)(p->count - releaseLocal);
            double dist = 0.5 * HAIKU_RELEASE_ACCEL * re * re;
            p->x = pEnemyShotSet->x + targetX + cos(p->param_d[4]) * dist;
            p->y = pEnemyShotSet->y + targetY + sin(p->param_d[4]) * dist;
        }
        else if (p->count < HAIKU_CONVERGE_FRAMES) {
            // 収束フェーズ: イーズアウトで外側始点→目標位置へ
            double t = p->count / (double)HAIKU_CONVERGE_FRAMES;
            double ease = 1.0 - pow(1.0 - t, 3.0);
            p->x = pEnemyShotSet->x + targetX + p->param_d[2] * (1.0 - ease);
            p->y = pEnemyShotSet->y + targetY + p->param_d[3] * (1.0 - ease);
        }
        else {
            // 静止保持フェーズ
            p->x = pEnemyShotSet->x + targetX;
            p->y = pEnemyShotSet->y + targetY;
        }

        p = p->next;
    }
}

// ============================================================
//  弾幕パターン2: 自機狙いNway(直線・formula駆動)
//  param_i[0]=way数, param_d[0]=総広がり角(rad), param_d[1]=速さ
// ============================================================
static void ShotAimedNWay(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int    n = pEnemyShotSet->param_i[0];
        double spread = pEnemyShotSet->param_d[0];
        double speed = pEnemyShotSet->param_d[1];
        int    colorIdx = pEnemyShotSet->kind;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = pEnemyShotSet->muki;
        for (int i = 0; i < n; i++) {
            sEnemyShot* shot = new sEnemyShot;
            double a = (n == 1) ? baseAngle
                : baseAngle - spread * 0.5 + spread * (i / (double)(n - 1));

            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = a;
            shot->speed = speed;
            shot->param_d[0] = a;
            shot->kind = img_enemyShotMediumBall[colorIdx];

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x = pEnemyShotSet->x + p->speed * cos(p->param_d[0]) * p->count;
        p->y = pEnemyShotSet->y + p->speed * sin(p->param_d[0]) * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕パターン3: 拡散リングバースト(formula駆動)
//  param_i[0]=弾数N, param_d[0]=速さ, param_d[1]=初期回転オフセット(rad)
// ============================================================
static void ShotRingBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int    n = pEnemyShotSet->param_i[0];
        double speed = pEnemyShotSet->param_d[0];
        double rotOffset = pEnemyShotSet->param_d[1];
        int    colorIdx = pEnemyShotSet->kind;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < n; i++) {
            sEnemyShot* shot = new sEnemyShot;
            double a = rotOffset + 2.0 * DX_PI * i / n;

            shot->x = pEnemyShotSet->x;
            shot->y = pEnemyShotSet->y;
            shot->muki = a;
            shot->speed = speed;
            shot->param_d[0] = a;
            shot->kind = img_enemyShotSmallBall[colorIdx];

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x = pEnemyShotSet->x + p->speed * cos(p->param_d[0]) * p->count;
        p->y = pEnemyShotSet->y + p->speed * sin(p->param_d[0]) * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕パターン4: 降下する弾の壁(隙間が正弦波で移動する。中七フェーズ用)
//  spawn時に pEnemyShotSet->param_d[0]=隙間中心x, param_d[1]=隙間幅 を受け取る。
// ============================================================
static void ShotWallRow(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        double gapCenter = pEnemyShotSet->param_d[0];
        double gapWidth = pEnemyShotSet->param_d[1];
        int    colorIdx = pEnemyShotSet->kind;

        constexpr double STEP = 24.0;
        for (double x = 12.0; x <= 468.0; x += STEP) {
            if (fabs(x - gapCenter) < gapWidth * 0.5) continue;

            sEnemyShot* shot = new sEnemyShot;
            shot->x = x;
            shot->y = -10.0;
            shot->param_d[0] = x;
            shot->muki = DX_PI * 0.5; // 下向き
            shot->speed = 1.3;
            shot->kind = img_enemyShotBullet[colorIdx];

            shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            shot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = shot;
            pEnemyShotSet->pEnemyShotHead->prev = shot;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x = p->param_d[0];
        p->y = -10.0 + p->speed * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕パターン5: 満月(点滅予告 → 結句フェーズで自ら5wayとリングを解放する)
//  pEnemyShotSet->param_i[0] = 解放までのローカル経過フレーム
// ============================================================
static void ShotMoonRelease(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* shot = new sEnemyShot;
        shot->x = pEnemyShotSet->x;
        shot->y = pEnemyShotSet->y;
        shot->muki = 0.0;
        shot->speed = 0.0;
        shot->kind = img_enemyShotLargeBall[8]; // 橙
        shot->param_i[0] = 0; // 発火済みフラグ(0=未発火)

        shot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        shot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = shot;
        pEnemyShotSet->pEnemyShotHead->prev = shot;
    }

    int releaseLocal = pEnemyShotSet->param_i[0];
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x = pEnemyShotSet->x;
        p->y = pEnemyShotSet->y;
        // 予告点滅: 8フレームごとに橙/白を切り替え
        p->kind = ((p->count / 8) % 2 == 0) ? img_enemyShotLargeBall[8] : img_enemyShotLargeBall[6];

        if (p->count == releaseLocal && p->param_i[0] == 0) {
            p->param_i[0] = 1; // 二重発火防止

            // 満月の輪郭を表す大型リング
            sEnemyShotSet* ring = new sEnemyShotSet;
            ring->count = 0;
            ring->patternFunc = ShotRingBurst;
            ring->x = pEnemyShotSet->x;
            ring->y = pEnemyShotSet->y;
            ring->kind = 1; // 黄
            ring->param_i[0] = 40;
            ring->param_d[0] = 1.9;
            ring->param_d[1] = 0.0;
            ring->pEnemyShotHead = new sEnemyShot;
            ring->pEnemyShotHead->prev = ring->pEnemyShotHead;
            ring->pEnemyShotHead->next = ring->pEnemyShotHead;
            ring->prev = enemyShotSetHead.prev;
            ring->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = ring;
            enemyShotSetHead.prev = ring;

            // 自機狙い5way
            sEnemyShotSet* aim = new sEnemyShotSet;
            aim->count = 0;
            aim->patternFunc = ShotAimedNWay;
            aim->x = pEnemyShotSet->x;
            aim->y = pEnemyShotSet->y;
            aim->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            aim->kind = 0; // 赤
            aim->param_i[0] = 5;
            aim->param_d[0] = 55.0 / 180.0 * DX_PI;
            aim->param_d[1] = 2.6;
            aim->pEnemyShotHead = new sEnemyShot;
            aim->pEnemyShotHead->prev = aim->pEnemyShotHead;
            aim->pEnemyShotHead->next = aim->pEnemyShotHead;
            aim->prev = enemyShotSetHead.prev;
            aim->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = aim;
            enemyShotSetHead.prev = aim;
        }
        p = p->next;
    }
}

// ============================================================
//  補助関数群: 各種 ShotSet を生成し enemyShotSetHead へ登録する
// ============================================================
static void SpawnKanjiGlyph(wchar_t ch, double x, double y, int colorIdx, int releaseLocalFrame)
{
    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->patternFunc = ShotKanjiConverge;
    set->x = x;
    set->y = y;
    set->kind = colorIdx;
    set->param_i[0] = releaseLocalFrame;
    set->param_i[1] = (int)ch;

    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;
}

static void SpawnAimedNWay(double x, double y, int n, double spreadDeg, double speed, int colorIdx)
{
    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->patternFunc = ShotAimedNWay;
    set->x = x;
    set->y = y;
    set->muki = atan2(player.y - y, player.x - x);
    set->kind = colorIdx;
    set->param_i[0] = n;
    set->param_d[0] = spreadDeg / 180.0 * DX_PI;
    set->param_d[1] = speed;

    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;
}

static void SpawnRingBurst(double x, double y, int n, double speed, double rotOffset, int colorIdx)
{
    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->patternFunc = ShotRingBurst;
    set->x = x;
    set->y = y;
    set->kind = colorIdx;
    set->param_i[0] = n;
    set->param_d[0] = speed;
    set->param_d[1] = rotOffset;

    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;
}

// ============================================================
//  敵本体パターン
//  俳句:「弾幕を　抜けて涼しき　夏の月」(5-7-5)
// ============================================================
void EnemyPat_Haiku_Claude()
{
    static const wchar_t kKamiGo[] = { L'弾', L'幕', L'を' };                     // 上五 3字
    static const wchar_t kNakaShichi[] = { L'抜', L'け', L'て', L'涼', L'し', L'き' }; // 中七 6字
    static const wchar_t kShimoGo[] = { L'夏', L'の', L'月' };                     // 下五 3字

    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.4 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    constexpr int TOTAL_CYCLE = 900; // 1周のフレーム数(結句後ループ)
    constexpr int RELEASE_FRAME = 800; // 満月解放タイミング(周期内の絶対フレーム)

    int localCount = (count - 1) % TOTAL_CYCLE + 1;

    // ---- フェーズ1: 上五「弾幕を」(50フレーム間隔で収束、収束後に自機狙い3way) ----
    for (int i = 0; i < 3; i++) {
        int spawnFrame = 60 + i * 50;
        double gx = 160.0 + i * 80.0;
        double gy = 90.0;

        if (localCount == spawnFrame) {
            SpawnKanjiGlyph(kKamiGo[i], gx, gy, 0 /*赤*/, RELEASE_FRAME - spawnFrame);
        }
        if (localCount == spawnFrame + HAIKU_CONVERGE_FRAMES + 10) {
            SpawnAimedNWay(gx, gy, 3, 40.0, 2.0, 0);
        }
    }

    // ---- フェーズ2: 中七「抜けて涼しき」(2段×3字、40フレーム間隔で収束) + 降下弾壁 ----
    for (int i = 0; i < 6; i++) {
        int spawnFrame = 260 + i * 40;
        if (localCount == spawnFrame) {
            double gx = 140.0 + (i % 3) * 100.0;
            double gy = (i < 3) ? 170.0 : 250.0;
            SpawnKanjiGlyph(kNakaShichi[i], gx, gy, 3 /*シアン*/, RELEASE_FRAME - spawnFrame);
        }
    }
    if (localCount >= 260 && localCount <= 520 && localCount % 18 == 0) {
        sEnemyShotSet* wall = new sEnemyShotSet;
        wall->count = 0;
        wall->patternFunc = ShotWallRow;
        wall->x = 0.0;
        wall->y = 0.0;
        wall->kind = 4; // 青
        wall->param_d[0] = 240.0 + 150.0 * sin((localCount - 260) * 0.05); // 隙間中心x
        wall->param_d[1] = 75.0; // 隙間幅

        wall->pEnemyShotHead = new sEnemyShot;
        wall->pEnemyShotHead->prev = wall->pEnemyShotHead;
        wall->pEnemyShotHead->next = wall->pEnemyShotHead;

        wall->prev = enemyShotSetHead.prev;
        wall->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = wall;
        enemyShotSetHead.prev = wall;
    }

    // ---- フェーズ3: 下五「夏の月」(50フレーム間隔で収束、収束後にリングバースト連鎖) ----
    for (int i = 0; i < 3; i++) {
        int spawnFrame = 560 + i * 50;
        double gx = 160.0 + i * 80.0;
        double gy = 330.0;

        if (localCount == spawnFrame) {
            SpawnKanjiGlyph(kShimoGo[i], gx, gy, 1 /*黄*/, RELEASE_FRAME - spawnFrame);
        }
        if (localCount == spawnFrame + HAIKU_CONVERGE_FRAMES + 10) {
            SpawnRingBurst(gx, gy, 20, 1.4, (double)i * 0.3, 1);
        }
    }

    // ---- フェーズ4: 結句・満月開放(予告点滅 → 全グリフ弾が放射状飛散 + 自機狙い5way + 大型リング) ----
    if (localCount == 760) {
        sEnemyShotSet* moon = new sEnemyShotSet;
        moon->count = 0;
        moon->patternFunc = ShotMoonRelease;
        moon->x = 240.0;
        moon->y = 200.0;
        moon->kind = 8;
        moon->param_i[0] = RELEASE_FRAME - 760; // ローカル40フレーム後に解放

        moon->pEnemyShotHead = new sEnemyShot;
        moon->pEnemyShotHead->prev = moon->pEnemyShotHead;
        moon->pEnemyShotHead->next = moon->pEnemyShotHead;

        moon->prev = enemyShotSetHead.prev;
        moon->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = moon;
        enemyShotSetHead.prev = moon;
    }
    // 各グリフのShotSetは生成時にRELEASE_FRAME基準の解放ローカルフレームを
    // 既に持っているため(SpawnKanjiGlyphの第5引数)、count==RELEASE_FRAMEの瞬間に
    // 全フェーズの文字弾が自動的に一斉解放される。
}