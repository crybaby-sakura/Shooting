// enemyPat_EbbinghausCage.cpp
// エビングハウス錯視をモチーフにした弾幕パターン
// 敵本体の関数名: void EnemyPat_Ebbinghaus_DeepSeek()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------- 前方宣言 ----------
static void ShotCage(sEnemyShotSet* pEnemyShotSet);
static void ShotAimedTripleLarge(sEnemyShotSet* pEnemyShotSet);
static void ShotScatterExplode(sEnemyShotSet* pEnemyShotSet);
static void CreateScatter(double x, double y);
static void RemoveShotSet(sEnemyShotSet* set);

// ---------- 弾幕パターン関数 ----------

// 檻（おり）のパターン：回転する円陣を形成し、収縮フラグで中心へ縮む
static void ShotCage(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int type = pEnemyShotSet->kind % 2;   // 0:左（大玉檻）, 1:右（小玉檻）

        if (type == 0) {
            pEnemyShotSet->param_i[0] = 8;                     // 弾数
            pEnemyShotSet->param_d[0] = 120.0;                 // 初期半径
            pEnemyShotSet->param_d[2] = (30.0 * DX_PI / 180.0) / 60.0; // 角速度（30°/s）
            pEnemyShotSet->param_d[3] = 20.0 / 60.0;           // 収縮速度
            pEnemyShotSet->param_i[1] = 2;                     // 色：緑（大玉用）
        }
        else {
            pEnemyShotSet->param_i[0] = 16;
            pEnemyShotSet->param_d[0] = 90.0;
            pEnemyShotSet->param_d[2] = (75.0 * DX_PI / 180.0) / 60.0; // 75°/s
            pEnemyShotSet->param_d[3] = 40.0 / 60.0;
            pEnemyShotSet->param_i[1] = 0;                     // 色：赤（小玉用）
        }

        pEnemyShotSet->param_d[1] = 0.0;    // 現在の回転角度
        pEnemyShotSet->param_d[5] = 28.0;   // 安全地帯の半径
        pEnemyShotSet->param_i[2] = 0;      // 収縮フラグ
        pEnemyShotSet->param_i[3] = 0;      // 炸裂開始フラグ

        int num = pEnemyShotSet->param_i[0];
        double radius = pEnemyShotSet->param_d[0];
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            double angle = i * 2.0 * DX_PI / num;
            p->x = pEnemyShotSet->x + radius * cos(angle);
            p->y = pEnemyShotSet->y + radius * sin(angle);
            p->muki = 0.0;
            p->speed = 0.0;
            p->count = 0;
            p->param_d[0] = angle;   // 弾ごとの角度オフセット

            if (type == 0) {
                p->kind = img_enemyShotLargeBall[pEnemyShotSet->param_i[1]];
            }
            else {
                p->kind = img_enemyShotSmallBall[pEnemyShotSet->param_i[1]];
            }

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 弾の位置更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    double rotAngle = pEnemyShotSet->param_d[1];
    double radius = pEnemyShotSet->param_d[0];
    double centerX = pEnemyShotSet->x;
    double centerY = pEnemyShotSet->y;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double offsetAngle = pShot->param_d[0];
        pShot->x = centerX + radius * cos(rotAngle + offsetAngle);
        pShot->y = centerY + radius * sin(rotAngle + offsetAngle);
        pShot = pShot->next;
    }

    // 収縮
    if (pEnemyShotSet->param_i[2] == 1) {
        pEnemyShotSet->param_d[0] -= pEnemyShotSet->param_d[3];
        if (pEnemyShotSet->param_d[0] < 0.0) pEnemyShotSet->param_d[0] = 0.0;
        if (pEnemyShotSet->param_i[3] == 0 &&
            pEnemyShotSet->param_d[0] <= pEnemyShotSet->param_d[5]) {
            pEnemyShotSet->param_i[3] = 1;
        }
    }

    pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[2];
    if (pEnemyShotSet->param_d[1] > 2.0 * DX_PI) pEnemyShotSet->param_d[1] -= 2.0 * DX_PI;
}

