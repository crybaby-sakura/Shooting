// enemyPat_KaitenBouNuke.cpp
//
// 回転棒抜け-くるくるくるりん- : GBA「くるくるくるりん」の基本ギミック
// (回転する棒を狭い隙間へぶつからずに通し抜けるゲーム性)をモチーフにした
// 4フェーズ無限ループパターン。専用素材が無いため、既存の敵弾素材の組み合わせで表現する。
//
// ①棒組み立てフェーズ : 中心から左右対称に、根本→先端へ時間差でセグメントが伸びていく
// ②定常回転フェーズ   : 棒が一定角速度で回転しつつ、隙間位置がゆっくり左右移動する壁が
//                         下からせり上がり、隙間位置から自機狙い3wayが周期発射される
// ③加速航行フェーズ   : 棒の角速度が二次関数的に加速し先端から曳光弾(短レーザー)を放出、
//                         壁の出現間隔が短縮し隙間の振幅も拡大していく
// ④警告→解放フェーズ  : 棒が停止し白→赤で点滅して警告。解放後は停止時点の角度をそのまま
//                         初速方向として放射状に加速飛散。同時に壁の残存弾も左右へ加速排出し、
//                         中心から自機狙い5wayフィニッシュを発射して①へループする
//
// 敵本体の関数名は指定により void EnemyPat_KuruKuruKururin_Claude() とする。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ---- 全体サイクル設定 ----
    constexpr int CYCLE_LEN = 900;  // 1周期のフレーム数
    constexpr int PHASE1_LEN = 120;  // ①組み立て
    constexpr int PHASE2_LEN = 320;  // ②定常回転
    constexpr int PHASE3_LEN = 240;  // ③加速航行
    // ④警告→解放は残り 900-120-300-240 = 240 フレーム

    constexpr int PHASE1_END = PHASE1_LEN;              // 120
    constexpr int PHASE2_END = PHASE1_END + PHASE2_LEN; // 420
    constexpr int PHASE3_END = PHASE2_END + PHASE3_LEN; // 660
    constexpr int RELEASE_AT = PHASE3_END + 40;          // 700 (警告40フレーム後に解放)

    // ---- 棒(回転バー)設定 ----
    constexpr int    ROD_SEG_PER_SIDE = 8;
    constexpr double ROD_SEG_SPACING = 18.0;
    constexpr double ROD_OMEGA0 = 0.030; // ②定常角速度(rad/frame)
    constexpr double ROD_OMEGA1 = 0.095; // ③終了時点の角速度
    constexpr double ROD_ACCEL = (ROD_OMEGA1 - ROD_OMEGA0) / static_cast<double>(PHASE3_LEN);
    constexpr double ROD_ANGLE_AT_PHASE2_END = ROD_OMEGA0 * static_cast<double>(PHASE2_LEN);

    // サイクル先頭からの累積回転角を返す(棒は剛体なので全セグメント共通)
    double RodAngle(int local)
    {
        if (local < PHASE1_END) {
            return 0.0; // ①組み立て中は静止
        }
        else if (local < PHASE2_END) {
            double t = static_cast<double>(local - PHASE1_END);
            return ROD_OMEGA0 * t;
        }
        else if (local < PHASE3_END) {
            double t = static_cast<double>(local - PHASE2_END);
            return ROD_ANGLE_AT_PHASE2_END + ROD_OMEGA0 * t + 0.5 * ROD_ACCEL * t * t;
        }
        else {
            // ④警告中は③終了時点の角度で凍結
            double t = static_cast<double>(PHASE3_LEN);
            return ROD_ANGLE_AT_PHASE2_END + ROD_OMEGA0 * t + 0.5 * ROD_ACCEL * t * t;
        }
    }

    // ---- 壁(コリドー)設定 ----
    constexpr int    WALL_SLOTS = 24;
    constexpr double WALL_SLOT_PITCH = 480.0 / WALL_SLOTS; // 20.0
    constexpr int    WALL_GAP_HALF = 3;   // 中心スロット±1 → 3スロット分(=60px)の隙間
    constexpr double WALL_SPAWN_Y = 510.0;
    constexpr double WALL_GAP_AMP_MIN = 3.0; // ②の隙間振幅(スロット数換算)
    constexpr double WALL_GAP_AMP_MAX = 5.0; // ③の隙間振幅

    // 壁の行が出現した瞬間(spawnLocal)における隙間中心スロットを返す
    double WallGapCenterSlot(int spawnLocal)
    {
        double amp, freq;
        if (spawnLocal < PHASE2_END) {
            amp = WALL_GAP_AMP_MIN;
            freq = 2.0 * DX_PI / 260.0;
        }
        else {
            amp = WALL_GAP_AMP_MAX;
            freq = 2.0 * DX_PI / 150.0;
        }
        return (WALL_SLOTS - 1) / 2.0 + amp * sin(freq * static_cast<double>(spawnLocal));
    }

} // namespace

