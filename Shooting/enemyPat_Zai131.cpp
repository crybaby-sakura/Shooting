#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：泡沫整列（バブル・アレンジ）
//  バブルソートをモチーフにした弾幕。
//  ・大きさ(=値)の異なる弾が横一列に並ぶ
//  ・隣り合う2発を比較し、大小関係が逆なら半円弧を描いて入れ替わる
//    (左の弾は上の弧、右の弾は下の弧 → 下の弧が自機側へ来るのが脅威)
//  ・ソートが完了すると整列した弾列から大弾(泡)が降下し、
//    弾列全体もゆっくり降下して押し込んでくる
//  ・その後値をシャッフルして再度ソート開始(ループ毎に高速化)
//  ※画面外に出た弾の消去はメインルーチン側で行う
// ============================================================

#define BUBBLE_N          8      // 整列させる弾の数
#define BUBBLE_SPACING  60.0     // スロット間隔(px)
#define BUBBLE_ROW_Y   130.0     // 弾列の基準Y座標
#define BUBBLE_ARC_R    45.0     // 入れ替わり時の弧の高さ(px)
#define BUBBLE_VALUE_MAX  5      // 弾の「値」の最大値(0～5の6段階)

// ===== 弾(sEnemyShot)側の param_i / param_d の用途 =========
//  param_i[0] : 弾の「値」(大きさ) 0～5
//  param_i[1] : 現在のスロット番号(0～7)
//  param_i[2] : 入れ替わりアニメ中フラグ (0:静止中 1:アニメ中)
//  param_i[3] : 移動方向 (+1:右へ=上の弧 / -1:左へ=下の弧)
//  param_i[4] : 弾列に属する弾か (1:弾列 0:飛沫・大泡などの飛び弾)
//  param_d[0] : アニメ進行度 t (0.0～1.0)
//  param_d[1] : 入れ替わり前のスロット番号(弧の中心計算用)

// ===== セット(sEnemyShotSet)側の param の用途 ==============
//  param_i[0] : 状態 (0:ソート中 1:整列完了・大泡降下 2:シャッフル待ち)
//  param_i[1] : 比較中のペアの左側スロット番号
//  param_i[2] : ステップ用タイマー
//  param_i[3] : 現在のパスでの交換回数(0ならソート完了)
//  param_i[4] : ループ回数(シャッフル回数・難易度に使用)
//  param_d[0] : 弾列の降下オフセット

// スロット番号 → X座標
static double BubbleSlotX(double slot)
{
    return (slot + 0.5) * BUBBLE_SPACING;
}

// 弾の「値」 → 弾画像(値が大きいほど大きな弾・目立つ色になる)
static int BubbleValueToKind(int value)
{
    // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)
    // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
    switch (value) {
    case 0: return img_enemyShotMediumBall[6];  // 白
    case 1: return img_enemyShotMediumBall[3];  // シアン
    case 2: return img_enemyShotMediumBall[1];  // 黄
    case 3: return img_enemyShotLargeBall[0];   // 赤
    case 4: return img_enemyShotLargeBall[4];   // 青
    default: return img_enemyShotLargeBall[5];  // マゼンタ
    }
}

// 指定スロットにいる「弾列の弾」を探す(見つからなければnullptr)
static sEnemyShot* BubbleFindShotAtSlot(sEnemyShotSet* pSet, int slot)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[4] == 1 && pShot->param_i[1] == slot) return pShot;
        pShot = pShot->next;
    }
    return nullptr;
}

// セットに弾を追加する共通処理(リストの末尾へ)
static void BubbleAddShot(sEnemyShotSet* pSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;
}

// 飛び弾(弾列に属さない弾)を生成
static void BubbleSpawnFreeShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    // param_i は全て0で初期化済み → param_i[4]==0 で飛び弾扱い
    BubbleAddShot(pSet, pEnemyShot);
}

// 交換アニメの開始(論理スロットは即時入れ替え、見た目は弧で追従)
static void BubbleStartSwap(sEnemyShot* pShot, int fromSlot, int toSlot)
{
    pShot->param_i[2] = 1;                          // アニメ中
    pShot->param_i[1] = toSlot;                     // スロットを先に交換
    pShot->param_i[3] = (toSlot > fromSlot) ? 1 : -1;
    pShot->param_d[0] = 0.0;                        // 進行度リセット
    pShot->param_d[1] = fromSlot;                   // 前スロット記録
}

