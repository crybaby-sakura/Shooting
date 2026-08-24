#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：処理落ちスパイラル (Frame Drop Spiral)
static void ShotFrameDrop(sEnemyShotSet* pEnemyShotSet)
{
    int lagInterval = pEnemyShotSet->param_i[0];
    double speedMul = pEnemyShotSet->param_d[0];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 本来の移動量を計算し、未適用分として param_d に蓄積する
        double dx = pShot->speed * cos(pShot->muki) * speedMul;
        double dy = pShot->speed * sin(pShot->muki) * speedMul;

        pShot->param_d[0] += dx;
        pShot->param_d[1] += dy;

        // 更新タイミング（処理落ち中は間引かれ、フリーズ中は来ない）
        if (lagInterval > 0 && pEnemyShotSet->count % lagInterval == 0) {
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            // 適用したらリセット
            pShot->param_d[0] = 0.0;
            pShot->param_d[1] = 0.0;
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Lag_Qwen()
{
    static sEnemyShotSet* pSet = nullptr;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFrameDrop;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->param_i[0] = 1; // lagInterval (更新間隔)
        pSet->param_d[0] = 1.0; // speedMul (速度倍率)

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵の移動（ゆっくり左右に揺れる）
        enemy.x = 240.0 + 140.0 * sin(count * 0.015);
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // フェーズ管理 (周期 360 フレーム = 6秒)
        int phase = count % 360;

        if (phase < 120) {
            // Phase 1: 通常 (60fps相当)
            pSet->param_i[0] = 1;
            pSet->param_d[0] = 1.0;
        }
        else if (phase < 180) {
            // Phase 2: 負荷上昇 (20fps相当、3フレームに1回更新)
            pSet->param_i[0] = 3;
            pSet->param_d[0] = 1.0;
        }
        else if (phase < 240) {
            // Phase 3: 処理落ち (10fps相当、6フレームに1回更新)
            pSet->param_i[0] = 6;
            pSet->param_d[0] = 1.0;
        }
        else if (phase < 300) {
            // Phase 4: 完全フリーズ (更新停止、移動量のみ蓄積)
            pSet->param_i[0] = 9999; // 事実上更新タイミングが来ない値
            pSet->param_d[0] = 1.0;

            // フリーズ突入時の予告音
            if (phase == 240) {
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
        }
        else {
            // Phase 5: 復旧＆ラッシュ (ワープ＆加速)
            pSet->param_i[0] = 1; // 更新再開
            pSet->param_d[0] = 1.3; // 30%加速 (溜まっていた移動量も一気に適用される)
        }

        // 弾の生成 (10フレームごとに7way)
        if (count % 15 == 0) {
            // サウンド再生 (フリーズ中は発射音を止める)
            if (phase >= 240 && phase < 300) {
                // フリーズ中は何もしない
            }
            else {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            double baseMuki = atan2(player.y - pSet->y, player.x - pSet->x);

            for (int i = 0; i < 7; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y + 15.0;
                // 自機狙いベースに扇状＋ランダムノイズ
                pShot->muki = baseMuki + (i - 3) * 0.15 + (GetRand(20) - 10) / 100.0;
                pShot->speed = 3.0;

                // 画像種類と色 (中玉のシアン: 3)
                pShot->kind = img_enemyShotMediumBall[3];

                // 未適用移動量の初期化
                pShot->param_d[0] = 0.0;
                pShot->param_d[1] = 0.0;

                // リストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
}