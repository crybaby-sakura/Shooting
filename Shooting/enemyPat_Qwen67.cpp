// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------------------------
// 弾幕パターン1：回転レーザー檻 (Rotating Laser Cage)
// 敵を中心に16本のレーザーを配置し、回転させ続ける
// -----------------------------------------------------------------------------
static void ShotLaserCage(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 初期化：16本のレーザーを生成
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = enemy.x;
            p->y = enemy.y;
            p->muki = (DX_PI * 2.0 / 16.0) * i;
            p->speed = 0.0; // 移動しない
            p->kind = img_enemyShotLaser[0]; // 赤レーザー

            // リストへ挿入
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 更新：回転と位置固定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 回転速度 (難易度調整: 0.025は比較的速い)
        pShot->muki += 0.025;

        // 位置は敵に追従 (あるいは固定)
        pShot->x = enemy.x;
        pShot->y = enemy.y;

        // speed=0なので座標更新は不要だが、メインルーチン側での挙動に合わせておく
        // (メインルーチンが x += speed*cos... を行う場合、speed=0なら動かない)

        pShot = pShot->next;
    }
}

// -----------------------------------------------------------------------------
// 弾幕パターン2：未来確定マーカー (Future Marker -> Spiral Explosion)
// プレイヤーの未来位置にマーカーを置き、時間差で螺旋弾を放出する
// -----------------------------------------------------------------------------
static double prev_px = 0, prev_py = 0;
static double prev_prev_px = 0, prev_prev_py = 0;
static void ShotFutureMarker(sEnemyShotSet* pEnemyShotSet)
{
    const int DELAY_FRAMES = 45; // 爆発までの猶予 (0.75秒)

    if (pEnemyShotSet->count == 0) {
        // 初期化：プレイヤーの速度を予測してマーカーを配置
        double vx = player.x - prev_prev_px;
        double vy = player.y - prev_prev_py;

        // 速度が速すぎる場合のノイズ対策
        double vlen = sqrt(vx * vx + vy * vy);
        if (vlen > 8.0) { vx *= 8.0 / vlen; vy *= 8.0 / vlen; }

        // 45フレーム後の予測座標
        double tx = player.x + vx * DELAY_FRAMES;
        double ty = player.y + vy * DELAY_FRAMES;

        // 画面内に収める
        if (tx < 20) tx = 20; if (tx > 460) tx = 460;
        if (ty < 20) ty = 20; if (ty > 460) ty = 460;

        // マーカー弾を1つだけ生成
        sEnemyShot* p = new sEnemyShot;
        p->x = tx;
        p->y = ty;
        p->muki = 0;
        p->speed = 0.0;
        p->kind = img_enemyShotLargeBall[5]; // マゼンタ大玉

        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;

        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else if (pEnemyShotSet->count == DELAY_FRAMES) {
        // 起爆：マーカーを消去し、螺旋弾を生成
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1. マーカー削除
        sEnemyShot* pMarker = pEnemyShotSet->pEnemyShotHead->next;
        if (pMarker != pEnemyShotSet->pEnemyShotHead) {
            double cx = pMarker->x;
            double cy = pMarker->y;

            pMarker->prev->next = pMarker->next;
            pMarker->next->prev = pMarker->prev;
            delete pMarker;

            // 2. 螺旋弾生成 (48方向)
            for (int i = 0; i < 48; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = cx;
                p->y = cy;
                // 初期角度
                p->muki = (DX_PI * 2.0 / 48.0) * i;
                p->speed = 3.2;
                p->kind = img_enemyShotSmallBall[6]; // 白小玉

                // 螺旋用のパラメータを保存 (旋回角速度)
                p->param_d[0] = 0.04; // 曲がり具合

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
        // 生成フレームは移動処理を行わず終了 (次のフレームから動く)
        return;
    }
    else if (pEnemyShotSet->count > DELAY_FRAMES) {
        // 移動：螺旋運動
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 旋回
            pShot->muki += pShot->param_d[0];
            // 直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// -----------------------------------------------------------------------------
// 弾幕パターン3：誘導分裂弾 (Homing -> Split)
// ゆっくり追跡し、一定時間後に8方向へ分裂する
// -----------------------------------------------------------------------------
static void ShotHomingSplit(sEnemyShotSet* pEnemyShotSet)
{
    const int SPLIT_FRAMES = 120; // 分裂までの時間 (2秒)

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        sEnemyShot* p = new sEnemyShot;
        p->x = enemy.x;
        p->y = enemy.y + 20;
        p->muki = atan2(player.y - p->y, player.x - p->x);
        p->speed = 2.0;
        p->kind = img_enemyShotDiamond[4]; // 青菱形

        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }
    else if (pEnemyShotSet->count == SPLIT_FRAMES) {
        // 分裂処理
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1. 本体削除
        sEnemyShot* pSelf = pEnemyShotSet->pEnemyShotHead->next;
        if (pSelf != pEnemyShotSet->pEnemyShotHead) {
            double cx = pSelf->x;
            double cy = pSelf->y;

            pSelf->prev->next = pSelf->next;
            pSelf->next->prev = pSelf->prev;
            delete pSelf;

            // 2. 8方向弾生成
            for (int i = 0; i < 8; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = cx;
                p->y = cy;
                p->muki = (DX_PI * 2.0 / 8.0) * i;
                p->speed = 4.0; // 高速
                p->kind = img_enemyShotBullet[1]; // 黄銃弾

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
        return;
    }
    else if (pEnemyShotSet->count > SPLIT_FRAMES) {
        // 分裂後の弾移動
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
    else {
        // 誘導フェーズ
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 自機への角度差分
            double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target_muki - pShot->muki;

            // 角度正規化 (-PI ～ PI)
            while (diff > DX_PI) diff -= DX_PI * 2;
            while (diff < -DX_PI) diff += DX_PI * 2;

            // 旋回 (誘導性能)
            double turn_rate = 0.03;
            if (diff > turn_rate) pShot->muki += turn_rate;
            else if (diff < -turn_rate) pShot->muki -= turn_rate;
            else pShot->muki = target_muki;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// -----------------------------------------------------------------------------
// 敵本体パターン：ラプラスの最終定理
// -----------------------------------------------------------------------------
void EnemyPat_TheHardest_Qwen()
{
    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 長持ちさせる
        prev_px = player.x;
        prev_py = player.y;
        prev_prev_px = player.x;
        prev_prev_py = player.y;

        // 1. 回転レーザー檻 (常時展開)
        sEnemyShotSet* pSetLaser = new sEnemyShotSet;
        pSetLaser->count = 0;
        pSetLaser->patternFunc = ShotLaserCage;
        pSetLaser->x = enemy.x;
        pSetLaser->y = enemy.y;
        pSetLaser->pEnemyShotHead = new sEnemyShot;
        pSetLaser->pEnemyShotHead->prev = pSetLaser->pEnemyShotHead;
        pSetLaser->pEnemyShotHead->next = pSetLaser->pEnemyShotHead;
        pSetLaser->prev = enemyShotSetHead.prev;
        pSetLaser->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetLaser;
        enemyShotSetHead.prev = pSetLaser;
    }
    else {
        // 敵の移動 (ゆっくり左右)
        enemy.x = 240.0 + 120.0 * sin(count * 0.008);
    }

    // 2. 未来確定マーカー (45フレーム = 0.75秒ごとに生成)
    // 難易度調整: 間隔を短くすると予測地点が埋め尽くされる
    if (count % 45 == 0) {
        sEnemyShotSet* pSetMarker = new sEnemyShotSet;
        pSetMarker->count = 0;
        pSetMarker->patternFunc = ShotFutureMarker;
        pSetMarker->x = enemy.x;
        pSetMarker->y = enemy.y;
        pSetMarker->pEnemyShotHead = new sEnemyShot;
        pSetMarker->pEnemyShotHead->prev = pSetMarker->pEnemyShotHead;
        pSetMarker->pEnemyShotHead->next = pSetMarker->pEnemyShotHead;
        pSetMarker->prev = enemyShotSetHead.prev;
        pSetMarker->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetMarker;
        enemyShotSetHead.prev = pSetMarker;
    }

    // 3. 誘導分裂弾 (120フレーム = 2秒ごとに生成)
    if (count % 120 == 60) {
        sEnemyShotSet* pSetHoming = new sEnemyShotSet;
        pSetHoming->count = 0;
        pSetHoming->patternFunc = ShotHomingSplit;
        pSetHoming->x = enemy.x;
        pSetHoming->y = enemy.y;
        pSetHoming->pEnemyShotHead = new sEnemyShot;
        pSetHoming->pEnemyShotHead->prev = pSetHoming->pEnemyShotHead;
        pSetHoming->pEnemyShotHead->next = pSetHoming->pEnemyShotHead;
        pSetHoming->prev = enemyShotSetHead.prev;
        pSetHoming->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetHoming;
        enemyShotSetHead.prev = pSetHoming;
    }

    // プレイヤー予測用変数更新
    prev_prev_px = prev_px;
    prev_prev_py = prev_py;
    prev_px = player.x;
    prev_py = player.y;
}