// menu.cpp

#include "DxLib.h"
#include "gv.h"
#include "stateManager.h"
#include "menu.h"
#include "stageData.h"
#include "replay.h"
#include "imgSoundLoad.h"
#include "initial.h"
#include "gameScreen.h"
#include "tasAutoMaker.h"
#include <string> 
#include <math.h>

// レイアウト設定（マクロは GAME_W を参照するため実行時に評価される）
#define MARGIN_X 60   // 左右余白
#define PANEL_X MARGIN_X
#define PANEL_Y 70
#define PANEL_W (GAME_W - MARGIN_X * 2)          // 画面幅に応じてパネル幅を変更
#define PANEL_H 240
#define CELL_W 56
#define CELL_H 24
#define GRID_COLS 10
#define GRID_ROWS 10
#define GRID_LEFT (PANEL_X + (PANEL_W - CELL_W * GRID_COLS) / 2)
#define GRID_TOP  (PANEL_Y + 30)
#define DESC_Y 335

// フォントサイズ関連
#define FONT_CHAR_W 8
#define FONT_CHAR_H 16
#define TEXT_W (FONT_CHAR_W * 3)
#define TEXT_H FONT_CHAR_H

static bool showReplayError = false;
static int replayErrorTimer = 0;

extern void iniGame();

// 文字列を指定幅で改行した行のリストを返す (ピクセル単位)
std::vector<std::string> WrapText(const char* text, int maxWidth)
{
    std::vector<std::string> lines;
    if (!text || !*text) {
        lines.push_back("");
        return lines;
    }

    const char* p = text;
    std::string currentLine;

    while (*p) {
        if (*p == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            p++;
            continue;
        }

        int charBytes = 1;
        if (((unsigned char)*p >= 0x81 && (unsigned char)*p <= 0x9F) ||
            ((unsigned char)*p >= 0xE0 && (unsigned char)*p <= 0xFC)) {
            charBytes = 2;  // Shift_JIS全角の先頭バイト
        }

        std::string testLine = currentLine + std::string(p, charBytes);
        int testWidth = GetDrawStringWidth(testLine.c_str(), -1);

        if (testWidth > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine.clear();
            continue;
        }
        else {
            currentLine.append(p, charBytes);
            p += charBytes;
        }
    }

    if (!currentLine.empty() || lines.empty())
        lines.push_back(currentLine);

    return lines;
}

