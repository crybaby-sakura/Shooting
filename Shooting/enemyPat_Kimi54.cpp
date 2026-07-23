// enemyPat_beerShower.cpp
// ビールかけモチーフ弾幕パターン「泡沫の宴（エビュリエンス・フェスト）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  共通ユーティリティ
// ============================================================

// 弾を1発生成してリストに追加
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->margin = 20.0;
    for (int i = 0; i < 8; i++) {
        p->param_i[i] = 0;
        p->param_d[i] = 0.0;
    }
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾の色番号を取得（kindの上位16bitに色が入っている想定、下位16bitに画像番号）
static int GetColorFromKind(int kind)
{
    return (kind - 4) % 9;
}

// ============================================================
//  Phase 1: 注ぎ（The Pour）
//  金色の直線レーザーが斜め下へ連続照射、左右にスイング
// ============================================================
static void ShotPouring(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    // 初回生成
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int numBeams = 5;
        for (int i = 0; i < numBeams; i++) {
            double baseAngle = pSet->muki;
            double spread = (i - numBeams / 2) * 0.18;
            int colorIdx = 1; // 黄（金色）
            int kind = img_enemyShotLaser[colorIdx];
            p = AddShot(pSet, pSet->x, pSet->y, baseAngle + spread, 3.5, kind);
            p->param_d[0] = baseAngle + spread; // 基準角度
            p->param_d[1] = (i % 2 == 0) ? 0.015 : -0.015; // スイング方向
            p->param_i[0] = i; // ビーム番号
            p->margin = 40;
        }
    }

    // 弾更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 角度をスイング（正弦波）
        double swing = sin(pSet->count * 0.03 + pShot->param_i[0] * 0.5) * 0.3;
        pShot->muki = pShot->param_d[0] + swing;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
