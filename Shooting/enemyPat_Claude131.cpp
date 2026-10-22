#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// バブルソート弾幕
// ------------------------------------------------------------
// 8個の数値要素を横一列のレーンに並べ、バブルソートのアルゴリズム
// （隣接比較→交換→パス反復→確定要素の浮上）をそのまま弾幕の
// 動きとして表現する。
//
//   ・比較         : 短レーザーで対象ペアをテレグラフ表示
//   ・交換あり     : 弧を描いて位置交換＋鱗弾の扇状バラ撒き
//   ・交換なし     : 素通り＋小玉の比較コスト弾
//   ・パス確定     : その時点の最大値が「泡」となり上部へ浮上、
//                    放射状の銃弾バーストを発生
//   ・確定バリア   : 上部に固定された要素が定期的に菱形弾を放つ
//   ・ソート完了   : 確定済み全要素が同時に全方位バースト（フィナーレ）
//                    後、しばらく待ってから配列をシャッフルしてループ
// ============================================================

static const int ELEM_COUNT = 8;

// 値(1〜ELEM_COUNT)ごとに固定で割り当てる色番号
// 色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
// 黒(7)は視認性が低いため除外し、残り8色を値の大小にそのまま対応させる
// → プレイヤーは色を見るだけで各要素のおおよその大小を判断できる
static const int VALUE_COLOR_TABLE[ELEM_COUNT] = { 0, 8, 1, 2, 3, 4, 5, 6 };

// レーン配置
static const double ROW_Y = 200.0;
static const double LANE_MARGIN = 60.0;
static const double LANE_SPACING = (480.0 - LANE_MARGIN * 2.0) / (double)(ELEM_COUNT - 1);
static const double LOCK_ROW_Y = 40.0;

// 各要素(sEnemyShotSet)の状態  ※param_i[0]に格納
enum ElemState {
    ELEM_STATE_ROW = 0,    // レーンで待機/比較中
    ELEM_STATE_SWAP_A = 1, // 交換アニメーション（弧を上に膨らませる側）
    ELEM_STATE_SWAP_B = 2, // 交換アニメーション（弧を下に膨らませる側）
    ELEM_STATE_RISE = 3,   // 確定して上部へ浮上中
    ELEM_STATE_LOCKED = 4, // 確定バリアとして上部に固定
};

// 発射リクエスト種別  ※param_i[2]に格納
enum FireRequest {
    FIRE_NONE = 0,
    FIRE_COMPARE_COST = 1, // 比較コスト（素通り時の小玉）
    FIRE_SWAP_BURST = 2,   // 交換時の鱗弾バラ撒き
    FIRE_BUBBLE_BURST = 3, // 浮上確定時の銃弾放射
    FIRE_FINALE = 4,       // ソート完了時の全方位バースト
};

// 本体の進行フェーズ
enum MainPhase {
    PHASE_SETUP = 0,      // 配列シャッフル＆要素スポーン
    PHASE_CHARGE = 1,     // 比較の予告（テレグラフ）
    PHASE_RESOLVE = 2,    // 比較結果に応じて交換/素通り演出を開始
    PHASE_ANIM_WAIT = 3,  // 交換/素通りアニメ待ち
    PHASE_PASS_LOCK = 4,  // パス終了、最大値（残り1個の場合はそれ自身）を確定させ浮上開始
    PHASE_LOCK_WAIT = 5,  // 浮上アニメ待ち
    PHASE_FINALE = 6,     // 全確定後、全要素で同時バースト
    PHASE_FINALE_WAIT = 7,
    PHASE_RESET_WAIT = 8, // 次のループまでの間合い
};

