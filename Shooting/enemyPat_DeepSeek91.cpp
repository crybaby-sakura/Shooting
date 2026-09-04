// enemyPat_slither.cpp
// slither.io風弾幕「スリザー・サーペント」
// 使用素材: 中玉(青)=頭, 小玉(赤)=体節, 小玉(黄)=エサ, 鱗弾(白)=鱗, 小玉(青)=狙い弾

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言
static void SnakePattern(sEnemyShotSet* pEnemyShotSet);

// 敵本体パターン
void EnemyPat_Slitherio_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // スリザー・サーペント用のショットセットを作成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = SnakePattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0; // 使用しない
        pEnemyShotSet->kind = 0;

        // ショットリスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵本体の横移動（サンプルと同様）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}

// スリザー・サーペントのパターン関数
static void SnakePattern(sEnemyShotSet* pSet)
{
    // 静的変数（パターン全体で共有・単一インスタンス想定）
    static bool initialized = false;
    static sEnemyShot* head = nullptr;           // 頭
    static sEnemyShot* body[64];                 // 体節（最大64）
    static int bodyCount = 0;                    // 現在の体節数
    static double pathX[512], pathY[512];        // 頭の軌跡
    static int pathWrite = 0;                    // 軌跡の書き込み位置
    static int pathLen = 0;                      // 有効な軌跡の長さ
    static double headX, headY;                  // 頭の座標
    static double headAngle = 0.0;               // 頭の進行方向
    static int headSpeed = 2;                    // 頭の移動速度
    static int fireTimer = 0;                    // 頭の狙い弾用タイマー
    static int scaleTimer = 0;                   // 体節の鱗弾用タイマー
    static int foodCount = 0;                    // 画面上のエサの数

    const double BODY_SPACING = 12.0;            // 体節間の距離（フレーム換算）
    const double FOOD_EAT_DIST = 15.0;           // 頭がエサを食べる距離

    // 初回初期化
    if (count == 1) {
        initialized = true;
        bodyCount = 0;
        pathWrite = 0;
        pathLen = 0;
        fireTimer = 0;
        scaleTimer = 0;

        // 頭を作成（中玉・青）
        head = new sEnemyShot;
        head->x = pSet->x;
        head->y = pSet->y;
        head->muki = 0.0;
        head->speed = 0.0;
        head->kind = img_enemyShotLargeBall[4]; // 中玉・青
        head->param_i[0] = 0; // 役割: 0=頭

        // ショットリストに追加
        head->prev = pSet->pEnemyShotHead;
        head->next = pSet->pEnemyShotHead->next;
        pSet->pEnemyShotHead->next->prev = head;
        pSet->pEnemyShotHead->next = head;

        headX = head->x;
        headY = head->y;

        // 初期体節を8個作成（小玉・赤）
        for (int i = 0; i < 8; i++) {
            sEnemyShot* seg = new sEnemyShot;
            seg->x = headX - (i + 1) * BODY_SPACING; // 仮位置（実際は軌跡で更新）
            seg->y = headY;
            seg->muki = 0.0;
            seg->speed = 0.0;
            seg->kind = img_enemyShotMediumBall[0]; // 小玉・赤
            seg->param_i[0] = 1; // 役割: 1=体節
            body[i] = seg;
            bodyCount++;
            // リスト末尾に追加
            seg->prev = pSet->pEnemyShotHead->prev;
            seg->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = seg;
            pSet->pEnemyShotHead->prev = seg;
        }

        // エサを散布（小玉・黄）
        for (int i = 0; i < 18; i++) {
            sEnemyShot* food = new sEnemyShot;
            food->x = 30.0 + GetRand(420); // 画面内 (30〜450)
            food->y = 30.0 + GetRand(420);
            food->muki = 0.0;
            food->speed = 0.0;
            food->kind = img_enemyShotSmallBall[1]; // 小玉・黄
            food->param_i[0] = 2; // 役割: 2=エサ
            // リスト末尾に追加
            food->prev = pSet->pEnemyShotHead->prev;
            food->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = food;
            pSet->pEnemyShotHead->prev = food;
            foodCount++;
        }
    }

    // 現在の頭の位置を軌跡に記録
    pathX[pathWrite] = headX;
    pathY[pathWrite] = headY;
    pathWrite = (pathWrite + 1) % 512;
    if (pathLen < 512) pathLen++;

    // 最も近いエサを探す
    sEnemyShot* nearestFood = nullptr;
    double minDist = 1e10;
    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        if (shot->param_i[0] == 2) { // エサ
            double dx = shot->x - headX;
            double dy = shot->y - headY;
            double dist = dx * dx + dy * dy;
            if (dist < minDist) {
                minDist = dist;
                nearestFood = shot;
            }
        }
        shot = shot->next;
    }

    // 頭の移動
    if (nearestFood != nullptr) {
        double dx = nearestFood->x - headX;
        double dy = nearestFood->y - headY;
        headAngle = atan2(dy, dx);
        headX += cos(headAngle) * headSpeed;
        headY += sin(headAngle) * headSpeed;
    }
    else {
        // エサがない場合はゆっくり直進
        headX += cos(headAngle) * headSpeed;
        headY += sin(headAngle) * headSpeed;
        // 画面端で跳ね返る簡易処理
        if (headX < 20.0 || headX > 460.0) headAngle = DX_PI - headAngle;
        if (headY < 20.0 || headY > 460.0) headAngle = -headAngle;
    }

    // 頭のショットを更新
    head->x = headX;
    head->y = headY;
    head->muki = headAngle;

    // エサを食べたか判定
    if (nearestFood != nullptr) {
        double dx = nearestFood->x - headX;
        double dy = nearestFood->y - headY;
        if (dx * dx + dy * dy < FOOD_EAT_DIST * FOOD_EAT_DIST) {
            // エサをリストから削除
            sEnemyShot* prev = nearestFood->prev;
            sEnemyShot* next = nearestFood->next;
            prev->next = next;
            next->prev = prev;
            delete nearestFood;
            foodCount--;

            // 体節を1つ追加（小玉・赤）
            if (bodyCount < 64) {
                sEnemyShot* newSeg = new sEnemyShot;
                newSeg->x = body[bodyCount - 1]->x; // 仮
                newSeg->y = body[bodyCount - 1]->y;
                newSeg->muki = 0.0;
                newSeg->speed = 0.0;
                newSeg->kind = img_enemyShotMediumBall[0];
                newSeg->param_i[0] = 1;
                body[bodyCount] = newSeg;
                bodyCount++;
                // リスト末尾に追加
                newSeg->prev = pSet->pEnemyShotHead->prev;
                newSeg->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = newSeg;
                pSet->pEnemyShotHead->prev = newSeg;
            }
        }
    }

    // 体節の位置更新（軌跡から取得）
    for (int i = 0; i < bodyCount; i++) {
        int index = pathWrite - (i + 1) * (int)(BODY_SPACING);
        while (index < 0) index += 512;
        index %= 512;
        // 軌跡の長さが足りない場合は頭の位置に固定
        if ((i + 1) * BODY_SPACING >= pathLen) {
            body[i]->x = headX;
            body[i]->y = headY;
        }
        else {
            body[i]->x = pathX[index];
            body[i]->y = pathY[index];
        }
        // 体節の向きは前後の体節との角度（見た目用に設定）
        if (i == 0) {
            body[i]->muki = headAngle;
        }
        else {
            double dx = body[i - 1]->x - body[i]->x;
            double dy = body[i - 1]->y - body[i]->y;
            body[i]->muki = atan2(dy, dx);
        }
    }

    // 頭の狙い弾（3-way、青小玉）
    fireTimer++;
    if (fireTimer >= 45) { // 約0.75秒ごと
        fireTimer = 0;
        double aimAngle = atan2(player.y - headY, player.x - headX);
        for (int k = -1; k <= 1; k++) {
            sEnemyShot* bullet = new sEnemyShot;
            bullet->x = headX;
            bullet->y = headY;
            bullet->muki = aimAngle + k * (10.0 * DX_PI / 180.0); // 10度間隔
            bullet->speed = 2.5;
            bullet->kind = img_enemyShotSmallBall[4]; // 青小玉
            bullet->param_i[0] = 4; // 役割: 4=狙い弾
            // リスト末尾に追加
            bullet->prev = pSet->pEnemyShotHead->prev;
            bullet->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = bullet;
            pSet->pEnemyShotHead->prev = bullet;
        }
        // 効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 体節から鱗弾（左右へ白鱗弾）
    scaleTimer++;
    if (scaleTimer >= 30) { // 約0.5秒ごと
        scaleTimer = 0;
        for (int i = 0; i < bodyCount; i++) {
            // 各体節から左右45度に発射
            for (int side = -1; side <= 1; side += 2) {
                sEnemyShot* scale = new sEnemyShot;
                scale->x = body[i]->x;
                scale->y = body[i]->y;
                scale->muki = body[i]->muki + side * (45.0 * DX_PI / 180.0);
                scale->speed = 2.0;
                scale->kind = img_enemyShotScale[6]; // 鱗弾・白
                scale->param_i[0] = 3; // 役割: 3=鱗弾
                // リスト末尾に追加
                scale->prev = pSet->pEnemyShotHead->prev;
                scale->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = scale;
                pSet->pEnemyShotHead->prev = scale;
            }
        }
    }

    // 移動する弾（狙い弾・鱗弾）の更新
    shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        if (shot->param_i[0] == 3 || shot->param_i[0] == 4) {
            // 直線移動
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
        }
        shot = shot->next;
    }
}