// gameScreen.cpp

#include "DxLib.h"
#include "stateManager.h"
#include "gameScreen.h"
#include "gv.h"
#include "stageData.h"
#include "imgSoundLoad.h"
#include "menu.h"
#include "replay.h"
#include "player.h"
#include "masterpieceViewer.h"
#include <math.h>
#include <string> 
#include <vector> 

int GAME_W = 640;
int GAME_H = 480;

#define NUM_STARS 30

typedef struct {
    double x, y, speedY;
    int brightness, twinklePhase, color, size;
} Star;

static Star stars[NUM_STARS];
static int starsInitialized = 0;
static int fieldBG = -1;
static int sidePanelBG = -1;
static int lastStageForSidePanel = -1;      // 生成時の stageNum
static int descLineCount = 0;               // 説明文の行数（静的）

// 情報領域の背景色
static const int INFO_BG_COLOR = GetColor(30, 30, 50);

// 戦闘フィールド背景を一枚の画像として生成
static void createFieldBG() {
    fieldBG = MakeScreen(480, 480, FALSE);
    int oldScreen = GetDrawScreen();
    SetDrawScreen(fieldBG);

    // 宇宙空間のグラデーション
    for (int i = 0; i < 480; i++) {
        int shade = (int)(20.0 + 50.0 * (1.0 - (double)i / 480.0));
        int col = GetColor(shade, shade, shade + 20);
        DrawBox(0, i, 480, i + 1, col, TRUE);
    }

    // グリッド線（半透明）
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 20);
    int gridColor = GetColor(255, 255, 255);
    for (int x = 80; x < 480; x += 80) {
        DrawLine(x, 0, x, 479, gridColor);
    }
    for (int y = 80; y < 480; y += 80) {
        DrawLine(0, y, 479, y, gridColor);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    SetDrawScreen(oldScreen);
}

// サイドパネル背景を一枚の画像として生成（動的要素を除く）
static void createSidePanelBG() {
    // 以前の画像を破棄
    if (sidePanelBG != -1) DeleteGraph(sidePanelBG);

    sidePanelBG = MakeScreen(160, 480, FALSE);
    int oldScreen = GetDrawScreen();
    SetDrawScreen(sidePanelBG);

    // パネル背景グラデーション（x=480～640 → 画像内 0～160）
    for (int i = 0; i < 160; i++) {
        int shade = (int)(60.0 * (double)(160 - i) / 160.0);
        int col = GetColor(shade, shade, shade);
        DrawLine(i, 0, i, 480, col);
    }
    // 左端の境界線（画面左端 x=480）
    DrawBox(0, 0, 1, 480, GetColor(100, 100, 100), TRUE);

    const int panelLeft = 10;   // 画像内での座標（画面座標480+10=490）
    const int panelRight = 150; // 画像内（480+150=630）
    const int descMaxWidth = 140;
    const int lineHeight = 16;
    const int halfLine = lineHeight / 2;

    // タイトル背景と文字（画面座標 490,0 相当）
    DrawBox(panelLeft - 1, 0, panelRight, 30, GetColor(0, 50, 100), TRUE);
    DrawFormatString(panelLeft + 2, 3, GetColor(255, 255, 255), "■ STAGE %d", stageNum);

    // 情報エリアの背景（クリア）
    DrawBox(5, 36, 155, 480, INFO_BG_COLOR, TRUE);  // 画像内 5～155 = 画面 485～635

    // stageId
    int currentY = 40;
    DrawFormatString(panelLeft, currentY, GetColor(200, 200, 255), "%s", stageData[stageNum].stageId);
    currentY += lineHeight + halfLine;

    // 説明文（折り返しあり・重い処理はここだけ）
    std::vector<std::string> descLines = WrapText(stageData[stageNum].description, descMaxWidth);
    descLineCount = (int)descLines.size();   // 後で使うために保存
    for (int i = 0; i < descLineCount; i++) {
        DrawFormatString(panelLeft, currentY, GetColor(180, 200, 220), "%s", descLines[i].c_str());
        currentY += lineHeight;
    }

    // 説明文とプレイ回数の間（1行あける）
    currentY += lineHeight;

    // プレイ回数行（動的描画で上書きするため、位置だけ進める）
    // 静的には何も描かない
    currentY += lineHeight;

    // Play Count と Best の間
    currentY += lineHeight;

    // BestTime 行（動的描画で上書き）
    currentY += lineHeight;

    // Time は動的なのでここでは描かない
    currentY += lineHeight;

    // ---- BOSS 区切り線（上下に0.5行のスペース） ----
    currentY += halfLine;
    DrawLine(panelLeft, currentY, panelLeft + 140, currentY, GetColor(100, 100, 150));
    currentY += 1 + halfLine;

    // BOSS ラベル
    DrawFormatString(panelLeft, currentY, GetColor(255, 255, 255), "BOSS");
    currentY += 18;

    // HP バー枠（塗りは動的）
    DrawBox(panelLeft, currentY, panelLeft + 140, currentY + 5, GetColor(0, 255, 255), FALSE);
    currentY += 10;
    // HP数値テキストは動的なので枠だけ

    currentY += lineHeight * 2;   // HP表示の下のスペース

    // Q: Quit（常に表示）
    if (!masterpieceMode) {
        DrawString(panelLeft + 5, currentY, "Q: Quit", GetColor(255, 255, 255));
        currentY += lineHeight;
    }

    // リプレイモード表示用の予約行（動的描画に任せる）
    currentY += lineHeight;

    // 傑作選コメント（静的描画）
    if (masterpieceMode && g_masterpieceComment) {
        currentY += lineHeight;
        const int commentAreaWidth = 140;
        std::vector<std::string> commentLines = WrapText(g_masterpieceComment, commentAreaWidth);
        for (const auto& line : commentLines) {
            DrawString(panelLeft + 2, currentY, line.c_str(), GetColor(255, 255, 200));
            currentY += lineHeight + 2;
        }
    }

    SetDrawScreen(oldScreen);
    lastStageForSidePanel = stageNum;
}

