// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：交差結界「リサジューの無限迷路」
static void ShotLissajous(sEnemyShotSet* pEnemyShotSet)
{
    // 予告音と発射音の制御
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (pEnemyShotSet->count == 30) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 30フレームの予告時間後、弾の発射を開始
    if (pEnemyShotSet->count >= 30) {
        // 位相の変化（時間経過で徐々に変化させ、図形を回転・変形させる）
        double delta = (pEnemyShotSet->count - 30) * 0.012;

        // 弾の間隔を決定するパラメータ
        // 大きくするほど弾の間隔が広がり、小さくするほど密になる
        double base_speed = 0.3 * 0.5;

        // 2層重ねて網目を形成する
        for (int layer = 0; layer < 2; layer++) {
            // 各レイヤーごとのオフセット（位相をずらして交差させる）
            double t_offset = layer * (DX_PI / 3.0);

            // 現在のtを取得（gv.hの仕様により初回は0.0で初期化されている）
            double t = pEnemyShotSet->param_d[layer];

            // 接線ベクトルの計算
            double dx_dt = 3.0 * cos(3.0 * t + delta + t_offset);
            double dy_dt = 2.0 * cos(2.0 * t + t_offset);

            // 接線ベクトルの大きさを計算
            double v = sqrt(dx_dt * dx_dt + dy_dt * dy_dt);

            // ゼロ割り防止（理論上は0にならないが安全対策）
            if (v < 0.01) v = 0.01;

            // 弧長が等速になるようにtを進める
            // これにより、画面上の弾の間隔が均等になり美しい曲線が描かれる
            t += base_speed / v;
            pEnemyShotSet->param_d[layer] = t;

            // ターゲット座標の計算（画面に合わせてスケーリング）
            double scale = 200.0;
            double targetX = pEnemyShotSet->x + scale * sin(3.0 * t + delta + t_offset);
            double targetY = pEnemyShotSet->y + scale * sin(2.0 * t + t_offset);

            // 弾の生成
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 敵の位置からターゲット座標に向かう角度を設定
            pEnemyShot->muki = atan2(targetY - pEnemyShot->y, targetX - pEnemyShot->x);
            pEnemyShot->speed = hypot(targetX - pEnemyShot->x, targetY - pEnemyShot->y) * 0.02;

            // 種類と色の設定（赤とシアンの小玉を使用し、視認性と美しさを確保）
            if (layer == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン
            }

            // 双方向リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->speed *= 0.995;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Lissajous_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        // 弾幕の中心を安定させるため、画面上部中央に配置
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 弾幕発射準備ができるまでは少しだけ移動（開始後は固定してシンメトリーを維持）
        if (count < 60) {
            enemy.x += 0.98 * (double)muki;
            if (count % 120 == 60) muki *= -1;
        }
    }

    // 60フレーム経過時に弾幕セットを生成（1回だけ生成し、後は関数内で継続発射）
    if (count == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLissajous;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        // param_d[0], [1] は gv.h の仕様により 0.0 で自動初期化されるため、ここでは何もしない

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}