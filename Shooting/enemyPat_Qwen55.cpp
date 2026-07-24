// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：星喰うネオン蓮（ロトス）の絶頂
// ============================================================
static void ShotNeonLotus(sEnemyShotSet* pSet)
{
    // count == 0: 初期化フェーズ
    if (pSet->count == 0) {
        // 予告音で緊張感を高める
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_d[0]: 蓮の花全体の回転角度（ラジアン）
        pSet->param_d[0] = 0.0;
        // param_i[0]: フェーズ管理 (0:収束, 1:絶頂開花)
        pSet->param_i[0] = 0;
    }

    // フェーズ1: 渦巻きの収束 (0フレーム 〜 119フレーム)
    // 画面外から中心に向かって、ネオンカラーの小玉が螺旋状に吸い込まれる
    if (pSet->count < 120) {
        if (pSet->count % 8 == 0) {
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShot = new sEnemyShot;

                // 4方向から中心へ向かう角度 + 螺旋のためのオフセット
                double baseAngle = pSet->param_d[0] + (i * DX_PI_F / 2.0);

                // 画面端付近から発生させる (半径300)
                pShot->x = pSet->x + cos(baseAngle) * 300.0;
                pShot->y = pSet->y + sin(baseAngle) * 300.0;

                // 中心に向かうベクトル(+PI) + 螺旋回転成分(+0.4)
                pShot->muki = baseAngle + DX_PI_F + 0.4;
                pShot->speed = 3.5;

                // マゼンタ(5) と シアン(3) を交互に配置
                int color = (i % 2 == 0) ? 5 : 3;
                pShot->kind = img_enemyShotSmallBall[color];

                pShot->margin = 480;

                // リストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // フェーズ2: 絶頂の花弁 (120フレーム 〜 300フレーム)
    // 中心から巨大な蓮の花弁状の弾幕が回転しながら拡散する
    else if (pSet->count == 120) {
        // 開花時のインパクトのある効果音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }
    else if (pSet->count > 120 && pSet->count < 300) {
        // 蓮の花全体をゆっくりと回転させる
        pSet->param_d[0] += 0.015;

        // 6フレームごとに「花弁の輪」を生成
        if (pSet->count % 6 == 0) {
            int petals = 16; // 16枚の花弁

            for (int i = 0; i < petals; i++) {
                double angle = pSet->param_d[0] + (i * 2.0 * DX_PI_F / petals);

                // 花弁は「外側(シアン)」「中間(マゼンタ)」「芯(ゴールド)」の3層構造にして厚みと発光感を演出
                for (int layer = 0; layer < 3; layer++) {
                    if (layer == 2 && pSet->count % 12 != 0) continue;

                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = pSet->x;
                    pShot->y = pSet->y;
                    pShot->muki = angle;

                    if (layer == 0) {
                        // 外側: シアンの菱形弾 (速め)
                        pShot->kind = img_enemyShotDiamond[3];
                        pShot->speed = 3.2;
                    }
                    else if (layer == 1) {
                        // 中間: マゼンタの鱗弾 (標準)
                        pShot->kind = img_enemyShotScale[5];
                        pShot->speed = 2.6;
                    }
                    else {
                        // 芯: ゴールド(黄)の大玉 (遅め)。ここが当たり判定の核であり、視覚的なアンカーになる
                        pShot->kind = img_enemyShotLargeBall[1];
                        pShot->speed = 2.0;
                    }

                    // 速度に微細な揺らぎ(GetRand)を与え、機械的すぎない「生物的なうねり」を表現
                    // GetRand(40) は 0〜40 を返すので、/100.0 で 0.0〜0.4 の揺らぎになる
                    pShot->speed += GetRand(40) / 100.0;

                    // リストに追加
                    pShot->prev = pSet->pEnemyShotHead->prev;
					pShot->next = pSet->pEnemyShotHead;
					pSet->pEnemyShotHead->prev->next = pShot;
					pSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }

    // 弾の移動処理 (メインルーチンがcountを管理し、画面外削除も行うため、ここでは移動のみ記述)
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体のパターン (新規作成用テンプレート)
// ============================================================
void EnemyPat_ThumbnailFriendly_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0; // やや下に配置し、蓮が広がるスペースを確保
        enemy.maxHp = enemy.hp = 200; // パターンに見合った耐久力
        muki = 1;
    }
    else {
        // 敵本体の動き: 左右にゆっくりと往復し、プレイヤーに狙い撃ちさせない動き
        enemy.x += 1.2 * (double)muki;

        // 画面端で折り返し (480x480の範囲、余裕を持って 80〜400 で折り返す)
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // count == 60 で弾幕セットを生成（最初の60フレームは登場演出とチャージ時間）
    if (count % 400 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNeonLotus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 蓮のパターン内では自前計算するため、ここでは初期値でOK
        pEnemyShotSet->kind = 0;

        // 弾リストのヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}