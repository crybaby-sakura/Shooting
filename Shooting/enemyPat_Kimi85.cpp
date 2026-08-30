// enemyPat_causalityCorridor.cpp
// 弾幕名：因果回廊（Causality Corridor）
// TAS前提の超高難易度位相幾何学弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  メインルーチン側で参照するプレイヤー位相
//  ※当たり判定処理で sEnemyShot::param_i[0] と比較し、
//    一致する場合のみダメージを与えるように改修してください。
// ============================================================
int g_PlayerPhase = 0;

// ============================================================
//  遡行弾（Retrospective）
//  発射後60F経過で、発射時のプレイヤー座標にワープして静止。
//  人間の記憶では「60F前にどこにいたか」は正確に把握できない。
// ============================================================
static void ShotRetrospective(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 12 way 全方位
        for (int i = 0; i < 12; i++) {
            p = new sEnemyShot;

            double angle = (DX_PI * 2.0 * i) / 12.0;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = angle;
            p->speed = 1.2;                         // 最初は遅く発射

            // 遡行先：発射時のプレイヤー座標（過去の自分を狙う）
            p->param_d[0] = pSet->param_d[0];       // targetX
            p->param_d[1] = pSet->param_d[1];       // targetY
            p->param_i[0] = pSet->param_i[0];       // phase (0,1,2)
            p->param_i[1] = 0;                      // state: 0=移動中, 1=遡行完了

            // 位相に応じた色：0=赤(α), 1=黄(β), 2=緑(γ)
            int col = p->param_i[0] % 3;
            p->kind = img_enemyShotSmallBall[col];  // 小玉
            p->margin = 20.0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[1] == 0) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);

            // 60F経過で「過去の座標」にワープし、そこで静止
            if (p->count >= 60) {
                p->param_i[1] = 1;
                p->x = p->param_d[0];
                p->y = p->param_d[1];
                p->speed = 0.0;
            }
        }
        p = p->next;
    }
}

// ============================================================
//  予言弾（Prophetic）
//  発射時に計算した未来座標に向かって60Fで到達、到達後静止。
//  プレイヤーの入力が変われば未来も変わるが、弾は予測された
//  座標にのみヒットボックスを持つ。
// ============================================================
static void ShotProphetic(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            p = new sEnemyShot;

            double tx = pSet->param_d[0];   // 未来予測X
            double ty = pSet->param_d[1];   // 未来予測Y

            p->x = pSet->x;
            p->y = pSet->y;
            double dx = tx - p->x;
            double dy = ty - p->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < 1.0) dist = 1.0;

            p->muki = atan2(dy, dx);
            p->speed = dist / 60.0;         // 60Fで到達する速度
            if (p->speed < 1.0) p->speed = 1.0;

            p->param_d[0] = tx;
            p->param_d[1] = ty;
            p->param_i[0] = pSet->param_i[0];
            p->param_i[1] = 0;              // 0=移動中, 1=到達・静止

            int col = p->param_i[0] % 3;
            p->kind = img_enemyShotDiamond[col]; // 菱形弾
            p->margin = 20.0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[1] == 0) {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);

            double dx = p->param_d[0] - p->x;
            double dy = p->param_d[1] - p->y;
            if (dx * dx + dy * dy < p->speed * p->speed * 2.0) {
                p->param_i[1] = 1;
                p->x = p->param_d[0];
                p->y = p->param_d[1];
                p->speed = 0.0;
            }
        }
        p = p->next;
    }
}

