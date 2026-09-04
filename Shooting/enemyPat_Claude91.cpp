// enemyPat_DaijaHadou.cpp
//
// 大蛇覇道（だいじゃはどう） -- slither.ioモチーフの弾幕パターン
// 専用素材は使わず、既存の丸弾・鱗弾・菱形弾・銃弾・楕円弾の組み合わせのみで
// 「エサ」「伸びる蛇の胴体」「複数の蛇による包囲」「討ち死に後のエサ化」を表現する。
//
// フェーズ構成（無限ループ、周期 CYCLE_LEN フレーム）:
//   フェーズ1 苗床   : エサ弾（小玉）が画面内を漂いながら散らばる
//   フェーズ2 成長   : 頭の蛇行軌跡を胴体弾が遅延追従する形で、蛇が3体・時間差で伸びていく
//   フェーズ3 包囲   : 自機の発生時位置を中心に環状の壁（48発）が縮小しながら包囲し、
//                       回転する隙間だけが安全回廊になる。壁の「頭」役の弾が周期的に自機狙い3wayを放つ
//   フェーズ4 討伐   : 環が中心から放射状に加速して弾け、画面外へ消える（死骸→エサの演出として
//                       放射状バーストのエサ弾と、自機狙い5wayのフィニッシュを追加で放つ）
//
// 設計方針:
//   - 全ての弾は pShot->count（自弾の生存フレーム数）から直接座標を計算する
//     「公式駆動」方式で動かしており、+= による速度積分は行っていない。
//   - 実際の「エサを食べて伸びる」当たり判定は行わず、時間経過で自動的に胴体が伸びる
//     簡略化を採用している（ショットセットをまたいだ衝突判定はコスト高のため）。
//   - 包囲フェーズの中心は、発生した瞬間の自機座標に固定するトラップ地点方式
//     （常に自機を追尾すると理不尽になるため）。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  共通ヘルパー
// ============================================================

// 角度を [-π, π] に正規化
static double WrapPi(double a)
{
    while (a > DX_PI) a -= 2.0 * DX_PI;
    while (a < -DX_PI) a += 2.0 * DX_PI;
    return a;
}

