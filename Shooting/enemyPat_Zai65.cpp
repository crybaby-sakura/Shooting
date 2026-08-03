// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：毒蛇の徘徊する吐息
static void ShotHydraPoison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 重い吐息のイメージで効果音を再生
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ヒュドラの頭の数
        int headCount = 5;
        double baseMuki = pEnemyShotSet->muki;

        // 頭ごとの角度オフセット（プレイヤー方向を中心に少し扇状に広げる）
        double angleOffsets[5] = { -0.4, -0.2, 0.0, 0.2, 0.4 };

        for (int i = 0; i < headCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double shotMuki = baseMuki + angleOffsets[i];
            pEnemyShot->muki = shotMuki;
            pEnemyShot->speed = 2.5;

            // 緑色(2)の中楕円弾を使用（向きが分かりやすいため蛇行弾に最適）
            pEnemyShot->kind = img_enemyShotMediumOval[2];

            // メイン弾であることを示すフラグとして 1 を設定
            pEnemyShot->param_i[0] = 1;
            // 蛇行の位相オフセット（頭ごとに波をずらして連続したうねりを作る）
            pEnemyShot->param_i[1] = i * 20;

            // 蛇行の基準となる初期位置を保存
            pEnemyShot->param_d[2] = pEnemyShotSet->x;
            pEnemyShot->param_d[3] = pEnemyShotSet->y;

            // 進行方向の単位ベクトルを保存
            pEnemyShot->param_d[0] = cos(shotMuki);
            pEnemyShot->param_d[1] = sin(shotMuki);

            // 蛇行の振幅(40.0)分、画面外に一時的にはみ出ても消されないようマージンを拡張
            pEnemyShot->margin = 60.0;

            // 弾リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // === メイン弾の移動（サインカーブによる蛇行） ===
            double dist = pShot->speed * pShot->count;
            double amplitude = 40.0; // 蛇行の振幅
            double frequency = 0.08; // 蛇行の周期
            double offset = amplitude * sin((pShot->count * frequency) + pShot->param_i[1]);

            // 進行方向に対する垂直（法線）ベクトル
            double nx = -pShot->param_d[1];
            double ny = pShot->param_d[0];

            // 座標の再計算（初期位置 ＋ 進行方向の移動 ＋ 法線方向の振動）
            pShot->x = pShot->param_d[2] + pShot->param_d[0] * dist + nx * offset;
            pShot->y = pShot->param_d[3] + pShot->param_d[1] * dist + ny * offset;

            // === 残滓（毒の沼）の生成 ===
            // 6フレームごとに軌跡上に残滓弾を生成
            if (pShot->count > 0 && pShot->count % 6 == 0) {
                sEnemyShot* pTrace = new sEnemyShot;
                pTrace->x = pShot->x;
                pTrace->y = pShot->y;
                pTrace->speed = 0.0;
                pTrace->muki = 0.0;

                // 毒の沼をイメージして緑色(2)の小玉を使用
                pTrace->kind = img_enemyShotSmallBall[2];

                // 残滓弾フラグ（param_iは0で初期化されるため明示的な設定は不要）
                // 残滓の寿命（フレーム数）を double 配列に保存
                pTrace->param_d[4] = 180.0;

                // メイン弾の直前にリスト挿入（同フレーム内で後から処理させるため）
                pTrace->prev = pShot->prev;
                pTrace->next = pShot;
                pShot->prev->next = pTrace;
                pShot->prev = pTrace;
            }
        }
        else {
            // === 残滓弾の寿命管理 ===
            pShot->param_d[4] -= 1.0;
            if (pShot->param_d[4] <= 0.0) {
                // 寿命が尽きたら画面外へ飛ばし、メインルーチンの仕様で自動消去させる
                pShot->y += 2000.0;
            }
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hydra_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくりと左右に移動するヒュドラ
        enemy.x += 0.5 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 180フレームごとに弾幕を発射
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHydraPoison;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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