static void DrawStylishBackground()
{
    // 深い宇宙風グラデーション
    const int topR = 4, topG = 6, topB = 28;
    const int botR = 16, botG = 4, botB = 36;

    for (int y = 0; y < GAME_H; ++y)
    {
        int ratio = (y * 255) / GAME_H;
        int r = topR + ((botR - topR) * ratio) / 255;
        int g = topG + ((botG - topG) * ratio) / 255;
        int b = topB + ((botB - topB) * ratio) / 255;
        DrawLine(0, y, GAME_W, y, GetColor(r, g, b));
    }

    int now = GetNowCount();

    // 薄いネビュラ帯（アルファブレンド）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 18);
    for (int i = 0; i < 5; ++i)
    {
        int cy = (int)(GAME_H * 0.25 +
            sin((double)now * 0.0005 + i * 1.7) * 35.0) + i * 35;
        if (cy < 0)          cy = 0;
        if (cy > GAME_H - 3) cy = GAME_H - 3;
        DrawBox(0, cy, GAME_W, cy + 3, GetColor(50, 100, 180), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 星フィールド（二層の流れ星風）
    static bool starInit = false;
    static int  starX[120], starY[120], starSize[120], starSpeed[120];
    static float starPhase[120];

    if (!starInit)
    {
        unsigned int seed = 123456789u;
        for (int i = 0; i < 120; ++i)
        {
            seed = seed * 1103515245u + 12345u;
            starX[i] = (int)((seed >> 16) % (unsigned)GAME_W);

            seed = seed * 1103515245u + 12345u;
            starY[i] = (int)((seed >> 16) % (unsigned)GAME_H);

            seed = seed * 1103515245u + 12345u;
            starSize[i] = (seed % 4 == 0) ? 1 : 0;  // 1=大きめ

            seed = seed * 1103515245u + 12345u;
            starSpeed[i] = (seed % 4 == 0) ? 2 : 1;

            seed = seed * 1103515245u + 12345u;
            starPhase[i] = (float)(seed % 628) / 100.0f;
        }
        starInit = true;
    }

    for (int i = 0; i < 120; ++i)
    {
        int y = (starY[i] + (now / 50) * starSpeed[i]) % GAME_H;
        int x = starX[i] + (int)(sin((double)now * 0.001 + starPhase[i]) * 10.0);
        if (x < 0)       x += GAME_W;
        if (x >= GAME_W) x -= GAME_W;

        int brightness = 140 + (int)(sin((double)now * 0.002 + starPhase[i]) * 70.0);
        if (brightness > 255) brightness = 255;
        if (brightness < 60)  brightness = 60;

        int blue = brightness + 20;
        if (blue > 255) blue = 255;

        int color = GetColor(brightness, blue, 255);

        if (starSize[i] == 1)
            DrawBox(x, y, x + 1, y + 1, color, TRUE);
        else
            DrawPixel(x, y, color);
    }

    // 上下のビネット（暗くして引き締める）
    for (int y = 0; y < 40; ++y)
    {
        int alpha = (int)(160 * (1.0 - (double)y / 40.0));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
        DrawBox(0, y, GAME_W, y + 1, GetColor(0, 0, 0), TRUE);
        DrawBox(0, GAME_H - 1 - y, GAME_W, GAME_H - y, GetColor(0, 0, 0), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // 外周のアクセントライン
    DrawBox(0, 0, GAME_W, GAME_H, GetColor(0, 128, 255), FALSE);
}

void menuDraw()
{
    int i, x, y;

    // 背景（スタイリッシュ版）
    DrawStylishBackground();

    // タイトル・操作説明（中央配置）
    const char* title = "STAGE SELECT";
    int titleWidth = GetDrawStringWidth(title, -1);
    DrawString((GAME_W - titleWidth) / 2, 20, title, GetColor(255, 255, 0));

    DrawString(MARGIN_X - 10, 46, "<7", GetColor(200, 200, 200));

    const char* instr = "選択:テンキー4568  決定:V  リプレイ再生:R  終了:Q";
    int instrWidth = GetDrawStringWidth(instr, -1);
    DrawString((GAME_W - instrWidth) / 2, 46, instr, GetColor(200, 200, 200));

    DrawString(GAME_W - MARGIN_X - 10, 46, "9>", GetColor(200, 200, 200));

    // 選択パネル背景
    SetDrawBlendMode(DX_BLENDMODE_ADD, 230);
    DrawBox(PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + PANEL_H,
        GetColor(0, 32, 64), TRUE);
    DrawBox(PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + PANEL_H,
        GetColor(64, 192, 255), FALSE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // ★ グリッドのセル幅と左端位置を動的に計算
    // パネル幅いっぱいにセルを配置（10列均等分割）
    int cellW = PANEL_W / GRID_COLS;          // セル幅（余りは両端の余白に吸収）
    int gridWidth = cellW * GRID_COLS;        // グリッド全体の幅
    int gridLeft = PANEL_X + (PANEL_W - gridWidth) / 2;  // グリッド左端

    // カーソル描画
    {
        int cx = gridLeft + cursor.x * cellW;
        int cy = GRID_TOP + cursor.y * CELL_H - 30;   // CELL_Hは固定（24）
        DrawBox(cx, cy, cx + cellW, cy + CELL_H, GetColor(0, 120, 180), TRUE);
        DrawBox(cx, cy, cx + cellW, cy + CELL_H, GetColor(0, 255, 255), FALSE);
    }

    // ステージ番号
    for (i = 0; i < GRID_COLS * GRID_ROWS; i++) {
        int idx = cursor.page * 100 + i;
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        x = gridLeft + col * cellW + (cellW - TEXT_W) / 2;  // セル内で中央揃え
        y = GRID_TOP + row * CELL_H + (CELL_H - TEXT_H) / 2 - 30;

        unsigned int color = (idx < (int)stageData.size()) ? GetColor(255, 255, 255) : GetColor(127, 127, 127);
        DrawFormatString(x, y, color, "%3d", idx);
    }

    // ---------- 説明文エリア（変更なし） ----------
    if (stageNum >= 0 && stageNum < (int)stageData.size()) {
        const int descAreaLeft = MARGIN_X;
        const int descAreaRight = GAME_W - MARGIN_X;
        const int descAreaWidth = descAreaRight - descAreaLeft;
        const int lineHeight = FONT_CHAR_H;

        std::vector<std::string> descLines = WrapText(stageData[stageNum].description, descAreaWidth);

        int titleHeight = lineHeight + 7;
        int sepHeight = 10;
        int descHeight = (int)descLines.size() * lineHeight;
        int bestTimeH = lineHeight + 5;
        const int bottomPadding = lineHeight / 2;
        int areaInnerH = titleHeight + sepHeight + descHeight + bottomPadding + bestTimeH;
        int descAreaTop = DESC_Y;
        int descAreaBottom = descAreaTop + areaInnerH;

        SetDrawBlendMode(DX_BLENDMODE_ADD, 230);
        DrawBox(descAreaLeft, descAreaTop, descAreaRight, descAreaBottom, GetColor(0, 64, 128), TRUE);
        DrawBox(descAreaLeft, descAreaTop, descAreaRight, descAreaBottom, GetColor(128, 255, 255), FALSE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

        DrawFormatString(descAreaLeft + 5, descAreaTop + 5, GetColor(255, 255, 100),
            "★ %s", stageData[stageNum].stageId);

        int sepY = descAreaTop + titleHeight;
        DrawLine(descAreaLeft + 5, sepY, descAreaRight - 5, sepY, GetColor(100, 100, 150));

        int textY = sepY + 8;
        for (const auto& line : descLines) {
            DrawFormatString(descAreaLeft + 5, textY, GetColor(180, 200, 220), "%s", line.c_str());
            textY += lineHeight;
        }

        int bestY = descAreaBottom - bestTimeH;
        int playCountWidth = GetDrawStringWidth("Play Count: 88888", -1);
        int bestTimeRightOffset = 150;

        int playCountX = descAreaRight - bestTimeRightOffset - playCountWidth - 10;
        DrawFormatString(playCountX, bestY,
            GetColor(255, 200, 100), "Play Count: %u", stageData[stageNum].playCount);

        int bestTimeX = descAreaRight - bestTimeRightOffset;
        if (stageData[stageNum].bestTime < 59999) {
            DrawFormatString(bestTimeX, bestY,
                GetColor(255, 200, 100), "BestTime: %5.2f",
                (double)stageData[stageNum].bestTime / 60.0);
        }
        else {
            DrawFormatString(bestTimeX, bestY,
                GetColor(100, 100, 100), "BestTime: --.--");
        }

        if (showReplayError) {
            if (replayErrorTimer > 0) {
                DrawString(descAreaLeft + 5, descAreaBottom + 5,
                    "リプレイファイルが存在しません", GetColor(255, 100, 100));
                replayErrorTimer--;
                if (replayErrorTimer == 0) showReplayError = false;
            }
        }
    }
}

void moveCursor()
{
    stageNum = cursor.page * 100 + cursor.y * 10 + cursor.x;

    if (key[KEY_INPUT_V] == 1 && stageNum < (int)stageData.size()) {
        if (g_isAutoTasMode) {
            TAS_AutoSearchStart();
            return;
        }

        StateManager::ChangeState(Joutai::Game);
        return;
    }

    if (key[KEY_INPUT_R] == 1 && stageNum < (int)stageData.size()) {
        if (!StateManager::ChangeState(Joutai::Replay)) {
            showReplayError = true;
            replayErrorTimer = 120;
        }
        return;
    }

    if (showReplayError) {
        if (key[KEY_INPUT_NUMPAD4] == 1 || key[KEY_INPUT_NUMPAD6] == 1 ||
            key[KEY_INPUT_NUMPAD8] == 1 || key[KEY_INPUT_NUMPAD5] == 1 ||
            key[KEY_INPUT_V] == 1 || key[KEY_INPUT_Q] == 1) {
            showReplayError = false;
        }
    }

    // カーソル移動・ページ切り替え（変更なし）
    if (key[KEY_INPUT_NUMPAD6] == 1 || (key[KEY_INPUT_NUMPAD6] % 4 == 0 && key[KEY_INPUT_NUMPAD6] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.x == 9) cursor.x = 0;
        else               cursor.x++;
    }
    if (key[KEY_INPUT_NUMPAD4] == 1 || (key[KEY_INPUT_NUMPAD4] % 4 == 0 && key[KEY_INPUT_NUMPAD4] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.x == 0) cursor.x = 9;
        else               cursor.x--;
    }
    if (key[KEY_INPUT_NUMPAD5] == 1 || (key[KEY_INPUT_NUMPAD5] % 4 == 0 && key[KEY_INPUT_NUMPAD5] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.y == 9) cursor.y = 0;
        else               cursor.y++;
    }
    if (key[KEY_INPUT_NUMPAD8] == 1 || (key[KEY_INPUT_NUMPAD8] % 4 == 0 && key[KEY_INPUT_NUMPAD8] > 18)) {
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
        if (cursor.y == 0) cursor.y = 9;
        else               cursor.y--;
    }

    if (key[KEY_INPUT_NUMPAD7] == 1) {
        cursor.page = (cursor.page == 0) ? ((int)stageData.size() - 1) / 100 : cursor.page - 1;
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
    }
    if (key[KEY_INPUT_NUMPAD9] == 1) {
        cursor.page = (cursor.page == ((int)stageData.size() - 1) / 100) ? 0 : cursor.page + 1;
        PlaySoundMem(sound_menuCursor, DX_PLAYTYPE_BACK);
    }

    stageNum = cursor.page * 100 + cursor.y * 10 + cursor.x;
}