// ============================================================
// 棒(回転バー)のセグメント弾
// ============================================================
static void ShotRod(sEnemyShotSet* pEnemyShotSet)
{
    int local = count % CYCLE_LEN;

    if (local == 0) {
        // 新しいサイクルの開始：本数カウンタと解放フラグをリセットして再構築へ
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
    }

    // --- ①組み立て：根本から先端へ時間差でセグメントを生やす ---
    if (local < PHASE1_END) {
        int spawnedPairs = pEnemyShotSet->param_i[0];
        int targetPairs = ((local + 1) * ROD_SEG_PER_SIDE) / PHASE1_LEN;
        while (spawnedPairs < targetPairs && spawnedPairs < ROD_SEG_PER_SIDE) {
            for (int side = -1; side <= 1; side += 2) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                bool isTip = (spawnedPairs == ROD_SEG_PER_SIDE - 1);
                pEnemyShot->kind = isTip ? img_enemyShotMediumOval[5]  // 先端: マゼンタの中楕円弾
                    : img_enemyShotDiamond[6];   // 本体: 白の菱形弾
                pEnemyShot->param_i[0] = side;                              // +1 or -1
                pEnemyShot->param_i[1] = spawnedPairs;                      // セグメント番号(0=根本側)
                pEnemyShot->param_d[0] = (spawnedPairs + 1) * ROD_SEG_SPACING; // 中心からの固定距離
                pEnemyShot->x = enemy.x;
                pEnemyShot->y = enemy.y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->margin = 480;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
            spawnedPairs++;
        }
        pEnemyShotSet->param_i[0] = spawnedPairs;
    }

    // --- ④解放：警告後、現在の角度を初速方向として確定する ---
    bool released = (local >= RELEASE_AT);
    if (released && pEnemyShotSet->param_i[1] == 0) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double baseAngle = RodAngle(PHASE3_END) + ((pShot->param_i[0] < 0) ? DX_PI : 0.0);
            pShot->muki = baseAngle;                 // 解放方向(=停止時点の半径方向)を確定
            pShot->param_d[1] = pShot->param_d[0];    // 解放時点の半径を保存
            pShot->param_i[2] = local;                // 解放開始フレーム(cycle-local)を記録
            pShot = pShot->next;
        }
        pEnemyShotSet->param_i[1] = 1; // 解放済みフラグ
    }

    // --- 全セグメントの位置更新 ---
    bool warning = (local >= PHASE3_END && local < RELEASE_AT); // 警告点滅ウィンドウ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (!released) {
            double angle = RodAngle(local) + ((pShot->param_i[0] < 0) ? DX_PI : 0.0);
            double r = pShot->param_d[0];
            pShot->x = enemy.x + r * cos(angle);
            pShot->y = enemy.y + r * sin(angle);
            pShot->muki = angle;

            if (warning) {
                // 白→赤の点滅で衝突寸前を警告
                bool flashRed = ((local / 4) % 2 == 0);
                bool isTip = (pShot->param_i[1] == ROD_SEG_PER_SIDE - 1);
                pShot->kind = isTip
                    ? (flashRed ? img_enemyShotMediumOval[0] : img_enemyShotMediumOval[5])
                    : (flashRed ? img_enemyShotDiamond[0] : img_enemyShotDiamond[6]);
            }
        }
        else {
            double t = static_cast<double>(local - pShot->param_i[2]);
            double r = pShot->param_d[1] + 0.6 * t + 0.02 * t * t; // 放射状に加速飛散
            pShot->x = enemy.x + r * cos(pShot->muki);
            pShot->y = enemy.y + r * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 棒先端から放出する曳光弾(③加速航行フェーズのみ)
// ============================================================
static void ShotRodTrail(sEnemyShotSet* pEnemyShotSet)
{
    int local = count % CYCLE_LEN;

    if (local >= PHASE2_END && local < PHASE3_END && (local - PHASE2_END) % 12 == 0) {
        double angle = RodAngle(local);
        double tipR = ROD_SEG_PER_SIDE * ROD_SEG_SPACING;
        for (int side = -1; side <= 1; side += 2) {
            double a = angle + ((side < 0) ? DX_PI : 0.0);
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = enemy.x + tipR * cos(a);
            pEnemyShot->y = enemy.y + tipR * sin(a);
            pEnemyShot->muki = a + DX_PI / 2.0; // 回転の接線方向へ流す
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotLaser[5]; // マゼンタの短レーザー
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 壁(コリドー)＋隙間位置からの自機狙い3way
// ============================================================
static void ShotWallCorridor(sEnemyShotSet* pEnemyShotSet)
{
    int local = count % CYCLE_LEN;
    bool spawning = (local >= PHASE1_END && local < PHASE3_END);

    if (spawning) {
        int spawnInterval = (local < PHASE2_END) ? 20 : 12; // ③で間隔短縮
        if ((local - PHASE1_END) % spawnInterval == 0) {
            double gapCenterSlot = WallGapCenterSlot(local);
            int gapCenterIdx = static_cast<int>(floor(gapCenterSlot + 0.5));

            for (int i = 0; i < WALL_SLOTS; i++) {
                if (i >= gapCenterIdx - WALL_GAP_HALF && i <= gapCenterIdx + WALL_GAP_HALF) continue; // 隙間

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->kind = img_enemyShotMediumBall[4]; // 青の中玉
                pEnemyShot->param_i[3] = 0; // 0:壁セグメント
                pEnemyShot->param_i[4] = 0; // 排出開始フラグ
                pEnemyShot->param_d[0] = i * WALL_SLOT_PITCH + WALL_SLOT_PITCH / 2.0; // 固定x
                pEnemyShot->param_d[1] = WALL_SPAWN_Y;                                 // 出現y
                pEnemyShot->param_d[2] = (local < PHASE2_END) ? 1.2 : 2.0;            // 上昇速度
                pEnemyShot->x = pEnemyShot->param_d[0];
                pEnemyShot->y = WALL_SPAWN_Y;
                pEnemyShot->muki = -DX_PI / 2.0; // 見た目上は上向き
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 隙間位置から自機狙い3way(棒がすり抜けるタイミングに同期)
            double gapX = gapCenterIdx * WALL_SLOT_PITCH + WALL_SLOT_PITCH / 2.0;
            double gapY = WALL_SPAWN_Y - 0.0;
            double baseMuki = atan2(player.y - gapY, player.x - gapX);

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int w = -1; w <= 1; w++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->kind = img_enemyShotBullet[6]; // 白の銃弾
                pEnemyShot->param_i[3] = 1; // 1:自機狙い弾
                pEnemyShot->x = gapX;
                pEnemyShot->y = gapY;
                pEnemyShot->muki = baseMuki + w * (DX_PI / 12.0); // ±15度
                pEnemyShot->speed = 3.0;
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // --- 移動更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[3] == 0) {
            if (local >= RELEASE_AT) {
                // ④解放：残存壁弾を左右へ加速排出
                if (pShot->param_i[4] == 0) {
                    pShot->param_i[4] = 1;
                    pShot->param_i[5] = local;
                    pShot->param_d[3] = pShot->x;
                    pShot->param_d[4] = pShot->y;
                }
                double t = static_cast<double>(local - pShot->param_i[5]);
                double dir = (pShot->param_d[3] < enemy.x) ? -1.0 : 1.0;
                pShot->x = pShot->param_d[3] + dir * (0.5 * t + 0.03 * t * t);
                pShot->y = pShot->param_d[4];
            }
            else {
                // 壁セグメント：pShot->count に基づく式で上昇(積分ではなく式で決定)
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1] - pShot->param_d[2] * pShot->count;
            }
        }
        else {
            // 自機狙い弾：通常の速度移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// フィナーレ：解放と同時に中心から自機狙い5way
// ============================================================
static void ShotFinishBurst(sEnemyShotSet* pEnemyShotSet)
{
    int local = count % CYCLE_LEN;

    if (local == RELEASE_AT) {
        double baseMuki = atan2(player.y - enemy.y, player.x - enemy.x);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int w = -2; w <= 2; w++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤の大玉
            pEnemyShot->x = enemy.x;
            pEnemyShot->y = enemy.y;
            pEnemyShot->muki = baseMuki + w * (DX_PI / 14.0);
            pEnemyShot->speed = 2.6;
            pEnemyShot->margin = 120;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->alive = 9999;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

void EnemyPat_KuruKuruKururin_Claude() // 新しく作成する場合、名前は void EnemyPat_KuruKuruKururin_Claude() にすること。
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        CreateShotSet(ShotRod);         // 回転棒本体
        CreateShotSet(ShotRodTrail);    // 棒先端の曳光弾
        CreateShotSet(ShotWallCorridor);// 隙間付きの壁＋自機狙い3way
        CreateShotSet(ShotFinishBurst); // フィナーレの自機狙い5way
    }

    // 本体はゆるやかに左右へ揺れる(式駆動、速度積分は行わない)
    enemy.x = 240.0 + 20.0 * sin(count * 0.01);

    int local = count % CYCLE_LEN;

    // ③→④の切り替わりで予告音を鳴らす
    if (local == PHASE3_END) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}