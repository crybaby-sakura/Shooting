// enemyPat_Tmp.cpp
// TAS前提・超高難易度弾幕パターン「因果律の縫い針（カルマ・ニードル）」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数：因果律の縫い針
// ============================================================
static void ShotKarmaNeedle(sEnemyShotSet* pSet)
{
    int t = pSet->count; // メインルーチンで自動インクリメントされる値を使用

    // --- 演出: 開始時の予告音 ---
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ========================================================
    // Phase 1: リサジュ曲線の壁と回転する「縫い針」 (t = 0 ~ 240)
    // 視覚的には隙間のない壁に見えるが、数学的に計算された狭い隙間が回転している。
    // ========================================================
    if (t < 240) {
        if (t % 4 == 0) { // 4フレームごとにリング状に展開
            int num_bullets = 72;
            double gap_angle = (t * 0.04);       // 時間とともに回転する安全角度
            double gap_width = 0.12;             // TASのみが通れる極限の隙間（ラジアン）

            for (int i = 0; i < num_bullets; i++) {
                double angle = (i * 2.0 * DX_PI) / num_bullets;

                // 現在の弾の角度と安全角度の差を計算（0〜PIに正規化）
                double diff = fabs(angle - gap_angle);
                while (diff > DX_PI) diff = 2.0 * DX_PI - diff;

                // 安全隙間以外の場所にのみ弾を配置
                if (diff > gap_width) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = pSet->x;
                    pShot->y = pSet->y;
                    pShot->muki = angle;
                    pShot->speed = 3.2;
                    // 白(6)の小玉(2.5x2.5)で緻密な壁を表現
                    pShot->kind = img_enemyShotSmallBall[6];
                    pShot->margin = 480;

                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
            // 効果音の間引き再生（負荷と聴覚的演出のバランス）
            if (t % 20 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }
    // ========================================================
    // Phase 2: サブピクセル同期の回廊 (t = 240 ~ 420)
    // 正弦波で動く幅3ピクセルの隙間。自機はこれに完全に同期する円運動・速度切り替えを要求される。
    // ========================================================
    else if (t < 420) {
        if (t % 3 == 0) {
            // 安全ルート中心座標（正弦波で左右に振れる）
            double gap_x = 240.0 + 90.0 * sin(t * 0.07);
            double gap_width = 1.5; // 片側1.5px = 合計3pxの隙間。自機当たり判定(2x2)でかすりが必須レベル

            // 左側の壁（青(4)の鱗弾 4.0x3.0）
            for (double y_offset = -15.0; y_offset <= 15.0; y_offset += 5.0) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = gap_x - gap_width - 3.5;
                pShot->y = pSet->y + y_offset;
                pShot->muki = DX_PI / 2.0; // 下向き
                pShot->speed = 4.5;
                pShot->kind = img_enemyShotScale[4];
                pShot->margin = 480;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
            // 右側の壁
            for (double y_offset = -15.0; y_offset <= 15.0; y_offset += 5.0) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = gap_x + gap_width + 3.5;
                pShot->y = pSet->y + y_offset;
                pShot->muki = DX_PI / 2.0;
                pShot->speed = 4.5;
                pShot->kind = img_enemyShotScale[4];
                pShot->margin = 480;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }

            // 【TAS用ガイド】隙間の中心を走るシアン(3)の銃弾(5.0x2.0)
            // 人間には邪魔な弾に見えるが、TASはこの弾の中心座標に自機を完璧にトレースさせる
            sEnemyShot* pGuide = new sEnemyShot;
            pGuide->x = gap_x;
            pGuide->y = pSet->y - 20.0;
            pGuide->muki = DX_PI / 2.0;
            pGuide->speed = 4.5;
            pGuide->kind = img_enemyShotBullet[3];
            pGuide->margin = 480;

            pGuide->prev = pSet->pEnemyShotHead->prev;
            pGuide->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pGuide;
            pSet->pEnemyShotHead->prev = pGuide;
        }
    }
    // ========================================================
    // Phase 3: 位相ズレのレーザー (t = 420 ~ 540)
    // 放射状に広がるレーザーのうち、1つだけが「短レーザー」として処理され、
    // 当たり判定の発生フレームが1フレーム分ズレる（仕様上の位相ズレを模倣）
    // ========================================================
    else if (t < 540) {
        if (t % 15 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 8; i++) {
                double angle = (i * 2.0 * DX_PI) / 8.0 + (t * 0.015);

                if (i == 3) {
                    // 位相ズレ枠：中心から離れた位置に「短レーザー(64.0x4.0)」を配置
                    // TASはここが「通れる瞬間」であることを計算上で知っている
                    sEnemyShot* pLaser = new sEnemyShot;
                    pLaser->x = pSet->x + 120.0 * cos(angle);
                    pLaser->y = pSet->y + 120.0 * sin(angle);
                    pLaser->muki = angle + DX_PI; // 内側を向く
                    pLaser->speed = 0.0; // 発生位置に固定（または非常に低速）
                    // マゼンタ(5)の短レーザー
                    pLaser->kind = img_enemyShotLaser[5];
                    pLaser->margin = 480;

                    pLaser->prev = pSet->pEnemyShotHead->prev;
                    pLaser->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pLaser;
                    pSet->pEnemyShotHead->prev = pLaser;
                }
                else {
                    // 通常の致死レーザー
                    sEnemyShot* pLaser = new sEnemyShot;
                    pLaser->x = pSet->x;
                    pLaser->y = pSet->y;
                    pLaser->muki = angle;
                    pLaser->speed = 7.0;
                    // 赤(0)の短レーザー
                    pLaser->kind = img_enemyShotLaser[0];
                    pLaser->margin = 480;

                    pLaser->prev = pSet->pEnemyShotHead->prev;
                    pLaser->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pLaser;
                    pSet->pEnemyShotHead->prev = pLaser;
                }
            }
        }
    }
    // ========================================================
    // Phase 4: 最適化ルートの可視化（クライマックス） (t = 540)
    // ========================================================
    else if (t == 540) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // TASが通ってきた軌跡を讃えるかのように、美しい正多角形の弾幕を展開
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (i * 2.0 * DX_PI) / 36.0;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 4.0;
            // シアン(3)の菱形弾(4.5x2.5)で鋭く美しい収束を表現
            pShot->kind = img_enemyShotDiamond[3];
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ========================================================
    // 弾の移動処理
    // （メインルーチンでcount管理はされるが、座標更新はパターン関数内で行う仕様にあわせる）
    // ========================================================
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_Tmp()
{
    // 静的変数でマスターとなる弾幕セットを保持
    static sEnemyShotSet* pMasterSet = nullptr;

    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 600; // 10秒パターンに見合う耐久力

        // 弾幕セットの作成（このパターンでは1つだけ作成し、使い回す）
        pMasterSet = new sEnemyShotSet;
        pMasterSet->count = 0; // メインルーチンで自動+1される
        pMasterSet->patternFunc = ShotKarmaNeedle;
        pMasterSet->x = enemy.x;
        pMasterSet->y = enemy.y + 10.0;
        pMasterSet->muki = DX_PI / 2.0;
        pMasterSet->kind = 0;

        // 弾リストのヘッダ初期化
        pMasterSet->pEnemyShotHead = new sEnemyShot;
        pMasterSet->pEnemyShotHead->prev = pMasterSet->pEnemyShotHead;
        pMasterSet->pEnemyShotHead->next = pMasterSet->pEnemyShotHead;

        // 全体リストに連結
        pMasterSet->prev = enemyShotSetHead.prev;
        pMasterSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMasterSet;
        enemyShotSetHead.prev = pMasterSet;
    }
    else {
        // 敵の移動制御
        if (count < 120) {
            // 開始120フレーム(2秒)で中央(y=120)へ滑らかに降下
            enemy.y += (120.0 - enemy.y) * 0.05;
        }
        else {
            // 120フレーム以降は、サブピクセル単位の美しいリサジュ曲線ホバリング
            // TASの視覚的美学として、規則的だが複雑な軌道を描く
            enemy.x = 240.0 + 12.0 * sin(count * 0.03);
            enemy.y = 120.0 + 8.0 * cos(count * 0.04);
        }

        // 弾幕発生基準点を敵の位置に追従させる
        if (pMasterSet != nullptr) {
            pMasterSet->x = enemy.x;
            pMasterSet->y = enemy.y + 10.0;
        }
    }
}