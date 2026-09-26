#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// バブルソート弾幕：Bubble Sort Danmaku
// ============================================================

// 弾の状態
enum {
    BUBBLE_STATE_FALLING = 0, // 未ソート配列として落下中
    BUBBLE_STATE_SWAPPING = 1, // 交換アニメーション中
    BUBBLE_STATE_FIXED = 2,   // 右端に確定済み
    BUBBLE_STATE_FREE = 3     // 最終ウェーブなどの自由弾
};

// バブルソート本体のパターン関数
static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
{
    // クールダウンを減らす
    if (pEnemyShotSet->param_i[7] > 0) {
        pEnemyShotSet->param_i[7]--;
    }

    // --------------------------------------------------------
    // 初回初期化
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int N = 8; // 配列の要素数

        pEnemyShotSet->param_i[0] = N;      // N
        pEnemyShotSet->param_i[1] = 0;      // pass
        pEnemyShotSet->param_i[2] = 0;      // j
        pEnemyShotSet->param_i[3] = 1;      // phase: 1=comparing, 2=swapping, 3=pass done, 4=finished
        pEnemyShotSet->param_i[4] = 0;      // swap timer
        pEnemyShotSet->param_i[5] = N;      // remaining
        pEnemyShotSet->param_i[6] = 0;      // fixed count
        pEnemyShotSet->param_i[7] = 0;      // cooldown

        pEnemyShotSet->param_d[0] = 80.0;   // baseY（未ソート配列の基準Y）
        pEnemyShotSet->param_d[1] = 0.25*4;   // 落下速度
        pEnemyShotSet->param_d[2] = 60.0;   // 未ソート配列の左X
        pEnemyShotSet->param_d[3] = 360.0;  // 未ソート配列の右X
        pEnemyShotSet->param_d[4] = 440.0;  // 確定弾のX（右壁）
        pEnemyShotSet->param_d[5] = 80.0;   // 確定弾の開始Y
        pEnemyShotSet->param_d[6] = 30.0;   // 確定弾のY間隔

        // 値 1..N をランダムにシャッフル
        int values[16];
        for (int i = 0; i < N; i++) values[i] = i + 1;
        for (int i = N - 1; i > 0; i--) {
            int j = GetRand(i); // 0..i の整数
            int tmp = values[i];
            values[i] = values[j];
            values[j] = tmp;
        }

        double leftX = pEnemyShotSet->param_d[2];
        double rightX = pEnemyShotSet->param_d[3];
        double spacing = (rightX - leftX) / (N - 1);

        for (int i = 0; i < N; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = leftX + i * spacing;
            p->y = pEnemyShotSet->param_d[0];
            p->muki = 0.0;
            p->speed = 0.0;
            // 値に応じて色を変える（1..8 → img_enemyShotMediumBall[0..7]）
            p->kind = img_enemyShotMediumBall[(values[i] - 1) % 8];
            p->param_i[0] = values[i]; // 値
            p->param_i[1] = i;         // 現在の配列インデックス
            p->param_i[2] = BUBBLE_STATE_FALLING;
            p->param_d[0] = p->x; // 交換開始X
            p->param_d[1] = p->y; // 交換開始Y
            p->param_d[2] = p->x; // 交換目標X
            p->param_d[3] = p->y; // 交換目標Y
            p->param_d[4] = 0.0;  // 弧の高さ
            p->margin = 240;

            // リストに接続
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        return;
    }

    // --------------------------------------------------------
    // 共通更新
    // --------------------------------------------------------
    int N = pEnemyShotSet->param_i[0];
    int phase = pEnemyShotSet->param_i[3];
    int pass = pEnemyShotSet->param_i[1];
    int j = pEnemyShotSet->param_i[2];
    int remaining = pEnemyShotSet->param_i[5];
    int fixedCount = pEnemyShotSet->param_i[6];

    // 未ソート配列の基準Yを更新（終了後は止める）
    if (phase != 4) {
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1];
    }
    double baseY = pEnemyShotSet->param_d[0];

    // --------------------------------------------------------
    // 状態機械
    // --------------------------------------------------------
    if (phase == 1) { // 比較フェーズ
        if (pEnemyShotSet->param_i[7] == 0) {
            sEnemyShot* shotA = nullptr;
            sEnemyShot* shotB = nullptr;
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[2] == BUBBLE_STATE_FALLING) {
                    if (p->param_i[1] == j) shotA = p;
                    if (p->param_i[1] == j + 1) shotB = p;
                }
                p = p->next;
            }

            if (shotA && shotB) {
                if (shotA->param_i[0] > shotB->param_i[0]) {
                    // 交換が必要
                    shotA->param_i[2] = BUBBLE_STATE_SWAPPING;
                    shotB->param_i[2] = BUBBLE_STATE_SWAPPING;

                    shotA->param_d[0] = shotA->x;
                    shotA->param_d[1] = shotA->y;
                    shotB->param_d[0] = shotB->x;
                    shotB->param_d[1] = shotB->y;

                    // インデックスを即交換
                    int tmpIdx = shotA->param_i[1];
                    shotA->param_i[1] = shotB->param_i[1];
                    shotB->param_i[1] = tmpIdx;

                    double leftX = pEnemyShotSet->param_d[2];
                    double rightX = pEnemyShotSet->param_d[3];
                    double spacing = (remaining > 1) ? (rightX - leftX) / (remaining - 1) : 0.0;

                    shotA->param_d[2] = leftX + shotA->param_i[1] * spacing;
                    shotB->param_d[2] = leftX + shotB->param_i[1] * spacing;
                    shotA->param_d[3] = baseY;
                    shotB->param_d[3] = baseY;
                    shotA->param_d[4] = 40.0;
                    shotB->param_d[4] = 40.0;

                    pEnemyShotSet->param_i[4] = 0; // swap timer
                    pEnemyShotSet->param_i[3] = 2; // swapping

                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
                else {
                    // 交換不要
                    pEnemyShotSet->param_i[7] = 4;
                    j++;
                    if (j >= remaining - 1) {
                        pEnemyShotSet->param_i[3] = 3; // pass done
                    }
                    else {
                        pEnemyShotSet->param_i[2] = j;
                    }
                }
            }
        }
    }
    else if (phase == 2) { // 交換アニメーション
        int swapTimer = pEnemyShotSet->param_i[4];
        const int SWAP_FRAMES = 15;
        swapTimer++;
        pEnemyShotSet->param_i[4] = swapTimer;
        double t = (double)swapTimer / SWAP_FRAMES;
        if (t > 1.0) t = 1.0;

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[2] == BUBBLE_STATE_SWAPPING) {
                double startX = p->param_d[0];
                double startY = p->param_d[1];
                double targetX = p->param_d[2];
                double targetY = p->param_d[3];
                double arc = p->param_d[4];
                p->x = startX + (targetX - startX) * t;
                p->y = startY + (targetY - startY) * t - arc * sin(DX_PI * t);
            }
            p = p->next;
        }

        if (swapTimer >= SWAP_FRAMES) {
            p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[2] == BUBBLE_STATE_SWAPPING) {
                    p->param_i[2] = BUBBLE_STATE_FALLING;
                    p->x = p->param_d[2];
                    p->y = p->param_d[3];
                }
                p = p->next;
            }
            pEnemyShotSet->param_i[7] = 4;
            j++;
            if (j >= remaining - 1) {
                pEnemyShotSet->param_i[3] = 3;
            }
            else {
                pEnemyShotSet->param_i[2] = j;
                pEnemyShotSet->param_i[3] = 1;
            }
        }
    }
    else if (phase == 3) { // パス終了 → 最大値を確定
        sEnemyShot* maxShot = nullptr;
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[2] == BUBBLE_STATE_FALLING && p->param_i[1] == remaining - 1) {
                maxShot = p;
                break;
            }
            p = p->next;
        }
        if (maxShot) {
            maxShot->param_i[2] = BUBBLE_STATE_FIXED;
            maxShot->x = pEnemyShotSet->param_d[4];
            //maxShot->y = pEnemyShotSet->param_d[5] + fixedCount * pEnemyShotSet->param_d[6];
            fixedCount++;
            pEnemyShotSet->param_i[6] = fixedCount;

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        remaining--;
        pEnemyShotSet->param_i[5] = remaining;

        if (remaining <= 1) {
            // 最後の1つを確定
            p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[2] == BUBBLE_STATE_FALLING) {
                    p->param_i[2] = BUBBLE_STATE_FIXED;
                    p->x = pEnemyShotSet->param_d[4];
                    //p->y = pEnemyShotSet->param_d[5] + fixedCount * pEnemyShotSet->param_d[6];
                    fixedCount++;
                    break;
                }
                p = p->next;
            }
            pEnemyShotSet->param_i[6] = fixedCount;
            pEnemyShotSet->param_i[3] = 4; // finished
        }
        else {
            // 次のパスへ
            pass++;
            pEnemyShotSet->param_i[1] = pass;
            pEnemyShotSet->param_i[2] = 0;
            pEnemyShotSet->param_i[3] = 1;
            pEnemyShotSet->param_i[7] = 8;

            // 残りの未ソート弾を均等に再配置
            double leftX = pEnemyShotSet->param_d[2];
            double rightX = pEnemyShotSet->param_d[3];
            double spacing = (remaining > 1) ? (rightX - leftX) / (remaining - 1) : 0.0;
            p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[2] == BUBBLE_STATE_FALLING) {
                    p->x = leftX + p->param_i[1] * spacing;
                    p->y = baseY;
                }
                p = p->next;
            }
        }
    }
    else if (phase == 4) { // 終了：確定弾から左向きの最終ウェーブ
        if (pEnemyShotSet->count % 29 == 0) {
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[2] == BUBBLE_STATE_FIXED) {
                    sEnemyShot* bullet = new sEnemyShot;
                    bullet->x = p->x;
                    bullet->y = p->y;
                    bullet->muki = DX_PI; // 左
                    bullet->speed = 3.0;
                    bullet->kind = img_enemyShotSmallBall[0]; // 赤
                    bullet->param_i[2] = BUBBLE_STATE_FREE;
                    bullet->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    bullet->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = bullet;
                    pEnemyShotSet->pEnemyShotHead->prev = bullet;
                }
                p = p->next;
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --------------------------------------------------------
    // 各弾の更新（落下中・自由弾）
    // --------------------------------------------------------
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->param_i[2] == BUBBLE_STATE_FALLING) {
            p->y = baseY;
        }
        else if (p->param_i[2] == BUBBLE_STATE_FREE) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_BubbleSort_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 400フレームごとにバブルソート弾幕を発生
    if (count % 400 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleSort;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}