// ============================================================
// 弾幕パターン本体
// ============================================================
static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium,
        //                   sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 値がランダムの弾を8発、横一列に生成
        // GetRand(x) は 0～x の x+1 種類を返す → GetRand(5) で 0～5
        for (int i = 0; i < BUBBLE_N; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->param_i[0] = GetRand(BUBBLE_VALUE_MAX); // 値
            pEnemyShot->param_i[1] = i;                          // 初期スロット
            pEnemyShot->param_i[4] = 1;                          // 弾列の弾
            pEnemyShot->kind = BubbleValueToKind(pEnemyShot->param_i[0]);
            pEnemyShot->x = BubbleSlotX(i);
            pEnemyShot->y = BUBBLE_ROW_Y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            BubbleAddShot(pEnemyShotSet, pEnemyShot);
        }
        // セットの状態初期化
        pEnemyShotSet->param_i[0] = 0;   // state = ソート中
        pEnemyShotSet->param_i[1] = 0;   // 比較位置
        pEnemyShotSet->param_i[2] = 0;   // タイマー
        pEnemyShotSet->param_i[3] = 0;   // パス内交換回数
        pEnemyShotSet->param_i[4] = 0;   // ループ回数
        pEnemyShotSet->param_d[0] = 0.0; // 降下オフセット
        return;
    }

    int    state = pEnemyShotSet->param_i[0];
    int    loop = pEnemyShotSet->param_i[4];
    // ループが進むほど入れ替わりが速くなる
    int    animFrames = (loop >= 4) ? 24 : 40 - loop * 4;

    // ---------------- 状態遷移 ----------------
    if (state == 0) {
        // ---- ソート中: 左から順に隣接ペアを比較していく ----
        if (pEnemyShotSet->param_i[2] <= 0) {
            int i = pEnemyShotSet->param_i[1];
            sEnemyShot* pA = BubbleFindShotAtSlot(pEnemyShotSet, i);
            sEnemyShot* pB = BubbleFindShotAtSlot(pEnemyShotSet, i + 1);
            if (pA != nullptr && pB != nullptr && pA->param_i[0] > pB->param_i[0]) {
                // 大小関係が逆 → 交換アニメ開始
                // 左の弾は右へ(上の弧)、右の弾は左へ(下の弧=自機側へ)
                BubbleStartSwap(pA, i, i + 1);
                BubbleStartSwap(pB, i + 1, i);
                pEnemyShotSet->param_i[3]++;             // 交換回数
                pEnemyShotSet->param_i[2] = animFrames;  // アニメ時間だけ次へ
            }
            else {
                pEnemyShotSet->param_i[2] = 12;          // 比較のみなら短く
            }
            pEnemyShotSet->param_i[1]++;
            if (pEnemyShotSet->param_i[1] > BUBBLE_N - 2) {
                // 1パス終了 → 先頭へ戻る
                pEnemyShotSet->param_i[1] = 0;
                if (pEnemyShotSet->param_i[3] == 0) {
                    // 交換ゼロ = ソート完了 → 整列弾から大泡が降下
                    pEnemyShotSet->param_i[0] = 1;       // state = 整列完了
                    pEnemyShotSet->param_i[2] = 90;
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    double rowY = BUBBLE_ROW_Y + pEnemyShotSet->param_d[0];
                    // 中央に大玉、両脇に中玉(バブルソートで最大値が先に浮かぶイメージ)
                    BubbleSpawnFreeShot(pEnemyShotSet, 240.0, rowY, DX_PI / 2.0,
                        1.1 + 0.2 * loop, img_enemyShotLargeBall[6]);
                    BubbleSpawnFreeShot(pEnemyShotSet, 190.0, rowY, DX_PI / 2.0,
                        2.0 + 0.2 * loop, img_enemyShotMediumBall[3]);
                    BubbleSpawnFreeShot(pEnemyShotSet, 290.0, rowY, DX_PI / 2.0,
                        2.0 + 0.2 * loop, img_enemyShotMediumBall[3]);
                }
                pEnemyShotSet->param_i[3] = 0;           // パスの交換回数リセット
            }
        }
        else {
            pEnemyShotSet->param_i[2]--;
        }
    }
    else if (state == 1) {
        // ---- 整列完了・大泡降下中 ----
        pEnemyShotSet->param_i[2]--;
        if (pEnemyShotSet->param_i[2] <= 0) {
            // シャッフル予告
            pEnemyShotSet->param_i[0] = 2;
            pEnemyShotSet->param_i[2] = 40;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // ---- シャッフル待ち → 値をシャッフルして再ソート ----
        pEnemyShotSet->param_i[2]--;
        if (pEnemyShotSet->param_i[2] <= 0) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[4] == 1 && pShot->param_i[2] == 0) {
                    pShot->param_i[0] = GetRand(BUBBLE_VALUE_MAX); // 新しい値
                    pShot->kind = BubbleValueToKind(pShot->param_i[0]);
                }
                pShot = pShot->next;
            }
            pEnemyShotSet->param_i[4]++;     // loop++ (難易度上昇)
            pEnemyShotSet->param_i[0] = 0;   // state = ソート中
            pEnemyShotSet->param_i[1] = 0;
            pEnemyShotSet->param_i[2] = 0;
            pEnemyShotSet->param_i[3] = 0;
            pEnemyShotSet->param_d[0] = 0.0; // 弾列を上段へ戻す
        }
    }

    // ソート中以外の間は弾列全体がゆっくり降下して自機を押し込む
    if (pEnemyShotSet->param_i[0] != 0) {
        pEnemyShotSet->param_d[0] += 0.4;
    }
    double rowY = BUBBLE_ROW_Y + pEnemyShotSet->param_d[0];

    // ---------------- 弾の移動・アニメ更新 ----------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[4] == 1) {
            // 弾列の弾
            if (pShot->param_i[2] == 1) {
                // 入れ替わりアニメ(半円弧を描いてすれ違う)
                pShot->param_d[0] += 1.0 / animFrames;
                if (pShot->param_d[0] >= 1.0) {
                    // アニメ完了 → スロット位置に着地
                    pShot->param_d[0] = 1.0;
                    pShot->param_i[2] = 0;
                    pShot->x = BubbleSlotX(pShot->param_i[1]);
                    pShot->y = rowY;
                    // 泡がはじけて飛沫(小弾)が下へ散る
                    for (int k = 0; k < 2*10; k++) {
                        double muki = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI;
                        double speed = 2.5 + GetRand(100) / 100.0;
                        int kind = (GetRand(1) == 1) ? img_enemyShotSmallBall[3]  // シアン
                            : img_enemyShotSmallBall[6]; // 白
                        BubbleSpawnFreeShot(pEnemyShotSet, pShot->x, pShot->y, muki, speed, kind);
                    }
                }
                else {
                    double from = pShot->param_d[1];
                    double to = pShot->param_i[1];
                    double cx = BubbleSlotX((from + to) / 2.0);
                    double half = BUBBLE_SPACING / 2.0;
                    double th = DX_PI * pShot->param_d[0];
                    if (to > from) {
                        // 左→右へ移動: 上の弧
                        pShot->x = cx - half * cos(th);
                        pShot->y = rowY - BUBBLE_ARC_R * sin(th);
                    }
                    else {
                        // 右→左へ移動: 下の弧(自機側へ食い込む)
                        pShot->x = cx + half * cos(th);
                        pShot->y = rowY + BUBBLE_ARC_R * sin(th);
                    }
                }
            }
            else {
                // 静止中の弾列 → スロット位置と降下に追従
                pShot->x = BubbleSlotX(pShot->param_i[1]);
                pShot->y = rowY;
            }
        }
        else {
            // 飛び弾(飛沫・大泡)は等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_BubbleSort_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // 弾幕セットを1つだけ生成(以後、内部でシャッフル⇔ソートを繰り返す)
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleSort;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 40.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // ゆったり左右に揺れる
        enemy.x = 240.0 + 110.0 * sin((count - 1) * 0.008);
        enemy.y = 40.0 + 8.0 * sin((count - 1) * 0.03);
    }
}