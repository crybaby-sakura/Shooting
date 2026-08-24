// enemyPat_clockdownSpiral.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：クロックダウン・スパイラル（時空歪曲・疑似処理落ち）
static void ShotClockdownSpiral(sEnemyShotSet* pEnemyShotSet)
{
    // 500フレームで1サイクルの状態遷移
    int local_c = pEnemyShotSet->count % 500;

    // ----------------------------------------------------
    // フェーズ1: 展開（0〜179フレーム）
    // ----------------------------------------------------
    if (local_c < 180) {
        // 4フレームに1回、4wayの青色中玉を渦巻き状に発射
        if (local_c % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            double baseMuki = local_c * 0.1; // 回転していく角度
            for (int i = 0; i < 4; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = pEnemyShotSet->x;
                p->y = pEnemyShotSet->y;
                p->muki = baseMuki + DX_PI / 2.0 * i;
                p->speed = 2.0;
                p->kind = img_enemyShotMediumBall[4]; // 青玉
                p->param_i[0] = 0; // 0: 通常弾（未来へワープする実体）

                // リストへの追加
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // ----------------------------------------------------
    // フェーズ2: 疑似処理落ち開始（180フレーム目）
    // ----------------------------------------------------
    if (local_c == 180) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 演出用：現在のプレイヤー座標を保存（フリーズ演出に使う）
        pEnemyShotSet->param_d[0] = player.x;
        pEnemyShotSet->param_d[1] = player.y;

        // すでに発射された通常弾から、未来位置を示すゴースト（残像）を生成する
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 40, 80, 120フレーム後の位置にゴーストを設置
                for (int f = 1; f <= 3; f++) {
                    sEnemyShot* g = new sEnemyShot;
                    g->x = pShot->x + (f * 40) * pShot->speed * cos(pShot->muki);
                    g->y = pShot->y + (f * 40) * pShot->speed * sin(pShot->muki);
                    g->muki = pShot->muki;
                    g->speed = 0; // ゴーストは動かない
                    g->kind = img_enemyShotSmallBall[3]; // シアンの小玉（点線状の残像）
                    g->param_i[0] = 1; // 1: ゴースト弾

                    g->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    g->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = g;
                    pEnemyShotSet->pEnemyShotHead->prev = g;
                }
            }
            pShot = pShot->next;
        }
    }

    // ----------------------------------------------------
    // フェーズ3: 画面完全フリーズ（181〜299フレーム）
    // ----------------------------------------------------
    if (180 < local_c && local_c < 300) {
        // 自機の座標を保存した位置で上書きし、操作不能の「処理落ち」を演出
        player.x = pEnemyShotSet->param_d[0];
        player.y = pEnemyShotSet->param_d[1];
    }

    // ----------------------------------------------------
    // フェーズ4: ラグ解消・一斉ワープ＆バースト（300フレーム目）
    // ----------------------------------------------------
    if (local_c == 300) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ====================================================
    // 全弾の更新とワープ処理ループ
    // ====================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        // ゴースト弾の消去（ワープの瞬間に消え去る）
        if (local_c == 300 && pShot->param_i[0] == 1) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = nextShot;
            continue;
        }

        // 弾の移動処理
        bool isFrozen = (180 <= local_c && local_c < 300);
        // フリーズ中以外、またはバースト弾（param_i[0]==2）なら動く
        if (!isFrozen || pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // コマ飛び（ワープ）とバースト処理
        if (local_c == 300 && pShot->param_i[0] == 0) {
            // 120フレーム分一気に未来へワープ
            pShot->x += 120 * pShot->speed * cos(pShot->muki);
            pShot->y += 120 * pShot->speed * sin(pShot->muki);
            pShot->kind = img_enemyShotMediumBall[0]; // 赤玉に変化

            // ワープ地点から小さな赤い鱗弾を四方に散らす（GetRand(x)仕様を考慮）
            for (int i = 0; i < 4 * 3; i++) {
                sEnemyShot* b = new sEnemyShot;
                b->x = pShot->x;
                b->y = pShot->y;
                b->speed = 1.0 + GetRand(100) / 100.0; // 1.0〜2.0のランダム速度
                b->muki = pShot->muki + DX_PI / 2.0 / 3 * i + DX_PI / 4.0;
                b->kind = img_enemyShotScale[0]; // 赤の鱗弾
                b->param_i[0] = 2; // 2: バースト弾

                b->prev = pEnemyShotSet->pEnemyShotHead->prev;
                b->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = b;
                pEnemyShotSet->pEnemyShotHead->prev = b;
            }
        }

        pShot = nextShot;
    }
}

// 敵本体のパターン
void EnemyPat_Lag_Gemini()
{
    const int T = 120 + 240 + 240 + 360;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0; // 画面中央
        enemy.y = 100.0; // 少し上
        enemy.maxHp = enemy.hp = 200 + T / 6;
    }

    // 少し待ってから弾幕管理セットを1つだけ生成
    if (count == T) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotClockdownSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 弾幕の発生源を敵の座標に追従させる
    if (count > T) {
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotClockdownSpiral) {
                pSet->x = enemy.x;
                pSet->y = enemy.y;
            }
            pSet = pSet->next;
        }
    }
}