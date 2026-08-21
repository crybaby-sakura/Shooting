// enemyPat_tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// スピログラフ風ギアバースト用パターン関数
static void SpirographPattern(sEnemyShotSet* pSet);

// 弾を1つ追加するヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;

    return p;
}

// スピログラフ弾幕パターン
static void SpirographPattern(sEnemyShotSet* pSet)
{
    // 歯車の基本設定
    const double R = 160.0;              // 外歯車の半径
    const int outerTeeth = 24;           // 外歯車の歯数
    const int cycleFrames = 240;         // 1周期のフレーム数
    const double aStep = (10.0 * DX_PI) / cycleFrames; // 1フレームあたりの公転角

    // 初期化または再生成
    if (pSet->count == 0 || pSet->param_i[2] == 1) {
        // 発射済みの軌跡弾（param_i[0] == 4）以外を全て削除
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            if (p->param_i[0] != 4) {
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
            p = next;
        }

        // 周期ごとに内歯車の半径を変更
        static const double innerRadii[] = { 60.0, 80.0, 100.0, 120.0 };
        int cycle = pSet->param_i[1];
        double r = innerRadii[cycle % 4];

        // 現在の内歯車の半径を保存
        pSet->param_d[2] = r;

        // 内歯車の歯数を外歯車に比例して計算
        int innerTeeth = (int)(r * outerTeeth / R + 0.5);
        if (innerTeeth < 5) innerTeeth = 5;

        // ペン先の距離（内歯車の半径に比例）
        double penDist = r * 0.4;

        // 歯車の歯元・歯先半径
        double outerRootR = R - 10.0;
        double outerTipR = R + 10.0;
        double innerRootR = r - 10.0;
        double innerTipR = r + 10.0;

        // --- 外歯車を配置（静止） ---
        for (int i = 0; i < outerTeeth; i++) {
            double ang = 2.0 * DX_PI * i / outerTeeth;

            double rootX = pSet->x + outerRootR * cos(ang);
            double rootY = pSet->y + outerRootR * sin(ang);
            double tipX = pSet->x + outerTipR * cos(ang);
            double tipY = pSet->y + outerTipR * sin(ang);

            sEnemyShot* pShot = AddShot(pSet, rootX, rootY, 0.0, 0.0, img_enemyShotMediumBall[6]);
            pShot->param_i[0] = 0; // 外歯車・歯元

            pShot = AddShot(pSet, tipX, tipY, 0.0, 0.0, img_enemyShotSmallBall[6]);
            pShot->param_i[0] = 0; // 外歯車・歯先
        }

        // --- 内歯車を配置（毎フレーム更新） ---
        for (int i = 0; i < innerTeeth; i++) {
            double ang = 2.0 * DX_PI * i / innerTeeth;

            sEnemyShot* pShot = AddShot(pSet, 0.0, 0.0, 0.0, 0.0, img_enemyShotMediumBall[8]);
            pShot->param_i[0] = 1;            // 内歯車・歯元
            pShot->param_d[0] = ang;          // 歯の基準角度
            pShot->param_d[1] = innerRootR;   // 中心からの距離

            pShot = AddShot(pSet, 0.0, 0.0, 0.0, 0.0, img_enemyShotSmallBall[8]);
            pShot->param_i[0] = 2;            // 内歯車・歯先
            pShot->param_d[0] = ang;
            pShot->param_d[1] = innerTipR;
        }

        // 周期内フレームカウントをリセット
        pSet->param_d[0] = 0.0;
        pSet->param_i[2] = 0; // 再生成フラグをクリア

        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 周期終了時の放射処理
    if (pSet->param_d[0] == cycleFrames) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 3) {
                // 軌跡弾を放射状に発射
                pShot->speed = (200 + GetRand(100)) / 100.0;
                pShot->muki = atan2(pShot->y - pSet->y, pShot->x - pSet->x);
                pShot->param_i[0] = 4; // 発射済み
            }
            pShot = pShot->next;
        }

        // 次周期用に半径を変更するためのフラグとカウンタ
        pSet->param_i[1]++;
        pSet->param_i[2] = 1;
        return;
    }

    // 現在の公転角・自転角
    double a = pSet->param_d[0] * aStep;
    double b = -((R - pSet->param_d[2]) / pSet->param_d[2]) * a;

    // 内歯車の中心座標
    double cx = pSet->x + (R - pSet->param_d[2]) * cos(a);
    double cy = pSet->y + (R - pSet->param_d[2]) * sin(a);

    // 内歯車の弾を現在位置に更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1 || pShot->param_i[0] == 2) {
            double ang = b + pShot->param_d[0];
            double rad = pShot->param_d[1];
            pShot->x = cx + rad * cos(ang);
            pShot->y = cy + rad * sin(ang);
        }
        pShot = pShot->next;
    }

    // スピログラフ軌跡を弾で描く（2フレームごと）
    if ((int)pSet->param_d[0] % 1 == 0) {
        double penDist = pSet->param_d[2] * 0.4;
        double px = cx + penDist * cos(b);
        double py = cy + penDist * sin(b);

        sEnemyShot* p = AddShot(pSet, px, py, 0.0, 0.0, img_enemyShotSmallBall[3]);
        p->param_i[0] = 3; // 軌跡弾（未発射）
    }

    // 弾の移動（発射済み弾のみ speed > 0）
    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }

    // フレームカウントを進める
    pSet->param_d[0] += 1.0;
}

// 敵本体のパターン
void EnemyPat_Spirograph_DeepSeek()
{
    if (count == 1) {
        // 画面中央に配置
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = SpirographPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->param_i[1] = 0; // 周期カウンタ（半径変更用）
        pSet->param_i[2] = 0; // 再生成フラグ
        pSet->param_d[2] = 100.0; // 初期内歯車半径（初期化で上書きされる）

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 敵本体は画面中央で静止
}