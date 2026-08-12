// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ペンタグラム・レゾナンス（五芒の共鳴）
static void ShotPentagram(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;
    double ex = pEnemyShotSet->x;
    double ey = pEnemyShotSet->y;
    double R = 190.0; // 五芒星の半径

    // 5つの頂点座標を計算
    double Px[5], Py[5];
    for (int i = 0; i < 5; i++) {
        // -DX_PI/2 (真上) を基準に、72度 (2π/5) ずつずらして頂点を配置
        Px[i] = ex + R * cos(-DX_PI / 2.0 + i * 2.0 * DX_PI / 5.0);
        Py[i] = ey + R * sin(-DX_PI / 2.0 + i * 2.0 * DX_PI / 5.0);
    }

    // --- 【フェーズ1】頂点となる大玉を5つ発射 ---
    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = ex;
            p->y = ey;
            p->muki = atan2(Py[i] - ey, Px[i] - ex);
            p->speed = R / 30.0; // 30フレームで頂点に到達する速度
            p->kind = img_enemyShotLargeBall[0]; // 赤い大玉
            p->param_i[0] = 0; // 役割0: 頂点大玉
            p->param_i[1] = i; // 頂点のインデックスを記憶

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // --- 【フェーズ2】大玉が頂点に到達して停止 ---
    if (c == 30) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) { // 大玉なら
                p->speed = 0.0;
                int i = p->param_i[1];
                p->x = Px[i]; // 座標のズレをピッタリ補正
                p->y = Py[i];
            }
            p = p->next;
        }
    }

    // --- 【フェーズ3】五芒星の線を描画（停止した弾を線上に配置） ---
    if (c >= 30 && c <= 60) {
        if (c == 30) {
            // シャキーンという予告音で描画開始
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        double rate = (c - 30) / 30.0; // 0.0 ～ 1.0 の進行度
        for (int i = 0; i < 5; i++) {
            int next_i = (i + 2) % 5; // 五芒星の線は「1つ飛ばしの頂点」を結ぶ

            sEnemyShot* p = new sEnemyShot;
            p->x = Px[i] + (Px[next_i] - Px[i]) * rate;
            p->y = Py[i] + (Py[next_i] - Py[i]) * rate;
            p->speed = 0.0;
            p->kind = img_enemyShotDiamond[3]; // シアン（青白）の菱形弾
            p->param_i[0] = 1; // 役割1: 線弾

            // 線の角度を計算し、垂直方向（+90度 または -90度）を記憶しておく
            double line_ang = atan2(Py[next_i] - Py[i], Px[next_i] - Px[i]);
            p->muki = line_ang;
            // 生成タイミングの偶奇で内側・外側へ交互に散開するように設定
            p->param_d[0] = line_ang + DX_PI / 2.0;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        {
            double rate = (c + 0.5 - 30) / 30.0; // 0.0 ～ 1.0 の進行度
            for (int i = 0; i < 5; i++) {
                int next_i = (i + 2) % 5; // 五芒星の線は「1つ飛ばしの頂点」を結ぶ

                sEnemyShot* p = new sEnemyShot;
                p->x = Px[i] + (Px[next_i] - Px[i]) * rate;
                p->y = Py[i] + (Py[next_i] - Py[i]) * rate;
                p->speed = 0.0;
                p->kind = img_enemyShotDiamond[3]; // シアン（青白）の菱形弾
                p->param_i[0] = 1; // 役割1: 線弾

                // 線の角度を計算し、垂直方向（+90度 または -90度）を記憶しておく
                double line_ang = atan2(Py[next_i] - Py[i], Px[next_i] - Px[i]);
                p->muki = line_ang;
                // 生成タイミングの偶奇で内側・外側へ交互に散開するように設定
                p->param_d[0] = line_ang + -DX_PI / 2.0;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // --- 【フェーズ4】完成した五芒星が一気に崩壊し、針弾が襲い掛かる ---
    if (c == 120) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) { // 線弾なら
                p->muki = p->param_d[0]; // 記憶しておいた垂直方向へ
                // GetRand(x) は 0～x の整数。弾抜けの隙間を作るため速度にムラ（1.0 ～ 1.6）を持たせる
                p->speed = 1.0 + GetRand(60) / 100.0;
                p->kind = img_enemyShotDiamond[5]; // 攻撃開始とともにマゼンタ（紫）へ色を変化
            }
            p = p->next;
        }
    }

    // --- 【フェーズ5】頂点の大玉から回転全方位弾 ---
    if (c >= 150 && c <= 300) {
        if (c % 10 == 0) { // 10フレーム間隔
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            double rot = (c - 150) * 0.05; // 時間経過とともに発射角を回転
            for (int i = 0; i < 5; i++) {
                // 3way弾
                for (int j = -1; j <= 1; j++) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = Px[i];
                    p->y = Py[i];
                    // 頂点ごとにベースの向きを変えつつ、回転とwayの広がりを加える
                    p->muki = rot + j * 0.2 + i * (2.0 * DX_PI / 5.0);
                    p->speed = 1.5;
                    p->kind = img_enemyShotSmallBall[0]; // 赤い小玉
                    p->param_i[0] = 2; // 役割2: 通常弾

                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }
        }
    }

    // --- 【フェーズ6】大玉の退場 ---
    if (c == 350) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) { // 大玉なら
                p->muki = atan2(p->y - ey, p->x - ex); // 中心から外側へ
                p->speed = 3.0; // 画面外へ飛んで自動消滅
            }
            p = p->next;
        }
    }

    // 全ての弾の座標更新（毎フレーム実行）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Pentagram_Gemini()
{
    // 敵の初期化と入場
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -50.0; // 画面上部から登場
        enemy.maxHp = enemy.hp = 200;
    }

    // y = 160 の位置（巨大な五芒星が画面内にちょうど収まる中央やや上）まで移動
    if (count < 60) {
        enemy.y += (240.0 - (-50.0)) / 60.0;
    }

    // ボスが定位置に着いた後、400フレーム周期で弾幕セットを発射
    if (count >= 60 && (count - 60) % 400 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPentagram;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをリストへリンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}