#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

#define CRACK_TIME 90 // 何フレームで殻が割れるか

// 焼き栗弾幕
static void ShotYakiguri(sEnemyShotSet* pSet)
{
    const double PI2 = DX_TWO_PI;

    // 0フレーム目: イガ栗を生成
    if (pSet->count == 0) {
        pSet->param_i[0] = 0; // 0:割れる前 1:割れた後

        // イガ 20発を円形に配置
        for (int i = 0; i < 20; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double ang = PI2 * i / 20.0;
            pShot->x = pSet->x + cos(ang) * 24.0;
            pShot->y = pSet->y + sin(ang) * 24.0;
            pShot->muki = ang; // 外向き
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotBullet[8]; // 銃弾(5.0x2.0) 橙 = イガ
            pShot->margin = 120.0;
            pShot->param_i[0] = 0; // type 0:イガ
            pShot->param_i[1] = 0; // 割れた後の初期化フラグ
            pShot->param_d[0] = ang; // 初期角度を保存
            pShot->param_d[2] = 0.0; // 重力用
            // 連結リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        // 中央の栗本体
        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = 0.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotMediumOval[8]; // 中楕円弾(10.5x7.0) 橙 = 栗本体
            pShot->margin = 40.0;
            pShot->param_i[0] = 1; // type 1:中央の栗
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 割れる瞬間
    if (pSet->count == CRACK_TIME) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        pSet->param_i[0] = 1;

        // イガに速度を付与、中央の栗を削除リストに
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* pNext = p->next;
            if (p->param_i[0] == 0) {
                // イガを外へ飛ばす GetRand注意: 0〜xのx+1種類
                p->speed = 1.2 + GetRand(100) / 100.0;
                p->param_i[1] = 0;
                // 少しランダムに角度を散らす
                p->muki += (GetRand(20) - 10) / 180.0 * DX_PI;
            }
            else if (p->param_i[0] == 1) {
                // 中央の栗は削除 = 破裂
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
            p = pNext;
        }

        // 中身(中玉黄) 12発 円形
        for (int i = 0; i < 12*2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = PI2 * i / 12.0/2;
            pShot->speed = 2.8 + GetRand(40) / 100.0;
            pShot->kind = img_enemyShotMediumBall[1]; // 中玉(7x7) 黄 = 中身
            pShot->margin = 40.0;
            pShot->param_i[0] = 2; // type 2:中身
            pShot->param_i[1] = 0; // 0:移動中 1:停止待ち
            pShot->param_i[2] = 0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        // 火花(小玉橙) 30発 ランダムばら撒き
        for (int i = 0; i < 30; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = PI2 * GetRand(360) / 360.0; // 0-360度ランダム
            pShot->speed = 4.0 + GetRand(200) / 100.0;
            pShot->kind = img_enemyShotSmallBall[8]; // 小玉(2.5x2.5) 橙 = 火花
            pShot->margin = 40.0;
            pShot->param_i[0] = 3; // type 3:火花
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレームの移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除に備えて先に保存

        if (pShot->param_i[0] == 0) { // イガ
            if (pSet->count < CRACK_TIME) {
                // 割れる前は栗の周りをゆっくり回転
                double baseAng = pShot->param_d[0];
                double ang = baseAng + pSet->count * 0.008;
                double rad = 24.0 + sin(pSet->count * 0.08) * 1.5;
                pShot->x = pSet->x + cos(ang) * rad;
                pShot->y = pSet->y + sin(ang) * rad;
                pShot->muki = ang;
            }
            else {
                // 割れた後は落下
                pShot->param_d[2] += 0.05; // 重力加算
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki) + pShot->param_d[2];
            }
        }
        else if (pShot->param_i[0] == 1) { // 中央の栗(割れる前のみ存在)
            pShot->x = pSet->x + sin(pSet->count * 0.15) * 2.0;
            pShot->y = pSet->y + cos(pSet->count * 0.12) * 2.0;
        }
        else if (pShot->param_i[0] == 2) { // 中身(黄中玉)
            if (pShot->param_i[1] == 0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                // 45フレーム飛んだら停止
                if (pShot->count > 45) {
                    pShot->speed = 0.0;
                    pShot->param_i[1] = 1;
                    pShot->param_i[2] = 18; // 停止後の待ち時間
                }
            }
            else if (pShot->param_i[1] == 1) {
                pShot->param_i[2]--;
                if (pShot->param_i[2] <= 0) {
                    // 3方向に分裂して自機狙い
                    double base = atan2(player.y - pShot->y, player.x - pShot->x);
                    for (int k = -1; k <= 1; k++) {
                        sEnemyShot* pNew = new sEnemyShot;
                        pNew->x = pShot->x;
                        pNew->y = pShot->y;
                        // GetRand(20)は0-20の21種類なので -10〜10に補正
                        pNew->muki = base + k * 0.55 + (GetRand(20) - 10) / 180.0 * DX_PI;
                        pNew->speed = 1.8 + GetRand(60) / 100.0;
                        pNew->kind = img_enemyShotSmallBall[1]; // 小玉 黄 = 崩れた欠片
                        pNew->margin = 40.0;
                        pNew->param_i[0] = 4;
                        pNew->prev = pSet->pEnemyShotHead->prev;
                        pNew->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pNew;
                        pSet->pEnemyShotHead->prev = pNew;
                    }
                    // 親は消滅
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }
        }
        else if (pShot->param_i[0] == 3) { // 火花
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.985; // 減速
        }
        else if (pShot->param_i[0] == 4) { // 分裂後の欠片
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

void EnemyPat_RoastChestnut_MetaAI()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.6 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 180フレーム毎に焼き栗を1つ生成
    if (count % 180 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotYakiguri;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}