// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：砕け散る紅硝子（クリムゾン・フラグメント）
static void ShotCrimsonFragment(sEnemyShotSet* pEnemyShotSet)
{
    // count == 0: 予告と鏡面プリズムの配置
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 鏡面プリズムをボス周囲に配置 (5個)
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pPrism = new sEnemyShot;
            double angle = (DX_PI * 2.0 / 5.0) * i;
            pPrism->x = pEnemyShotSet->x + cos(angle) * 40.0;
            pPrism->y = pEnemyShotSet->y + sin(angle) * 40.0;
            pPrism->muki = 0.0;
            pPrism->speed = 0.0;

            // 大玉、白色(6) をプリズムとする
            pPrism->kind = img_enemyShotLargeBall[6];
            pPrism->param_i[0] = 0; // 0: 未炸裂

            pPrism->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pPrism->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pPrism;
            pEnemyShotSet->pEnemyShotHead->prev = pPrism;
        }
    }
    // count == 90: 炸裂！ (1.5秒後)
    else if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* nextShot = pShot->next;

            if (pShot->kind == img_enemyShotLargeBall[6] && pShot->param_i[0] == 0) {
                // プリズムが炸裂！紅硝子を扇状にばら撒く
                int num_shards = 15 * 3;
                double base_muki = atan2(player.y - pShot->y, player.x - pShot->x);
                for (int i = 0; i < num_shards; i++) {
                    sEnemyShot* pShard = new sEnemyShot;
                    // GetRand(60) は 0〜60 の 61 種類を返す。-30 して -30〜+30 の範囲のオフセットにする
                    double angle_offset = (GetRand(60) - 30) / 180.0 * DX_PI;
                    pShard->x = pShot->x;
                    pShard->y = pShot->y;
                    pShard->muki = base_muki + angle_offset;
                    pShard->speed = 3.5 + GetRand(100) / 100.0; // 3.5 〜 4.5

                    // 菱形弾、赤色(0) を紅硝子とする
                    pShard->kind = img_enemyShotDiamond[0];
                    pShard->param_i[0] = 0; // 反射回数 (0:未反射, 1:反射済み)

                    pShard->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShard->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShard;
                    pEnemyShotSet->pEnemyShotHead->prev = pShard;
                }

                // 炸裂済みのプリズムは役目を終えたのでリストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = nextShot;
        }

        // 同時にボスから拘束の円環（青い弾）を発射
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num_rings = 16;
        for (int i = 0; i < num_rings; i++) {
            sEnemyShot* pRing = new sEnemyShot;
            pRing->x = pEnemyShotSet->x;
            pRing->y = pEnemyShotSet->y;
            pRing->muki = (DX_PI * 2.0 / num_rings) * i;
            pRing->speed = 1.5; // 遅い

            // 中玉、青色(4) を円環とする
            pRing->kind = img_enemyShotMediumBall[4];
            pRing->param_i[0] = 99; // 反射しないフラグ代わり

            pRing->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRing->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRing;
            pEnemyShotSet->pEnemyShotHead->prev = pRing;
        }
    }
    // count > 0 (かつ 90 以外): 弾の移動と反射処理
    else {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 拘束の円環 (param_i[0] == 99) は反射しない
            if (pShot->param_i[0] == 99) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            // 紅硝子 (param_i[0] == 0 or 1)
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 画面端での反射判定 (画面サイズ 480x480, marginを考慮して10.0〜470.0で判定)
                // else if を使うことで、角での多重反射バグを防ぐ
                if (pShot->param_i[0] == 0) {
                    if (pShot->x <= 10.0 && cos(pShot->muki) < 0) {
                        pShot->muki = DX_PI - pShot->muki;
                        pShot->x = 10.0;
                        pShot->param_i[0] = 1; // 反射済み
                    }
                    else if (pShot->x >= 470.0 && cos(pShot->muki) > 0) {
                        pShot->muki = DX_PI - pShot->muki;
                        pShot->x = 470.0;
                        pShot->param_i[0] = 1;
                    }
                    else if (pShot->y <= 10.0 && sin(pShot->muki) < 0) {
                        pShot->muki = -pShot->muki;
                        pShot->y = 10.0;
                        pShot->param_i[0] = 1;
                    }
                    else if (pShot->y >= 470.0 && sin(pShot->muki) > 0) {
                        pShot->muki = -pShot->muki;
                        pShot->y = 470.0;
                        pShot->param_i[0] = 1;
                    }
                }
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Counter_Qwen()
{
    static int last_hp = 200;
    static int phase = 0; // 0:通常, 1:反撃中
    static sEnemyShotSet* pActiveShotSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        last_hp = 200;
        phase = 0;
        pActiveShotSet = nullptr;
    }

    // HP減少検知 (攻撃を受けた！)
    if (enemy.hp < last_hp) {
        if (phase == 0) {
            phase = 1; // 反撃シーケンス開始
        }
        last_hp = enemy.hp;
    }

    // フェーズごとのボスの挙動
    if (phase == 0) {
        // 通常移動：左右にゆっくり移動
        enemy.x += 1.2 * sin(count * 0.03);
        if (enemy.x < 60.0) enemy.x = 60.0;
        if (enemy.x > 420.0) enemy.x = 420.0;
        enemy.y = 40.0;
    }
    else if (phase == 1) {
        // 反撃中：ボスがその場で微動（苦悶の表現）
        enemy.y = 40.0 + sin(count * 0.5) * 2.0;

        // 反撃弾幕セットの生成は1回だけ
        if (pActiveShotSet == nullptr) {
            pActiveShotSet = new sEnemyShotSet;
            pActiveShotSet->count = 0;
            pActiveShotSet->patternFunc = ShotCrimsonFragment;
            pActiveShotSet->x = enemy.x;
            pActiveShotSet->y = enemy.y + 10.0;
            pActiveShotSet->kind = 0;

            pActiveShotSet->pEnemyShotHead = new sEnemyShot;
            pActiveShotSet->pEnemyShotHead->prev = pActiveShotSet->pEnemyShotHead;
            pActiveShotSet->pEnemyShotHead->next = pActiveShotSet->pEnemyShotHead;

            pActiveShotSet->prev = enemyShotSetHead.prev;
            pActiveShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pActiveShotSet;
            enemyShotSetHead.prev = pActiveShotSet;
        }

        // 反撃弾幕の終了判定 (count >= 150 で終了)
        if (pActiveShotSet != nullptr && pActiveShotSet->count >= 330) {
            // 残っている弾を削除
            sEnemyShot* pShot = pActiveShotSet->pEnemyShotHead->next;
            while (pShot != pActiveShotSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                delete pShot;
                pShot = next;
            }
            delete pActiveShotSet->pEnemyShotHead;

            // セット自体をリストから外して削除
            pActiveShotSet->prev->next = pActiveShotSet->next;
            pActiveShotSet->next->prev = pActiveShotSet->prev;
            delete pActiveShotSet;
            pActiveShotSet = nullptr;

            phase = 0; // 通常状態へ復帰
        }
    }
}