// enemyPat_kitaKazeTaiyou.cpp
// 北風と太陽をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の色定数（img_enemyShot***[色] のインデックス）
#define COLOR_RED     0
#define COLOR_YELLOW  1
#define COLOR_GREEN   2
#define COLOR_CYAN    3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_WHITE   6
#define COLOR_BLACK   7
#define COLOR_ORANGE  8

//------------------------------------------------
// 北風の弾幕：高速の風針と氷晶
//------------------------------------------------
static void ShotNorthWind(sEnemyShotSet* p)
{
    if (p->count == 0) {
        // 効果音：中
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 風の針弾（青い銃弾）を高速で横方向にばらまく
        for (int i = 0; i < 14; i++) {
            sEnemyShot* s = new sEnemyShot;

            s->x = p->x + GetRand(40) - 20;
            s->y = p->y + GetRand(30) - 15;

            // プレイヤーが左右どちらにいるかで基本角度を決める
            double baseAngle = (player.x > p->x) ? 0.0 : DX_PI;
            s->muki = baseAngle + (GetRand(50) - 25) / 180.0 * DX_PI;
            s->speed = (350 + GetRand(200)) / 100.0; // 3.5～5.5

            s->kind = img_enemyShotBullet[COLOR_BLUE]; // 青い銃弾

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }

        // 氷晶弾（シアンの鱗弾）をゆっくり渦のように
        for (int i = 0; i < 8; i++) {
            sEnemyShot* s = new sEnemyShot;

            s->x = p->x + GetRand(50) - 25;
            s->y = p->y + GetRand(50) - 25;

            s->muki = p->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            s->speed = (120 + GetRand(120)) / 100.0; // 1.2～2.4

            s->kind = img_enemyShotScale[COLOR_CYAN]; // シアンの鱗弾
            // 将来の曲線用パラメータ（今回は未使用）
            s->param_d[0] = (GetRand(100) - 50) / 100.0;

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }
    }

    // 弾の移動
    sEnemyShot* shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

//------------------------------------------------
// 太陽の弾幕：放射状の光弾と日輪
//------------------------------------------------
static void ShotSun(sEnemyShotSet* p)
{
    if (p->count == 0) {
        // 効果音：軽
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 放射状の光弾（黄色の小玉）を24方向に
        for (int i = 0; i < 24; i++) {
            sEnemyShot* s = new sEnemyShot;

            double angle = p->muki + (i * 15.0) / 180.0 * DX_PI; // 15°刻み
            s->x = p->x + 10.0 * cos(angle);
            s->y = p->y + 10.0 * sin(angle);
            s->muki = angle;
            s->speed = 1.5; // ゆっくり

            s->kind = img_enemyShotSmallBall[COLOR_YELLOW]; // 黄色の小玉

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }

        // 日輪リングを模したオレンジの中楕円弾を8方向に
        for (int i = 0; i < 8; i++) {
            sEnemyShot* s = new sEnemyShot;

            double angle = p->muki + (i * 45.0) / 180.0 * DX_PI; // 45°刻み
            s->x = p->x + 15.0 * cos(angle);
            s->y = p->y + 15.0 * sin(angle);
            s->muki = angle;
            s->speed = 0.8; // さらにゆっくり

            s->kind = img_enemyShotMediumOval[COLOR_ORANGE]; // オレンジの中楕円弾

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }
    }

    // 弾の移動
    sEnemyShot* shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

//------------------------------------------------
// 最終段階：北風と太陽の同時攻撃（十字弾幕）
//------------------------------------------------
static void ShotFinalCross(sEnemyShotSet* p)
{
    if (p->count == 0) {
        // 効果音：重
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 横方向の高速青針弾
        for (int i = 0; i < 16; i++) {
            sEnemyShot* s = new sEnemyShot;

            s->x = p->x;
            s->y = p->y + GetRand(60) - 30;
            s->muki = (GetRand(1) == 0) ? 0.0 : DX_PI; // 右か左
            s->speed = 4.0;

            s->kind = img_enemyShotBullet[COLOR_BLUE];

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }

        // 縦方向の黄色光弾
        for (int i = 0; i < 16; i++) {
            sEnemyShot* s = new sEnemyShot;

            s->x = p->x + GetRand(60) - 30;
            s->y = p->y;
            // 上か下（画面座標では下が正、上が負）
            s->muki = (GetRand(1) == 0) ? DX_PI / 2.0 : -DX_PI / 2.0;
            s->speed = 2.5;

            s->kind = img_enemyShotSmallBall[COLOR_YELLOW];

            s->prev = p->pEnemyShotHead->prev;
            s->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = s;
            p->pEnemyShotHead->prev = s;
        }
    }

    // 弾の移動
    sEnemyShot* shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);
        shot = shot->next;
    }
}

