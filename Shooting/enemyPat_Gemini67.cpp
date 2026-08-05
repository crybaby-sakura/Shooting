// enemyPat_QuantumPhase.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：量子位相干渉波 (Quantum Phase Horizon)
static void ShotQuantumPhase(sEnemyShotSet* pEnemyShotSet)
{
    // 初期化：弾の生成
    if (pEnemyShotSet->count == 0) {
        // 全域結晶弾の生成
        // （4096発のメモリプール上限に配慮し、1回24way・秒間12回 = 秒間288発の発射とする）
        int way = 24;
        double base_muki = pEnemyShotSet->muki;

        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_muki + (DX_PI * 2.0 / way) * i;
            pEnemyShot->speed = 1.8; // 基準となる拡散速度

            // param_d を使用して「本来の等速直線運動の座標」を記憶しておく
            pEnemyShot->param_d[0] = pEnemyShot->x;    // 基準X
            pEnemyShot->param_d[1] = pEnemyShot->y;    // 基準Y
            pEnemyShot->param_d[2] = pEnemyShot->muki; // 基準向き

            // 初期弾画像
            pEnemyShot->kind = img_enemyShotMediumBall[0];

            // リストへ接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の更新ループ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 1. 基準座標の更新（何の影響も受けていない場合の等速直線運動）
        pShot->param_d[0] += pShot->speed * cos(pShot->param_d[2]);
        pShot->param_d[1] += pShot->speed * sin(pShot->param_d[2]);

        double bx = pShot->param_d[0];
        double by = pShot->param_d[1];

        // 2. 量子位相の計算（干渉波）
        // ① ボスを中心とした外へ広がる波
        double distFromBoss = sqrt((bx - enemy.x) * (bx - enemy.x) + (by - enemy.y) * (by - enemy.y));
        double wave1 = sin(distFromBoss * 0.04 - count * 0.06);

        // ② 自機（観測者）を中心とした内へ収束する波（自機が動くと波源が移動する）
        double distFromPlayer = sqrt((bx - player.x) * (bx - player.x) + (by - player.y) * (by - player.y));
        double wave2 = sin(distFromPlayer * 0.05 + count * 0.08);

        // ③ 干渉波の合成 (-2.0 ～ +2.0 の値を取る)
        double W = wave1 + wave2;

        // 3. 弾道歪みの適用
        // 進行方向に対して垂直（横方向）に、干渉波の強さに応じたオフセットを掛ける
        double offset_muki = pShot->param_d[2] + DX_PI / 2.0;
        double offset_length = W * 25.0; // 最大±50pxの強烈な空間の歪みが発生

        pShot->x = bx + offset_length * cos(offset_muki);
        pShot->y = by + offset_length * sin(offset_muki);

        // 4. 視覚的フィードバック（弾色の変化）
        // 波の位相によって弾の色を変え、モアレ（干渉縞）を可視化する
        if (W > 0.8) {
            // 波の山：弾が密集して壁になる高危険地帯
            pShot->kind = img_enemyShotMediumBall[0]; // 赤
        }
        else if (W > -0.8) {
            // 遷移領域
            pShot->kind = img_enemyShotMediumBall[5]; // マゼンタ
        }
        else {
            // 波の谷：弾が反発しあって「空間（安地）」が広がる領域
            pShot->kind = img_enemyShotMediumBall[4]; // 青
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン：量子位相干渉：事象の地平線
void EnemyPat_TheHardest_Gemini()
{
    static double base_angle;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 極限難易度のためHPは絶望的に高く設定

        base_angle = 0.0;
    }

    // ボス自身の揺らぎ移動（微小な移動が波源を揺らし、全体の干渉縞を複雑にする）
    enemy.x = 240.0 + sin(count * 0.02) * 40.0;
    enemy.y = 80.0 + cos(count * 0.015) * 15.0;

    // 5フレームに1回（秒間12回）、黄金角を用いた全方位弾を発射
    // これにより美しい螺旋（葉序）を描きながら画面全体が結晶状の弾で埋め尽くされる
    if (count % 5 == 0) {
        // 発射音の制御（毎フレーム鳴らすと五月蝿いので、一定間隔で鳴らす）
        if (count % 30 == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQuantumPhase;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 137.5度（黄金角）ずつ発射角度をずらすことで、弾の密度を均等かつ複雑にする
        base_angle += 137.5 / 180.0 * DX_PI;
        pEnemyShotSet->muki = base_angle;

        // リスト初期化と接続
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}