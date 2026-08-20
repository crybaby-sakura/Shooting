// EnemyPat_TwoBoss_DeepSeek.cpp
// 双星交差「クロスファイア・ロンド」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾追加ヘルパー
// ------------------------------------------------------------
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    // リンクリストへ挿入
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// セット初期化ヘルパー
// ------------------------------------------------------------
static void InitShotSet(sEnemyShotSet* pSet)
{
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
}

static void AddShotSetToList(sEnemyShotSet* pSet)
{
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ------------------------------------------------------------
// フェーズ1：交差散弾（3way）
//   param_i[0] : 0=左ボス（右方向へ） / 1=右ボス（左方向へ）
//   param_i[1] : 弾の色
// ------------------------------------------------------------
static void ShotCrossOnce(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double base = (pSet->param_i[0] == 0) ? 0.0 : DX_PI;
        for (int i = -1; i <= 1; i++) {
            double angle = base + i * (15.0 * DX_PI / 180.0);
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 2.0,
                img_enemyShotSmallBall[pSet->param_i[1]]);
        }
    }

    // 既存弾の移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ1：低速狙い弾
//   param_i[1] : 弾の色
// ------------------------------------------------------------
static void ShotAimedSlow(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double angle = atan2(player.y - pSet->y, player.x - pSet->x);
        AddEnemyShot(pSet, pSet->x, pSet->y, angle, 1.2,
            img_enemyShotMediumBall[pSet->param_i[1]]);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ2：共振連結レーザー
//   param_i[0] : チャージ時間（フレーム）
//   param_i[1] : レーザー弾の色
//   param_i[2] : 滴下小弾の色
//   param_d[0] : 掃引速度（下方向）
//   param_d[1]〜[3] : 隙間の中心オフセット
// ------------------------------------------------------------
static void LaserSweep(sEnemyShotSet* pSet)
{
    if (pSet->count < pSet->param_i[0]) {
        // チャージ中は何もしない
        return;
    }

    if (pSet->count == pSet->param_i[0]) {
        // レーザー本体を生成（隙間を空けて配置）
        double gapOffsets[3] = { pSet->param_d[1], pSet->param_d[2], pSet->param_d[3] };

        for (double xRel = -120.0; xRel <= 120.0; xRel += 16.0) {
            bool inGap = false;
            for (int g = 0; g < 3; g++) {
                if (fabs(xRel - gapOffsets[g]) < 12.0) {
                    inGap = true;
                    break;
                }
            }
            if (!inGap) {
                AddEnemyShot(pSet, pSet->x + xRel, pSet->y,
                    0.0, 0.0,
                    img_enemyShotLaser[pSet->param_i[1]]);
            }
        }

        // レーザー展開音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    else {
        // 掃引移動
        double sweep = pSet->param_d[0];
        pSet->y += sweep;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->y += sweep;
            pShot = pShot->next;
        }

        // 隙間から小弾を滴下
        if ((pSet->count - pSet->param_i[0]) % 20 == 0) {
            for (int g = 0; g < 3; g++) {
                double gapX = pSet->x + pSet->param_d[1 + g];
                AddEnemyShot(pSet, gapX, pSet->y,
                    DX_PI / 2.0, 0.0,
                    img_enemyShotSmallBall[pSet->param_i[2]]);
            }
        }
    }
}

