#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  焼き栗弾幕「爆ぜ栗・ほくほく」
//
//  param_i[0] の用途:
//     1 : 栗の輪郭（爆発時に外側へ飛散 / 湯気フェーズ中は揺らぐ）
//     2 : 底の針（爆発時に下向きへ飛散）
//     3 : 割れ目レーザー（爆発時に外側へ飛散）
//    10 : 湯気（上昇する白い小玉）
//    20 : 栗の実（中楕円弾）
//    21 : 剥けた皮（銃弾の扇状）
//    30 : 残り火（落下する小玉）
// ============================================================
static void ShotYakiGuri(sEnemyShotSet* pEnemyShotSet)
{
    const int    c = pEnemyShotSet->count;
    const double px = pEnemyShotSet->x;
    const double py = pEnemyShotSet->y;

    // 弾を1つ追加するヘルパ（元の位置を param_d[0], param_d[1] に保存）
    auto addShot = [&](double x, double y, double muki, double speed, int kind, int flag) {
        sEnemyShot* s = new sEnemyShot;
        s->x = x;
        s->y = y;
        s->muki = muki;
        s->speed = speed;
        s->kind = kind;
        s->param_i[0] = flag;
        s->param_d[0] = x;
        s->param_d[1] = y;
        s->prev = pEnemyShotSet->pEnemyShotHead->prev;
        s->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = s;
        pEnemyShotSet->pEnemyShotHead->prev = s;
    };

    // === Phase 1: 栗の形を作る ===
    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 上部の丸み（中玉・橙）
        addShot(px - 12.0, py - 18.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);
        addShot(px, py - 22.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);
        addShot(px + 12.0, py - 18.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);
        // 左右の膨らみ
        addShot(px - 18.0, py - 6.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);
        addShot(px + 18.0, py - 6.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);
        addShot(px - 18.0, py + 6.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);
        addShot(px + 18.0, py + 6.0, 0.0, 0.0, img_enemyShotMediumBall[8], 1);

        // 底の尖り（銃弾・黒を下向きに）
        addShot(px - 8.0, py + 16.0, DX_PI / 2.0, 0.0, img_enemyShotBullet[7], 2);
        addShot(px, py + 18.0, DX_PI / 2.0, 0.0, img_enemyShotBullet[7], 2);
        addShot(px + 8.0, py + 16.0, DX_PI / 2.0, 0.0, img_enemyShotBullet[7], 2);

        // 表面の焼き目（小玉・黒）をランダムに散らす
        for (int i = 0; i < 6; i++) {
            double ox = (GetRand(200) - 100) / 10.0;
            double oy = (GetRand(200) - 100) / 10.0;
            addShot(px + ox, py + oy, 0.0, 0.0, img_enemyShotSmallBall[7], 1);
        }

        // 割れ目（短レーザー・黄でV字）
        addShot(px - 6.0, py - 6.0, -DX_PI / 4.0, 0.0, img_enemyShotLaser[1], 3);
        addShot(px + 6.0, py - 6.0, DX_PI / 4.0, 0.0, img_enemyShotLaser[1], 3);
    }

    // === Phase 2: 湯気（上昇する白い小玉）＋予告音 ===
    if (c == 10) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    if (c >= 1 && c <= 60 && c % 6 == 0) {
        for (int i = 0; i < 2; i++) {
            double ox = (double)(GetRand(40) - 20);
            double oy = (double)(GetRand(20) - 10);
            double muki = -DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            double speed = 0.5 + GetRand(50) / 100.0;
            addShot(px + ox, py - 8.0 + oy, muki, speed, img_enemyShotSmallBall[6], 10);
        }
    }

    // === Phase 3: 爆ぜる ===
    if (c == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 栗の実（中楕円弾）を2波で全方位に放射（黄→橙）
        for (int wave = 0; wave < 2; wave++) {
            int    color = (wave == 0) ? 1 : 8;
            double speed = (wave == 0) ? 2.5 : 1.8;
            for (int i = 0; i < 16; i++) {
                double ang = i * 2.0 * DX_PI / 16.0 + wave * DX_PI / 16.0;
                addShot(px, py, ang, speed, img_enemyShotMediumOval[color], 20);
            }
        }

        // 皮が剥ける（銃弾・橙を下向き扇状に）
        const int N = 13;
        for (int i = 0; i < N; i++) {
            double ang = DX_PI / 2.0 + (i - N/2) * DX_PI / 12.0;
            addShot(px, py + 10.0, ang, 3.0, img_enemyShotBullet[8], 21);
        }
    }

    // === Phase 4: 残り火（落下する小玉） ===
    if (c > 60 && c < 180 && c % 15 == 0) {
        for (int i = 0; i < 2*3; i++) {
            double ox = (double)(GetRand(120) - 60);
            double muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            double speed = 1.0 + GetRand(100) / 100.0;
            int    color = GetRand(1) ? 8 : 1; // 橙 or 黄
            addShot(px + ox, py, muki, speed, img_enemyShotSmallBall[color], 30);
        }
    }

    // === 全弾の更新 ===
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 爆発の瞬間に速度・向きをセット
        if (c == 60) {
            if (pShot->param_i[0] == 1 || pShot->param_i[0] == 3) {
                // 輪郭・割れ目 → 外側へ
                double dx = pShot->param_d[0] - px;
                double dy = pShot->param_d[1] - py;
                if (dx == 0.0 && dy == 0.0) dx = 1.0;
                pShot->muki = atan2(dy, dx);
                pShot->speed = (pShot->param_i[0] == 1) ? 2.5 : 3.0;
            }
            else if (pShot->param_i[0] == 2) {
                // 底の針 → 下向き
                pShot->muki = DX_PI / 2.0;
                pShot->speed = 4.0;
            }
        }

        // 湯気フェーズ中の輪郭揺らぎ（焼けて震える演出）
        if (c >= 1 && c < 60 && (pShot->param_i[0] == 1 || pShot->param_i[0] == 3)) {
            pShot->x = pShot->param_d[0] + sin(c * 0.3) * 1.5;
            pShot->y = pShot->param_d[1];
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_RoastChestnut_DeepSeek()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で焼き栗弾幕を発動
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotYakiGuri;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
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