// 自機狙い3-way大玉（プレイヤーが檻の外にいる間、常時射出）
static void ShotAimedTripleLarge(sEnemyShotSet* pEnemyShotSet)
{
    // 射出位置を常に敵本体に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 20.0;

    // 30フレームに1回、自機狙いの3-way大玉を発射
    if (pEnemyShotSet->count % 5 == 0) {
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double spread = 6.0 * DX_PI / 180.0; // 左右12度の3-way

        for (int i = -1; i <= 1; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = baseAngle + i * spread;
            p->speed = 10.8;                              // やや速めの大玉
            p->kind = img_enemyShotLargeBall[0];         // 赤色大玉

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 発射音（連射になりすぎないよう、ここでのみ再生）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 全弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 炸裂時のばらまき弾
static void ShotScatterExplode(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            double angle = i * 2.0 * DX_PI / 12;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = angle;
            p->speed = 1.5;
            p->kind = img_enemyShotSmallBall[8];   // 橙色

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------- ヘルパー関数 ----------

static void CreateScatter(double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotScatterExplode;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = 0;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

static void RemoveShotSet(sEnemyShotSet* set)
{
    if (!set) return;

    sEnemyShot* pShot = set->pEnemyShotHead->next;
    while (pShot != set->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;
        delete pShot;
        pShot = next;
    }
    delete set->pEnemyShotHead;

    set->prev->next = set->next;
    set->next->prev = set->prev;
    delete set;
}

// ---------- 敵本体のパターン ----------
void EnemyPat_Ebbinghaus_DeepSeek()
{
    static int phase = 0;
    static int timer = 0;
    static sEnemyShotSet* cageLeft = nullptr;
    static sEnemyShotSet* cageRight = nullptr;
    static sEnemyShotSet* aimedSet = nullptr;
    static int bossMuki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        timer = 0;
        cageLeft = nullptr;
        cageRight = nullptr;
        aimedSet = nullptr;
        bossMuki = 1;
    }
    else {
        enemy.x += 1.28 * (double)bossMuki;
        if (count % 120 == 60) bossMuki *= -1;
    }

    switch (phase) {
    case 0: // 檻の生成と初期待機
        if (timer == 0) {
            // 左の檻（大玉で囲まれている）
            cageLeft = new sEnemyShotSet;
            cageLeft->count = 0;
            cageLeft->patternFunc = ShotCage;
            cageLeft->x = 120.0;
            cageLeft->y = 260.0;
            cageLeft->muki = 0.0;
            cageLeft->kind = 0;   // 左
            cageLeft->pEnemyShotHead = new sEnemyShot;
            cageLeft->pEnemyShotHead->prev = cageLeft->pEnemyShotHead;
            cageLeft->pEnemyShotHead->next = cageLeft->pEnemyShotHead;
            cageLeft->prev = enemyShotSetHead.prev;
            cageLeft->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = cageLeft;
            enemyShotSetHead.prev = cageLeft;

            // 右の檻（小玉で囲まれている）
            cageRight = new sEnemyShotSet;
            cageRight->count = 0;
            cageRight->patternFunc = ShotCage;
            cageRight->x = 360.0;
            cageRight->y = 260.0;
            cageRight->muki = 0.0;
            cageRight->kind = 1;   // 右
            cageRight->pEnemyShotHead = new sEnemyShot;
            cageRight->pEnemyShotHead->prev = cageRight->pEnemyShotHead;
            cageRight->pEnemyShotHead->next = cageRight->pEnemyShotHead;
            cageRight->prev = enemyShotSetHead.prev;
            cageRight->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = cageRight;
            enemyShotSetHead.prev = cageRight;

            timer = 1;
        }

        if (timer >= 60) {
            // 予告音を鳴らして攻撃フェーズへ
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            phase = 1;
            timer = 0;
            aimedSet = nullptr; // 念のため
        }
        else {
            timer++;
        }
        break;

    case 1: // 攻撃管理フェーズ（プレイヤーの檻出入りに応じて3-way大玉をON/OFF）
    {
        bool leftInside = false, rightInside = false;
        if (cageLeft) {
            double dx = player.x - cageLeft->x;
            double dy = player.y - cageLeft->y;
            if (sqrt(dx * dx + dy * dy) < 90.0) leftInside = true;
        }
        if (cageRight) {
            double dx = player.x - cageRight->x;
            double dy = player.y - cageRight->y;
            if (sqrt(dx * dx + dy * dy) < 90.0) rightInside = true;
        }

        // 檻の外にいる間は3-way大玉を射出し続ける
        if (!leftInside && !rightInside) {
            if (aimedSet == nullptr) {
                aimedSet = new sEnemyShotSet;
                aimedSet->count = 0;
                aimedSet->patternFunc = ShotAimedTripleLarge;
                aimedSet->x = enemy.x;
                aimedSet->y = enemy.y + 20.0;
                aimedSet->muki = 0.0;
                aimedSet->kind = 0;
                aimedSet->pEnemyShotHead = new sEnemyShot;
                aimedSet->pEnemyShotHead->prev = aimedSet->pEnemyShotHead;
                aimedSet->pEnemyShotHead->next = aimedSet->pEnemyShotHead;
                aimedSet->prev = enemyShotSetHead.prev;
                aimedSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = aimedSet;
                enemyShotSetHead.prev = aimedSet;
            }
        }
        else {
            // 檻の中に入ったら攻撃を止める
            if (aimedSet != nullptr) {
                RemoveShotSet(aimedSet);
                aimedSet = nullptr;
            }

            // 初めて侵入した檻の収縮を開始
            if (leftInside && cageLeft && cageLeft->param_i[2] == 0) {
                cageLeft->param_i[2] = 1;
            }
            if (rightInside && cageRight && cageRight->param_i[2] == 0) {
                cageRight->param_i[2] = 1;
            }
        }

        // 檻の炸裂チェック
        bool exploded = false;
        if (cageLeft && cageLeft->param_i[3] == 1) {
            CreateScatter(cageLeft->x, cageLeft->y);
            RemoveShotSet(cageLeft); cageLeft = nullptr;
            if (cageRight) { RemoveShotSet(cageRight); cageRight = nullptr; }
            exploded = true;
        }
        else if (cageRight && cageRight->param_i[3] == 1) {
            CreateScatter(cageRight->x, cageRight->y);
            RemoveShotSet(cageRight); cageRight = nullptr;
            if (cageLeft) { RemoveShotSet(cageLeft); cageLeft = nullptr; }
            exploded = true;
        }

        if (exploded) {
            // 攻撃セットも消去してクールダウンへ
            if (aimedSet) {
                RemoveShotSet(aimedSet);
                aimedSet = nullptr;
            }
            phase = 4;
            timer = 0;
        }
    }
    break;

    case 4: // クールダウン → リピート
        if (timer > 60) {
            phase = 0;
            timer = 0;
            cageLeft = nullptr;
            cageRight = nullptr;
            aimedSet = nullptr;
        }
        else {
            timer++;
        }
        break;
    }
}