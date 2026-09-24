#include <math.h>

// ============================================================
// 高難易度版弾幕：トリプル・ターゲット・リング（輪投げモチーフ）
// ============================================================
static void ShotTargetRingHard(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    // 1. 発射（初期化）処理
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int MAIN_RING_BULLETS = 24;  // メインリング弾数
        const int INNER_TRAP_BULLETS = 6*4;   // 収縮時トラップ弾数
        const double RADIUS_INIT = 85.0;    // 初期半径

        // --- 外輪（締め付けリング） ---
        for (int i = 0; i < MAIN_RING_BULLETS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;     // リング中心X
            pEnemyShot->param_d[1] = pEnemyShotSet->y;     // リング中心Y
            pEnemyShot->param_d[2] = pEnemyShotSet->muki;  // 移動角度
            pEnemyShot->param_d[3] = 4.2;                  // 移動速度（高速化）
            pEnemyShot->param_d[4] = RADIUS_INIT;          // 半径
            pEnemyShot->param_d[5] = 0.0;                  // 自転角度
            pEnemyShot->param_d[6] = (2.0 * DX_PI / MAIN_RING_BULLETS) * i; // 相対角度
            pEnemyShot->param_i[0] = 0;                    // 弾種別：メインリング

            // 6発ごとに青/赤の中玉、他は赤色小玉
            if (i % 6 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青色中玉
            }
            else if (i % 6 == 3) {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤色中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[0];  // 赤色小玉
            }

            pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->param_d[4] * cos(pEnemyShot->param_d[6]);
            pEnemyShot->y = pEnemyShot->param_d[1] + pEnemyShot->param_d[4] * sin(pEnemyShot->param_d[6]);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 内輪（中心から放出される安地潰し弾） ---
        for (int i = 0; i < INNER_TRAP_BULLETS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = pEnemyShotSet->muki;
            pEnemyShot->param_d[3] = 4.2;
            pEnemyShot->param_d[4] = 0.0;
            pEnemyShot->param_d[5] = 0.0;
            pEnemyShot->param_d[6] = (2.0 * DX_PI / INNER_TRAP_BULLETS) * i;
            pEnemyShot->param_i[0] = 1; // 弾種別：トラップ弾

            pEnemyShot->kind = img_enemyShotDiamond[1]; // 黄色菱形弾
            pEnemyShot->muki = pEnemyShot->param_d[6];

            pEnemyShot->x = pEnemyShot->param_d[0];
            pEnemyShot->y = pEnemyShot->param_d[1];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 2. 毎フレームの更新処理
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int t = pShot->count;
        int type = pShot->param_i[0];

        if (type == 0) {
            // --- メインリングの挙動 ---
            if (t < 12) {
                // Phase 1: 構え（高速自転）
                pShot->param_d[5] += 0.25 / 3;
            }
            else if (t < 50) {
                // Phase 2: 高速飛翔
                pShot->param_d[0] += pShot->param_d[3] * cos(pShot->param_d[2]);
                pShot->param_d[1] += pShot->param_d[3] * sin(pShot->param_d[2]);
                pShot->param_d[5] += 0.08;
            }
            else if (t < 70) {
                // Phase 3: 超急速締め付け（85.0 -> 18.0へ強固に収縮）
                if (t == 50 && pShot == pEnemyShotSet->pEnemyShotHead->next) {
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }

                if (pShot->param_d[4] > 18.0) {
                    pShot->param_d[4] -= 3.35;
                }

                pShot->param_d[0] += (pShot->param_d[3] * 0.3) * cos(pShot->param_d[2]);
                pShot->param_d[1] += (pShot->param_d[3] * 0.3) * sin(pShot->param_d[2]);
            }
            else {
                // Phase 4: 縮小維持のまま再加速突破
                pShot->param_d[0] += (pShot->param_d[3] * 1.1) * cos(pShot->param_d[2]);
                pShot->param_d[1] += (pShot->param_d[3] * 1.1) * sin(pShot->param_d[2]);
            }

            double currentAngle = pShot->param_d[6] + pShot->param_d[5];
            pShot->x = pShot->param_d[0] + pShot->param_d[4] * cos(currentAngle);
            pShot->y = pShot->param_d[1] + pShot->param_d[4] * sin(currentAngle);
        }
        else if (type == 1) {
            // --- トラップ弾（中心放射）の挙動 ---
            if (t < 50) {
                // リング中心に同期移動
                pShot->param_d[0] += pShot->param_d[3] * cos(pShot->param_d[2]);
                pShot->param_d[1] += pShot->param_d[3] * sin(pShot->param_d[2]);
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
            }
            else if (t == 50) {
                // 締め付け開始と同時に全方位散弾として放出
                pShot->muki = pShot->param_d[6] + pShot->param_d[2];
                pShot->speed = 2.6;
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
            }
            else {
                // 放射状へ直進
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_RingToss_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) muki *= -1;
    }

    // 110フレームごとに自機狙いの輪を17フレーム間隔で3連投擲
    int cycle = count % 110;
    if (cycle == 1 || cycle == 18 || cycle == 35) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTargetRingHard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 発射時点の自機位置を毎回精度高く狙う
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}