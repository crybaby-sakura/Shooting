#include <cmath>
#include "gv.h"

// ----------------------------------------------------------------
// 風船弾幕：バルーン・クラッカー
// ----------------------------------------------------------------
static void ShotBalloonPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 【1. 風船本体の射出（最初の1フレーム目）】
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 0.8 + (GetRand(40) / 100.0); // 0.8～1.2のゆっくりした速度

        // 大玉（風船）をランダムな色で設定
        pEnemyShot->kind = img_enemyShotLargeBall[GetRand(7)];
        pEnemyShot->param_i[0] = 0;                     // 0: 風船本体
        pEnemyShot->param_d[0] = pEnemyShot->x;         // 左右にゆらゆら揺れる基準位置

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 【2. 弾の更新ループ】
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pShotNext = pShot->next; // 途中で delete される可能性に備えて次のノードを保持

        // ----------------------------------------------------
        // 種別 0 : 風船本体
        // ----------------------------------------------------
        if (pShot->param_i[0] == 0) {
            // ゆらゆらと漂いながら降下
            pShot->param_d[0] += cos(pShot->muki) * pShot->speed;
            pShot->y += sin(pShot->muki) * pShot->speed;
            pShot->x = pShot->param_d[0] + sin(pShot->count * 0.08) * 12.0;

            // 自機ショットとの当たり判定チェック
            bool isHit = false;
            sPlayerShot* pPShot = playerShotHead.next;
            while (pPShot != &playerShotHead) {
                sPlayerShot* pPShotNext = pPShot->next;
                double dx = pShot->x - pPShot->x;
                double dy = pShot->y - pPShot->y;

                // 大玉の判定半径 20.0px
                if (dx * dx + dy * dy <= 20.0 * 20.0) {
                    // 当たった自機ショットを削除
                    pPShot->prev->next = pPShot->next;
                    pPShot->next->prev = pPShot->prev;
                    delete pPShot;

                    isHit = true;
                    break;
                }
                pPShot = pPShotNext;
            }

            // 着弾して風船が割れた場合の処理
            if (isHit) {
                // 破裂音
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                double burstX = pShot->x;
                double burstY = pShot->y;

                // A. 全方位針弾（赤の菱形弾）
                int way = 16*3;
                double baseAngle = (GetRand(360) / 180.0) * DX_PI;
                for (int i = 0; i < way; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = burstX;
                    pNew->y = burstY;
                    pNew->muki = baseAngle + (2.0 * DX_PI / way) * i;
                    pNew->speed = 3.5;
                    pNew->kind = img_enemyShotDiamond[0]; // 赤色
                    pNew->param_i[0] = 1;                 // 1: 直線弾

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }

                // B. 破片紙吹雪弾（減速・停滞後自機へ飛ぶ小玉）
                int pieceCount = 12;
                for (int i = 0; i < pieceCount; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = burstX;
                    pNew->y = burstY;
                    pNew->muki = (GetRand(360) / 180.0) * DX_PI;
                    pNew->speed = 1.5 + (GetRand(250) / 100.0);       // 初期飛び出し速度
                    pNew->kind = img_enemyShotSmallBall[1 + GetRand(7)]; // カラフルな小玉
                    pNew->param_i[0] = 2;                             // 2: 破片弾
                    pNew->param_i[1] = 0;                             // 状態: 0=減速中
                    pNew->param_i[2] = 0;                             // タイマー

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }

                // 風船本体を削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                pShot = pShotNext;
                continue;
            }
        }
        // ----------------------------------------------------
        // 種別 1 : 破裂全方位弾
        // ----------------------------------------------------
        else if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // ----------------------------------------------------
        // 種別 2 : 破片紙吹雪弾（減速 -> 停滞 -> 自機狙い）
        // ----------------------------------------------------
        else if (pShot->param_i[0] == 2) {
            if (pShot->param_i[1] == 0) {
                // 減速フェーズ
                pShot->speed *= 0.88;
                if (pShot->speed < 0.15) {
                    pShot->speed = 0.0;
                    pShot->param_i[1] = 1; // 停止・溜めフェーズへ
                    pShot->param_i[2] = 0;
                }
            }
            else if (pShot->param_i[1] == 1) {
                // 停滞フェーズ（少しタイミングを散らして自機へ向ける）
                pShot->param_i[2]++;
                if (pShot->param_i[2] > 20 + GetRand(25)) {
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 1.8 + (GetRand(60) / 100.0);
                    pShot->param_i[1] = 2; // 発射完了・直進フェーズへ
                }
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShotNext;
    }
}

// ----------------------------------------------------------------
// 敵本体パターン関数
// ----------------------------------------------------------------
void EnemyPat_Balloon_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期位置とパラメータ設定
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体の左右揺動移動
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) muki *= -1;
    }

    // 40フレームごとに風船弾セット（ShotBalloonPattern）を発射
    if (count % 40 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBalloonPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 真下 ±30度のランダムな方向へ撃ち出し
        pEnemyShotSet->muki = (DX_PI * 0.5) + ((GetRand(60) - 30) / 180.0 * DX_PI);
        pEnemyShotSet->kind = shot_count++;

        // ダミーヘッドノード生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}