// EnemyPat_PopUpAds_MetaAI.cpp - ポップアップ広告弾幕
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==========================================
// ポップアップ1枚の挙動
// ==========================================
static void ShotPopupWindow(sEnemyShotSet* pSet)
{
    // pSet->param_i の使い方
    // [1] = width
    // [2] = height
    // [3] = generation (0:大 1:中 2:小 これ以上分裂しない)
    // [4] = 分裂済みフラグ 0:未 1:済
    // pSet->param_d の使い方
    // [0] = targetX
    // [1] = targetY

    // pShot->param_i の使い方
    // [0] = role 0:枠 1:タイトルバー 2:✕ボタン 3:テキスト 4:OKボタン 10:OKから発射された自由弾
    // [1] = 1ならウィンドウに吸着、0なら自由移動
    // pShot->param_d
    // [0],[1] = ウィンドウ中心からのオフセット

    auto AddPart = [](sEnemyShotSet* set, double offX, double offY, int imgKind, int role, int sticky) {
        sEnemyShot* p = new sEnemyShot;
        p->x = set->x + offX;
        p->y = set->y + offY;
        p->muki = 0.0;
        p->speed = 0.0;
        p->kind = imgKind;
        p->param_i[0] = role;
        p->param_i[1] = sticky;
        p->param_d[0] = offX;
        p->param_d[1] = offY;
        p->prev = set->pEnemyShotHead->prev;
        p->next = set->pEnemyShotHead;
        set->pEnemyShotHead->prev->next = p;
        set->pEnemyShotHead->prev = p;
    };

    if (pSet->count == 0) {
        // --- 生成時 ---
        int gen = pSet->param_i[3];
        int w = 0, h = 0;
        if (gen == 0) { w = 120 + GetRand(20); h = 80 + GetRand(15); }
        else if (gen == 1) { w = 84 + GetRand(15); h = 56 + GetRand(10); }
        else { w = 52 + GetRand(10); h = 36 + GetRand(8); }

        pSet->param_i[1] = w;
        pSet->param_i[2] = h;
        pSet->param_i[4] = 0;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 素材選定
        // 枠: 小玉 白(6) 2.5x2.5 一番小さく枠線に最適
        // タイトルバー: 中玉 青(4) 7x7 少し大きくベタ塗りに見える
        // ✕ボタン: 中玉 赤(0) 危険色で目立つ
        // テキスト: 小玉 黄(1) / 橙(8) 広告っぽい
        // OKボタン: 中楕円弾 緑(2) 10.5x7.0 ボタンらしい形状
        // 発射弾: 小玉 シアン(3) OKボタンから出る弾と区別

        // 1. 枠線
        for (int x = -w / 2; x <= w / 2; x += 8) {
            AddPart(pSet, (double)x, (double)-h / 2, img_enemyShotSmallBall[6], 0, 1); // 上
            AddPart(pSet, (double)x, (double)h / 2, img_enemyShotSmallBall[6], 0, 1); // 下
        }
        for (int y = -h / 2 + 8; y <= h / 2 - 8; y += 8) {
            AddPart(pSet, (double)-w / 2, (double)y, img_enemyShotSmallBall[6], 0, 1); // 左
            AddPart(pSet, (double)w / 2, (double)y, img_enemyShotSmallBall[6], 0, 1); // 右
        }

        // 2. タイトルバー (青で埋める)
        for (int yy = -h / 2 + 2; yy <= -h / 2 + 12; yy += 7) {
            for (int xx = -w / 2 + 4; xx <= w / 2 - 16; xx += 9) {
                AddPart(pSet, (double)xx, (double)yy, img_enemyShotMediumBall[4], 1, 1);
            }
        }

        // 3. ✕ボタン (罠)
        AddPart(pSet, (double)w / 2 - 10, (double)-h / 2 + 7, img_enemyShotMediumBall[0], 2, 1);

        // 4. OKボタン
        AddPart(pSet, 0.0, (double)h / 2 - 11, img_enemyShotMediumOval[2], 4, 1);

        // 5. 偽テキスト $$$!!! 的な
        int textNum = 5 + GetRand(3);
        for (int i = 0; i < textNum; i++) {
            double ox = (double)(GetRand(w - 20) - (w - 20) / 2);
            double oy = (double)(GetRand(h - 32) - (h - 32) / 2 + 8);
            int col = (i % 2 == 0) ? 1 : 8; // 黄と橙交互
            AddPart(pSet, ox, oy, img_enemyShotSmallBall[col], 3, 1);
        }
    }
    else {
        // --- 毎フレーム更新 ---
        double tx = pSet->param_d[0];
        double ty = pSet->param_d[1];
        pSet->x += (tx - pSet->x) * 0.06;
        pSet->y += (ty - pSet->y) * 0.06;

        sEnemyShot* pXShot = nullptr;
        sEnemyShot* pOKShot = nullptr;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[1] == 1) { // 吸着弾
                pShot->x = pSet->x + pShot->param_d[0];
                pShot->y = pSet->y + pShot->param_d[1];
                if (pShot->param_i[0] == 2) pXShot = pShot;
                if (pShot->param_i[0] == 4) pOKShot = pShot;
            }
            else { // 自由弾
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        // --- ✕ボタンに触れたら分裂 ---
        if (pXShot != nullptr && pSet->param_i[4] == 0 && pSet->count > 40 && pSet->param_i[3] < 2) {
            sPlayerShot* pPlayerShot = playerShotHead.next;
            while (pPlayerShot != &playerShotHead) {
                double dx = pPlayerShot->x - pXShot->x;
                double dy = pPlayerShot->y - pXShot->y;
                if (dx * dx + dy * dy < 21.0 * 21.0) {
                    // 2つに分裂生成
                    for (int k = 0; k < 2; k++) {
                        sEnemyShotSet* ns = new sEnemyShotSet;
                        ns->count = 0;
                        ns->patternFunc = ShotPopupWindow;
                        ns->x = pSet->x + (GetRand(40) - 20);
                        ns->y = pSet->y + (GetRand(40) - 20);
                        ns->muki = 0.0;
                        ns->kind = 0;
                        ns->param_i[3] = pSet->param_i[3] + 1; // generation++
                        ns->param_d[0] = player.x + (GetRand(120) - 60) + (k == 0 ? -35 : 35);
                        ns->param_d[1] = player.y + (GetRand(80) - 40);
                        // 画面内に収める
                        //if (ns->param_d[0] < 60) ns->param_d[0] = 60;
                        //if (ns->param_d[0] > 420) ns->param_d[0] = 420;
                        //if (ns->param_d[1] < 60) ns->param_d[1] = 60;
                        //if (ns->param_d[1] > 380) ns->param_d[1] = 380;

                        ns->pEnemyShotHead = new sEnemyShot;
                        ns->pEnemyShotHead->prev = ns->pEnemyShotHead;
                        ns->pEnemyShotHead->next = ns->pEnemyShotHead;

                        ns->prev = enemyShotSetHead.prev;
                        ns->next = &enemyShotSetHead;
                        enemyShotSetHead.prev->next = ns;
                        enemyShotSetHead.prev = ns;
                    }

                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    // 自分自身を削除 (メインループがnextをキャッシュしている前提の安全な削除)
                    sEnemyShot* cur = pSet->pEnemyShotHead->next;
                    while (cur != pSet->pEnemyShotHead) {
                        sEnemyShot* nxt = cur->next;
                        delete cur;
                        cur = nxt;
                    }
                    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;

                    pSet->next->prev = pSet->prev;
                    pSet->prev->next = pSet->next;
                    delete pSet->pEnemyShotHead;
                    delete pSet;

                    pPlayerShot->prev->next = pPlayerShot->next;
                    pPlayerShot->next->prev = pPlayerShot->prev;
                    delete pPlayerShot;

                    return; // このSetはもう無いので即return
                }
                pPlayerShot = pPlayerShot->next;
            }
        }

        // --- OKボタンから自機狙い ---
        if (pOKShot != nullptr && pSet->count > 90 && pSet->count % 28 == 0) {
            sEnemyShot* b = new sEnemyShot;
            b->x = pOKShot->x;
            b->y = pOKShot->y;
            b->muki = atan2(player.y - b->y, player.x - b->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
            b->speed = 2.0 + GetRand(80) / 100.0; // 2.0〜2.8
            b->kind = img_enemyShotSmallBall[3]; // シアン小玉
            b->param_i[0] = 10;
            b->param_i[1] = 0; // 自由移動

            b->prev = pSet->pEnemyShotHead->prev;
            b->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = b;
            pSet->pEnemyShotHead->prev = b;

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }
}

// ==========================================
// 敵本体
// ==========================================
void EnemyPat_PopUpAds_MetaAI()
{
    static int muki;
    static int spawnCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        spawnCount = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 80 || enemy.x > 400) muki *= -1;
    }

    // 一定間隔でポップアップを生成
    if (count % 85 == 1) {
        // 同時存在数を制限 (重くなりすぎ防止)
        int setNum = 0;
        for (sEnemyShotSet* s = enemyShotSetHead.next; s != &enemyShotSetHead; s = s->next) setNum++;
        if (setNum > 10) return;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPopupWindow;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_i[3] = 0; // generation 0

        // GetRand(x)は0〜xを返すので -100〜100 は GetRand(200)-100
        pSet->param_d[0] = 80.0 + GetRand(320); // targetX 80〜400
        pSet->param_d[1] = 100.0 + GetRand(200); // targetY 100〜300

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        spawnCount++;
    }
}