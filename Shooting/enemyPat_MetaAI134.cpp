#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 前方宣言（干渉処理で識別に使うため）
static void ShotKagomeCage(sEnemyShotSet* pEnemyShotSet);
static void ShotKagomeLarge(sEnemyShotSet* pEnemyShotSet);

// ============================================================
// 禁忌「カゴメカゴメ」: 緑小弾による格子・檻
// 仕様:
// - 格子状に緑小弾を設置しつつ黄大弾を撃つ【9220857794759288928†L198-L201】
// - 設置パターンは ↑↓←→, ↙, ↘ を繰り返す【9220857794759288928†L201-L207】
// - 設置後しばらくして崩れ始める【9220857794759288928†L198-L201】
// ============================================================
static void ShotKagomeCage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int STATIC_TIME = 90; // 静止時間

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int pattern = pEnemyShotSet->kind % 3; // 0:↑↓←→ 1:↙ 2:↘
        // 生成数は重くなりすぎないよう1セット20〜42個程度に抑える

        if (pattern == 0) {
            // ↑↓←→の組み合わせ: プレイヤー位置に十字を生成 = 囲い
            // 縦線
            for (int i = 0; i < 21; i++) {
                pEnemyShot = new sEnemyShot;
                while (true) {
                    pEnemyShot->x = player.x + (double)(GetRand(40) - 20); // 少しばらけさせる
                    pEnemyShot->y = (double)(i * 24 - 20);
                    if (hypot(pEnemyShot->x - player.x, pEnemyShot->y - player.y) > 10) break;
                }
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[2]; // 2:緑
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            // 横線
            for (int i = 0; i < 20; i++) {
                pEnemyShot = new sEnemyShot;
                while (true) {
                    pEnemyShot->x = (double)(i * 24 + 10);
                    pEnemyShot->y = player.y + (double)(GetRand(40) - 20);
                    if (hypot(pEnemyShot->x - player.x, pEnemyShot->y - player.y) > 10) break;
                }
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[2];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else if (pattern == 1) {
            // ↙ 斜め (右上→左下)
            double base = player.x + player.y; // x+y=const
            for (int i = -10; i <= 10; i++) {
                pEnemyShot = new sEnemyShot;
                while (true) {
                    pEnemyShot->x = player.x + i * 24.0 + (double)(GetRand(10) - 5);
                    pEnemyShot->y = base - pEnemyShot->x + (double)(GetRand(10) - 5);
                    if (hypot(pEnemyShot->x - player.x, pEnemyShot->y - player.y) > 10) break;
                }
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[2];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else {
            // ↘ 斜め (左上→右下)
            double base = player.x - player.y; // x-y=const
            for (int i = -10; i <= 10; i++) {
                pEnemyShot = new sEnemyShot;
                while (true) {
                    pEnemyShot->x = player.x + i * 24.0 + (double)(GetRand(10) - 5);
                    pEnemyShot->y = pEnemyShot->x - base + (double)(GetRand(10) - 5);
                    if (hypot(pEnemyShot->x - player.x, pEnemyShot->y - player.y) > 10) break;
                }
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[2];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 更新：90F経過後にゆっくり崩落
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count >= STATIC_TIME) {
            if (pShot->speed < 0.01) {
                // 落下開始時に速度を与える
                pShot->speed = 0.3 + (double)GetRand(60) / 100.0; // 0.3-0.9 原作の超低速
                pShot->muki = DX_PI / 2.0 + (double)(GetRand(40) - 20) / 180.0 * DX_PI;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 禁忌「カゴメカゴメ」: 黄大弾による破壊
// 仕様:
// - 大弾6発で1ループ、1,2,3,5がランダム、4,6が自機狙い
// - 3→4、5→6の間隔が短い
// - 緑弾の周期とは連動せず同時対応を強いられる
// ============================================================
static void ShotKagomeLarge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = pEnemyShotSet->param_d[0]; // 3.0前後
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 1:黄 大玉(20x20)
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // --- 弾幕干渉処理 ---
        // 大玉が緑小弾を押しのける = 原作の「打ち破るように大きな弾幕を放つ」
        // 5フレーム毎にチェックして負荷軽減
        if (pShot->count % 3 == 0) {
            sEnemyShotSet* pSet = enemyShotSetHead.next;
            while (pSet != &enemyShotSetHead) {
                if (pSet != pEnemyShotSet && pSet->pEnemyShotHead) {
                    sEnemyShot* pGreen = pSet->pEnemyShotHead->next;
                    // 緑小弾セットのみを対象（種類で判定）
                    while (pGreen != pSet->pEnemyShotHead) {
                        if (pGreen->kind == img_enemyShotSmallBall[2]) {
                            double dx = pGreen->x - pShot->x;
                            double dy = pGreen->y - pShot->y;
                            double d2 = dx * dx + dy * dy;
                            if (d2 < 900.0 && d2 > 0.1) { // 半径30以内
                                double d = sqrt(d2);
                                double push = (30.0 - d) * 0.12; // 押し出し量
                                pGreen->x += dx / d * push;
                                pGreen->y += dy / d * push;
                                // まだ静止中なら落下を開始させる
                                if (pGreen->speed < 0.01) {
                                    pGreen->speed = 0.4 + (double)GetRand(40) / 100.0;
                                }
                            }
                        }
                        pGreen = pGreen->next;
                    }
                }
                pSet = pSet->next;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_KagomeKagome_MetaAI()
{
    static int muki;
    static int cage_counter; // 格子生成カウンタ
    static int large_loop; // 黄大弾 0-5
    static int large_timer; // 次の大弾までの待機

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        cage_counter = 0;
        large_loop = 0;
        large_timer = 40; // 最初は少し待つ
    }
    else {
        // EXボスは中央付近で微移動
        enemy.x += 0.45 * (double)muki;
        if (count % 200 == 100) muki *= -1;
        if (enemy.x < 180.0) { enemy.x = 180.0; muki = 1; }
        if (enemy.x > 300.0) { enemy.x = 300.0; muki = -1; }
    }

    // --- 緑格子 檻の生成 ---
    // 45F毎に1セット
    if (count % 67 == 1 && count > 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKagomeCage;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = cage_counter % 3; // パターン法則を再現
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        cage_counter++;
    }

    // --- 黄大弾の生成 ---
    if (large_timer <= 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKagomeLarge;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->param_d[0] = 2.8 + (double)GetRand(40) / 100.0; // 2.8-3.2

        // 1,2,3,5発目がランダム、4,6発目が自機狙い
        double baseAim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        if (large_loop == 3 || large_loop == 5) {
            // 自機狙い
            pEnemyShotSet->muki = baseAim;
        }
        else {
            // ランダムだが自機方向を中心に±40度程度
            // GetRandは0〜x inclusiveなので GetRand(80)-40 で -40〜+40
            pEnemyShotSet->muki = baseAim + (double)(GetRand(80) - 40) / 180.0 * DX_PI;
        }
        pEnemyShotSet->kind = large_loop;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次までの間隔: 3→4と5→6は短い(10F)、他は24F
        if (large_loop == 2 || large_loop == 4) {
            large_timer = 10+5;
        }
        else {
            large_timer = 24+12;
        }
        large_loop = (large_loop + 1) % 6;
    }
    else {
        large_timer--;
    }
}