#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// Bubble Sort 弾幕
// 泡弾自体が配列要素。param_i[1] = 論理位置(0..N-1), param_i[0] = サイズ値(0..4), param_i[2] = 0:泡 / 1:攻撃弾
static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 8;
    const double LEFT_X = 60.0;
    const double SPACING = 50.0;
    const double BASE_Y = 110.0;
    const int COMPARE_WAIT = 24;
    const int SWAP_FRAMES = 30;
    const int PASS_WAIT = 40;

    // ローカルヘルパ: 攻撃弾生成
    auto CreateAttackShot = [&](double x, double y, double muki, double speed, int imgKind) {
        sEnemyShot* p = new sEnemyShot;
        p->x = x; p->y = y;
        p->muki = muki; p->speed = speed;
        p->kind = imgKind;
        p->param_i[0] = 0;
        p->param_i[1] = -1;
        p->param_i[2] = 1; // 攻撃弾
        p->margin = 20.0;
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    };

    // ローカルヘルパ: 論理位置から泡を探す
    auto FindBubbleByPos = [&](int pos) -> sEnemyShot* {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[2] == 0 && p->param_i[1] == pos) return p;
            p = p->next;
        }
        return nullptr;
    };

    // 初回生成
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = N;
        pEnemyShotSet->param_i[1] = 0; // pass
        pEnemyShotSet->param_i[2] = 0; // i
        pEnemyShotSet->param_i[3] = 0; // state 0:WAIT 1:SWAP 2:PASS_END 3:FINISH
        pEnemyShotSet->param_i[12] = 0; // swappedThisPass
        pEnemyShotSet->param_i[13] = COMPARE_WAIT; // wait counter
        pEnemyShotSet->param_d[0] = BASE_Y;
        pEnemyShotSet->param_d[1] = LEFT_X;
        pEnemyShotSet->param_d[2] = SPACING;
        pEnemyShotSet->param_d[3] = 0.0; // swapT

        for (int i = 0; i < N; i++) {
            sEnemyShot* p = new sEnemyShot;
            int sizeVal = GetRand(4); // 0..4 注意: x+1種類
            p->param_i[0] = sizeVal;
            p->param_i[1] = i; // 論理位置
            p->param_i[2] = 0; // 泡
            p->x = LEFT_X + i * SPACING;
            p->y = BASE_Y;
            p->muki = 0;
            p->speed = 0;
            p->margin = 60.0; // 泡は消えにくくする

            // サイズと色で見分けやすく - 必要なものだけ抜粋して使用
            // 小玉(2.5)、中玉(7.0)、大玉(20.0)を使い分け
            switch (sizeVal) {
            case 0: p->kind = img_enemyShotSmallBall[6]; break; // 白 小
            case 1: p->kind = img_enemyShotMediumBall[1]; break; // 黄 中
            case 2: p->kind = img_enemyShotMediumBall[2]; break; // 緑 中
            case 3: p->kind = img_enemyShotLargeBall[3]; break; // シアン 大
            case 4: p->kind = img_enemyShotLargeBall[5]; break; // マゼンタ 特大
            }

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
            pEnemyShotSet->param_i[4 + i] = sizeVal; // デバッグ用に保持
        }
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }

    int pass = pEnemyShotSet->param_i[1];
    int i = pEnemyShotSet->param_i[2];
    int state = pEnemyShotSet->param_i[3];

    // 攻撃弾は常に移動
    {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[2] == 1) {
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            p = p->next;
        }
    }

    if (state == 3) { // FINISH - 破裂後ゆっくり落下
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[2] == 0) {
                p->y += 0.6 + p->param_i[0] * 0.2;
            }
            p = p->next;
        }
        return;
    }

    if (state == 0) { // WAIT BEFORE COMPARE
        if (pEnemyShotSet->param_i[13] > 0) {
            pEnemyShotSet->param_i[13]--;
            // 比較中の2つを光らせる演出: 一時的に白に
            sEnemyShot* a = FindBubbleByPos(i);
            sEnemyShot* b = FindBubbleByPos(i + 1);
            // ゆらゆら
            if (a) a->y = BASE_Y + sin(pEnemyShotSet->count * 0.08 + a->param_i[1]) * 4.0;
            if (b) b->y = BASE_Y + sin(pEnemyShotSet->count * 0.08 + b->param_i[1]) * 4.0;
            return;
        }

        sEnemyShot* leftBubble = FindBubbleByPos(i);
        sEnemyShot* rightBubble = FindBubbleByPos(i + 1);
        if (!leftBubble || !rightBubble) {
            pEnemyShotSet->param_i[2]++;
            pEnemyShotSet->param_i[13] = COMPARE_WAIT / 2;
            return;
        }

        if (leftBubble->param_i[0] > rightBubble->param_i[0]) {
            // 要交換 -> 攻撃発生
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            double baseMukiLarge = atan2(player.y - leftBubble->y, player.x - leftBubble->x);
            for (int k = -1-5; k <= 1+5; k++) {
                double mk = baseMukiLarge + k * 0.35;
                int col = 4 + GetRand(1); // 青系
                // 鱗弾(4.0x3.0)で低速バラマキ
                CreateAttackShot(leftBubble->x, leftBubble->y, mk, 1.4 + GetRand(30) / 100.0, img_enemyShotScale[col]);
            }
            for (int k = 0; k < 10; k++) {
                // 小さい方は針弾(銃弾 5.0x2.0)を真下寄りに高速で1発
                double mkSmall = atan2(player.y - rightBubble->y, player.x - rightBubble->x);
                // 少し下にばらす
                mkSmall += (GetRand(40) - 20) / 180.0 * DX_PI;
                CreateAttackShot(rightBubble->x, rightBubble->y, mkSmall, 3.2 + GetRand(50) / 100.0, img_enemyShotBullet[0]);
            }

            pEnemyShotSet->param_i[3] = 1; // -> SWAP
            pEnemyShotSet->param_d[3] = 0.0;
        }
        else {
            // 交換なし
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            pEnemyShotSet->param_i[2]++; // i++
            pEnemyShotSet->param_i[13] = 12;
            if (pEnemyShotSet->param_i[2] >= N - 1 - pass) {
                pEnemyShotSet->param_i[3] = 2; // PASS_END
                pEnemyShotSet->param_i[13] = PASS_WAIT;
            }
        }
    }
    else if (state == 1) { // SWAPPING
        double t = pEnemyShotSet->param_d[3];
        t += 1.0 / SWAP_FRAMES;
        if (t > 1.0) t = 1.0;
        pEnemyShotSet->param_d[3] = t;

        sEnemyShot* a = FindBubbleByPos(i);
        sEnemyShot* b = FindBubbleByPos(i + 1);
        if (a && b) {
            double leftTx = LEFT_X + i * SPACING;
            double rightTx = LEFT_X + (i + 1) * SPACING;
            // 弧を描いて入れ替え
            double arc = sin(t * DX_PI) * 38.0;
            a->x = leftTx + (rightTx - leftTx) * t;
            a->y = BASE_Y - arc;
            b->x = rightTx + (leftTx - rightTx) * t;
            b->y = BASE_Y - arc * 0.8;
        }

        if (t >= 1.0) {
            // 論理位置を実際に交換
            sEnemyShot* a2 = FindBubbleByPos(i);
            sEnemyShot* b2 = FindBubbleByPos(i + 1);
            if (a2 && b2) {
                a2->param_i[1] = i + 1;
                b2->param_i[1] = i;
            }
            pEnemyShotSet->param_i[12] = 1; // swapped
            pEnemyShotSet->param_i[2]++; // next i
            pEnemyShotSet->param_i[3] = 0;
            pEnemyShotSet->param_i[13] = 14;
            pEnemyShotSet->param_d[3] = 0.0;

            if (pEnemyShotSet->param_i[2] >= N - 1 - pass) {
                pEnemyShotSet->param_i[3] = 2;
                pEnemyShotSet->param_i[13] = PASS_WAIT;
            }
        }
    }
    else if (state == 2) { // PASS END
        if (pEnemyShotSet->param_i[13] > 0) {
            pEnemyShotSet->param_i[13]--;
            return;
        }
        if (pEnemyShotSet->param_i[12] == 0) {
            // 交換なし -> ソート完了
            pEnemyShotSet->param_i[3] = 3;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            // 最後のご褒美全方位: 泡から中楕円弾(10.5x7.0)を放出
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[2] == 0) {
                    for (int d = 0; d < 6*3; d++) {
                        double mk = (DX_PI * 2.0 / 6.0/3) * d + (p->param_i[1] * 0.2);
                        CreateAttackShot(p->x, p->y, mk, 2.0 + p->param_i[0] * 0.15, img_enemyShotMediumOval[6]);
                    }
                }
                p = p->next;
            }
        }
        else {
            // 次のパスへ
            pEnemyShotSet->param_i[1]++; // pass++
            pEnemyShotSet->param_i[2] = 0; // i=0
            pEnemyShotSet->param_i[12] = 0;
            pEnemyShotSet->param_i[3] = 0;
            pEnemyShotSet->param_i[13] = COMPARE_WAIT;
            if (pEnemyShotSet->param_i[1] >= N - 1) {
                pEnemyShotSet->param_i[3] = 3;
            }
        }
    }

    // 交換中でない泡の位置補正
    if (state != 1) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[2] == 0) {
                double targetX = LEFT_X + p->param_i[1] * SPACING;
                p->x += (targetX - p->x) * 0.25;
                p->y = BASE_Y + sin(pEnemyShotSet->count * 0.07 + p->param_i[1] * 0.9) * 3.0;
            }
            p = p->next;
        }
    }
}

void EnemyPat_BubbleSort_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBubbleSort;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵本体は上部でゆっくり左右
        enemy.x = 240.0 + sin(count * 0.015) * 80.0;
        enemy.y = 60.0 + cos(count * 0.011) * 8.0;
    }
}