// 新しい sEnemyShotSet を生成し、グローバルリストへ連結する
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// 指定した sEnemyShotSet の末尾に新しい sEnemyShot を連結する
static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->margin = 240;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
//  弾幕：エサ弾
//  param_i[0] : モード  0=苗床（漂うエサ） 1=放射状バースト（討ち死に演出） 2=自機狙い扇形フィニッシュ
//  param_i[1] : 生成数
//  param_d[0] : 速度
// ============================================================
static void FoodBullets(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int mode = pEnemyShotSet->param_i[0];
        int n = pEnemyShotSet->param_i[1];
        double speed = pEnemyShotSet->param_d[0];
        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 使える色一覧: 0:赤,1:黄,2:緑,3:シアン,4:青,5:マゼンタ,8:橙（白黒は避けて彩り豊かなエサにする）
        static const int foodColors[] = { 0, 1, 2, 3, 4, 5, 8 };

        for (int i = 0; i < n; i++) {
            double ox = pEnemyShotSet->x;
            double oy = pEnemyShotSet->y;
            double angle;

            if (mode == 0) {
                // 画面中心付近から少しランダムにばら撒き、中心から外側へゆっくり流れていく向きにする
                ox += GetRand(200) - 100;
                oy += GetRand(200) - 100;
                angle = atan2(oy - 240.0, ox - 240.0) + (GetRand(60) - 30) / 180.0 * DX_PI;
            }
            else if (mode == 1) {
                angle = 2.0 * DX_PI * i / n;
            }
            else {
                angle = aimAngle + (i - (n - 1) / 2.0) * (18.0 / 180.0 * DX_PI);
            }

            sEnemyShot* pShot = AddShot(pEnemyShotSet);
            pShot->param_d[0] = ox;              // 基準座標（公式の原点）
            pShot->param_d[1] = oy;
            pShot->param_d[2] = angle;
            pShot->param_d[3] = GetRand(628) / 100.0; // 苗床モード用のふらつき位相
            pShot->speed = speed;
            pShot->muki = angle;
            pShot->x = ox;
            pShot->y = oy;

            if (mode == 2) {
                pShot->kind = img_enemyShotBullet[0]; // 赤い銃弾＝自機狙いの警告色
            }
            else {
                int c = foodColors[GetRand(6)];
                pShot->kind = img_enemyShotSmallBall[c];
            }
        }
    }

    int mode = pEnemyShotSet->param_i[0];
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double ox = pShot->param_d[0];
        double oy = pShot->param_d[1];
        double angle = pShot->param_d[2];

        if (mode == 0) {
            // 一定方向へゆっくり流れつつ、進行方向に直交する向きに小さく揺れる（漂うエサ）
            double bobPhase = pShot->param_d[3];
            pShot->x = ox + pShot->speed * t * cos(angle) - 6.0 * sin(0.05 * t + bobPhase) * sin(angle);
            pShot->y = oy + pShot->speed * t * sin(angle) + 6.0 * sin(0.05 * t + bobPhase) * cos(angle);
        }
        else if (mode == 1) {
            // 徐々に加速しながら放射状に飛び散る（討ち死にの弾け）
            t += 5;
            double r = pShot->speed * t + 0.01 * t * t;
            pShot->x = ox + r * cos(angle);
            pShot->y = oy + r * sin(angle);
        }
        else {
            // 等速直線の自機狙い扇形ショット
            pShot->x = ox + pShot->speed * t * cos(angle);
            pShot->y = oy + pShot->speed * t * sin(angle);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：成長する蛇
//  頭が蛇行しながら進み、胴体弾は頭の過去の軌跡を一定フレーム遅れで辿ることで
//  「数珠つなぎに伸びていく胴体」を、速度積分を使わずに公式だけで表現する。
//
//  shotSet param_d[0..6] = baseX, baseY, dirAngle, wanderAmp, wanderFreq, wanderPhase, speed
//  shotSet param_i[0] = 胴体の色番号, param_i[1] = 現在の胴体節数
//  shot   param_i[0] = 胴体の節番号（0=頭）。 -1 の場合は頭が放った自機狙いの単発弾（フリーショット）
// ============================================================
static void SnakeGrow(sEnemyShotSet* pEnemyShotSet)
{
    const int SEGMENT_DELAY = 6;     // 1節あたりの遅延フレーム数
    const int MAX_SEGMENTS = 34;     // 胴体の最大節数
    const int GROWTH_INTERVAL = 9;   // 何フレームごとに1節伸ばすか
    const int NIBBLE_INTERVAL = 70;  // 頭からの自機狙い弾の間隔

    double baseX = pEnemyShotSet->param_d[0];
    double baseY = pEnemyShotSet->param_d[1];
    double dirAngle = pEnemyShotSet->param_d[2];
    double wanderAmp = pEnemyShotSet->param_d[3];
    double wanderFreq = pEnemyShotSet->param_d[4];
    double wanderPhase = pEnemyShotSet->param_d[5];
    double speed = pEnemyShotSet->param_d[6];
    int colorIndex = pEnemyShotSet->param_i[0];

    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pHead = AddShot(pEnemyShotSet);
        pHead->param_i[0] = 0;
        pHead->kind = img_enemyShotMediumOval[colorIndex]; // 楕円弾＝頭（向きが分かりやすい）
        pHead->x = baseX;
        pHead->y = baseY;
        pEnemyShotSet->param_i[1] = 1;
    }

    // 成長：一定間隔で胴体を1節伸ばす
    if (pEnemyShotSet->param_i[1] < MAX_SEGMENTS && pEnemyShotSet->count % GROWTH_INTERVAL == 0) {
        int segIndex = pEnemyShotSet->param_i[1];
        sEnemyShot* pSeg = AddShot(pEnemyShotSet);
        pSeg->param_i[0] = segIndex;
        pSeg->kind = img_enemyShotMediumBall[colorIndex]; // 丸玉＝胴体
        pSeg->x = baseX;
        pSeg->y = baseY;
        pEnemyShotSet->param_i[1]++;
    }

    // 頭からの威嚇弾（自機狙い単発）
    if (pEnemyShotSet->count > 40 && pEnemyShotSet->count % NIBBLE_INTERVAL == 0) {
        double t0 = (double)pEnemyShotSet->count;
        double hx = baseX + speed * t0 * cos(dirAngle) - wanderAmp * sin(wanderFreq * t0 + wanderPhase) * sin(dirAngle);
        double hy = baseY + speed * t0 * sin(dirAngle) + wanderAmp * sin(wanderFreq * t0 + wanderPhase) * cos(dirAngle);
        double aimAngle = atan2(player.y - hy, player.x - hx);

        sEnemyShot* pNibble = AddShot(pEnemyShotSet);
        pNibble->param_i[0] = -1;
        pNibble->param_d[0] = hx;
        pNibble->param_d[1] = hy;
        pNibble->param_d[2] = aimAngle;
        pNibble->speed = 2.4;
        pNibble->muki = aimAngle;
        pNibble->kind = img_enemyShotBullet[colorIndex];
        pNibble->x = hx;
        pNibble->y = hy;
    }

    // 全弾の位置更新（count から直接計算。速度積分はしない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] >= 0) {
            int segIndex = pShot->param_i[0];
            double t = (double)pEnemyShotSet->count - segIndex * SEGMENT_DELAY;
            if (t < 0) t = 0; // 生まれたばかりの節は頭の初期位置に留まる

            pShot->x = baseX + speed * t * cos(dirAngle) - wanderAmp * sin(wanderFreq * t + wanderPhase) * sin(dirAngle);
            pShot->y = baseY + speed * t * sin(dirAngle) + wanderAmp * sin(wanderFreq * t + wanderPhase) * cos(dirAngle);
            pShot->muki = dirAngle;
        }
        else {
            double t = (double)pShot->count;
            pShot->x = pShot->param_d[0] + pShot->speed * t * cos(pShot->param_d[2]);
            pShot->y = pShot->param_d[1] + pShot->speed * t * sin(pShot->param_d[2]);
            pShot->muki = pShot->param_d[2];
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：大蛇包囲陣
//  複数の蛇の胴体が合わさった環（48発）が自機の発生時位置を取り囲み、
//  回転する1本の隙間だけが逃げ道になる。保持時間が終わると環はそのまま
//  中心から放射状に加速して弾け、画面外へ抜けて自然消滅する（討ち死に演出）。
//
//  shotSet param_d[0],[1] = 包囲の中心座標（発生時の自機位置で固定）
//  shot    param_i[0] = 環上のインデックス（-1ならヘッドが放った自機狙いのフリーショット）
//  shot    param_i[1] = 1ならヘッド役（自機狙いショットを放つ）
// ============================================================
static void SiegeRing(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM_RING = 48;
    const double R_START = 210.0;
    const double R_END = 100.0;
    const int SHRINK_DURATION = 180;             // 縮小にかけるフレーム数
    const int HOLD_DURATION = 240;               // 包囲を維持する時間
    const int RELEASE_START = SHRINK_DURATION + HOLD_DURATION; // 420フレームで解放
    const double RING_ROT_SPEED = 0.006;         // 環全体のゆっくりした自転
    const double GAP_ROT_SPEED = 0.017;          // 安全回廊が回転する速さ
    const double GAP_HALF_WIDTH = 0.30;          // 安全回廊の半角（ラジアン）
    const double RELEASE_ACCEL = 0.03;           // 解放後の加速度

    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 4色 x 12発 = 48発。複数の蛇が壁を分担して包囲しているイメージ
        static const int ringColors[] = { 2, 3, 5, 1 }; // 緑・シアン・マゼンタ・黄
        for (int i = 0; i < NUM_RING; i++) {
            bool isHead = (i % 6 == 0);
            int colorIndex = ringColors[(i / 12) % 4];
            double baseAngle = 2.0 * DX_PI * i / NUM_RING;

            sEnemyShot* pSeg = AddShot(pEnemyShotSet);
            pSeg->param_i[0] = i;
            pSeg->param_i[1] = isHead ? 1 : 0;
            pSeg->kind = isHead ? img_enemyShotDiamond[colorIndex] : img_enemyShotScale[colorIndex];
            pSeg->x = cx + R_START * cos(baseAngle);
            pSeg->y = cy + R_START * sin(baseAngle);
        }
    }

    int t = pEnemyShotSet->count;
    if (t == RELEASE_START) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] >= 0) {
            int index = pShot->param_i[0];
            bool isHead = (pShot->param_i[1] == 1);
            double baseAngle = 2.0 * DX_PI * index / NUM_RING;
            double angle = baseAngle + RING_ROT_SPEED * t;
            double gapAngle = GAP_ROT_SPEED * t;
            bool inGap = fabs(WrapPi(angle - gapAngle)) < GAP_HALF_WIDTH;

            double radius;
            if (t < SHRINK_DURATION) {
                radius = R_START - (R_START - R_END) * (t / (double)SHRINK_DURATION);
            }
            else if (t < RELEASE_START) {
                radius = R_END;
            }
            else {
                double rt = (double)(t - RELEASE_START);
                radius = R_END + RELEASE_ACCEL * rt * rt * 0.5;
            }

            // 包囲中のみ、安全回廊に入った弾は中心へ収縮させて無害化する
            if (inGap && t < RELEASE_START) {
                pShot->margin = 550;
                radius = 500.0;
            }

            pShot->x = cx + radius * cos(angle);
            pShot->y = cy + radius * sin(angle);
            pShot->muki = angle + DX_PI / 2.0;

            // ヘッド役が周期的に自機狙い3wayを放つ（包囲を維持している間のみ）
            if (isHead && !inGap && t >= SHRINK_DURATION && t < RELEASE_START && (t + index) % 90 == 0) {
                double aimAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                for (int w = -1; w <= 1; w++) {
                    double angleW = aimAngle + w * (14.0 / 180.0 * DX_PI);
                    sEnemyShot* pAtk = AddShot(pEnemyShotSet);
                    pAtk->param_i[0] = -1;
                    pAtk->param_d[0] = pShot->x;
                    pAtk->param_d[1] = pShot->y;
                    pAtk->param_d[2] = angleW;
                    pAtk->speed = 2.6;
                    pAtk->muki = angleW;
                    pAtk->kind = img_enemyShotBullet[0];
                    pAtk->x = pShot->x;
                    pAtk->y = pShot->y;
                }
            }
        }
        else {
            double tf = (double)pShot->count;
            pShot->x = pShot->param_d[0] + pShot->speed * tf * cos(pShot->param_d[2]);
            pShot->y = pShot->param_d[1] + pShot->speed * tf * sin(pShot->param_d[2]);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン：大蛇覇道（slither.ioモチーフ）
// ============================================================
void EnemyPat_Slitherio_Claude()
{
    const int PHASE1_END = 200;                    // 苗床フェーズの長さ
    const int PHASE2_END = 500;                     // 成長フェーズの終わり（300フレーム）
    const int PHASE3_LEN = 420;                      // SiegeRing内部の SHRINK+HOLD と一致させる
    const int PHASE3_END = PHASE2_END + PHASE3_LEN;  // 920
    const int CYCLE_LEN = 1100;                       // 1サイクルの総フレーム数

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 本体はゆるく左右に揺れる（蛇のイメージ）
    enemy.x = 240.0 + 40.0 * sin(count * 0.01);

    int t = (count - 1) % CYCLE_LEN;

    // ---- フェーズ1：苗床（エサ散布） ----
    if (t < PHASE1_END && t % 15 == 0) {
        double ox = 100.0 + GetRand(280);
        double oy = 100.0 + GetRand(280);
        sEnemyShotSet* pSet = SpawnShotSet(FoodBullets, ox, oy);
        pSet->param_i[0] = 0; // 苗床モード
        pSet->param_i[1] = 8; // 1回あたり8個
        pSet->param_d[0] = 0.35;
    }

    // ---- フェーズ2：成長（蛇が3体、時間差で出現し蛇行しながら伸びる） ----
    if (t == PHASE1_END || t == PHASE1_END + 70 || t == PHASE1_END + 140) {
        int edge = GetRand(2); // 0:左, 1:上, 2:右
        double bx, by, dirAngle;
        if (edge == 0) {
            bx = -20.0; by = 100.0 + GetRand(280);
            dirAngle = (GetRand(60) - 30) / 180.0 * DX_PI; // おおむね右向き
        }
        else if (edge == 1) {
            bx = 100.0 + GetRand(280); by = -20.0;
            dirAngle = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI; // おおむね下向き
        }
        else {
            bx = 500.0; by = 100.0 + GetRand(280);
            dirAngle = DX_PI + (GetRand(60) - 30) / 180.0 * DX_PI; // おおむね左向き
        }

        sEnemyShotSet* pSet = SpawnShotSet(SnakeGrow, bx, by);
        pSet->param_d[0] = bx;
        pSet->param_d[1] = by;
        pSet->param_d[2] = dirAngle;
        pSet->param_d[3] = 14.0 + GetRand(10);            // wanderAmp
        pSet->param_d[4] = 0.02 + GetRand(10) / 1000.0;   // wanderFreq
        pSet->param_d[5] = GetRand(628) / 100.0;          // wanderPhase
        pSet->param_d[6] = 1.1;                           // speed

        static const int snakeColors[] = { 2, 3, 5 }; // 緑・シアン・マゼンタの3匹
        static int snakeSpawnCount = 0;
        pSet->param_i[0] = snakeColors[snakeSpawnCount % 3];
        snakeSpawnCount++;
    }

    // ---- フェーズ3：包囲（環の壁が自機を取り囲む） ----
    if (t == PHASE2_END) {
        sEnemyShotSet* pSet = SpawnShotSet(SiegeRing, player.x, player.y);
        pSet->param_d[0] = player.x; // 発生時点の自機位置に中心を固定
        pSet->param_d[1] = player.y;
    }

    // ---- フェーズ4：討伐（環が弾けた瞬間、死骸のエサと自機狙いフィニッシュを追加） ----
    if (t == PHASE3_END) {
        sEnemyShotSet* pBurst = SpawnShotSet(FoodBullets, player.x, player.y);
        pBurst->param_i[0] = 1;  // 放射状バースト
        pBurst->param_i[1] = 28;
        pBurst->param_d[0] = 1.3;

        sEnemyShotSet* pFinish = SpawnShotSet(FoodBullets, player.x, player.y - 40.0);
        pFinish->param_i[0] = 2; // 自機狙い扇形フィニッシュ
        pFinish->param_i[1] = 6;
        pFinish->param_d[0] = 2.6;
    }
}