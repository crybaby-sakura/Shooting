#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：ピッチドロップ実験モチーフ
//  - 大玉(黒)でほぼ動かない「ピッチ塊」を形成
//  - 中玉(黒)で極遅の滴を垂らす
//  - 一定距離で小玉に砕けてゆっくり広がる
// ============================================================
static void ShotPitchDrop(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ----- 初期化（count==0 のときだけ） -----
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // パラメータ初期化
        // param_i[0] : 次に滴を生成するまでのカウンタ制御用（未使用でも可）
        // param_i[1] : 生成した滴の回数
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;

        // ピッチ塊を生成（大玉・黒・ほぼ静止）
        // 画面上部中央付近に密集させて「固体に見える塊」を作る
        for (int i = 0; i < 18; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(70) - 35);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(50) - 25);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒・大玉
            pEnemyShot->param_i[0] = 1;                   // 1 = 塊本体（動かない）
            pEnemyShot->param_d[0] = pEnemyShot->x;       // 基準位置を記録（わずかな揺らぎ用）
            pEnemyShot->param_d[1] = pEnemyShot->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 塊の下部を少し埋める中玉も追加
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(50) - 25);
            pEnemyShot->y = pEnemyShotSet->y + 20.0 + (GetRand(30) - 10);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒・中玉
            pEnemyShot->param_i[0] = 1;                    // 塊の一部
            pEnemyShot->param_d[0] = pEnemyShot->x;
            pEnemyShot->param_d[1] = pEnemyShot->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----- 滴の生成（一定間隔で極遅の滴を垂らす） -----
    // 最初の滴は形成後少し待ってから、以降は約3秒おき
    int dropInterval = 180;
    int firstDropTime = 90;
    if (pEnemyShotSet->count >= firstDropTime &&
        (pEnemyShotSet->count - firstDropTime) % dropInterval == 0 &&
        pEnemyShotSet->param_i[1] < 6) {   // 最大6滴まで（弾数抑制）

        // 滴生成音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double dropBaseX = pEnemyShotSet->x + (GetRand(40) - 20);
        double dropBaseY = pEnemyShotSet->y + 35.0;

        // 縦に連なる中玉で「伸びた滴」を表現（極遅で落下）
        int dropLen = 5 + GetRand(2); // 5〜7個
        for (int j = 0; j < dropLen*2; j++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = dropBaseX + (GetRand(6) - 3); // わずかな横ブレ
            pEnemyShot->y = dropBaseY + j * 9.0/2;
            pEnemyShot->muki = DX_PI * 0.5;               // 真下
            pEnemyShot->speed = 0.28 + GetRand(12) / 100.0; // 0.28〜0.40 の極遅
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒・中玉
            pEnemyShot->param_i[0] = 2;                    // 2 = 滴本体
            pEnemyShot->param_i[1] = 0;                    // 砕け判定用フラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        pEnemyShotSet->param_i[1]++; // 滴生成回数をカウント
    }

    // ----- 全弾の更新 -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除される可能性があるので先に取得

        if (pShot->param_i[0] == 1) {
            // 塊本体：ほぼ静止。わずかに揺らいで「粘性のある固体」感を出す
            double t = pShot->count * 0.03;
            pShot->x = pShot->param_d[0] + sin(t + pShot->param_d[0] * 0.1) * 1.2;
            pShot->y = pShot->param_d[1] + cos(t * 0.7 + pShot->param_d[1] * 0.1) * 0.8;
            // speed は 0 のまま
        }
        else if (pShot->param_i[0] == 2) {
            // 滴本体：極遅で落下
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 一定距離（または一定時間）で「砕け」状態へ移行
            if (pShot->param_i[1] == 0 && (pShot->y > 260.0)) {
                pShot->param_i[1] = 1; // 砕け済みフラグ

                // 自分自身を小玉に変えてゆっくり拡散
                pShot->kind = img_enemyShotSmallBall[7]; // 黒・小玉
                pShot->speed = 0.6 + GetRand(40) / 100.0;
                // 真下〜斜め下方向に散らす
                double baseAngle = DX_PI * 0.5;
                pShot->muki = baseAngle + (GetRand(100) - 50) / 180.0 * DX_PI;

                // 砕けた衝撃で周囲に追加の小玉を少し飛ばす（粘性が飛び散るイメージ）
                int scatterNum = 2 + GetRand(2); // 2〜4個
                for (int k = 0; k < scatterNum*5; k++) {
                    sEnemyShot* pSc = new sEnemyShot;
                    pSc->x = pShot->x + (GetRand(10) - 5);
                    pSc->y = pShot->y + (GetRand(10) - 5);
                    pSc->muki = baseAngle + (GetRand(140) - 70) / 180.0 * DX_PI;
                    pSc->speed = 0.7 + GetRand(50) / 100.0;
                    pSc->kind = img_enemyShotSmallBall[7]; // 黒・小玉
                    pSc->param_i[0] = 3;                   // 3 = 砕け後の拡散弾
                    pSc->param_i[1] = 1;

                    pSc->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSc->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSc;
                    pEnemyShotSet->pEnemyShotHead->prev = pSc;
                }

                // 砕け音（軽め）
                if (CheckSoundMem(sound_enemyShot_light) == 0) {
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
            }
        }
        else if (pShot->param_i[0] == 3) {
            // 砕け後の拡散弾：普通に移動（画面外で自動消去される）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_PitchDrop_Grok()
{
    static int muki;
    static int patternStarted;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        patternStarted = 0;
    }
    else {
        // ゆっくり左右に移動（塊の印象を壊さない程度）
        enemy.x += 0.45 * (double)muki;
        if (enemy.x < 160.0) muki = 1;
        if (enemy.x > 320.0) muki = -1;
    }

    // パターン開始（敵登場直後に1回だけショットセットを生成）
    if (count == 30 && patternStarted == 0) {
        patternStarted = 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPitchDrop;
        pEnemyShotSet->x = 240.0;          // 塊の中心は画面中央固定
        pEnemyShotSet->y = 70.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}