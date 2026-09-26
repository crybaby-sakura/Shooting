#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：青天の霹靂
//  「青空」の中から突然「稲妻」が落ち、衝撃で青空が弾ける。
// ============================================================
//
//  フェーズ遷移（pEnemyShotSet->param_i[0] で管理）
//    0: 青天形成   … 青い小型弾を横一列に降らせて青空の層を作る
//    1: 一瞬の停止 … 青空弾の動きを止め、霹靂の直前を演出
//    2: 霹靂発生   … 白い銃弾でジグザグの稲妻を落とす
//    3: 衝撃波     … 青空弾を放射状に散らし、衝撃波を広げる
//    → 0 に戻る（無限ループ）
//
//  param_i[0] : フェーズ
//  param_i[1] : フェーズ内経過フレーム（自前管理）
//  param_i[2] : ループ回数
//  param_i[3] : 稲妻の基準X座標
//  param_i[4] : 稲妻の生成カウンタ
//
static void ShotAotenNoHekirei(sEnemyShotSet* pEnemyShotSet)
{
    int& phase = pEnemyShotSet->param_i[0];
    int& t = pEnemyShotSet->param_i[1];
    int& loopCnt = pEnemyShotSet->param_i[2];
    int& boltX = pEnemyShotSet->param_i[3];
    int& boltCnt = pEnemyShotSet->param_i[4];

    // ---- 弾の移動（一瞬の停止フェーズ以外） ----
    if (phase != 1) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }

    switch (phase) {
        // --------------------------------------------------------
    case 0: // 青天形成
    {
        // 最初のループ開始時のみ音を鳴らす
        if (t == 0 && loopCnt == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 12フレームごとに横一列の青空弾（行ごとに速度を変える）
        if (t % 12 == 0) {
            int row = t / 12;
            for (int i = 0; i < 12; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = 20.0 + i * 40.0 + (row % 2) * 20.0;
                p->y = pEnemyShotSet->y;
                p->muki = DX_PI / 2.0;               // 下向き
                p->speed = 1.0 + (row % 4) * 0.2;    // 1.0～1.6
                p->kind = img_enemyShotSmallBall[4]; // 青い小型弾
                p->param_i[0] = 0;                   // 青空弾マーカー
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }

        if (t >= 84) { phase = 1; t = -1; }
        break;
    }
    // --------------------------------------------------------
    case 1: // 一瞬の停止
    {
        // 予告音（チャージ音）を鳴らす
        if (t == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 青空弾は「動かない」ことで霹靂の直前を表現
        if (t >= 20) {
            boltX = GetRand(360) + 60; // 60～420
            boltCnt = 0;
            phase = 2;
            t = -1;
        }
        break;
    }
    // --------------------------------------------------------
    case 2: // 霹靂発生
    {
        // 0.2秒差で3本の稲妻を落とす
        if (t == 14 || t == 28 || t == 42 || t == 56) {
            boltX = GetRand(360) + 60;
            boltCnt = 0;
        }

        // 2フレームごとに白い銃弾を生成してジグザグの稲妻を描く
        if (boltCnt < 24 && t % 2 == 0) {
            double zig = sin(boltCnt * 0.9) * 22.0 + (GetRand(12) - 6);
            sEnemyShot* p = new sEnemyShot;
            p->x = boltX + zig;
            p->y = pEnemyShotSet->y - 40.0; // 画面上部から
            p->muki = DX_PI / 2.0;             // 下向き
            p->speed = 14.0;                   // 高速
            p->kind = img_enemyShotBullet[6]; // 白い銃弾（針状）
            p->param_i[0] = 1;                 // 稲妻弾マーカー
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;

            boltCnt++;

            // 各稲妻の1発目で雷鳴音
            if (boltCnt == 1) {
                if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            }
        }

        if (t >= 80) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            phase = 3;
            t = -1;
        }
        break;
    }
    // --------------------------------------------------------
    case 3: // 衝撃波
    {
        if (t == 0) {
            // 既存の青空弾を中央から放射状に散らす
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] == 0) {
                    double dx = p->x - 240.0;
                    double dy = p->y - 240.0;
                    double dist = sqrt(dx * dx + dy * dy);
                    if (dist > 1.0) {
                        p->muki = atan2(dy, dx);
                        p->speed = 2.5;
                    }
                }
                p = p->next;
            }

            // 中央から二重の衝撃波
            const int num = 24*3;
            for (int i = 0; i < num; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = 240.0;
                p->y = 240.0;
                p->muki = (double)i / num * 2.0 * DX_PI;
                p->speed = 3.2;
                p->kind = img_enemyShotSmallBall[4]; // 青
                p->param_i[0] = 3;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            for (int i = 0; i < num; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = 240.0;
                p->y = 240.0;
                p->muki = ((double)i + 0.5) / num * 2.0 * DX_PI;
                p->speed = 2.2;
                p->kind = img_enemyShotSmallBall[3]; // シアン
                p->param_i[0] = 3;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }

        if (t >= 48) {
            loopCnt++;
            phase = 0;
            t = -1;
        }
        break;
    }
    // --------------------------------------------------------
    default:
        break;
    }

    t++;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_SeitenNoHekireki_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 弾幕セットを1回だけ生成
    // （以降は ShotAotenNoHekirei が全フェーズを回す）
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAotenNoHekirei;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;   // phase
        pEnemyShotSet->param_i[1] = 0;   // t
        pEnemyShotSet->param_i[2] = 0;   // loopCnt
        pEnemyShotSet->param_i[3] = 240; // boltX
        pEnemyShotSet->param_i[4] = 0;   // boltCnt
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}