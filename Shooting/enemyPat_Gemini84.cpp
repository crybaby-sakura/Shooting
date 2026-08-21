// enemyPat_spiro.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：幾何軌跡「スピロ・トロコイド」
static void ShotSpiro(sEnemyShotSet* pEnemyShotSet)
{
    // === パラメータ設定 ===
    const double R = 150.0; // 大歯車の半径
    const double r = 60.0;  // 小歯車の半径 (R:r = 5:2 で5芒星風の軌跡になる)
    const double d = 60.0;  // ペン位置の小歯車中心からの距離
    const double Cx = 240.0; // 歯車全体の中心X (画面中央)
    const double Cy = 200.0; // 歯車全体の中心Y
    const int phase2_frames = 200; // 描画にかけるフレーム数
    const double theta_step = (4.0 * DX_PI) / phase2_frames; // 大歯車を2周（5/2比率なのでこれで元の位置に戻る）
    const double base_angle = pEnemyShotSet->param_d[0];

    int c = pEnemyShotSet->count;

    // -----------------------------------------------------
    // フェーズ1: 歯車とペンの生成 (初回のみ)
    // -----------------------------------------------------
    if (c == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 1. 固定大歯車の生成 (赤・中玉 72個)
        for (int i = 0; i < 72; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = base_angle + (DX_PI * 2.0 / 72.0) * i;
            double radius = R;

            // 内側に突き出す「歯」の表現 (12個の歯)
            if (i % 6 == 0) radius -= 12.0;
            else if (i % 6 == 1 || i % 6 == 5) radius -= 6.0;

            pShot->x = Cx + radius * cos(angle);
            pShot->y = Cy + radius * sin(angle);
            pShot->speed = 0.0;
            pShot->muki = angle;
            pShot->kind = img_enemyShotMediumBall[0]; // 赤
            pShot->param_i[0] = 0; // 役割：大歯車

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 2. 回転小歯車の生成 (青・中玉 36個)
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotMediumBall[4]; // 青
            pShot->param_i[0] = 1; // 役割：小歯車
            pShot->param_i[1] = i; // インデックス保存

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 3. 描画ペンの生成 (白・大玉 1個)
        sEnemyShot* pPen = new sEnemyShot;
        pPen->speed = 0.0;
        pPen->kind = img_enemyShotLargeBall[6]; // 白
        pPen->param_i[0] = 2; // 役割：ペン

        pPen->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pPen->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pPen;
        pEnemyShotSet->pEnemyShotHead->prev = pPen;
    }

    // -----------------------------------------------------
    // フェーズ2: 小歯車の回転と軌跡の描画
    // -----------------------------------------------------
    if (c >= 0 && c <= phase2_frames) {
        // 小歯車中心の公転角度
        double theta = base_angle + theta_step * c;
        // 小歯車の中心座標
        double cx = Cx + (R - r) * cos(theta);
        double cy = Cy + (R - r) * sin(theta);
        // 小歯車の自転角度 (内歯車として滑らず転がる幾何学計算)
        double phi = base_angle + -(R - r) / r * theta;

        // ペンの現在座標
        double penX = cx + d * cos(phi);
        double penY = cy + d * sin(phi);

        // 既存の歯車とペンの座標更新
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) { // 小歯車
                int idx = pShot->param_i[1];
                double angle = base_angle + phi + (DX_PI * 2.0 / 36.0) * idx;

                // 外側に突き出す「歯」の表現 (6個の歯)
                double radius = r;
                if (idx % 6 == 0) radius += 12.0;
                else if (idx % 6 == 1 || idx % 6 == 5) radius += 6.0;

                pShot->x = cx + radius * cos(angle);
                pShot->y = cy + radius * sin(angle);
            }
            else if (pShot->param_i[0] == 2) { // ペン
                pShot->x = penX;
                pShot->y = penY;
            }
            pShot = pShot->next;
        }

        // 軌跡弾の設置 (毎フレーム設置して線をつなぐ)
        if (c < phase2_frames) {
            // SEがうるさすぎないように4フレームに1回鳴らす
            if (c % 4 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            sEnemyShot* pTrack = new sEnemyShot;
            pTrack->x = penX;
            pTrack->y = penY;
            pTrack->speed = 0.0;

            // 軌跡の接線方向を計算して弾の向きとする（鱗弾が軌跡に沿うため綺麗になる）
            double dx_dt = -(R - r) * sin(theta) * theta_step - d * sin(phi) * (-(R - r) / r) * theta_step;
            double dy_dt = (R - r) * cos(theta) * theta_step + d * cos(phi) * (-(R - r) / r) * theta_step;
            pTrack->muki = atan2(dy_dt, dx_dt);

            pTrack->kind = img_enemyShotScale[5]; // マゼンタの鱗弾
            pTrack->param_i[0] = 3; // 役割：軌跡弾

            pTrack->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pTrack->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pTrack;
            pEnemyShotSet->pEnemyShotHead->prev = pTrack;
        }
    }

    // -----------------------------------------------------
    // フェーズ3: 模様の完成と一斉発射 (二次弾幕化)
    // -----------------------------------------------------
    if (c == phase2_frames + 30) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1 || pShot->param_i[0] == 2) {
                // 用済みの歯車とペンは、ランダムな速度で外側に吹き飛んで退場する
                pShot->muki = atan2(pShot->y - Cy, pShot->x - Cx) + (GetRand(20) - 10) / 100.0;
                pShot->speed = 4.0 + (GetRand(200) / 100.0);
            }
            else if (pShot->param_i[0] == 3) {
                // 描画された軌跡弾は、中心から外側に向かって放射状に動き出す
                pShot->muki = atan2(pShot->y - Cy, pShot->x - Cx);
                pShot->speed = 0.5; // ゆっくり動き始める
            }
            pShot = pShot->next;
        }
    }

    // 発射後の軌跡弾の制御（スパイラル状に加速しながら広がる）
    if (c > phase2_frames + 30) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 3) {
                pShot->speed += 0.015;  // 徐々に加速
                pShot->muki += 0.003;   // 少しずつ曲がって渦を巻く
            }

            // 速度を持った弾を動かす
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン関数
void EnemyPat_Spirograph_Gemini()
{
    // 初期化 (1フレーム目)
    if (count == 1) {
        // 画面上部中央に配置
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 演出開始タイミング (例として60フレーム目に弾幕セット生成)
    if (count % 300 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiro;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = 0; // 今回は固定
        pEnemyShotSet->param_d[0] = GetRand(100) / 100.0 * 2.0 * DX_PI;

        // リストのダミーヘッド生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // システムの親リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}