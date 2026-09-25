#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// このファイル内でのみ参照する「時間停止中かどうか」フラグ。
// EnemyPat_TimeStop_Claude() が状態遷移のたびに更新し、ShotFreezeField() が
// 弾の移動処理をスキップするかどうかの判定に使う。
static bool s_timeStopped = false;

// 弾幕：時間停止フィールド
// ボスが動きながら位置にリング状の弾を撒いていく。
// s_timeStopped が true の間、既存弾の移動処理を一切行わない
// （＝時間停止解除の瞬間に、溜まった弾が一斉に動き出す）。
static void ShotFreezeField(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // kind を奇偶で分けて2種類のリングを交互に撒き、見た目に変化をつける
        bool altPattern = (pEnemyShotSet->kind % 2) == 1;
        int ringCount = altPattern ? 12+6 : 16+8;

        // バースト回数が進むごとに開始角をずらし、螺旋状に見えるようにする
        double baseAngle = pEnemyShotSet->kind * (DX_PI / 9.0);

        for (int i = 0; i < ringCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double angle = baseAngle + i * (2.0 * DX_PI / ringCount);
            double speed = altPattern ? (1.2 + (i % 3) * 0.25) : (1.6 + (i % 4) * 0.2);

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;   // 放射状に飛ぶので見た目の向きも進行方向に合わせる
            pEnemyShot->speed = speed;

            // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
            // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
            // 時間停止のイメージに合わせ、氷結を思わせるシアン/青/白を基調に、リングごとにマゼンタで変化をつける
            if (altPattern) {
                pEnemyShot->kind = img_enemyShotDiamond[5]; // マゼンタの菱形弾
            }
            else {
                int colorIdx = (i % 3 == 0) ? 6 : (i % 2 == 0 ? 3 : 4); // 白／シアン／青を混在
                pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 時間停止状態では敵弾の移動処理を一切行わない
    if (!s_timeStopped) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン：時間停止能力を使うボス
//
// 周期構成（1周期 = 7秒 / 420フレーム、60fps想定）:
//   [0]                予告音が鳴る
//   [0,60)              予告〜時間停止まで1秒の通常状態
//   [60,240)            時間停止状態（3秒間）
//   [240,420)           通常状態（3秒間）
// 以後この周期を繰り返す。
void EnemyPat_TimeStop_Claude()
{
    static int    muki;            // 通常時、左右パトロールの向き
    static int    burstIndex;      // 時間停止中に撒いた弾幕の通し番号（リングの回転・色分けに使用）
    static double frozenX, frozenY; // 時間停止中、自機を固定しておく座標

    const int FRAME_WARN = 60;  // 予告音から時間停止までの1秒
    const int FRAME_STOP = 180; // 時間停止状態の3秒
    const int FRAME_NORMAL = 180; // 通常状態の3秒
    const int FRAME_CYCLE = FRAME_WARN + FRAME_STOP + FRAME_NORMAL;
    const int BURST_INTERVAL = 5;   // 時間停止中、何フレームおきに弾幕リングを撒くか

    int localFrame = (count - 1) % FRAME_CYCLE;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        burstIndex = 0;
        s_timeStopped = false;
    }

    // --- 状態遷移の検出 ---
    if (localFrame == 0) {
        // 予告音（時間停止の前触れ）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (localFrame == FRAME_WARN) {
        // 時間停止状態へ移行。この瞬間の自機位置に固定する
        s_timeStopped = true;
        frozenX = player.x;
        frozenY = player.y;
        burstIndex = 0;
    }
    else if (localFrame == FRAME_WARN + FRAME_STOP) {
        // 時間停止解除。以後、溜まった弾が一斉に動き出す
        s_timeStopped = false;
    }

    // --- ボスの移動 ---
    if (s_timeStopped) {
        // 時間停止中はフィールドを縫うように動き回りながら弾を撒く（8の字軌道）
        double t = (double)(localFrame - FRAME_WARN) / (double)FRAME_STOP; // 0.0〜1.0
        double angle = 2.0 * DX_PI * t;
        enemy.x = 240.0 + 130.0 * cos(angle);
        enemy.y = 140.0 + 70.0 * sin(angle * 2.0);
    }
    else {
        // 通常時は左右にゆっくり往復
        enemy.x += 0.9 * (double)muki;
        if (enemy.x < 60.0 || enemy.x > 420.0) muki *= -1;
    }

    // --- 時間停止中の自機拘束処理 ---
    if (s_timeStopped) {
        // 自機の位置は毎フレーム強制的に、停止直前の位置へ戻す
        player.x = frozenX;
        player.y = frozenY;

        // 自機ショットのy座標を毎フレーム+15.0し、メインルーチンでの-15.0を打ち消す
        sPlayerShot* pPlayerShot = playerShotHead.next;
        while (pPlayerShot != &playerShotHead) {
            pPlayerShot->y += 15.0;
            pPlayerShot = pPlayerShot->next;
        }
    }

    // --- 弾幕生成（時間停止中のみ、一定間隔でリング状に撒く）---
    if (s_timeStopped && (localFrame - FRAME_WARN) % BURST_INTERVAL == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFreezeField;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0; // このパターンでは未使用
        pEnemyShotSet->kind = burstIndex++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}