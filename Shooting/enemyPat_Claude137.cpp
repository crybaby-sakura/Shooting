// 弾幕：残響の輪舞（エコー・ロンド）
// 制限時間180秒の耐久型スペルカード。乱数は一切使わず、毎回まったく同じ弾幕が来る。
//   A層: 回転する螺旋弾幕（7.3秒ごとに回転方向が反転）… 青系
//   B層: 自機が過去にいた場所に「種」が現れ、0.4秒後にリング弾を放つ … 赤・橙系（終盤は桃・黄）
//   C層: 2.9秒周期の自機狙い扇状弾 … 緑
// 60FPS前提。時間の単位はすべてフレーム。
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  調整用の定数
// ============================================================
static const double kTwoPi = DX_PI * 2.0;

// 全体
static const int  kEchoTotalFrames = 30 * 60;   // 制限時間（180秒）
static const bool kEchoSurvivalMode = true;      // true: 敵HPを経過時間で減らし、自機弾ではダメージを受けない（耐久型）

// A層：螺旋
static const int    kSpiralArms = 12;                     // 腕の本数
static const int    kSpiralInterval = 4;                  // 何フレームごとに発射するか
static const double kSpiralSpeed = 1.8;                   // 弾速
static const double kSpiralOmega = 2.0 / 180.0 * DX_PI;   // 1フレームあたりの回転角（rad）
static const int    kSpiralFlipFrames = 438;              // 回転方向の反転周期（7.3秒）
static const int    kSpiralWarnFrames = 30;               // 反転の何フレーム前に予告音を鳴らすか
// 安全レーン幅（腕の間の隙間）は距離によらずほぼ一定で、
//   レーン幅 ≒ 2π × 弾速 ÷ (腕数 × 角速度[rad/frame])   ≒ 27px（この設定の場合）
// 狭くしたいときは 腕数 か 角速度 を上げる。

// B層：自機の過去をなぞる遅延弾
static const int    kEchoSpawnInterval = 30;              // 種の出現間隔（0.5秒）
static const int    kEchoDelayStart = 180;                // 遅延（3.0秒）
static const int    kEchoDelayEnd = 60;                   // 遅延の最終値（1.0秒）
static const int    kEchoShrinkStart = 120 * 60;          // 遅延の短縮を始める時刻（120秒）
static const int    kEchoSeedFrames = 24;                 // 種が出てからリングを放つまで（0.4秒）
static const int    kEchoLateStart = kEchoTotalFrames - 30 * 60; // 終盤（最後の30秒）
static const int    kEchoDirsNormal = 8;                  // リングの方向数（通常）
static const int    kEchoDirsLate = 16;                   // リングの方向数（終盤）
static const double kEchoRingSpeed[2] = { 1.7, 2.6 };     // 内側リング・外側リングの弾速

// C層：追加ばらまき（自機狙い扇）
static const int    kFanInterval = 174;                   // 2.9秒周期
static const int    kFanWays = 21;                        // 1層あたりの弾数
static const int    kFanLayers = 4;                       // 速度違いの層数
static const double kFanSpread = 100.0 / 180.0 * DX_PI;   // 扇の全開き角
static const double kFanSpeedBase = 1.5;
static const double kFanSpeedStep = 0.4;

// 自機の位置履歴（リングバッファ）。最大遅延180フレームより大きい2のべき乗にしておく
static const int kHistSize = 256;
static const int kHistMask = kHistSize - 1;
static double s_histX[kHistSize];
static double s_histY[kHistSize];

// ============================================================
//  共通ヘルパー
// ============================================================
// 効果音を頭から鳴らし直す
static void PlaySeRestart(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾セットを生成して敵弾セットのリストの末尾につなぐ
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// 弾を1発生成して弾セットの弾リストの末尾につなぐ
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 弾セット内の全弾を muki 方向に speed だけ進める（画面外の消去はメインルーチンが行う）
static void MoveShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  A層：螺旋弾幕
//  pEnemyShotSet->muki を基準角として、腕の本数ぶんの弾を等間隔で放射状に出す。
//  基準角を毎回少しずつ回すことで螺旋の腕になる。
// ============================================================
static void ShotSpiral(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < kSpiralArms; i++) {
            double muki = pEnemyShotSet->muki + kTwoPi * i / kSpiralArms;
            // 腕ごとにシアンと青を交互に
            int color = (i % 2 == 0) ? 3 : 4;
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, kSpiralSpeed, img_enemyShotSmallBall[color]);
        }
    }
    MoveShots(pEnemyShotSet);
}