// ------------------------------------------------------------
// 弾幕：比較テレグラフ用の短レーザー
// 一定フレーム数だけ2要素の中間に表示し、その後は画面外へ逃がして
// メインルーチンの自動消去に委ねる（自前ではdeleteしない）
// ------------------------------------------------------------
static void ShotCompareLaser(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pLaser = new sEnemyShot;
        pLaser->x = pEnemyShotSet->x;
        pLaser->y = pEnemyShotSet->y;
        pLaser->muki = 0.0; // レーンは水平配置なので向きは常に水平
        pLaser->speed = 0.0;
        pLaser->kind = img_enemyShotLaser[6]; // 白
        pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pLaser->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
        pEnemyShotSet->pEnemyShotHead->prev = pLaser;
    }
    // 表示時間(param_i[0])を過ぎたら画面外へ逃がして自然消滅させる
    if (pEnemyShotSet->count >= pEnemyShotSet->param_i[0]) {
        sEnemyShot* pLaser = pEnemyShotSet->pEnemyShotHead->next;
        pLaser->y = -9999.0;
    }
}

// ------------------------------------------------------------
// 弾幕：数値ボール本体
// レーン待機/交換の弧移動/浮上移動＋各種バラ撒きの発射リクエスト処理
// ------------------------------------------------------------
static void ShotValueBall(sEnemyShotSet* pEnemyShotSet)
{
    int value = (int)pEnemyShotSet->param_d[0];
    int colorIndex = VALUE_COLOR_TABLE[value - 1];

    if (pEnemyShotSet->count == 0) {
        // 値を表す中玉を1個だけ生成（以後ずっとリストの先頭に居続ける）
        sEnemyShot* pValueShot = new sEnemyShot;
        pValueShot->x = pEnemyShotSet->x;
        pValueShot->y = pEnemyShotSet->y;
        pValueShot->speed = 0.0;
        pValueShot->muki = -DX_PI / 2.0;
        pValueShot->kind = img_enemyShotMediumBall[colorIndex];
        pValueShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pValueShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pValueShot;
        pEnemyShotSet->pEnemyShotHead->prev = pValueShot;
    }

    sEnemyShot* pValueShot = pEnemyShotSet->pEnemyShotHead->next;
    int state = pEnemyShotSet->param_i[0];

    if (state == ELEM_STATE_SWAP_A || state == ELEM_STATE_SWAP_B || state == ELEM_STATE_RISE) {
        int motionStartCount = pEnemyShotSet->param_i[4];
        double duration = pEnemyShotSet->param_d[6];
        double t = (double)(count - motionStartCount) / duration;
        if (t > 1.0) t = 1.0;
        double startX = pEnemyShotSet->param_d[4];
        double startY = pEnemyShotSet->param_d[5];
        double targetX = pEnemyShotSet->param_d[1];
        double targetY = pEnemyShotSet->param_d[2];
        double baseX = startX + (targetX - startX) * t;
        double baseY = startY + (targetY - startY) * t;
        if (state == ELEM_STATE_SWAP_A || state == ELEM_STATE_SWAP_B) {
            // 交換中は進行方向に対して垂直に弧を描いてすれ違わせる
            double bulge = sin(t * DX_PI) * 46.0 * (double)pEnemyShotSet->param_i[1];
            baseY += bulge;
        }
        pValueShot->x = baseX;
        pValueShot->y = baseY;
    }
    else {
        // 待機/確定中はtarget座標(param_d[1],[2])をそのまま実座標として使う
        pValueShot->x = pEnemyShotSet->param_d[1];
        pValueShot->y = pEnemyShotSet->param_d[2];
    }

    // 発射リクエストの処理
    int fireRequest = pEnemyShotSet->param_i[2];
    if (fireRequest != FIRE_NONE) {
        double originX = pValueShot->x;
        double originY = pValueShot->y;
        sEnemyShot* pNew;

        if (fireRequest == FIRE_COMPARE_COST) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 2; i++) {
                pNew = new sEnemyShot;
                pNew->x = originX;
                pNew->y = originY;
                pNew->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
                pNew->speed = 2.5;
                pNew->kind = img_enemyShotSmallBall[colorIndex];
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
        else if (fireRequest == FIRE_SWAP_BURST) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            int burstCount = pEnemyShotSet->param_i[5];
            for (int i = 0; i < burstCount; i++) {
                pNew = new sEnemyShot;
                pNew->x = originX;
                pNew->y = originY;
                pNew->muki = GetRand(3599) / 3600.0 * 2.0 * DX_PI;
                pNew->speed = 2.0 + GetRand(250) / 100.0;
                pNew->kind = img_enemyShotScale[colorIndex];
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
        else if (fireRequest == FIRE_BUBBLE_BURST) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            int burstCount = pEnemyShotSet->param_i[5];
            for (int i = 0; i < burstCount; i++) {
                pNew = new sEnemyShot;
                pNew->x = originX;
                pNew->y = originY;
                pNew->muki = (double)i / (double)burstCount * 2.0 * DX_PI;
                pNew->speed = 2.2;
                pNew->kind = img_enemyShotBullet[colorIndex];
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
        else if (fireRequest == FIRE_FINALE) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
            const int burstCount = 24;
            for (int i = 0; i < burstCount; i++) {
                pNew = new sEnemyShot;
                pNew->x = originX;
                pNew->y = originY;
                pNew->muki = (double)i / (double)burstCount * 2.0 * DX_PI;
                pNew->speed = 3.0;
                pNew->kind = img_enemyShotMediumOval[colorIndex];
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }
        pEnemyShotSet->param_i[2] = FIRE_NONE;
    }

    // 確定バリアの定期パルス：確定要素が増えるほど圧迫感が増す
    if (state == ELEM_STATE_LOCKED) {
        int rank = pEnemyShotSet->param_i[3];
        if ((count + rank * 15) % 90 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pPulse = new sEnemyShot;
                pPulse->x = pValueShot->x;
                pPulse->y = pValueShot->y;
                pPulse->muki = DX_PI / 2.0 + (double)i * (DX_PI / 2.0);
                pPulse->speed = 1.8;
                pPulse->kind = img_enemyShotDiamond[colorIndex];
                pPulse->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pPulse->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pPulse;
                pEnemyShotSet->pEnemyShotHead->prev = pPulse;
            }
        }
    }

    // バラ撒き弾（先頭の数値ボール自身を除く）の移動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン：バブルソート弾幕
// ------------------------------------------------------------
void EnemyPat_BubbleSort_Claude()
{
    static sEnemyShotSet* laneElem[ELEM_COUNT]; // 現在のレーン配置（左から右＝配列そのもの）
    static int phase;
    static int phaseStartCount;
    static int passNumber;       // 現在のパス番号（0始まり、密度スケーリングに使用）
    static int compareIndex;     // パス内での比較位置
    static int unsortedBoundary; // 未確定区間の要素数（毎パス1減る）
    static int needSwap;         // 直近の比較が交換ありだったか
    static int currentAnimDuration;
    static double enemyMoveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 300;
        enemyMoveDir = 1.0;
        phase = PHASE_SETUP;
        phaseStartCount = count;
        passNumber = 0;
        compareIndex = 0;
        unsortedBoundary = 0;
        needSwap = 0;
        currentAnimDuration = 0;
        for (int i = 0; i < ELEM_COUNT; i++) laneElem[i] = nullptr;
        return;
    }

    // 本体はゆっくり左右へ往復（弾幕そのものが主役なので控えめな動き）
    enemy.x += 0.4 * enemyMoveDir;
    if (enemy.x < 80.0 || enemy.x > 400.0) enemyMoveDir *= -1.0;

    int elapsed = count - phaseStartCount;

    switch (phase) {

    case PHASE_SETUP:
    {
        // 1〜ELEM_COUNTの値をシャッフルして各レーンに配置
        int values[ELEM_COUNT];
        for (int i = 0; i < ELEM_COUNT; i++) values[i] = i + 1;
        for (int i = ELEM_COUNT - 1; i > 0; i--) {
            int j = GetRand(i); // 0〜i（i+1通り）
            int tmp = values[i];
            values[i] = values[j];
            values[j] = tmp;
        }

        for (int i = 0; i < ELEM_COUNT; i++) {
            double laneX = LANE_MARGIN + (double)i * LANE_SPACING;

            if (laneElem[i] == nullptr) {
                // 初回のみ新規生成。2周目以降は使い回すのでnewはここだけ。
                sEnemyShotSet* pNewSet = new sEnemyShotSet;
                pNewSet->count = 0;
                pNewSet->patternFunc = ShotValueBall;
                pNewSet->pEnemyShotHead = new sEnemyShot;
                pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
                pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;
                pNewSet->prev = enemyShotSetHead.prev;
                pNewSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pNewSet;
                enemyShotSetHead.prev = pNewSet;
                laneElem[i] = pNewSet;
            }

            sEnemyShotSet* pSet = laneElem[i];
            pSet->x = laneX;
            pSet->y = ROW_Y;
            pSet->param_d[0] = (double)values[i]; // 値
            pSet->param_d[1] = laneX;             // 目標x（待機時はこれが実座標）
            pSet->param_d[2] = ROW_Y;             // 目標y
            pSet->param_i[0] = ELEM_STATE_ROW;
            pSet->param_i[1] = 1;
            pSet->param_i[2] = FIRE_NONE;
            pSet->param_i[3] = -1; // 確定ランク（未確定）
            pSet->param_i[5] = 0;

            // 2周目以降：既存の数値ボールの色を新しい値に合わせて更新
            // （ボール自体は毎周期作り直さず使い回す＝プールを圧迫しない）
            sEnemyShot* pValueShot = pSet->pEnemyShotHead->next;
            if (pValueShot != pSet->pEnemyShotHead) {
                pValueShot->kind = img_enemyShotMediumBall[VALUE_COLOR_TABLE[values[i] - 1]];
            }
        }

        passNumber = 0;
        compareIndex = 0;
        unsortedBoundary = ELEM_COUNT;
        phase = PHASE_CHARGE;
        phaseStartCount = count;
        break;
    }

    case PHASE_CHARGE:
    {
        int chargeDuration = 24 - passNumber * 2;
        if (chargeDuration < 10) chargeDuration = 10;

        if (elapsed == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 比較対象2要素の中間に予告レーザーを表示
            sEnemyShotSet* leftSet = laneElem[compareIndex];
            sEnemyShotSet* rightSet = laneElem[compareIndex + 1];
            double midX = (leftSet->param_d[1] + rightSet->param_d[1]) / 2.0;

            sEnemyShotSet* pLaserSet = new sEnemyShotSet;
            pLaserSet->count = 0;
            pLaserSet->patternFunc = ShotCompareLaser;
            pLaserSet->x = midX;
            pLaserSet->y = ROW_Y;
            pLaserSet->param_i[0] = chargeDuration;
            pLaserSet->pEnemyShotHead = new sEnemyShot;
            pLaserSet->pEnemyShotHead->prev = pLaserSet->pEnemyShotHead;
            pLaserSet->pEnemyShotHead->next = pLaserSet->pEnemyShotHead;
            pLaserSet->prev = enemyShotSetHead.prev;
            pLaserSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pLaserSet;
            enemyShotSetHead.prev = pLaserSet;
        }

        if (elapsed >= chargeDuration) {
            phase = PHASE_RESOLVE;
            phaseStartCount = count;
        }
        break;
    }

    case PHASE_RESOLVE:
    {
        sEnemyShotSet* left = laneElem[compareIndex];
        sEnemyShotSet* right = laneElem[compareIndex + 1];
        double leftValue = left->param_d[0];
        double rightValue = right->param_d[0];
        double leftX = left->param_d[1];
        double rightX = right->param_d[1];

        needSwap = (leftValue > rightValue) ? 1 : 0;

        if (needSwap) {
            int swapDuration = 34 - passNumber * 2;
            if (swapDuration < 20) swapDuration = 20;
            int swapBurstCount = 12 + passNumber * 5;

            left->param_i[0] = ELEM_STATE_SWAP_A;
            left->param_d[4] = leftX;   // 開始x
            left->param_d[5] = ROW_Y;   // 開始y
            left->param_d[1] = rightX;  // 目標x
            left->param_i[1] = -1;      // 弧を上に膨らませる
            left->param_i[4] = count;
            left->param_d[6] = (double)swapDuration;
            left->param_i[5] = swapBurstCount;
            left->param_i[2] = FIRE_SWAP_BURST;

            right->param_i[0] = ELEM_STATE_SWAP_B;
            right->param_d[4] = rightX;
            right->param_d[5] = ROW_Y;
            right->param_d[1] = leftX;
            right->param_i[1] = 1;      // 弧を下に膨らませる
            right->param_i[4] = count;
            right->param_d[6] = (double)swapDuration;
            right->param_i[5] = swapBurstCount;
            right->param_i[2] = FIRE_SWAP_BURST;

            laneElem[compareIndex] = right;
            laneElem[compareIndex + 1] = left;

            currentAnimDuration = swapDuration;
        }
        else {
            left->param_i[2] = FIRE_COMPARE_COST;
            right->param_i[2] = FIRE_COMPARE_COST;
            currentAnimDuration = 16;
        }

        phase = PHASE_ANIM_WAIT;
        phaseStartCount = count;
        break;
    }

    case PHASE_ANIM_WAIT:
    {
        if (elapsed >= currentAnimDuration) {
            if (needSwap) {
                // 交換完了：両者をレーン待機状態へ戻す
                laneElem[compareIndex]->param_i[0] = ELEM_STATE_ROW;
                laneElem[compareIndex]->param_d[2] = ROW_Y;
                laneElem[compareIndex + 1]->param_i[0] = ELEM_STATE_ROW;
                laneElem[compareIndex + 1]->param_d[2] = ROW_Y;
            }

            compareIndex++;
            if (compareIndex >= unsortedBoundary - 1) {
                phase = PHASE_PASS_LOCK;
            }
            else {
                phase = PHASE_CHARGE;
            }
            phaseStartCount = count;
        }
        break;
    }

    case PHASE_PASS_LOCK:
    {
        // 未確定区間の右端＝このパスで確定した最大値
        sEnemyShotSet* maxElem = laneElem[unsortedBoundary - 1];
        int bubbleBurstCount = 16 + passNumber * 2;

        maxElem->param_i[0] = ELEM_STATE_RISE;
        maxElem->param_d[4] = maxElem->param_d[1]; // 開始x（水平移動なし）
        maxElem->param_d[5] = ROW_Y;               // 開始y
        maxElem->param_d[2] = LOCK_ROW_Y;          // 目標y（まっすぐ浮上）
        maxElem->param_i[4] = count;
        maxElem->param_d[6] = 50.0;
        maxElem->param_i[3] = passNumber; // 確定ランク
        maxElem->param_i[5] = bubbleBurstCount;
        maxElem->param_i[2] = FIRE_BUBBLE_BURST;

        phase = PHASE_LOCK_WAIT;
        phaseStartCount = count;
        break;
    }

    case PHASE_LOCK_WAIT:
    {
        if (elapsed >= 50) {
            sEnemyShotSet* maxElem = laneElem[unsortedBoundary - 1];
            maxElem->param_i[0] = ELEM_STATE_LOCKED;

            unsortedBoundary--;
            passNumber++;
            compareIndex = 0;

            if (unsortedBoundary <= 0) {
                phase = PHASE_FINALE;
            }
            else if (unsortedBoundary == 1) {
                // 残り1個は比較不要、そのまま浮上させて確定させる
                phase = PHASE_PASS_LOCK;
            }
            else {
                phase = PHASE_CHARGE;
            }
            phaseStartCount = count;
        }
        break;
    }

    case PHASE_FINALE:
    {
        // 昇順に並んだ確定済み全要素が同時に全方位バースト
        for (int i = 0; i < ELEM_COUNT; i++) {
            laneElem[i]->param_i[2] = FIRE_FINALE;
        }
        phase = PHASE_FINALE_WAIT;
        phaseStartCount = count;
        break;
    }

    case PHASE_FINALE_WAIT:
    {
        if (elapsed >= 120) {
            phase = PHASE_RESET_WAIT;
            phaseStartCount = count;
        }
        break;
    }

    case PHASE_RESET_WAIT:
    {
        // このあいだにバラ撒き済みの子弾が画面外へ抜けてメインルーチンに回収される。
        // 要素本体のshotSetはdeleteせず、次のPHASE_SETUPで使い回す。
        if (elapsed >= 90) {
            phase = PHASE_SETUP;
            phaseStartCount = count;
        }
        break;
    }

    } // switch
}