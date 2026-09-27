#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：禁忌「カゴメカゴメ」
// 緑の中玉を籠目（六角格子）状に配置し、黄色の大玉で内側から弾き飛ばす
static void ShotKagomeKagome(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 籠目状に緑の中玉を配置（3重の同心リング・六角格子）
        // 中心付近を空けることで「籠の目」を表現
        for (int ring = 1; ring <= 3; ring++) {
            int num = ring * 6;              // 6, 12, 18 → 計36発
            double radius = ring * 45.0;     // 45, 90, 135 px
            for (int i = 0; i < num; i++) {
                double angle = (2.0 * DX_PI * i) / num + ring * 0.3;
                sEnemyShot* p = new sEnemyShot;
                p->x = 240.0 + radius * cos(angle);
                p->y = 200.0 + radius * sin(angle);
                p->muki = 0.0;
                p->speed = 0.0;              // 初期状態は静止
                p->kind = img_enemyShotMediumBall[2]; // 緑の中玉
                p->param_i[0] = 0;           // 0 = 籠目弾（緑）
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 黄色の大玉を自機狙い3wayで定期的に発射
    if (pEnemyShotSet->count >= 40 && (pEnemyShotSet->count - 40) % 40 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 3; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = atan2(player.y - p->y, player.x - p->x) + (i - 1) * 0.4;
            p->speed = 3.5;
            p->kind = img_enemyShotLargeBall[1]; // 黄色の大玉
            p->param_i[0] = 1;                   // 1 = 干渉用の黄色大玉
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 衝突判定：黄色大玉が静止中の緑中玉に触れたら弾き飛ばす
    // （大玉半径10 + 中玉半径3.5 = 13.5 を考慮し、余裕をみて20で判定）
    sEnemyShot* y = pEnemyShotSet->pEnemyShotHead->next;
    while (y != pEnemyShotSet->pEnemyShotHead) {
        if (y->param_i[0] == 1) {
            sEnemyShot* g = pEnemyShotSet->pEnemyShotHead->next;
            while (g != pEnemyShotSet->pEnemyShotHead) {
                if (g->param_i[0] == 0 && g->speed == 0.0) {
                    double dx = g->x - y->x;
                    double dy = g->y - y->y;
                    if (dx * dx + dy * dy < 20.0 * 20.0) {
                        // 黄色大玉の進行方向を基準に、緑弾を外側へ拡散
                        double base = atan2(dy, dx);
                        g->muki = base + (GetRand(60) - 30) / 180.0 * DX_PI;
                        g->speed = 2.0 + GetRand(200) / 100.0;
                    }
                }
                g = g->next;
            }
        }
        y = y->next;
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KagomeKagome_DeepSeek()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと。
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }
    // 240フレーム間隔でカゴメカゴメを発動
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKagomeKagome;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
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