// ------------------------------------------------------------
// フェーズ3：スパイラル（回転弾幕）
//   param_i[0] : 回転方向（1 or -1）
//   param_i[1] : 弾の色
// ------------------------------------------------------------
static void Spiral(sEnemyShotSet* pSet)
{
    if (pSet->count % 4 == 0) {
        double dir = (pSet->param_i[0] == 0) ? 1.0 : -1.0;
        double angle = pSet->muki + pSet->count * 0.12 * dir;
        AddEnemyShot(pSet, pSet->x, pSet->y, angle, 1.6,
            img_enemyShotSmallBall[pSet->param_i[1]]);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// フェーズ3：8方向同時発射
//   param_i[1] : 弾の色
// ------------------------------------------------------------
static void EightWayOnce(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 8; i++) {
            double angle = i * (2.0 * DX_PI / 8.0);
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 2.0,
                img_enemyShotDiamond[pSet->param_i[1]]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_TwoBoss_DeepSeek()
{
    static int muki = 1;          // フェーズ1移動用
    static double theta = 0.0;    // フェーズ3回転用
    static int prevPhase = 0;     // フェーズ遷移検出用

    // 初期化
    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 240.0;
        enemy.x2 = 360.0;
        enemy.y2 = 240.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        theta = 0.0;
        prevPhase = 0;
        return;
    }

    // 現在のフェーズ判定（count基準でデモ用に切り替え）
    int phase;
    if (count < 400) {
        phase = 0;   // 交差散弾
    }
    else if (count < 800) {
        phase = 1;   // 連結レーザー
    }
    else {
        phase = 2;   // 双星ロンド
    }

    // ボス2体の移動
    if (phase == 0) {
        // 上下反対に移動
        enemy.y += 0.7 * muki * 4;
        enemy.y2 -= 0.7 * muki * 4;
        if (enemy.y < 60.0 || enemy.y > 420.0) muki *= -1;
        if (enemy.y < 60.0) enemy.y = 60.0;
        if (enemy.y > 420.0) enemy.y = 420.0;
        if (enemy.y2 < 60.0) enemy.y2 = 60.0;
        if (enemy.y2 > 420.0) enemy.y2 = 420.0;
        enemy.x = 120.0;
        enemy.x2 = 360.0;
    }
    else if (phase == 1) {
        // 同高度で上下にスイング
        enemy.x = 120.0;
        enemy.x2 = 360.0;
        enemy.y = enemy.y2 = 240.0 + 50.0 * sin(count * 0.015);
    }
    else {
        // 円軌道上を正反対に回転（互いに逆方向）
        theta += 0.025;
        double angleA = theta;
        double angleB = -theta + DX_PI;
        enemy.x = 240.0 + 150.0 * cos(angleA);
        enemy.y = 240.0 + 150.0 * sin(angleA);
        enemy.x2 = 240.0 + 150.0 * cos(angleB);
        enemy.y2 = 240.0 + 150.0 * sin(angleB);
    }

    // フェーズ別攻撃スケジュール
    if (phase == 0) {
        // 交差3way散弾
        if (count % 45 == 1) {
            // 左ボス（青）
            sEnemyShotSet* pSetA = new sEnemyShotSet;
            pSetA->count = 0;
            pSetA->patternFunc = ShotCrossOnce;
            pSetA->x = enemy.x;
            pSetA->y = enemy.y;
            pSetA->muki = 0.0;
            pSetA->param_i[0] = 0; // 左側
            pSetA->param_i[1] = 4; // 青
            InitShotSet(pSetA);
            AddShotSetToList(pSetA);

            // 右ボス（赤）
            sEnemyShotSet* pSetB = new sEnemyShotSet;
            pSetB->count = 0;
            pSetB->patternFunc = ShotCrossOnce;
            pSetB->x = enemy.x2;
            pSetB->y = enemy.y2;
            pSetB->muki = DX_PI;
            pSetB->param_i[0] = 1; // 右側
            pSetB->param_i[1] = 0; // 赤
            InitShotSet(pSetB);
            AddShotSetToList(pSetB);

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 低速狙い弾
        if (count % 36 == 1) {
            sEnemyShotSet* pSetA = new sEnemyShotSet;
            pSetA->count = 0;
            pSetA->patternFunc = ShotAimedSlow;
            pSetA->x = enemy.x;
            pSetA->y = enemy.y;
            pSetA->muki = 0.0;
            pSetA->param_i[1] = 1; // 黄
            InitShotSet(pSetA);
            AddShotSetToList(pSetA);

            sEnemyShotSet* pSetB = new sEnemyShotSet;
            pSetB->count = 0;
            pSetB->patternFunc = ShotAimedSlow;
            pSetB->x = enemy.x2;
            pSetB->y = enemy.y2;
            pSetB->muki = 0.0;
            pSetB->param_i[1] = 1; // 黄
            InitShotSet(pSetB);
            AddShotSetToList(pSetB);

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 1) {
        // 共振連結レーザー（定期発動）
        if (count % 120 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = LaserSweep;
            pSet->x = 240.0;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->param_i[0] = 60;   // チャージ60F
            pSet->param_i[1] = 6;    // レーザー白
            pSet->param_i[2] = 5;    // 滴下小弾マゼンタ
            pSet->param_d[0] = 0.8;  // 掃引速度
            pSet->param_d[1] = -60.0; // 隙間1
            pSet->param_d[2] = 0.0;   // 隙間2
            pSet->param_d[3] = 60.0;  // 隙間3
            InitShotSet(pSet);
            AddShotSetToList(pSet);

            // チャージ予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 2) {
        // フェーズ2突入時にスパイラルを2体分生成（永続）
        if (prevPhase != 2) {
            // 左ボス（シアン）
            sEnemyShotSet* pSetA = new sEnemyShotSet;
            pSetA->count = 0;
            pSetA->patternFunc = Spiral;
            pSetA->x = enemy.x;
            pSetA->y = enemy.y;
            pSetA->muki = atan2(enemy.y - 240.0, enemy.x - 240.0);
            pSetA->param_i[0] = 1;  // 回転方向
            pSetA->param_i[1] = 3;  // シアン
            InitShotSet(pSetA);
            AddShotSetToList(pSetA);

            // 右ボス（マゼンタ）
            sEnemyShotSet* pSetB = new sEnemyShotSet;
            pSetB->count = 0;
            pSetB->patternFunc = Spiral;
            pSetB->x = enemy.x2;
            pSetB->y = enemy.y2;
            pSetB->muki = atan2(enemy.y2 - 240.0, enemy.x2 - 240.0);
            pSetB->param_i[0] = -1; // 逆回転
            pSetB->param_i[1] = 5;  // マゼンタ
            InitShotSet(pSetB);
            AddShotSetToList(pSetB);

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        // 8方向同時発射
        if (count % 60 == 1) {
            sEnemyShotSet* pSetA = new sEnemyShotSet;
            pSetA->count = 0;
            pSetA->patternFunc = EightWayOnce;
            pSetA->x = enemy.x;
            pSetA->y = enemy.y;
            pSetA->muki = 0.0;
            pSetA->param_i[1] = 6; // 白
            InitShotSet(pSetA);
            AddShotSetToList(pSetA);

            sEnemyShotSet* pSetB = new sEnemyShotSet;
            pSetB->count = 0;
            pSetB->patternFunc = EightWayOnce;
            pSetB->x = enemy.x2;
            pSetB->y = enemy.y2;
            pSetB->muki = 0.0;
            pSetB->param_i[1] = 6; // 白
            InitShotSet(pSetB);
            AddShotSetToList(pSetB);
        }
    }

    // 前回フェーズを更新
    prevPhase = phase;
}