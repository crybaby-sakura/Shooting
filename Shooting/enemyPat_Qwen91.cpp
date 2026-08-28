// enemyPat_Tmp.cpp
// slither.ioをモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

int enemy_param_i[16];

// ============================================================
// 1. 【出現・蛇行】ヘビの体を模した蛇行弾
// ============================================================
static void ShotSlitherBody(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // ヘビの体のように連なって出現させる
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 敵の進行方向（下）に対して、後ろ（上）に並ぶように配置
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y - i * 15.0;
            pEnemyShot->muki = DX_PI / 2.0; // 下向き
            pEnemyShot->speed = 2.5;

            // 緑(2)の中楕円弾と、青(4)の鱗弾を交互に使って体のうねりを表現
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotMediumOval[2] : img_enemyShotScale[4];

            // 蛇行運動用の位相パラメータ
            pEnemyShot->param_d[0] = i * 0.5;
            pEnemyShot->margin = 240;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 下方向へ進みつつ、横にサイン波でうねる
        pShot->x += pShot->speed * cos(pShot->muki) + sin(pShot->count * 0.15 + pShot->param_d[0]) * 2.0;
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 2. 【ブースト・加速】進行方向と逆向きに餌を撒き散らす
// ============================================================
static void ShotBoostScatter(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 6; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 進行方向と逆（おおよそ上向き）＋ばらつき
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;

            // 餌っぽい色：黄(1) または 橙(8) の小円弾
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 3. 【断末魔】死亡時に画面全体に大量の餌（光る点）をばら撒く
// ============================================================
static void ShotDeathScatter(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 大量の餌をばら撒く
        for (int i = 0; i < 80; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(120) - 60;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(120) - 60;

            double angle = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->muki = angle;

            // 滞留感を演出するため、初期速度は遅め〜中程度に設定
            pEnemyShot->speed = 1.0 + GetRand(150) / 100.0;

            // キラキラした餌の表現：白(6) または 黄(1) の小円弾
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[1];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 摩擦で減速させ、その場に滞留する「餌」の挙動を再現
        pShot->speed *= 0.98;
        if (pShot->speed < 0.4) {
            pShot->speed = 0.4; // 完全には止まらず、ゆっくり漂う
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_Slitherio_Qwen()
{
    // param_i[0]: phase (0: 蛇行, 1: ブースト, 2: 断末魔)
    // param_i[1]: shot_count (弾幕セットの識別用)
    // param_i[2]: phase開始時のcount記録用
    // param_i[3]: 断末魔発動済みフラグ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        enemy_param_i[0] = 0; // phase 0: 蛇行
        enemy_param_i[1] = 0;
        enemy_param_i[2] = 0;
        enemy_param_i[3] = 0;
    }

    // --------------------------------------------------------
    // Phase 0: 蛇行徘徊
    // --------------------------------------------------------
    if (enemy_param_i[0] == 0) {
        // 横に大きくうねりながら、ゆっくりと降りてくる
        enemy.x = 240.0 + 140.0 * sin((count - enemy_param_i[2]) * 0.03);
        enemy.y = 60.0 + (count - enemy_param_i[2]) * 0.4;

        // 定期的に「体」の弾幕セットを生成
        if (count % 20 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSlitherBody;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->kind = enemy_param_i[1]++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 一定時間経過、または画面下端に近づいたらブーストフェーズへ移行
        if (count - enemy_param_i[2] > 240 || enemy.y > 360.0) {
            enemy_param_i[0] = 1;
            enemy_param_i[2] = count; // ブースト開始時刻を記録
        }
    }
    // --------------------------------------------------------
    // Phase 1: ブースト（加速と餌撒き）
    // --------------------------------------------------------
    else if (enemy_param_i[0] == 1) {
        // プレイヤー方向へ加速しつつ、全体として下へ押し流される
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 0) {
            enemy.x += (dx / dist) * 4.5; // 加速
            enemy.y += (dy / dist) * 4.5 + 1.5;
        }

        // ブースト中は高頻度で後ろに餌を撒く
        if (count % 4 == 1) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotBoostScatter;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->kind = enemy_param_i[1]++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 一定時間経過、またはHPが0になったら断末魔へ
        if (count - enemy_param_i[2] > 180 || enemy.hp <= 0) {
            enemy_param_i[0] = 2;
            enemy_param_i[2] = count;
        }
    }
    // --------------------------------------------------------
    // Phase 2: 断末魔（死亡と大量の餌）
    // --------------------------------------------------------
    else if (enemy_param_i[0] == 2) {
        // 時間切れなどでHPが残っていた場合でも、ここで強制0にする
        if (enemy.hp > 0) {
            //enemy.hp = 0;
        }

        // 断末魔の弾幕発射は1回だけ行う
        if (enemy_param_i[3] == 0) {
            enemy_param_i[3] = 1; // 発動フラグON

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotDeathScatter;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->kind = 99; // 識別用

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 敵本体はその場に留め、点滅などの演出はメインルーチン側でHP==0の処理に委ねる

        if (count - enemy_param_i[2] > 180) {
            enemy_param_i[0] = 0;
            enemy_param_i[2] = count;
            enemy_param_i[3] = 0;
        }
    }
}