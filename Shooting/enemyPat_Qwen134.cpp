#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：禁忌「カゴメカゴメ」
static void ShotKagomeKagome(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ管理 (param_i[0] を使用)
    // 0: 初期化（網目配置）
    // 1: 黄大弾発射待ち
    // 2: 黄大弾発射＆干渉判定中

    if (pEnemyShotSet->count == 0) {
        // 効果音: 重く不気味な音で網目配置を開始
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 画面全体をカバーする格子状に緑小弾を配置 (10x10 = 100発)
        for (int gy = 0; gy < 10; gy++) {
            for (int gx = 0; gx < 10; gx++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = 60.0 + gx * 40.0;
                pShot->y = 60.0 + gy * 40.0;
                pShot->muki = 0.0;
                pShot->speed = 0.0; // 待機状態
                pShot->kind = img_enemyShotSmallBall[2]; // 緑小玉
                pShot->param_i[0] = 0; // 0:待機, 1:活性化済み

                // 双方向リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
        pEnemyShotSet->param_i[0] = 1; // 次のフェーズへ
    }
    else if (pEnemyShotSet->count == 60) {
        // 60フレーム(約1秒)後に黄大弾を発射
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pBigShot = new sEnemyShot;
        pBigShot->x = pEnemyShotSet->x;
        pBigShot->y = pEnemyShotSet->y;
        pBigShot->muki = atan2(player.y - pBigShot->y, player.x - pBigShot->x);
        pBigShot->speed = 3.5;
        pBigShot->kind = img_enemyShotLargeBall[1]; // 黄大玉
        pBigShot->param_i[0] = 2; // 大弾フラグ

        pBigShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBigShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBigShot;
        pEnemyShotSet->pEnemyShotHead->prev = pBigShot;

        pEnemyShotSet->param_i[0] = 2; // 干渉判定フェーズへ
    }

    // 弾の移動と干渉判定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            // 黄大弾の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 0) {
            // 待機中の緑小弾
            bool activated = false;

            // 1. 黄大弾との干渉判定
            sEnemyShot* pBig = pEnemyShotSet->pEnemyShotHead->next;
            while (pBig != pEnemyShotSet->pEnemyShotHead) {
                if (pBig->param_i[0] == 2) {
                    double dx = pShot->x - pBig->x;
                    double dy = pShot->y - pBig->y;
                    double dist = sqrt(dx * dx + dy * dy);
                    if (dist < 30.0) { // 干渉距離
                        activated = true;
                    }
                    break; // 大弾は1つなので見つかったらループ抜けてOK
                }
                pBig = pBig->next;
            }

            // 2. 時間経過による自然崩壊（禁忌の制御不能感）
            // 180フレーム(3秒)経過以降、毎フレーム2%の確率でランダムに崩れ始める
            if (pEnemyShotSet->count > 180 && GetRand(100) < 2) {
                activated = true;
            }

            if (activated) {
                pShot->param_i[0] = 1;
                // 崩壊表現として、白鱗弾に変化させて飛び散らせる
                pShot->kind = img_enemyShotScale[6]; // 白鱗弾

                // 自機から遠ざかる方向を基準に、ばらつきを持たせる
                double away_muki = atan2(pShot->y - player.y, pShot->x - player.x);
                // GetRand(60)は0〜60を返すので、-30〜30の範囲になる。これをラジアンに変換
                away_muki += (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->muki = away_muki;

                // 速度は 2.0 〜 4.0 の範囲でランダム (GetRand(100)は0〜100)
                pShot->speed = 2.0 + GetRand(100) / 50.0;
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 活性化して飛び散る弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KagomeKagome_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 左右へのゆっくりした移動（画面端で折り返し）
        enemy.x += 1.5 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 240フレーム(約4秒)ごとにカゴメカゴメを発動
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKagomeKagome;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0; // フェーズ初期化

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}