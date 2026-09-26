#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：バブルソート・アセンション
static void ShotBubbleSort(sEnemyShotSet* pEnemyShotSet)
{
    // 初期化フラグとして param_i[15] を使用 (0:未初期化, 1:初期化済み)
    if (pEnemyShotSet->param_i[15] == 0) {
        pEnemyShotSet->param_i[15] = 1;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 5つの泡を生成（5列の配列に見立てる）
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = 120.0 + i * 60.0;       // 列: 120, 180, 240, 300, 360
            pShot->y = 480.0 + i * 10.0;       // 画面下部から少しずらして発生

            pShot->param_i[0] = GetRand(8) + 1; // 値: 1〜9
            pShot->param_i[1] = i;              // 現在の列インデックス 0〜4

            // 値に応じて色を変更
            int val = pShot->param_i[0];
            int color = 4; // デフォルト青
            if (val == 1) color = 4;
            else if (val == 2) color = 3;
            else if (val == 3) color = 2;
            else if (val == 4) color = 1;
            else if (val == 5) color = 8;
            else if (val == 6) color = 0;
            else if (val == 7) color = 5;
            else if (val == 8) color = 6;
            else if (val == 9) color = 7;

            // 値が大きいほど泡も大きくする
            if (val >= 7) {
                pShot->kind = img_enemyShotLargeBall[color];
            }
            else {
                pShot->kind = img_enemyShotMediumBall[color];
            }

            pShot->speed = 2.5 / 2; // 上昇速度
            pShot->margin = 100.0; // 画面外消去マージン

            // リンクリストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // バブルソートの状態初期化
        pEnemyShotSet->param_i[0] = 0; // pass (比較ラウンド)
        pEnemyShotSet->param_i[1] = 0; // j (比較インデックス)
        pEnemyShotSet->param_i[2] = 0; // 状態 (0:ソート中, 1:完了)
        pEnemyShotSet->param_i[3] = 0; // レーザー発射フラグ
    }

    // ==========================================
    // ソート完了後の処理（泡の破裂とレーザー発射）
    // ==========================================
    if (pEnemyShotSet->param_i[2] == 1) {
        if (pEnemyShotSet->param_i[3] == 0) {
            pEnemyShotSet->param_i[3] = 1;

            // 整列された値の最大値を取得し、泡をすべて削除
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            int max_val = 0;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] > max_val) max_val = pShot->param_i[0];

                // 泡をリンクリストから外して削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                pShot = pEnemyShotSet->pEnemyShotHead->next;
            }

            // 完了時のレーザー発射
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            int laser_count = max_val; // 最大値の本数だけ発射
            pEnemyShotSet->y = 0.0;
            double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            for (int i = 0; i < laser_count; i++) {
                sEnemyShot* pLaser = new sEnemyShot;
                pLaser->x = pEnemyShotSet->x;
                pLaser->y = pEnemyShotSet->y;
                // 扇状に広がるように角度を調整
                pLaser->muki = base_muki + (i - (laser_count - 1) / 2.0) * (DX_PI / 16.0);
                pLaser->speed = 4.0;
                pLaser->kind = img_enemyShotLaser[6]; // 白レーザー
                pLaser->margin = 100.0;

                pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pLaser->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
                pEnemyShotSet->pEnemyShotHead->prev = pLaser;
            }
        }

        // レーザーの移動処理
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
        return;
    }

    // ==========================================
    // ソート中の処理（上昇と比較・交換）
    // ==========================================
    // 泡の上昇処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y -= pShot->speed;
        pShot = pShot->next;
    }

    // バブルソートの比較・交換処理 (30フレームごとに1ステップ進行)
    if (pEnemyShotSet->count % 30 == 0) {
        int pass = pEnemyShotSet->param_i[0];
        int j = pEnemyShotSet->param_i[1];

        if (pass < 5) {
            if (j < 4 - pass) {
                // j 番目と j+1 番目の泡をリンクリストから探す
                sEnemyShot* shot_j = nullptr;
                sEnemyShot* shot_j1 = nullptr;

                pShot = pEnemyShotSet->pEnemyShotHead->next;
                while (pShot != pEnemyShotSet->pEnemyShotHead) {
                    if (pShot->param_i[1] == j) shot_j = pShot;
                    if (pShot->param_i[1] == j + 1) shot_j1 = pShot;
                    pShot = pShot->next;
                }

                if (shot_j && shot_j1) {
                    // 左 > 右 なら交換（スワップ）
                    if (shot_j->param_i[0] > shot_j1->param_i[0]) {
                        int tmp_col = shot_j->param_i[1];
                        shot_j->param_i[1] = shot_j1->param_i[1];
                        shot_j1->param_i[1] = tmp_col;

                        // 2. x座標を即時交換（視覚的な位置入れ替え）
                        double tmp_x = shot_j->x;
                        shot_j->x = shot_j1->x;
                        shot_j1->x = tmp_x;

                        // 3. 交換ノイズ弾（自機狙い）を発生させて「反発」を表現
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                        double mid_x = (shot_j->x + shot_j1->x) / 2.0;
                        double mid_y = (shot_j->y + shot_j1->y) / 2.0;
                        double noise_muki = atan2(player.y - mid_y, player.x - mid_x);

                        for (int k = 0; k < 3; k++) {
                            sEnemyShot* pNoise = new sEnemyShot;
                            pNoise->x = mid_x;
                            pNoise->y = mid_y;

                            pNoise->param_i[0] = 0;  // ソート完了時の最大値判定に影響させない
                            pNoise->param_i[1] = -1; // バブルソートの列検索(j)対象から除外

                            pNoise->muki = noise_muki + (k - 1) * (DX_PI / 8.0);
                            pNoise->speed = 3.0;
                            pNoise->kind = img_enemyShotSmallBall[1]; // 黄色小玉
                            pNoise->margin = 50.0;

                            pNoise->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pNoise->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pNoise;
                            pEnemyShotSet->pEnemyShotHead->prev = pNoise;
                        }
                    }
                }
                pEnemyShotSet->param_i[1]++; // j++
            }
            else {
                // 1ラウンド終了。次のpassへ
                pEnemyShotSet->param_i[0]++; // pass++
                pEnemyShotSet->param_i[1] = 0; // j = 0
            }
        }
        else {
            // 全てのpassが完了 = ソート完了
            pEnemyShotSet->param_i[2] = 1;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // HPは300で固定
        shot_count = 0;
    }
    else {
        // 敵は左右にゆっくりと正弦波移動
        enemy.x += 0.5 * sin(count * 0.02);
    }

    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleSort;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = 480.0; // 画面下部から泡が発生する起点
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_i[15] = 0; // 初期化フラグを明示的にリセット

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}