// ============================================================
//  軌跡結界（Trajectory Barrier）
//  30F前のプレイヤー座標に配置される静止弾。
//  「今通った道」が「30F後に塞がる」表現。
// ============================================================
static void ShotBarrier(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        p = new sEnemyShot;

        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki;           // プレイヤーの移動方向に垂直な向き
        p->speed = 0.0;                 // 完全静止

        p->param_i[0] = pSet->param_i[0];

        int col = p->param_i[0] % 3;
        p->kind = img_enemyShotMediumOval[col]; // 中楕円弾（壁の厚み）
        p->margin = 20.0;

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    // 静止したまま何もしない
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン：因果回廊
//  180F周期で「種蒔き→収穫→位相収束」を繰り返す。
// ============================================================
void EnemyPat_TAS_Kimi()
{
    static int muki;
    static int shot_count;
    static double playerHistX[120];
    static double playerHistY[120];
    static int histIndex = 0;
    static int phaseCooldown = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        histIndex = 0;
        g_PlayerPhase = 0;
        phaseCooldown = 0;

        for (int i = 0; i < 120; i++) {
            playerHistX[i] = player.x;
            playerHistY[i] = player.y;
        }
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 200.0) muki = 1;
        if (enemy.x > 280.0) muki = -1;
    }

    // --------------------------------------------------------
    //  プレイヤー座標履歴を循環バッファに記録（120F分）
    // --------------------------------------------------------
    playerHistX[histIndex] = player.x;
    playerHistY[histIndex] = player.y;
    histIndex = (histIndex + 1) % 120;

    // --------------------------------------------------------
    //  位相シフト（Xキー / 3Fクールタイム）
    //  Zキーはショットに使われている可能性があるためXキー割当。
    //  シフト中（3F）は攻撃不可をメインルーチン側で実装推奨。
    // --------------------------------------------------------
    if (phaseCooldown > 0) phaseCooldown--;
    if (key[KEY_INPUT_X] && phaseCooldown == 0) {
        g_PlayerPhase = (g_PlayerPhase + 1) % 3;
        phaseCooldown = 3;
    }

    int cycle = count % 180;

    // ========================================================
    //  0F：種蒔き（遡行弾12way + 予言弾3way）
    // ========================================================
    if (cycle == 0) {
        // --- 遡行弾セット ---
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRetrospective;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;
        pSet->muki = 0.0;
        pSet->param_d[0] = player.x;        // 遡行先：現在のプレイヤー位置
        pSet->param_d[1] = player.y;
        pSet->param_i[0] = GetRand(2);      // 位相ランダム（0,1,2）
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // --- 予言弾セット ---
        sEnemyShotSet* pSet2 = new sEnemyShotSet;
        pSet2->count = 0;
        pSet2->patternFunc = ShotProphetic;
        pSet2->x = enemy.x;
        pSet2->y = enemy.y + 15.0;
        pSet2->muki = 0.0;

        // プレイヤー速度を過去履歴から近似し、30F後の位置を予測
        int idxNow = (histIndex - 1 + 120) % 120;
        int idxPrev = (histIndex - 2 + 120) % 120;
        double pvx = playerHistX[idxNow] - playerHistX[idxPrev];
        double pvy = playerHistY[idxNow] - playerHistY[idxPrev];

        pSet2->param_d[0] = player.x + pvx * 30.0;
        pSet2->param_d[1] = player.y + pvy * 30.0;
        // 画面内にクランプ
        if (pSet2->param_d[0] < 0.0)   pSet2->param_d[0] = 0.0;
        if (pSet2->param_d[0] > 480.0) pSet2->param_d[0] = 480.0;
        if (pSet2->param_d[1] < 0.0)   pSet2->param_d[1] = 0.0;
        if (pSet2->param_d[1] > 480.0) pSet2->param_d[1] = 480.0;

        pSet2->param_i[0] = GetRand(2);
        pSet2->kind = shot_count++;

        pSet2->pEnemyShotHead = new sEnemyShot;
        pSet2->pEnemyShotHead->prev = pSet2->pEnemyShotHead;
        pSet2->pEnemyShotHead->next = pSet2->pEnemyShotHead;

        pSet2->prev = enemyShotSetHead.prev;
        pSet2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet2;
        enemyShotSetHead.prev = pSet2;
    }

    // ========================================================
    //  30F〜150F：軌跡結界生成（30F前のプレイヤー座標に配置）
    // ========================================================
    if (cycle >= 30 && cycle < 150 && count % 5 == 0) {
        int pastIdx = (histIndex - 30 + 120) % 120;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBarrier;
        pSet->x = playerHistX[pastIdx];
        pSet->y = playerHistY[pastIdx];

        // プレイヤーの移動方向を履歴から計算し、垂直な向きを設定
        int idx1 = (histIndex - 1 + 120) % 120;
        int idx2 = (histIndex - 8 + 120) % 120;
        double moveDir = atan2(playerHistY[idx1] - playerHistY[idx2],
            playerHistX[idx1] - playerHistX[idx2]);
        pSet->muki = moveDir + DX_PI / 2.0; // 移動方向に対して垂直

        pSet->param_i[0] = GetRand(2);
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ========================================================
    //  120F：位相シフト予告音（位相収束の演出）
    // ========================================================
    if (cycle == 120) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}