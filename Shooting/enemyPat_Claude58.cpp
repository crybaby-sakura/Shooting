// enemyPat_arijigoku.cpp
//
// 蟻地獄陥穽（ありじごくかんせい）
// アリジゴクのすり鉢状の巣をモチーフにした4フェーズパターン
//
//   フェーズ1：すり鉢陣　　　中心から螺旋状にすり鉢の壁を形成
//   フェーズ2：砂被せ　　　　中心から放物線状に砂粒を撒き上げる
//   フェーズ3：底なし引き込み　すり鉢の壁が回転する隙間を残しながら中心へ収束
//   フェーズ4：顎の一閃　　　溜めの後、V字に開いた顎で自機周辺を挟撃
//
// 上記4フェーズを1サイクルとしてループする。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ==================== 調整用定数 ====================

// すり鉢（螺旋）関連
static const int    ARM_NUM = 8;     // すり鉢を構成する螺旋の本数
static const double FUNNEL_MAX_R = 240.0; // すり鉢の最大半径
static const double FUNNEL_TAU = 50.0;  // 半径が収束する時定数（フェーズ1）
static const double FUNNEL_OMEGA = 0.03;  // 螺旋の回転角速度(rad/frame)
static const double COLLAPSE_TAU = 70.0;  // 半径が収束する時定数（フェーズ3・崩壊）

// 砂被せ関連
static const double SAND_GRAVITY = 0.06;  // 落下の重力加速度(px/frame^2)

// 顎関連
static const int    MANDIBLE_FAN_NUM = 6;    // 片顎あたりの弾数
static const double MANDIBLE_SPREAD0 = 0.9;  // 開いた状態の扇の広がり(rad)
static const double MANDIBLE_TAU = 25.0; // 顎が閉じていく時定数
static const double MANDIBLE_SPEED = 3.2;  // 顎の弾の速さ(px/frame)

// フェーズタイミング（フレーム数、count基準）
static const int PHASE1_START = 1;
static const int PHASE1_LEN = 180;
static const int PHASE2_START = PHASE1_START + PHASE1_LEN;   // 181
static const int PHASE2_LEN = 150;
static const int PHASE3_START = PHASE2_START + PHASE2_LEN;   // 331
static const int PHASE3_LEN = 150;
static const int PHASE4_START = PHASE3_START + PHASE3_LEN;   // 481
static const int PHASE4_LEN = 110;
static const int CYCLE_LEN = PHASE4_START + PHASE4_LEN;   // 591

static const int MANDIBLE_TELEGRAPH = 40; // フェーズ4開始から発射までの溜め時間

// ==================== フェーズ1：すり鉢陣 ====================
// 中心から螺旋状に伸び、徐々に減速しながらすり鉢の壁の形へ収束する点弾。
// param_d[0], param_d[1] = 中心座標　param_d[2] = 初期角度
static void ShotFunnelWall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < ARM_NUM; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotDiamond[8]; // 橙：砂粒をイメージした菱形弾
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            // 各アームの角度オフセット + セット生成時の螺旋位相
            pShot->param_d[2] = (double)i / ARM_NUM * 2.0 * DX_PI + pSet->muki;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double r = FUNNEL_MAX_R * (1.0 - exp(-t / FUNNEL_TAU));
        double ang = pShot->param_d[2] + FUNNEL_OMEGA * t;
        pShot->x = pShot->param_d[0] + r * cos(ang);
        pShot->y = pShot->param_d[1] + r * sin(ang);

        // 速度ベクトル(dx/dt, dy/dt)から弾の向きを算出して設定
        double drdt = (FUNNEL_MAX_R / FUNNEL_TAU) * exp(-t / FUNNEL_TAU);
        double dxdt = drdt * cos(ang) - r * FUNNEL_OMEGA * sin(ang);
        double dydt = drdt * sin(ang) + r * FUNNEL_OMEGA * cos(ang);
        pShot->muki = atan2(dydt, dxdt);

        pShot = pShot->next;
    }
}

// ==================== フェーズ2：砂被せ ====================
// 中心から放物線状に砂粒（小玉）を撒き上げる。
// param_d[0], param_d[1] = 発射位置　param_d[2], param_d[3] = 初速(Vx, Vy)
static void ShotSandToss(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int num = 5;
        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotSmallBall[1]; // 黄：砂粒

            double mag = 2.5 + GetRand(150) / 100.0;               // 2.5〜4.0
            double ang = -DX_PI / 2.0 + (GetRand(1200) - 600) / 1000.0; // 真上±0.6rad

            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->param_d[2] = mag * cos(ang);
            pShot->param_d[3] = mag * sin(ang);

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t + 0.5 * SAND_GRAVITY * t * t;

        // 速度ベクトル(dx/dt, dy/dt)から弾の向きを算出して設定
        double dxdt = pShot->param_d[2];
        double dydt = pShot->param_d[3] + SAND_GRAVITY * t;
        pShot->muki = atan2(dydt, dxdt);

        pShot = pShot->next;
    }
}

