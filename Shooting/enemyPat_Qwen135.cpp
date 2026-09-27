#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：バックドラフト（密室の燻りと爆炎）
// ============================================================
static void ShotBackdraft(sEnemyShotSet* pSet)
{
    // pSet->param_i[0]: 現在のフェーズ (0:燻り, 1:換気, 2:逆風, 3:爆炎, 4:延焼)

    // フェーズ開始時の初期化
    if (pSet->count == 0) {
        pSet->param_i[0] = 0;
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 不気味な予告音
    }

    // --------------------------------------------------------
    // フェーズ0:【燻り】煤煙の充満 (0 ~ 180フレーム)
    // 画面内に黒い弾をゆっくりと滞留させ、圧迫感を高める
    // --------------------------------------------------------
    if (pSet->param_i[0] == 0) {
        if (pSet->count % 12 == 0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x + (double)(GetRand(200) - 100); // ボス周辺にばら撒く
            pShot->y = pSet->y + (double)(GetRand(100) - 50);
            pShot->muki = (double)GetRand(360) / 180.0 * DX_PI;
            pShot->speed = 0.3 + (double)GetRand(50) / 100.0; // 非常に遅い

            // 7:黒色 の大玉を使用
            pShot->kind = img_enemyShotLargeBall[7];

            // 状態管理用パラメータ初期化
            pShot->param_i[0] = 0; // 0:元は煤煙(大玉)
            pShot->param_i[1] = 0; // 0:火の粉をまだ生成していない
            pShot->param_i[2] = 0; // 爆発発生フレーム記録用

            // リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        if (pSet->count >= 180) {
            pSet->param_i[0] = 1; // 次のフェーズへ
            pSet->count = 0;      // フェーズ内のカウントをリセット
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK); // 風が吹くような重低音
        }
    }
    // --------------------------------------------------------
    // フェーズ1:【換気】酸素の流入 (0 ~ 60フレーム)
    // 画面左から右へ高速のシアンレーザーを走らせ、酸素供給を演出
    // --------------------------------------------------------
    else if (pSet->param_i[0] == 1) {
        if (pSet->count % 8 == 0 && pSet->count < 50) {
            sEnemyShot* pLaser = new sEnemyShot;
            pLaser->x = -50.0; // 画面左外から
            pLaser->y = 120.0 + (double)GetRand(160); // 画面中上部を横断
            pLaser->muki = 0.0; // 右向き
            pLaser->speed = 18.0; // 超高速
            pLaser->margin = 80;

            // 3:シアン色 のレーザーを使用
            pLaser->kind = img_enemyShotLaser[3];
            pLaser->param_i[0] = 1; // 1:レーザー

            pLaser->prev = pSet->pEnemyShotHead->prev;
            pLaser->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pLaser;
            pSet->pEnemyShotHead->prev = pLaser;
        }

        if (pSet->count >= 60) {
            pSet->param_i[0] = 2; // 次のフェーズへ
            pSet->count = 0;
        }
    }
    // --------------------------------------------------------
    // フェーズ2:【逆風】気流の発生 (0 ~ 30フレーム)
    // 滞留していた煤煙弾を、レーザーの進行方向(右)へ引き寄せる
    // --------------------------------------------------------
    else if (pSet->param_i[0] == 2) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) { // 煤煙弾であれば
                // 右方向(0.0)を基調としつつ、少しばらけさせる
                pShot->muki = 0.0 + (double)(GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = 4.0; // 吸い寄せられるように加速
            }
            pShot = pShot->next;
        }

        if (pSet->count >= 30) {
            pSet->param_i[0] = 3; // 次のフェーズへ
            pSet->count = 0;
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK); // 爆発音！
        }
    }
    // --------------------------------------------------------
    // フェーズ3:【爆炎】バックドラフト発動 (0 ~ 90フレーム)
    // 煤煙弾が一斉に赤/橙に変化し、自機方向へ爆発的な速度で飛翔する
    // --------------------------------------------------------
    else if (pSet->param_i[0] == 3) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 煤煙弾で、かつまだ爆発処理をしていないもの
            if (pShot->param_i[0] == 0 && pShot->param_i[2] == 0) {
                pShot->param_i[2] = pShot->count; // 爆発したフレームを記録

                // 0:赤 または 8:橙 をランダムに選択して炎を表現
                int color = (GetRand(1) == 0) ? 0 : 8;
                pShot->kind = img_enemyShotLargeBall[color];

                // 爆発的な加速
                pShot->speed = 7.0 + (double)GetRand(200) / 100.0;

                // 自機を狙う方向 + 爆発による多少のブレ
                double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->muki = targetMuki + (double)(GetRand(80) - 40) / 180.0 * DX_PI;
            }
            pShot = pShot->next;
        }

        if (pSet->count >= 90) {
            pSet->param_i[0] = 4; // 次のフェーズへ
            pSet->count = 0;
        }
    }
    // --------------------------------------------------------
    // フェーズ4:【延焼】二次災害 (90フレーム〜)
    // 爆炎が一定距離進んだ後、周囲に小さな火の粉を撒き散らす
    // --------------------------------------------------------
    else if (pSet->param_i[0] == 4) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            // 爆炎弾で、かつ爆発しており、かつまだ火の粉を生成していないもの
            if (pShot->param_i[0] == 0 && pShot->param_i[2] > 0 && pShot->param_i[1] == 0) {
                // 爆発から30フレーム経過したら火の粉を生成
                if (pShot->count - pShot->param_i[2] > 30) {
                    pShot->param_i[1] = 1; // 生成済みフラグ

                    // 火の粉の生成 (5方向)
                    for (int i = 0; i < 5; i++) {
                        sEnemyShot* pEmber = new sEnemyShot;
                        pEmber->x = pShot->x;
                        pEmber->y = pShot->y;
                        pEmber->muki = pShot->muki + (double)(GetRand(120) - 60) / 180.0 * DX_PI;
                        pEmber->speed = 2.5 + (double)GetRand(100) / 100.0;

                        // 1:黄色 の小玉を使用
                        pEmber->kind = img_enemyShotSmallBall[1];
                        pEmber->param_i[0] = 2; // 2:小玉(火の粉)

                        pEmber->prev = pSet->pEnemyShotHead->prev;
                        pEmber->next = pSet->pEnemyShotHead;
                        pSet->pEnemyShotHead->prev->next = pEmber;
                        pSet->pEnemyShotHead->prev = pEmber;
                    }
                }
            }

            // 全弾の移動処理
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_Backdraft_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // 煤煙が下に溜まる演出のため、やや下めに配置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // ボスは画面内でゆっくりと横揺れしながら浮遊する
        enemy.x = 240.0 + sin((double)count / 120.0 * DX_PI) * 100.0;
        enemy.y = 80.0 + cos((double)count / 180.0 * DX_PI) * 20.0;
    }

    // 開始から少し間を置いて(60フレーム後)、バックドラフトパターンを開始
    if (count % 500 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBackdraft;
        pSet->x = 240;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 弾リストのヘッド初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}