void resetStars() {
    starsInitialized = 0;
}

void initStars() {
    if (starsInitialized) return;
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = GetRand(478);
        stars[i].y = GetRand(478);
        stars[i].speedY = 0.3 + (double)GetRand(120) / 100.0;
        stars[i].brightness = GetRand(255);
        stars[i].twinklePhase = GetRand(360);
        stars[i].size = GetRand(3) + 1;
        int r = 200 + GetRand(55);
        int g = 200 + GetRand(55);
        int b = 200 + GetRand(55);
        stars[i].color = GetColor(r, g, b);
    }
    starsInitialized = 1;
}

void backGround()
{
    initStars();

    if (fieldBG == -1) createFieldBG();
    DrawGraph(0, 0, fieldBG, FALSE);

    for (int i = 0; i < NUM_STARS; i++) {
        if (StateManager::GetState() == Joutai::Game || StateManager::GetState() == Joutai::Replay) {
            stars[i].y += stars[i].speedY;
            if (stars[i].y > 480.0) {
                stars[i].y -= 480.0;
                stars[i].x = GetRand(478);
            }
        }

        double twinkle = 0.5 + 0.5 * sin((double)(count * 2 + stars[i].twinklePhase) / 60.0);
        int alpha = (int)(stars[i].brightness * (0.5 + 0.5 * twinkle));
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

        int cx = (int)stars[i].x;
        int cy = (int)stars[i].y;
        int half = stars[i].size;
        DrawLine(cx - half, cy, cx + half, cy, stars[i].color);
        DrawLine(cx, cy - half, cx, cy + half, stars[i].color);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 特殊演出
void special_performance() {
    static int img_ChatGPT33 = -1;
    static int img_Zai48 = -1;
    static int img_Kimi59_1 = -1;
    static int img_Kimi59_2 = -1;
    static int img_Kimi59_3 = -1;
    static int drawX = 0, drawY = 0;

    if (stageData[stageNum].stageId == "ChatGPT33") {
        if (count == 120) {
            if (img_ChatGPT33 == -1) {
                img_ChatGPT33 = LoadGraph("assets/images/ChatGPT33.png");
                if (img_ChatGPT33 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_ChatGPT33, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        if (count >= 120 && img_ChatGPT33 != -1) {
            DrawGraph(drawX, drawY, img_ChatGPT33, TRUE);
        }
    }
    else if (stageData[stageNum].stageId == "Zai48") {
        if (count == 120) {
            if (img_Zai48 == -1) {
                img_Zai48 = LoadGraph("assets/images/Zai48.png");
                if (img_Zai48 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Zai48, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        if (count >= 120 && img_Zai48 != -1) {
            DrawGraph(drawX, drawY, img_Zai48, TRUE);
        }
    }
    else if (stageData[stageNum].stageId == "Kimi59") {
        const int T1 = 120;
        if (count == T1) {
            if (img_Kimi59_1 == -1) {
                img_Kimi59_1 = LoadGraph("assets/images/Kimi59_1.png");
                if (img_Kimi59_1 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Kimi59_1, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T2 = 240;
        if (count >= T1 && count < T1 + T2 && img_Kimi59_1 != -1) {
            DrawGraph(drawX, drawY, img_Kimi59_1, TRUE);
        }

        if (count == T1 + T2) {
            if (img_Kimi59_2 == -1) {
                img_Kimi59_2 = LoadGraph("assets/images/Kimi59_2.png");
                if (img_Kimi59_2 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Kimi59_2, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        const int T3 = 240;
        if (count >= T1 + T2 && count < T1 + T2 + T3 && img_Kimi59_2 != -1) {
            DrawGraph(drawX, drawY, img_Kimi59_2, TRUE);
        }

        if (count == T1 + T2 + T3) {
            if (img_Kimi59_3 == -1) {
                img_Kimi59_3 = LoadGraph("assets/images/Kimi59_3.png");
                if (img_Kimi59_3 != -1) {
                    int imgW, imgH;
                    GetGraphSize(img_Kimi59_3, &imgW, &imgH);
                    int scrW, scrH;
                    GetScreenState(&scrW, &scrH, NULL);
                    drawX = (scrW - imgW) / 2;
                    drawY = (scrH - imgH) / 2;
                }
            }            
        }

        if (count >= T1 + T2 + T3 && img_Kimi59_3 != -1) {
            static int centerX = 0, centerY = 0;
            if (count == T1 + T2 + T3) {
                int w, h;
                GetGraphSize(img_Kimi59_3, &w, &h);
                centerX = drawX + w / 2;
                centerY = drawY + h / 2 - 30;
            }

            for (int t = T1 + T2 + T3; t <= count; t += 20) {
                // 1. 拡大率：t に比例して線形に増加
                double scale = 1.0 + (t - (T1 + T2 + T3)) * 0.002;   // 1フレームあたり0.01ずつ拡大

                // 2. 回転角度：t に応じた疑似乱数（毎回同じ値になる決定論的なハッシュ）
                //    例: 大きめの素数を使って疑似的にランダムな角度を生成
                unsigned int hash = (unsigned int)(t * 2654435761u); // Knuth's multiplicative hash
                double angle = hash % 360 * (DX_PI / 180.0);      // 0～360°をラジアンに

                // 描画
                DrawRotaGraph(centerX, centerY, scale, angle, img_Kimi59_3, TRUE);
            }

            if ((count - (T1 + T2 + T3)) % 20 == 0) {
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            }
        }
    }
}

void foreGround() {
    // 距離の閾値（自機のY座標）
    const double NEAR_Y = 40.0;    // このY座標以下で完全透明
    const double FAR_Y = 100.0;    // このY座標以上で完全不透明

    int titleAlpha = 255;
    if (player.y <= FAR_Y) {
        if (player.y <= NEAR_Y) {
            titleAlpha = 0;
        }
        else {
            double t = (player.y - NEAR_Y) / (FAR_Y - NEAR_Y);
            titleAlpha = (int)(255 * t);
        }
    }

    if (titleAlpha <= 0) return;

    // 文字サイズを設定
    const int FONT_SIZE = 16;               // 好みのサイズに変更
    int BORDER_WIDTH = FONT_SIZE / 10;      // 縁取り太さをフォントサイズから自動計算（ここでは2px相当）
    if (BORDER_WIDTH < 1) BORDER_WIDTH = 1;

    // 縁取り文字の描画設定
    const int TITLE_X = 10;
    const int TITLE_Y = 10;
    const int BORDER_COLOR = GetColor(0, 0, 0);             // 縁取り色（黒）
    const int TEXT_COLOR = GetColor(255, 255, 255);         // 文字色（白）
    const char* titleText = stageData[stageNum].stageTitle; // 実際のステージタイトル

    // デフォルトのフォントサイズを保存してから設定
    int defaultFontSize = GetFontSize();   // DxLib 3.20以降で使用可。なければ無視してOK
    SetFontSize(FONT_SIZE);

    if (titleAlpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, titleAlpha / 6);
    }

    // 縁取り（BORDER_WIDTHだけずらす）
    DrawFormatString(TITLE_X - BORDER_WIDTH, TITLE_Y, BORDER_COLOR, "%s", titleText);
    DrawFormatString(TITLE_X + BORDER_WIDTH, TITLE_Y, BORDER_COLOR, "%s", titleText);
    DrawFormatString(TITLE_X, TITLE_Y - BORDER_WIDTH, BORDER_COLOR, "%s", titleText);
    DrawFormatString(TITLE_X, TITLE_Y + BORDER_WIDTH, BORDER_COLOR, "%s", titleText);

    if (titleAlpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, titleAlpha);
    }

    // 本体の文字を描画
    DrawFormatString(TITLE_X, TITLE_Y, TEXT_COLOR, "%s", titleText);

    if (titleAlpha < 255) {
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }

    // フォントサイズを元に戻す（必要に応じて）
    SetFontSize(defaultFontSize);

    // 特殊演出
    special_performance();
}

void drawSidePanel()
{
    // ステージが変わったら背景画像を作り直す
    if (sidePanelBG == -1 || stageNum != lastStageForSidePanel) {
        createSidePanelBG();
    }
    DrawGraph(480, 0, sidePanelBG, FALSE);

    const int panelLeft = 490;
    const int lineHeight = 16;
    const int halfLine = lineHeight / 2;

    // 動的要素の描画位置を計算（静的要素と同じ順序で currentY を進める）
    int currentY = 40;                                  // stageId の始点
    currentY += lineHeight + halfLine;                  // stageId の高さ
    currentY += descLineCount * lineHeight;             // 説明文の行数分（WrapTextしない）
    currentY += lineHeight;             // 説明文とプレイ回数の間の空行
    int playCountY = currentY;          // プレイ回数の行
    currentY += lineHeight;             // プレイ回数の行の高さ分
    currentY += lineHeight;             // プレイ回数と BestTime の間の隙間
    int bestTimeY = currentY;
    currentY += lineHeight;             // BestTime の行の高さ分
    int timeY = currentY;               // Time の行
    currentY += lineHeight;

    // BOSS 区切り線の位置
    currentY += halfLine;                               // 線の上スペース
    int bossSeparatorY = currentY;
    currentY += 1 + halfLine;                           // 線の下スペース

    // BOSS ラベル
    currentY += 18;

    // HP バーと数値の位置
    int hpBarY = currentY;
    currentY += 10;
    int hpTextY = currentY;

    // リプレイモード表示位置は Q: Quit の下
    int replayY = currentY + (masterpieceMode ? 0 : lineHeight * 2) + lineHeight * 2;  // HP下1行空け + Q行 + 1行空け

    // ---- 動的描画 ----
    // プレイ回数（BestTimeの上、隙間あり）
    DrawBox(panelLeft, playCountY, 630, playCountY + lineHeight, INFO_BG_COLOR, TRUE);
    if (masterpieceMode) {
        DrawFormatString(panelLeft, playCountY, GetColor(255, 255, 255), "Play Count: --");
    }
    else {
        DrawFormatString(panelLeft, playCountY, GetColor(255, 255, 255), "Play Count: %u", stageData[stageNum].playCount);
    }

    // BestTime（更新される可能性があるため動的描画）
    DrawBox(panelLeft, bestTimeY, 630, bestTimeY + lineHeight, INFO_BG_COLOR, TRUE);
    if (stageData[stageNum].bestTime >= 59999) {
        DrawFormatString(panelLeft, bestTimeY, GetColor(255, 255, 255), "BestTime: --.--");
    }
    else {
        DrawFormatString(panelLeft, bestTimeY, GetColor(255, 255, 255), "BestTime: %5.2f", (double)stageData[stageNum].bestTime / 60);
    }

    // Time（背景を塗りつぶしてから描画）
    DrawBox(panelLeft, timeY, 630, timeY + lineHeight, INFO_BG_COLOR, TRUE);
    DrawFormatString(panelLeft, timeY, GetColor(255, 255, 255), "    Time: %5.2f", (double)count / 60);

    // HP バー（塗りのみ、枠は静的）
    DrawBox(panelLeft, hpBarY, panelLeft + 140, hpBarY + 5, INFO_BG_COLOR, TRUE); // 前フレームの塗りを消す
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
    DrawBox(panelLeft, hpBarY, panelLeft + 140 * enemy.hp / enemy.maxHp, hpBarY + 5, GetColor(0, 255, 255), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // HP 数値
    DrawBox(panelLeft, hpTextY, 630, hpTextY + lineHeight, INFO_BG_COLOR, TRUE);
    DrawFormatString(panelLeft, hpTextY, GetColor(255, 255, 255), "HP: %d / %d", enemy.hp, enemy.maxHp);

    // リプレイモード
    if (replayActive) {
        DrawBox(panelLeft, replayY, 630, replayY + lineHeight, INFO_BG_COLOR, TRUE);
        DrawString(panelLeft + 5, replayY, "<Replay Mode>", GetColor(255, 255, 128));
    }

    // デバッグ用：無敵モード中、存在する敵弾数を画面右下に表示
    if (isMuteki) {
        int bulletCount = 0;
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                ++bulletCount;
                pShot = pShot->next;
            }
            pSet = pSet->next;
        }
        // FPS表示の少し上 (y=436) に表示。FPSは通常 (565,460) 付近。
        DrawFormatString(485, 416, GetColor(255, 255, 0), "<Invincible Mode>");
        DrawFormatString(519, 436, GetColor(255, 255, 255), "Bullets: %d", bulletCount);
    }
}

// 勝利・敗北時の半透明オーバーレイとメッセージ（変更なし）
void drawGameOverlay()
{
    int overlayColor;
    if (StateManager::GetState() == Joutai::Win) {
        overlayColor = GetColor(30, 20, 10);
    }
    else {
        overlayColor = GetColor(30, 10, 10);
    }

    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 80);
    DrawBox(0, 0, 480, 480, overlayColor, TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    const char* message;
    int msgColor;
    if (StateManager::GetState() == Joutai::Win) {
        message = "You Win!!";
        msgColor = GetColor(255, 200, 50);
    }
    else {
        message = "You Lose...";
        msgColor = GetColor(255, 100, 100);
    }

    DrawBox(140, 200, 340, 300, GetColor(20, 20, 40), TRUE);
    DrawBox(140, 200, 340, 202, msgColor, TRUE);
    DrawString(240 - (int)strlen(message) * 8, 205, message, GetColor(255, 255, 255));

    if (!replayActive) DrawString(155, 230, "V   : Retry", GetColor(255, 255, 255));
    else               DrawString(155, 230, "R   : Replay", GetColor(255, 255, 255));
    DrawString(155, 250, "N   : Next Stage", GetColor(255, 255, 255));
    DrawString(155, 270, "Q   : Quit", GetColor(255, 255, 255));
}