//------------------------------------------------
// 弾幕セットを生成してリストに追加する共通関数
//------------------------------------------------
static void AddShotSet(double x, double y, double muki, void(*patternFunc)(sEnemyShotSet*))
{
    sEnemyShotSet* p = new sEnemyShotSet;

    p->count = 0;
    p->patternFunc = patternFunc;
    p->x = x;
    p->y = y;
    p->muki = muki;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    // グローバルな敵弾幕リストへ挿入
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}

//------------------------------------------------
// 敵本体パターン
//------------------------------------------------
void EnemyPat_NorthWindAndSun_DeepSeek()
{
    static int northDir; // 北風の移動方向
    static int sunDir;   // 太陽の移動方向

    if (count == 1) {
        // 初期配置
        enemy.x = 120.0;
        enemy.y = 100.0;
        enemy.x2 = 360.0;
        enemy.y2 = 100.0;
        enemy.maxHp = enemy.hp = 200;
        northDir = 1;
        sunDir = -1;
    }
    else {
        // 北風（左側）を左右に往復移動
        enemy.x += 1.2 * northDir;
        if (enemy.x < 60.0 || enemy.x > 180.0) northDir *= -1;

        // 太陽（右側）を左右に往復移動
        enemy.x2 += 1.2 * sunDir;
        if (enemy.x2 < 300.0 || enemy.x2 > 420.0) sunDir *= -1;
    }

    // HPによってフェーズを切り替える
    int phase;
    if (enemy.hp > 130) {
        phase = 0; // 北風が優勢
    }
    else if (enemy.hp > 70) {
        phase = 1; // 太陽が優勢
    }
    else {
        phase = 2; // 両者同時
    }

    // フェーズごとに弾幕セットを生成
    switch (phase) {
    case 0: // 北風メイン
        if (count % 8 == 1) {
            AddShotSet(enemy.x, enemy.y,
                atan2(player.y - enemy.y, player.x - enemy.x),
                ShotNorthWind);
        }
        if (count % 20 == 11) {
            AddShotSet(enemy.x2, enemy.y2,
                atan2(player.y - enemy.y2, player.x - enemy.x2),
                ShotSun);
        }
        break;

    case 1: // 太陽メイン
        if (count % 6 == 1) {
            AddShotSet(enemy.x2, enemy.y2,
                atan2(player.y - enemy.y2, player.x - enemy.x2),
                ShotSun);
        }
        if (count % 18 == 11) {
            AddShotSet(enemy.x, enemy.y,
                atan2(player.y - enemy.y, player.x - enemy.x),
                ShotNorthWind);
        }
        break;

    case 2: // 最終段階：同時攻撃＋十字
        if (count % 6 == 1) {
            AddShotSet(enemy.x, enemy.y,
                atan2(player.y - enemy.y, player.x - enemy.x),
                ShotNorthWind);
        }
        if (count % 6 == 4) {
            AddShotSet(enemy.x2, enemy.y2,
                atan2(player.y - enemy.y2, player.x - enemy.x2),
                ShotSun);
        }
        if (count % 15 == 10) {
            AddShotSet(240.0, 100.0,
                atan2(player.y - 100.0, player.x - 240.0),
                ShotFinalCross);
        }
        break;
    }
}