// ==================== フェーズ3：底なし引き込み ====================
// すり鉢の壁が最大半径から中心へ収束していく。1本だけ弾を生成しない
// 「隙間」アームを設け、生成タイミングごとに隙間の位置を切り替えることで
// 隙間が回転しながら中心へ引き込まれていくように見せる。
// param_d[0], param_d[1] = 中心座標　param_d[2] = 初期角度
static void ShotCollapse(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int gapArm = pSet->kind % ARM_NUM; // 生成順に応じて隙間アームを切り替える

        for (int i = 0; i < ARM_NUM; i++) {
            if (i == gapArm) continue; // ここだけ弾を生成しない＝安全な隙間

            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotDiamond[0]; // 赤：崩壊の危険を示す
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->param_d[2] = (double)i / ARM_NUM * 2.0 * DX_PI + pSet->muki;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double r = FUNNEL_MAX_R * exp(-t / COLLAPSE_TAU); // 最大半径から中心へ収束
        double ang = pShot->param_d[2] + FUNNEL_OMEGA * t;
        pShot->x = pShot->param_d[0] + r * cos(ang);
        pShot->y = pShot->param_d[1] + r * sin(ang);

        // 速度ベクトル(dx/dt, dy/dt)から弾の向きを算出して設定
        double drdt = -(FUNNEL_MAX_R / COLLAPSE_TAU) * exp(-t / COLLAPSE_TAU);
        double dxdt = drdt * cos(ang) - r * FUNNEL_OMEGA * sin(ang);
        double dydt = drdt * sin(ang) + r * FUNNEL_OMEGA * cos(ang);
        pShot->muki = atan2(dydt, dxdt);

        pShot = pShot->next;
    }
}

// ==================== フェーズ4：顎の一閃 ====================
// トリガー時点の自機方向を中心に、左右の顎が扇状に開いた状態で射出され、
// 進みながら扇の広がりと左右の開き角の両方が縮んでいく＝噛みつくように見える。
// param_d[0], param_d[1] = 発射位置　param_d[2] = 狙い角
// param_d[3] = 左右(-1 or 1)　param_d[4] = 扇内での位置(0〜1)
static void ShotMandible(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double aimAngle = pSet->muki;

        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < MANDIBLE_FAN_NUM; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->kind = img_enemyShotScale[7]; // 黒：牙をイメージした鱗弾
                pShot->param_d[0] = pSet->x;
                pShot->param_d[1] = pSet->y;
                pShot->param_d[2] = aimAngle;
                pShot->param_d[3] = (double)side;
                pShot->param_d[4] = (MANDIBLE_FAN_NUM == 1) ? 0.5
                    : (double)i / (MANDIBLE_FAN_NUM - 1);

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double spread = MANDIBLE_SPREAD0 * exp(-t / MANDIBLE_TAU); // 扇が徐々に閉じる
        double fanOffset = (pShot->param_d[4] - 0.5) * spread;
        double sideBias = pShot->param_d[3] * (spread + 0.15);     // 左右の顎の開き
        double ang = pShot->param_d[2] + sideBias + fanOffset;
        double r = MANDIBLE_SPEED * t;
        pShot->x = pShot->param_d[0] + r * cos(ang);
        pShot->y = pShot->param_d[1] + r * sin(ang);

        // 速度ベクトル(dx/dt, dy/dt)から弾の向きを算出して設定
        double dspreaddt = -spread / MANDIBLE_TAU;
        double dangdt = (pShot->param_d[3] + (pShot->param_d[4] - 0.5)) * dspreaddt;
        double dxdt = MANDIBLE_SPEED * cos(ang) - r * dangdt * sin(ang);
        double dydt = MANDIBLE_SPEED * sin(ang) + r * dangdt * cos(ang);
        pShot->muki = atan2(dydt, dxdt);

        pShot = pShot->next;
    }
}

// ヘルパー：新規sEnemyShotSetを生成し、リストへ連結して返す
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ==================== 敵本体のパターン ====================
void EnemyPat_Antlion_Claude()
{
    static int gapTick = 0; // フェーズ3の隙間アーム切り替え用カウンタ

    if (count == 1) {
        // ゲーム画面は 480x480。すり鉢の中心を画面中央に配置。
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        gapTick = 0;
    }

    // 中心はゆるく上下に揺れる（アリジゴクが砂に潜って待ち構えるイメージ）
    enemy.x = 240.0;
    enemy.y = 240.0 + 6.0 * sin(count / 50.0);

    int localCount = (count - 1) % CYCLE_LEN + 1; // 1〜CYCLE_LENでループ

    if (localCount < PHASE2_START) {
        // フェーズ1：すり鉢陣
        int local1 = localCount - PHASE1_START;
        if (local1 % 8 == 0) {
            CreateShotSet(ShotFunnelWall, enemy.x, enemy.y, FUNNEL_OMEGA * local1, 0);
        }
    }
    else if (localCount < PHASE3_START) {
        // フェーズ2：砂被せ
        int local2 = localCount - PHASE2_START;
        if (local2 % 15 == 0) {
            CreateShotSet(ShotSandToss, enemy.x, enemy.y, 0.0, 0);
        }
    }
    else if (localCount < PHASE4_START) {
        // フェーズ3：底なし引き込み
        int local3 = localCount - PHASE3_START;
        if (local3 % 8 == 0) {
            CreateShotSet(ShotCollapse, enemy.x, enemy.y, FUNNEL_OMEGA * local3, gapTick);
            gapTick++;
        }
    }
    else {
        // フェーズ4：顎の一閃（前半は溜め、溜め終わりで発射）
        int local4 = localCount - PHASE4_START;
        if (local4 == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        if (local4 == MANDIBLE_TELEGRAPH) {
            double aimAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            CreateShotSet(ShotMandible, enemy.x, enemy.y, aimAngle, 0);
        }
    }
}