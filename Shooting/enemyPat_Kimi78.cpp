// enemyPat_resonanceSpiral.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：共鳴螺旋（Resonance Spiral）
// プレイヤー周囲に衛星弾が出現し、プレイヤーの動きに引きずられながら回転。
// 一定時間後、螺旋を描いてプレイヤーへ収束する。
static void ShotResonanceSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 定数 ---
    const int    NUM_SHOTS = 24;        // 衛星弾の数
    const double BASE_DIST = 150.0;     // 初期軌道半径
    const double ROT_SPEED = DX_PI / 90.0; // 回転角速度（約2秒で1周）
    const int    APPEAR_DELAY = 30;        // 予兆後の出現遅延フレーム
    const int    CONVERGE_START = 330;       // 収束開始フレーム
    const double LERP_NORMAL = 0.03;      // 通常時の位置追従率（引きずりの強さ）
    const double LERP_CONVERGE = 0.03;      // 収束時の追従率

    // ===== 初期化フェーズ =====
    if (pEnemyShotSet->count == 0) {
        // 予告音：大技の前触れ
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ShotSet のパラメータ初期化
        pEnemyShotSet->param_i[0] = 0;      // [0] フェーズ 0:衛星 1:収束
        pEnemyShotSet->param_d[0] = ROT_SPEED; // [0] 現在の回転速度（加速させるため可変）
    }

    // ===== 弾出現フェーズ =====
    if (pEnemyShotSet->count == APPEAR_DELAY) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < NUM_SHOTS; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = (2.0 * DX_PI * i) / NUM_SHOTS;

            // 初期位置：プレイヤー周囲の正円上
            pEnemyShot->x = player.x + BASE_DIST * cos(angle);
            pEnemyShot->y = player.y + BASE_DIST * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0; // 直接座標を更新するため speed は不使用
            pEnemyShot->margin = 240;

            // 弾種：小玉（2.5x2.5）— 衛星弾に最適。数が多くても視認性が良い。
            // 初期色は青（遠距離＝安全）
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            // 弾ごとのパラメータ
            pEnemyShot->param_d[0] = angle;     // [0] 基準角度（回転の中心となる論理角度）
            pEnemyShot->param_d[1] = BASE_DIST; // [1] 基準距離（軌道半径）
            pEnemyShot->param_i[0] = i;         // [0] 弾インデックス

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾がまだ生成されていなければここで終了
    if (pEnemyShotSet->count < APPEAR_DELAY) {
        return;
    }

    // ===== フェーズ遷移 =====
    if (pEnemyShotSet->count == CONVERGE_START) {
        pEnemyShotSet->param_i[0] = 1; // 収束フェーズへ
    }

    int    phase = pEnemyShotSet->param_i[0];
    double rotSpeed = pEnemyShotSet->param_d[0];

    // 収束時は回転を加速（螺旋感を強調）
    if (phase == 1) {
        rotSpeed *= 1.008;
        if (rotSpeed > DX_PI / 30.0) rotSpeed = DX_PI / 30.0; // 上限
        pEnemyShotSet->param_d[0] = rotSpeed;
    }

    // ===== 各弾の更新 =====
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double baseAngle = pShot->param_d[0];
        double baseDist = pShot->param_d[1];

        // 基準角度を回転
        baseAngle += rotSpeed;

        // 収束フェーズ：軌道半径を縮小（螺旋を描いて中心へ）
        if (phase == 1) {
            baseDist *= 0.965;
            if (baseDist < 6.0) baseDist = 6.0;
        }

        // 目標位置（プレイヤー中心＋極座標オフセット）
        double targetX = player.x + baseDist * cos(baseAngle);
        double targetY = player.y + baseDist * sin(baseAngle);

        // 遅延追従（引きずり効果の核心）
        // プレイヤーが動くと targetX/Y が大きく変わるが、
        // 弾の実座標は LERP 率で追いつくため、プレイヤー移動方向と逆側に弾が「遅れ」、
        // 結果として円が楕円に歪む。
        double lerp = (phase == 1) ? LERP_CONVERGE : LERP_NORMAL;
        pShot->x += (targetX - pShot->x) * lerp;
        pShot->y += (targetY - pShot->y) * lerp;

        // 描画向きは移動方向に
        pShot->muki = atan2(targetY - pShot->y, targetX - pShot->x);

        // 距離に応じた色変化（視覚的な危険度表示）
        // 遠い=青(4) → マゼンタ(5) → 黄(1) → 近い=赤(0)
        int colorIdx;
        if (baseDist > 110.0) colorIdx = 4; // 青
        else if (baseDist > 75.0)  colorIdx = 5; // マゼンタ
        else if (baseDist > 40.0)  colorIdx = 1; // 黄
        else                       colorIdx = 0; // 赤

        pShot->kind = img_enemyShotSmallBall[colorIdx];

        // パラメータを保存して次フレームへ
        pShot->param_d[0] = baseAngle;
        pShot->param_d[1] = baseDist;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_TheMostFun_Kimi()
{
    static int shot_count = 0;

    if (count == 1) {
        // ゲーム画面 480x480 の中央上部
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }

    // 敵は微かに上下に揺れるだけ（大技中は動かない方が避けやすい）
    enemy.y = 60.0 + sin(count / 60.0) * 3.0;

    // 8秒間隔（480フレーム）で共鳴螺旋を発動
    if (count % 480 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotResonanceSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 本パターンでは不使用
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}