// ============================================================
//  B層：残響リング
//  過去の自機位置に白い小玉（種）が重なって出現 → kEchoSeedFrames 後に一斉にリング状へ飛び出す。
//  param_i[0]: リングの方向数、param_i[1]: 終盤フラグ（色替え用）
//  弾の param_i[0]: 0=内側リング、1=外側リング
// ============================================================
static void ShotEcho(sEnemyShotSet* pEnemyShotSet)
{
    const int dirs = pEnemyShotSet->param_i[0];
    const int late = pEnemyShotSet->param_i[1];

    if (pEnemyShotSet->count == 0) {
        // 種：速度0で同じ座標に全弾を重ねておく。muki は飛び出す方向を先に入れておく
        for (int layer = 0; layer < 2; layer++) {
            for (int i = 0; i < dirs; i++) {
                double muki = pEnemyShotSet->muki + kTwoPi * (i + 0.5 * layer) / dirs;
                sEnemyShot* pShot = AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, 0.0, img_enemyShotSmallBall[6]);
                pShot->param_i[0] = layer;
            }
        }
    }
    if (pEnemyShotSet->count == kEchoSeedFrames) {
        // 種が割れてリングが飛び出す
        PlaySeRestart(sound_enemyShot_light);
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int layer = pShot->param_i[0];
            int color;
            if (late) color = (layer == 0) ? 5 : 1;   // 終盤: マゼンタ / 黄
            else      color = (layer == 0) ? 0 : 8;   // 通常: 赤 / 橙
            pShot->kind = img_enemyShotDiamond[color];
            pShot->speed = kEchoRingSpeed[layer];
            pShot = pShot->next;
        }
    }
    MoveShots(pEnemyShotSet);
}

// ============================================================
//  C層：自機狙いの扇状ばらまき（乱数なし）
//  pEnemyShotSet->muki: 扇の中心方向（生成時に自機へ向ける）
// ============================================================
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySeRestart(sound_enemyShot_medium);
        const double step = kFanSpread / (kFanWays - 1);
        for (int layer = 0; layer < kFanLayers; layer++) {
            for (int i = 0; i < kFanWays; i++) {
                // 奇数層は半歩ずらして隙間を埋める
                double muki = pEnemyShotSet->muki + (i - (kFanWays - 1) / 2.0) * step + (layer % 2) * step * 0.5;
                double speed = kFanSpeedBase + kFanSpeedStep * layer;
                AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, speed, img_enemyShotMediumBall[2]);
            }
        }
    }
    MoveShots(pEnemyShotSet);
}

// ============================================================
//  遅延フレーム数（120秒以降は3.0秒→1.0秒へ線形に短縮）
// ============================================================
static int EchoDelayFrames(int c)
{
    if (c <= kEchoShrinkStart) return kEchoDelayStart;
    if (c >= kEchoTotalFrames) return kEchoDelayEnd;
    return kEchoDelayStart + (kEchoDelayEnd - kEchoDelayStart) * (c - kEchoShrinkStart) / (kEchoTotalFrames - kEchoShrinkStart);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_TenYears_Claude()
{
    static double spiralAngle; // A層の現在の基準角

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200;
        spiralAngle = 0.0;
        for (int i = 0; i < kHistSize; i++) {
            s_histX[i] = player.x;
            s_histY[i] = player.y;
        }
    }

    // 制限時間まで耐えきったらHPを0にして終了（新しい弾は出さない）
    if (count >= kEchoTotalFrames) {
        enemy.hp = 0;
        return;
    }
    // 耐久型: 経過時間に比例してHPが減る（自機弾のダメージは上書きされる）
    if (kEchoSurvivalMode) {
        enemy.hp = enemy.maxHp - enemy.maxHp * count / kEchoTotalFrames;
    }

    // 自機の位置を履歴に記録
    s_histX[count & kHistMask] = player.x;
    s_histY[count & kHistMask] = player.y;

    // ---- A層：螺旋 ----
    {
        // 回転方向は kSpiralFlipFrames ごとに反転
        int phase = (count - 1) / kSpiralFlipFrames;
        double dir = (phase % 2 == 0) ? 1.0 : -1.0;
        spiralAngle += dir * kSpiralOmega;

        // 反転の少し前に予告音
        if ((count + kSpiralWarnFrames) % kSpiralFlipFrames == 1) {
            PlaySeRestart(sound_enemyCharge);
        }
        if (count % kSpiralInterval == 1) {
            CreateShotSet(enemy.x, enemy.y, spiralAngle, ShotSpiral);
        }
    }

    // ---- B層：残響リング ----
    {
        int delay = EchoDelayFrames(count);
        // 履歴が十分に溜まってから（開始から delay フレーム後以降）出す
        if (count % kEchoSpawnInterval == 0 && count > delay) {
            int idx = (count - delay) & kHistMask;
            bool late = (count >= kEchoLateStart);
            // リングの向きは時刻に応じて決まった量だけ回す（乱数なし）
            sEnemyShotSet* pSet = CreateShotSet(s_histX[idx], s_histY[idx], count * 0.1, ShotEcho);
            pSet->param_i[0] = late ? kEchoDirsLate : kEchoDirsNormal;
            pSet->param_i[1] = late ? 1 : 0;
        }
    }

    // ---- C層：自機狙いの扇 ----
    if (count % kFanInterval == 0) {
        double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        CreateShotSet(enemy.x, enemy.y, aim, ShotFan);
    }
}