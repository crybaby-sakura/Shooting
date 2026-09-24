// enemyPat_Tmp.cpp
// 輪投げモチーフ弾幕：リング状に配置した弾が拡大しながらプレイヤー方向へ飛来する

#include "gv.h"  // 必要に応じてインクルードパスを調整してください

// 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
// 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
// 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙

// ------------------------------------------------------------
// 輪投げリング弾のパターン関数
// 中玉（または小玉）を円周上に配置し、共通の進行方向 + 半径方向への拡大速度を与えることで
// 「投げられた輪が膨らみながら飛んでくる」表現を実現
// ------------------------------------------------------------
static void ShotRingToss(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (pEnemyShotSet->param_i[0]) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // リングを構成する弾の数（10〜14個で輪らしさを出す）
        const int num = 10 + GetRand(4) + (pEnemyShotSet->param_i[0] ? 50 : 0);

        // 進行方向（スポーン時点のプレイヤー方向）
        const double base_muki = pEnemyShotSet->muki;

        // 基本速度と拡大速度
        const double base_speed = 1.8 + GetRand(60) / 100.0;   // 1.8〜2.4
        const double expand_speed = 0.45 + GetRand(30) / 100.0;  // 0.45〜0.75

        // 初期半径（小さめから始めて徐々に膨らむ）
        const double init_radius = 6.0 + GetRand(40) / 10.0;     // 6.0〜10.0

        // 色（kind でバリエーション。シアン・青・マゼンタ・橙などを優先）
        const int color_table[] = { 3, 4, 5, 8, 1, 0 }; // シアン, 青, マゼンタ, 橙, 黄, 赤
        const int color = color_table[pEnemyShotSet->kind % 6];

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;

            // 円周上の初期位置
            const double ang = 2.0 * DX_PI * i / num;
            pEnemyShot->x = pEnemyShotSet->x + init_radius * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + init_radius * sin(ang);

            // 速度ベクトル = 進行方向成分 + 半径方向への拡大成分
            const double vx = base_speed * cos(base_muki) + expand_speed * cos(ang);
            const double vy = base_speed * sin(base_muki) + expand_speed * sin(ang);

            pEnemyShot->muki = atan2(vy, vx);
            pEnemyShot->speed = sqrt(vx * vx + vy * vy);

            // 中玉を使用（輪の見た目がはっきりする）。色は上で決定
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // 必要に応じてパラメータを保存（今回は固定速度なので未使用だが拡張用に）
            pEnemyShot->param_d[0] = ang;           // リング内角度
            pEnemyShot->param_d[1] = expand_speed;  // 拡大速度
            pEnemyShot->param_d[2] = base_muki;     // 基本進行方向
            pEnemyShot->param_d[3] = base_speed;    // 基本速度

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：通常の移動のみ（速度は発射時に固定済みなので剛体的に拡大しながら平行移動する）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// 画面上部を左右にゆっくり移動しながら、一定間隔で輪投げリングを投げる
// ------------------------------------------------------------
void EnemyPat_RingToss_Grok()
{
    static int muki;          // 移動方向（+1 or -1）
    static int shot_count;    // ショットセットの種類カウンタ（色変化用）

    if (count == 1) {
        // 初期位置：画面上部中央
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0)  muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    // 一定間隔でリングを投げる（約40フレームごと）
    // たまに少し早めに連続で投げる感じを出すために % で調整
    if (count > 30 && count % 40 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRingToss;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // プレイヤー方向を狙う（少しランダムで揺らぎを加えて輪投げ感を出す）
        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->muki = aim + (GetRand(40) - 20) / 180.0 * DX_PI;  // ±約11度程度の揺らぎ
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットをリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // たまに予告音付きの大きめリングを追加で投げる（演出用）
    if (count > 90 && count % 180 == 60) {
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRingToss;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++ + 3;  // 色を少しずらす
        pEnemyShotSet->param_i[0] = 1;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}