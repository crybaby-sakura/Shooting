// enemyPat_yoyo.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾種別
#define SHOT_TYPE_YOYO   1
#define SHOT_TYPE_STRING 2
#define SHOT_TYPE_BULLET 3

// 弾を追加するヘルパー
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int type)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = type;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ヨーヨーパターン
static void YoyoPattern(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // ヨーヨー本体を生成
        sEnemyShot* pYoyo = new sEnemyShot;
        pYoyo->x = pSet->x;
        pYoyo->y = pSet->y;
        pYoyo->muki = pSet->muki;
        pYoyo->speed = 3.0;
        pYoyo->kind = img_enemyShotMediumBall[8]; // 橙の中玉
        pYoyo->param_i[0] = SHOT_TYPE_YOYO;
        pYoyo->param_d[0] = pSet->param_d[0]; // 目標X
        pYoyo->param_d[1] = pSet->param_d[1]; // 目標Y
        pYoyo->margin = 480;

        pYoyo->prev = pSet->pEnemyShotHead->prev;
        pYoyo->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pYoyo;
        pSet->pEnemyShotHead->prev = pYoyo;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        return;
    }

    // 古い糸（静止レーザー）を一定時間で削除
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;
        if (p->param_i[0] == SHOT_TYPE_STRING && p->count > 180) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }
        p = next;
    }

    // ヨーヨー本体を探す
    sEnemyShot* pYoyo = nullptr;
    for (sEnemyShot* q = pSet->pEnemyShotHead->next; q != pSet->pEnemyShotHead; q = q->next) {
        if (q->param_i[0] == SHOT_TYPE_YOYO) {
            pYoyo = q;
            break;
        }
    }
    if (!pYoyo) return;

    int phase = pSet->param_i[1];
    int phaseStart = pSet->param_i[2];
    int elapsed = pSet->count - phaseStart;

    // ===== フェーズごとの移動設定 =====
    switch (phase) {
    case 0: { // 投げつけ
        double dx = pYoyo->param_d[0] - pYoyo->x;
        double dy = pYoyo->param_d[1] - pYoyo->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 4.0 || elapsed > 90) {
            pYoyo->speed = 0.0;
            pSet->param_i[1] = 1; // 回転フェーズへ
            pSet->param_i[2] = pSet->count;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            pYoyo->speed = 3.0;
        }
        break;
    }
    case 1: { // 高速回転
        pYoyo->speed = 0.0;
        if (elapsed > 60) {
            pSet->param_i[1] = 2; // 巻き戻りへ
            pSet->param_i[2] = pSet->count;
        }
        break;
    }
    case 2: { // 糸を巻き戻り
        double dx = enemy.x - pYoyo->x;
        double dy = enemy.y - pYoyo->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 6.0) {
            pYoyo->speed = 0.0;
            pSet->param_i[1] = 3; // 大車輪へ
            pSet->param_i[2] = pSet->count;
        }
        else {
            pYoyo->speed = 3.0;
            pYoyo->muki = atan2(dy, dx);
        }
        break;
    }
    case 3: { // 大車輪
        double orbitRadius = 80.0;
        double orbitAngle = (pSet->count - pSet->param_i[2]) * 0.08;
        pYoyo->x = enemy.x + orbitRadius * cos(orbitAngle);
        pYoyo->y = enemy.y + orbitRadius * sin(orbitAngle);
        pYoyo->speed = 0.0;
        if (elapsed > 120) {
            pSet->param_i[1] = 0;
            pSet->param_i[2] = pSet->count;
            pYoyo->param_d[0] = player.x;
            pYoyo->param_d[1] = player.y;
            pYoyo->x = enemy.x;
            pYoyo->y = enemy.y;
            pYoyo->speed = 3.0;
            pYoyo->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        }
        break;
    }
    }

    // ===== 全弾の移動 =====
    for (sEnemyShot* q = pSet->pEnemyShotHead->next; q != pSet->pEnemyShotHead; q = q->next) {
        if (q->speed > 0.0) {
            q->x += q->speed * cos(q->muki);
            q->y += q->speed * sin(q->muki);
        }
    }

    // ===== フェーズごとの発射処理 =====
    int phaseNow = pSet->param_i[1];
    int elapsedNow = pSet->count - pSet->param_i[2];

    switch (phaseNow) {
    case 0: { // 投げつけ中は糸を残す
        if (pYoyo->speed > 0.0 && pSet->count % 2 == 0) {
            AddShot(pSet, pYoyo->x, pYoyo->y, pYoyo->muki, 0.0, img_enemyShotLaser[6], SHOT_TYPE_STRING);
        }
        break;
    }
    case 1: { // 回転中は自機狙い扇状弾と全方位円形弾
        if (elapsedNow > 0 && elapsedNow % 4 == 0) {
            double ang = atan2(player.y - pYoyo->y, player.x - pYoyo->x);
            for (int k = -2; k <= 2; k++) {
                AddShot(pSet, pYoyo->x, pYoyo->y, ang + k * 0.25, 2.2, img_enemyShotSmallBall[1], SHOT_TYPE_BULLET);
            }
        }
        if (elapsedNow > 0 && elapsedNow % 5 == 0) {
            for (int k = 0; k < 12; k++) {
                double ang = k * DX_PI * 2 / 12;
                AddShot(pSet, pYoyo->x, pYoyo->y, ang, 1.8, img_enemyShotSmallBall[8], SHOT_TYPE_BULLET);
            }
        }
        break;
    }
    case 2: { // 巻き戻り中は軌道の左右に弾
        if (pYoyo->speed > 0.0 && pSet->count % 2 == 0) {
            double ang = pYoyo->muki + DX_PI / 2;
            AddShot(pSet, pYoyo->x, pYoyo->y, ang, 1.5, img_enemyShotSmallBall[1], SHOT_TYPE_BULLET);
            AddShot(pSet, pYoyo->x, pYoyo->y, ang + DX_PI, 1.5, img_enemyShotSmallBall[1], SHOT_TYPE_BULLET);
        }
        break;
    }
    case 3: { // 大車輪中は外側へ放射
        if (elapsedNow > 0 && elapsedNow % 2 == 0) {
            double orbitAngle = (pSet->count - pSet->param_i[2]) * 0.08;
            for (int k = 0; k < 8; k++) {
                double ang = orbitAngle + k * DX_PI * 2 / 8;
                AddShot(pSet, pYoyo->x, pYoyo->y, ang, 2.5, img_enemyShotSmallBall[2], SHOT_TYPE_BULLET);
            }
        }
        break;
    }
    }
}

// 敵本体パターン
void EnemyPat_Yoyo_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // ヨーヨー弾幕セットを1つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = YoyoPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;
        pSet->param_i[1] = 0; // 初期フェーズ
        pSet->param_i[2] = 0; // フェーズ開始カウント
        pSet->param_d[0] = player.x; // ヨーヨーの目標X
        pSet->param_d[1] = player.y; // ヨーヨーの目標Y

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}