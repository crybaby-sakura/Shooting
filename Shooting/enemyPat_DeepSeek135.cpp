// ============================================================
//  enemyPat_backdraft.cpp
//  弾幕：バックドラフト
//    「酸欠の密閉空間に酸素が流入し、内側で発火して
//      開口部から炎が噴き出す」現象を弾幕で表現する。
//
//  使用素材（既存のみ）
//    煤   : 大玉(黒)    … 低速で燃焼室の壁を作る
//    酸素 : 小玉(青)    … 高速で中心Pへ流入
//    炎   : 中玉(赤)    … 開口部から扇状に噴出
//    火花 : 小玉(橙)    … 余韻で周囲に散る
//
//  フェーズ
//    t=0    密閉フェーズ : 煤で半円の壁を作る（下に開口部）
//    t=90   酸素流入     : 酸素が下方から中心Pへ流入
//    t=180  予告音
//    t=210  発火         : 炎が開口部から噴出、煤が吹き飛ぶ
//    t=240  余韻         : 火花が散る
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾をセットのリスト末尾（head の手前）に追加するヘルパ
static void AddShotToList(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ============================================================
//  バックドラフト弾幕本体
//    param_d[0] : 燃焼室中心 Px
//    param_d[1] : 燃焼室中心 Py
//    param_d[2] : 開口部の中心角（下向き = +PI/2）
//    param_d[3] : 開口部の半幅（ラジアン）
//
//  弾のタグ（param_i[0]）
//    1 = 煤、2 = 酸素、3 = 炎、4 = 吹き飛び煤、5 = 火花
// ============================================================
static void ShotBackdraft(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count; // 自動インクリメントされる

    const double Px = pEnemyShotSet->param_d[0];
    const double Py = pEnemyShotSet->param_d[1];
    const double openDir = pEnemyShotSet->param_d[2];
    const double openHalf = pEnemyShotSet->param_d[3];

    // --------------------------------------------------------
    // 1) 密閉フェーズ t=0 : 煤で半円の壁を作る
    // --------------------------------------------------------
    if (t == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int N = 22;
        for (int i = 0; i < N; i++) {
            double a = (DX_PI * 2.0) * i / N;

            // 開口部の角度範囲内ならスキップ
            double diff = a - openDir;
            while (diff > DX_PI) diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;
            if (fabs(diff) < openHalf) continue;

            sEnemyShot* p = new sEnemyShot;
            p->x = Px + 10.0 * cos(a);
            p->y = Py + 10.0 * sin(a);
            p->muki = a;
            p->speed = 2.0;
            p->kind = img_enemyShotLargeBall[7]; // 黒：煤
            p->param_i[0] = 1;      // 煤タグ
            p->param_d[0] = 0.015;  // 毎フレームの減速量
            AddShotToList(pEnemyShotSet, p);
        }
    }

    // --------------------------------------------------------
    // 2) 酸素流入フェーズ t=90..210
    // --------------------------------------------------------
    if (t == 90) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    if (t >= 90 && t < 210 && (t % 10 == 0)) {
        for (int lane = 0; lane < 2; lane++) {
            double sx = Px + (lane * 2 - 1) * 45.0;
            double sy = 500.0; // 画面下外側
            double ang = atan2(Py - sy, Px - sx);

            sEnemyShot* p = new sEnemyShot;
            p->x = sx;
            p->y = sy;
            p->muki = ang;
            p->speed = 5.0;
            p->kind = img_enemyShotSmallBall[4]; // 青：酸素
            p->param_i[0] = 2; // 酸素タグ
            AddShotToList(pEnemyShotSet, p);
        }
    }

    // 予告音（発火 0.5 秒前）
    if (t == 180) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 3) 発火フェーズ t=210
    // --------------------------------------------------------
    if (t == 210) {
        if (CheckSoundMem(sound_enemyCharge))        StopSoundMem(sound_enemyCharge);
        if (CheckSoundMem(sound_enemyShot_extreme))  StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 炎（中弾）を開口部から扇状に噴出
        const int N = 11;
        for (int i = 0; i < N; i++) {
            double a = openDir
                + (i - (N - 1) / 2.0) * (openHalf * 2.0 / (N - 1));

            sEnemyShot* p = new sEnemyShot;
            p->x = Px;
            p->y = Py;
            p->muki = a;
            p->speed = 3.5 + (i % 3) * 0.6;
            p->kind = img_enemyShotMediumBall[0]; // 赤：炎
            p->param_i[0] = 3; // 炎タグ
            AddShotToList(pEnemyShotSet, p);
        }

        // 既存弾を走査して状態遷移
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;

            if (p->param_i[0] == 1) {
                // 煤を外向きに吹き飛ばす
                p->muki = atan2(p->y - Py, p->x - Px);
                p->speed = 2.5;
                p->param_i[0] = 4; // 吹き飛び煤タグ
            }
            else if (p->param_i[0] == 2) {
                // 残っている酸素も炎に変えて開口部から出す
                p->kind = img_enemyShotMediumBall[0];
                p->muki = openDir
                    + (GetRand(100) - 50) / 100.0 * openHalf;
                p->speed = 3.5 + GetRand(150) / 100.0;
                p->param_i[0] = 3;
            }
            p = next;
        }
    }

    // --------------------------------------------------------
    // 4) 余韻フェーズ t=240 : 火花を散らす
    // --------------------------------------------------------
    if (t == 240) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            if (p->param_i[0] == 4) {
                for (int i = 0; i < 2; i++) {
                    sEnemyShot* s = new sEnemyShot;
                    s->x = p->x;
                    s->y = p->y;
                    s->muki = GetRand(359) / 180.0 * DX_PI;
                    s->speed = 1.5;
                    s->kind = img_enemyShotSmallBall[8]; // 橙：火花
                    s->param_i[0] = 5; // 火花タグ
                    AddShotToList(pEnemyShotSet, s);
                }
            }
            p = next;
        }
    }

    // --------------------------------------------------------
    // 毎フレームの弾更新
    //  煤だけは「目標半径で停止する」ように減速制御する
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[0] == 1) {
            double dx = pShot->x - Px;
            double dy = pShot->y - Py;
            double r = sqrt(dx * dx + dy * dy);
            if (r >= 140.0) {
                pShot->speed = 0.1; // 壁としてほぼ停止
            }
            else {
                pShot->speed -= pShot->param_d[0];
                if (pShot->speed < 0.1) pShot->speed = 0.1;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Backdraft_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右に往復（燃焼室が画面内に収まる範囲）
        enemy.x += 0.5 * (double)muki;
        if (enemy.x > 300.0) muki = -1;
        if (enemy.x < 180.0) muki = 1;
    }

    // 5 秒周期でバックドラフトを 1 回発生させる
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBackdraft;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        // 燃焼室中心 P（敵の下 160px）
        pEnemyShotSet->param_d[0] = enemy.x;
        pEnemyShotSet->param_d[1] = enemy.y + 160.0;

        // 開口部をサイクルごとに左右へずらす（下向き ±60°、5 段階）
        int idx = pEnemyShotSet->kind % 5;
        pEnemyShotSet->param_d[2] = DX_PI / 2.0
            + (idx - 2) * (DX_PI / 6.0);
        pEnemyShotSet->param_d[3] = DX_PI / 6.0; // 半幅 30°

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをグローバルリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}