//  Phase 2: 溢れ（The Overflow）
//  画面下部に泡の帯（低速密集フィールド）+ 波状カーブ弾
// ============================================================
static void ShotOverflow(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 泡の帯：画面下部に横一列の低速弾（白）
        for (int i = 0; i < 24; i++) {
            double px = 20.0 + i * 18.7;
            double py = 480.0 + GetRand(30) - 15;
            int colorIdx = 6; // 白（泡）
            int kind = img_enemyShotSmallBall[colorIdx];
            p = AddShot(pSet, px, py, -DX_PI / 2, 0.3 + GetRand(10) / 100.0, kind);
            p->param_d[0] = px; // 基準X
            p->param_d[1] = py; // 基準Y
            p->param_i[0] = i;  // インデックス
            p->margin = 40;
        }

        // 波状カーブ弾：画面端から中央へ（琥珀色・中玉）
        for (int i = 0; i < 6; i++) {
            double startX = (i % 2 == 0) ? -10.0 : 490.0;
            double startY = 100.0 + i * 50.0;
            double targetAngle = (i % 2 == 0) ? 0.0 : DX_PI;
            int colorIdx = 8; // 橙（琥珀色）
            int kind = img_enemyShotMediumBall[colorIdx];
            p = AddShot(pSet, startX, startY, targetAngle, 1.8, kind);
            p->param_i[0] = 100 + i;        // 弾番号
            p->param_i[1] = i % 2;    // 0:左から、1:右から
            p->param_d[0] = 0.0;      // 波の位相
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int shotType = pShot->param_i[0];

        if (shotType < 24) {
            // 泡の帯：ゆらゆら上下に揺れる
            pShot->x = pShot->param_d[0] + sin(pSet->count * 0.05 + shotType * 0.3) * 8.0;
            pShot->y += pShot->speed * sin(pShot->muki);
            // 泡は画面下で跳ね返る
            //if (pShot->y > 470.0) {
            //    pShot->y = 470.0;
            //    pShot->muki = -DX_PI / 2 - 0.2 + GetRand(40) / 100.0;
            //}
        }
        else {
            // 波状カーブ弾：放物線的なカーブ
            int fromRight = pShot->param_i[1];
            double wave = sin(pSet->count * 0.08 + shotType * 0.5) * 1.5;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += wave;
            // 重力で下に加速
            pShot->speed += 0.01;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  Phase 3: 泡立ち（The Froth）
//  大小の泡弾がランダム生成、合体・分裂 + 麦粒高速弾
// ============================================================
static void ShotFroth(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 大きな泡（中玉・白）を数個生成
        for (int i = 0; i < 5; i++) {
            double px = 80.0 + GetRand(320);
            double py = 80.0 + GetRand(100);
            double angle = GetRand(360) / 180.0 * DX_PI;
            int colorIdx = 6; // 白
            int kind = img_enemyShotMediumBall[colorIdx];
            p = AddShot(pSet, px, py, angle, 0.8 + GetRand(15) / 10.0, kind);
            p->param_i[0] = 0; // タイプ: 大泡
            p->param_i[1] = 60 + GetRand(60); // 分裂までのカウント
            p->param_d[0] = px; // 基準X
            p->param_d[1] = py; // 基準Y
        }

        // 小さな泡（小玉・白）を多数生成
        for (int i = 0; i < 16; i++) {
            double px = 40.0 + GetRand(400);
            double py = 40.0 + GetRand(120);
            double angle = GetRand(360) / 180.0 * DX_PI;
            int colorIdx = 6; // 白
            int kind = img_enemyShotSmallBall[colorIdx];
            p = AddShot(pSet, px, py, angle, 1.2 + GetRand(20) / 10.0, kind);
            p->param_i[0] = 1; // タイプ: 小泡
        }
    }

    // 大泡が一定時間経過で分裂（麦粒弾を内部から放出）
    if (pSet->count > 0 && pSet->count % 40 == 0) {
        sEnemyShot* pCheck = pSet->pEnemyShotHead->next;
        while (pCheck != pSet->pEnemyShotHead) {
            if (pCheck->param_i[0] == 0 && pCheck->count >= pCheck->param_i[1]) {
                // 大泡が割れる！麦粒弾（銃弾・黄色）を8方向に
                for (int j = 0; j < 8; j++) {
                    double angle = j * DX_PI / 4.0;
                    int colorIdx = 1; // 黄（麦粒色）
                    int kind = img_enemyShotBullet[colorIdx];
                    p = AddShot(pSet, pCheck->x, pCheck->y, angle, 2.5, kind);
                    p->param_i[0] = 2; // タイプ: 麦粒
                }
                // 大泡は消滅
                sEnemyShot* pNext = pCheck->next;
                pCheck->prev->next = pCheck->next;
                pCheck->next->prev = pCheck->prev;
                delete pCheck;
                pCheck = pNext;
                continue;
            }
            pCheck = pCheck->next;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int bubbleType = pShot->param_i[0];

        if (bubbleType == 0) {
            // 大泡：ゆっくり浮遊、画面端で跳ね返り
            pShot->x += pShot->speed * cos(pShot->muki) * 0.5;
            pShot->y += pShot->speed * sin(pShot->muki) * 0.5;
            if (pShot->x < 20.0 || pShot->x > 460.0) pShot->muki = DX_PI - pShot->muki;
            if (pShot->y < 20.0 || pShot->y > 300.0) pShot->muki = -pShot->muki;
        }
        else if (bubbleType == 1) {
            // 小泡：素直に直進、軽い揺れ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->muki += sin(pShot->count * 0.1) * 0.02;
        }
        else if (bubbleType == 2) {
            // 麦粒弾：高速直進、少し重力で落下
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed += 0.02; // 加速
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  Phase 4: 乾杯（The Toast / Climax）
//  全方向から扇形放射弾 + 画面中央に泡の竜巻（吸引+弾幕）
// ============================================================
static void ShotToast(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 4方向から扇形の飛沫（大玉・金色/琥珀色）
        int directions[4] = { 0, 1, 2, 3 }; // 上、右、下、左
        double baseAngles[4] = { DX_PI / 2, DX_PI, -DX_PI / 2, 0 };
        int colors[4] = { 1, 8, 1, 8 }; // 黄、橙、黄、橙

        for (int d = 0; d < 4; d++) {
            double baseX = (d == 1) ? 520.0 : (d == 3) ? -40.0 : 240.0;
            double baseY = (d == 0) ? -40.0 : (d == 2) ? 520.0 : 240.0;
            for (int i = -3; i <= 3; i++) {
                double angle = baseAngles[d] + i * 0.12;
                int colorIdx = colors[d];
                int kind = img_enemyShotLargeBall[colorIdx];
                p = AddShot(pSet, baseX, baseY, angle, 2.0 + GetRand(15) / 10.0, kind);
                p->param_i[0] = 0; // タイプ: 飛沫
                p->param_i[1] = d; // 方向
                p->param_d[0] = angle; // 基準角度
                p->margin = 50;
            }
        }

        // 泡の竜巻：中央付近に複数の吸引弾（中玉・白）
        const int N = 120;
        for (int i = 0; i < N; i++) {
            double angle = i * 2 * DX_PI / N;
            double dist = 30.0 + GetRand(80);
            double px = 240.0 + dist * cos(angle);
            double py = 240.0 + dist * sin(angle);
            int colorIdx = 6; // 白
            int kind = img_enemyShotMediumBall[colorIdx];
            p = AddShot(pSet, px, py, angle + DX_PI / 2, 1.5, kind);
            p->param_i[0] = 1; // タイプ: 竜巻
            p->param_i[1] = i; // インデックス
            p->param_d[0] = 240.0; // 竜巻中心X
            p->param_d[1] = 240.0; // 竜巻中心Y
            p->param_d[2] = dist;  // 初期距離
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int shotType = pShot->param_i[0];

        if (shotType == 0) {
            // 飛沫：直進後、重力で放物線落下
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // y方向に重力加速
            pShot->muki += 0.008; // 下向きに曲がる
            pShot->speed += 0.015;
        }
        else if (shotType == 1) {
            // 竜巻：中心を周回しながら縮む
            double cx = pShot->param_d[0];
            double cy = pShot->param_d[1];
            double currentDist = sqrt((pShot->x - cx) * (pShot->x - cx) + (pShot->y - cy) * (pShot->y - cy));

            // 中心に向かう角度
            double toCenter = atan2(cy - pShot->y, cx - pShot->x);
            // 周回角度（時計回り）
            double orbit = toCenter + DX_PI / 2;

            // 中心に向かいながら周回
            pShot->x += pShot->speed * cos(orbit) * 0.7 + 0.3 * cos(toCenter);
            pShot->y += pShot->speed * sin(orbit) * 0.7 + 0.3 * sin(toCenter);

            // 中心に近づきすぎたら外に弾き出す
            if (currentDist < 15.0) {
                pShot->muki = atan2(pShot->y - cy, pShot->x - cx);
                pShot->speed = 4.0;
                pShot->param_i[0] = 2; // 弾き出しモードへ
            }
        }
        else if (shotType == 2) {
            // 弾き出された弾：一直線に外へ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：泡沫の宴
// ============================================================
void EnemyPat_BeerSpray_Kimi()
{
    static int phase;        // 現在のフェーズ (0〜3)
    static int phaseTimer;   // フェーズ内タイマー
    static int shotInterval; // 弾幕生成間隔
    static int muki;         // 敵の移動方向

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 0;
        shotInterval = 0;
        muki = 1;
    }

    // === 敵の移動 ===
    // フェーズごとに異なる動き
    switch (phase) {
    case 0: // Phase1: 左右にゆっくり
        enemy.x += 0.6 * muki;
        if (count % 90 == 45) muki *= -1;
        break;
    case 1: // Phase2: 中央で静止
        enemy.x += (240.0 - enemy.x) * 0.02;
        enemy.y += (60.0 - enemy.y) * 0.02;
        break;
    case 2: // Phase3: 円を描く
        enemy.x = 240.0 + cos(count * 0.03) * 80.0;
        enemy.y = 80.0 + sin(count * 0.05) * 30.0;
        break;
    case 3: // Phase4: 中央に固定
        enemy.x += (240.0 - enemy.x) * 0.05;
        enemy.y += (100.0 - enemy.y) * 0.05;
        break;
    }

    // === フェーズ管理 ===
    phaseTimer++;

    // フェーズ切り替え（約5秒毎）
    int phaseDuration[4] = { 300, 300, 360, 200 }; // 各フェーズのフレーム数
    if (phaseTimer >= phaseDuration[phase]) {
        phase = (phase + 1) % 4;
        phaseTimer = 0;
        shotInterval = 0;
    }

    // === 弾幕生成 ===
    shotInterval++;

    // 各フェーズの弾幕生成条件
    bool shouldFire = false;
    switch (phase) {
    case 0: shouldFire = (shotInterval % 80 == 1); break;  // Phase1: 約1.3秒毎
    case 1: shouldFire = (shotInterval % 100 == 1); break; // Phase2: 約1.7秒毎
    case 2: shouldFire = (shotInterval % 60 == 1); break;  // Phase3: 約1秒毎
    case 3: shouldFire = (shotInterval == 1); break;       // Phase4: 1回のみ（クリマックス）
    }

    if (shouldFire) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->kind = phase;

        // プレイヤー方向を基準角度に
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        // フェーズに応じた弾幕パターンを選択
        switch (phase) {
        case 0: pSet->patternFunc = ShotPouring;  break;
        case 1: pSet->patternFunc = ShotOverflow; break;
        case 2: pSet->patternFunc = ShotFroth;    break;
        case 3: pSet->patternFunc = ShotToast;    break;
        }

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}