// enemyPat_snake.cpp
// slither.io モチーフ弾幕「スネークチェイサー」
// ヘビが餌を食べて成長し、体を連ねて自機を追いかける弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ヘビ1匹分の弾幕パターン
// ------------------------------------------------------------
static void ShotSnake(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ===== 初期化（count==0 の1回だけ実行） =====
    if (pEnemyShotSet->count == 0) {
        // ヘビ出現音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // param_i[0] : 現在のセグメント数（頭含む）
        // param_i[1] : 最大セグメント数（頭+体）
        // param_i[2] : 餌出現タイマー兼汎用カウンタ
        // param_i[3] : ヘビの色パターン（0:赤→黄, 1:青→シアン, 2:緑→黄）
        pEnemyShotSet->param_i[0] = 1;  // 最初は頭だけ
        pEnemyShotSet->param_i[1] = 6*10;  // 頭+体5 = 最大6セグメント
        pEnemyShotSet->param_i[2] = 0;
        pEnemyShotSet->param_i[3] = pEnemyShotSet->kind % 3;

        // --- 頭（大玉）を生成 ---
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 2.2;
        switch (pEnemyShotSet->param_i[3]) {
        case 0: pEnemyShot->kind = img_enemyShotLargeBall[0]; break; // 赤
        case 1: pEnemyShot->kind = img_enemyShotLargeBall[4]; break; // 青
        case 2: pEnemyShot->kind = img_enemyShotLargeBall[2]; break; // 緑
        }
        pEnemyShot->param_i[0] = 0;  // セグメント番号 0 = 頭
        pEnemyShot->param_i[1] = pEnemyShotSet->param_i[3];
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // --- 餌を3つ生成（小玉、緑、固定） ---
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 40 + GetRand(400);
            pEnemyShot->y = 40 + GetRand(400);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑
            pEnemyShot->param_i[0] = -1; // -1 = 餌
            pEnemyShot->param_i[1] = 2;  // 色:緑
            pEnemyShot->margin = 10.0;   // 画面外判定を小さく
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 毎フレーム更新 =====

    // --- 1. 頭を探す ---
    sEnemyShot* pHead = nullptr;
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) {
            pHead = pEnemyShot;
            break;
        }
        pEnemyShot = pEnemyShot->next;
    }
    if (!pHead) return; // 頭が消滅していたら何もしない

    // --- 2. 頭を自機方向に旋回・移動（slither.io 風の滑らかな蛇行）---
    double targetMuki = atan2(player.y - pHead->y, player.x - pHead->x);
    double diff = targetMuki - pHead->muki;
    while (diff > DX_PI)  diff -= 2.0 * DX_PI;
    while (diff < -DX_PI) diff += 2.0 * DX_PI;
    double turnLimit = 0.05; // 1フレームあたりの最大旋回角
    if (diff > turnLimit)  diff = turnLimit;
    if (diff < -turnLimit) diff = -turnLimit;
    pHead->muki += diff;

    pHead->x += pHead->speed * cos(pHead->muki);
    pHead->y += pHead->speed * sin(pHead->muki);

    // 画面端でバウンド（slither.io 風の折り返し）
    if (pHead->x < 20.0) {
        pHead->muki = DX_PI - pHead->muki;
        pHead->x = 20.0;
    }
    if (pHead->x > 460.0) {
        pHead->muki = DX_PI - pHead->muki;
        pHead->x = 460.0;
    }
    if (pHead->y < 20.0) {
        pHead->muki = -pHead->muki;
        pHead->y = 20.0;
    }
    if (pHead->y > 460.0) {
        pHead->muki = -pHead->muki;
        pHead->y = 460.0;
    }

    // --- 3. 体の追従（各セグメントが前のセグメントを追う）---
    int segCount = pEnemyShotSet->param_i[0];
    for (int seg = 1; seg < segCount; seg++) {
        // 現在のセグメントを探す
        sEnemyShot* pBody = nullptr;
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == seg) {
                pBody = pEnemyShot;
                break;
            }
            pEnemyShot = pEnemyShot->next;
        }
        if (!pBody) continue;

        // 前のセグメントを探す
        sEnemyShot* pPrev = nullptr;
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == seg - 1) {
                pPrev = pEnemyShot;
                break;
            }
            pEnemyShot = pEnemyShot->next;
        }
        if (!pPrev) continue;

        double dx = pPrev->x - pBody->x;
        double dy = pPrev->y - pBody->y;
        double dist = sqrt(dx * dx + dy * dy);
        double followDist = 13.0; // セグメント間の目標距離

        if (dist > followDist) {
            double moveSpeed = pHead->speed * 1.05; // 体は頭より少し速く（追いつくように）
            double maxMove = dist - followDist;
            if (moveSpeed > maxMove) moveSpeed = maxMove;
            pBody->x += (dx / dist) * moveSpeed;
            pBody->y += (dy / dist) * moveSpeed;
        }
        pBody->muki = atan2(dy, dx);
    }

    // --- 4. 餌との距離判定＆成長 ---
    int maxSeg = pEnemyShotSet->param_i[1];
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == -1) { // 餌
            double dx = pHead->x - pEnemyShot->x;
            double dy = pHead->y - pEnemyShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 20.0) { // 頭が餌に接触
                // 餌をリストから外して削除
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                sEnemyShot* pDel = pEnemyShot;
                pEnemyShot = pEnemyShot->prev; // 次のイテレーション用に戻す
                delete pDel;

                // 成長：新しい体セグメントを追加
                int newSeg = pEnemyShotSet->param_i[0];
                if (newSeg < maxSeg) {
                    sEnemyShot* pNew = new sEnemyShot;
                    // 最後尾のセグメントの背後に生成
                    sEnemyShot* pTail = nullptr;
                    sEnemyShot* pTmp = pEnemyShotSet->pEnemyShotHead->next;
                    while (pTmp != pEnemyShotSet->pEnemyShotHead) {
                        if (pTmp->param_i[0] == newSeg - 1) pTail = pTmp;
                        pTmp = pTmp->next;
                    }
                    if (pTail) {
                        pNew->x = pTail->x - cos(pTail->muki) * 8.0;
                        pNew->y = pTail->y - sin(pTail->muki) * 8.0;
                    }
                    else {
                        pNew->x = pHead->x;
                        pNew->y = pHead->y;
                    }
                    pNew->muki = pHead->muki;
                    pNew->speed = 0.0;

                    // 見た目：先頭に近いほど中玉、後ろほど小玉
                    int colorPat = pEnemyShotSet->param_i[3];
                    if (newSeg <= 2*10) {
                        switch (colorPat) {
                        case 0: pNew->kind = img_enemyShotMediumBall[1]; break; // 黄
                        case 1: pNew->kind = img_enemyShotMediumBall[3]; break; // シアン
                        case 2: pNew->kind = img_enemyShotMediumBall[1]; break; // 黄
                        }
                    }
                    else {
                        switch (colorPat) {
                        case 0: pNew->kind = img_enemyShotSmallBall[1]; break; // 黄
                        case 1: pNew->kind = img_enemyShotSmallBall[3]; break; // シアン
                        case 2: pNew->kind = img_enemyShotSmallBall[1]; break; // 黄
                        }
                    }
                    pNew->param_i[0] = newSeg;
                    pNew->param_i[1] = colorPat;

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;

                    pEnemyShotSet->param_i[0] = newSeg + 1;

                    // 成長音
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
                continue;
            }
        }
        pEnemyShot = pEnemyShot->next;
    }

    // --- 5. 一定間隔で新しい餌を出現（最大3つまで）---
    pEnemyShotSet->param_i[2]++;
    int foodCount = 0;
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == -1) foodCount++;
        pEnemyShot = pEnemyShot->next;
    }
    if (foodCount < 3*10 && pEnemyShotSet->param_i[2] % 15 == 0) {
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 40 + GetRand(400);
        pEnemyShot->y = 40 + GetRand(400);
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑
        pEnemyShot->param_i[0] = -1;
        pEnemyShot->param_i[1] = 2;
        pEnemyShot->margin = 10.0;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    if (segCount == maxSeg) {
        pHead->kind = img_enemyShotLargeBall[5];
        if (hypot(pHead->x - enemy.x, pHead->y - enemy.y) < 30) {
            enemy.hp = 0;
        }
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Slitherio_Kimi()
{
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 9999;
        shot_count = 0;
    }
    else {
        // 敵本体は画面中央付近でゆっくり左右に揺れる
        enemy.x = 240.0 + 60.0 * sin(count / 180.0 * DX_PI);
    }

    // 200フレームごとにヘビを出現（最大3匹）
    if (count % 200 == 1 && shot_count < 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }
}