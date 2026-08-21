// enemyPat_KatenKamon.cpp
//
// 渦転花紋（かてんかもん） - スピログラフモチーフの4フェーズパターン
//
// 専用の歯車素材が無いため、通常弾を円周上に密に並べ、半径を「歯底円/歯末円」の
// 2値で交互配置することでギザギザの歯車シルエットを表現する。
//
// フェーズ構成：
//   1. 組立フェーズ   : 外側リングギア→内側ピニオンの順に、弾を1発ずつ円周上へ
//                       配置してゆき歯車を描き上げる（弾の出現タイミングをずらす
//                       だけで構築演出になるため、位置補間は不要）
//   2. 定常転動フェーズ: 内側ピニオンが外側リング内側を転がり公転+自転。ピニオン先端の
//                       仮想ペン位置がハイポトロコイド曲線を弾で描き足してゆく。
//                       並行してピニオン歯先/外側歯先から自機狙い弾を継続的にパルス、
//                       噛み合い接触点からは断続的な煌めきを飛ばす
//   3. 意匠拡大フェーズ: 転動が止まり、描き終えた曲線が中心から緩やかに加速しながら
//                       外側へ拡大してゆく（花が開くような演出）
//   4. 崩壊放散フェーズ: 予告点滅ののち、歯車・曲線の全弾がその場の位置から中心基準の
//                       放射方向へ一斉に加速飛散し、中心から自機狙い5wayで締める
//
// 座標計算はすべて pShot->count（生成からの経過フレーム）+ 生成時に記録した
// グローバルカウントを使った純粋な式で行い、速度の逐次積算は行わない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  配置・タイミング定数
// ============================================================
const double HubX = 240.0;   // 歯車機構の中心（画面は480x480）
const double HubY = 200.0;

const double OuterBaseR = 140.0;    // 外側リングギア：歯底円半径
const double OuterToothH = 12.0;    // 外側：歯の高さ
const int    OuterTeeth = 24;       // 外側：歯数
const int    OuterPoints = 96;      // 外側：円周上の配置点数
const double OuterSpinRate = 0.0015; // 外側リングの自転速度[rad/frame]（演出用の緩やかな自転）

const double InnerBaseR = 60.0;     // 内側ピニオン：歯底円半径
const double InnerToothH = 9.0;     // 内側：歯の高さ
const int    InnerTeeth = 14;       // 内側：歯数
const int    InnerPoints = 56;      // 内側：円周上の配置点数
const double OrbitR = OuterBaseR - InnerBaseR;   // ピニオン中心の公転半径 = 80
const double GearRatio = OrbitR / InnerBaseR;    // 内転がり拘束：自転角 = -GearRatio * 公転角
const double PenD = 40.0;           // 仮想ペン先のピニオン中心からのオフセット（軌跡の形を決める）

const int OuterSpawnInterval = 2;   // 外側リング：何フレームおきに1発配置するか
const int InnerSpawnInterval = 2;   // 内側ピニオン：同上
const int TraceSpawnInterval = 2;   // 軌跡曲線：同上

const int Phase1OuterEnd = 1 + OuterPoints * OuterSpawnInterval;               // = 193 外側組立完了目安
const int Phase1InnerEnd = Phase1OuterEnd + InnerPoints * InnerSpawnInterval;  // = 305 内側組立完了目安
const int Phase2Start = Phase1InnerEnd + 30;                                  // = 335 定常転動フェーズ開始
const int Phase2Duration = 600;                                               // 転動継続フレーム数
const int Phase2End = Phase2Start + Phase2Duration;                           // = 935
// θ: 0→6π で完全に閉じる（R:r=140:60、gcd=20なのでR/gcd=7弁の花模様。
// 曲線が閉じるのはθ = 2π * (r/gcd) = 2π*3 = 6π）
const double OrbitOmega = (6.0 * DX_PI) / (double)Phase2Duration;

const int Phase3Start = Phase2End + 1;              // = 936 意匠拡大フェーズ開始
const int TelegraphStart = Phase3Start + 180;        // = 1116 予告点滅開始
const int ReleaseTrigger = TelegraphStart + 40;      // = 1156 全弾解放トリガー

const double BloomAccel = 0.0025;   // フェーズ3：軌跡曲線の拡大加速度
const double ReleaseAccel = 0.02;   // フェーズ4：歯車弾の解放加速度

// sEnemyShot の param インデックス（歯車リング / 軌跡曲線 共通）
enum { PD_SPAWN_T = 0, PD_FX = 1, PD_FY = 2, PD_FANGLE = 3 };
enum { PI_INDEX = 0, PI_FROZEN = 1 };

// ============================================================
//  幾何ヘルパー
// ============================================================

// 角度と歯数から「歯底円」か「歯末円（歯の出っ張り）」かを判定して半径を返す
// （デューティ比50%の矩形波：1歯あたり前半を山、後半を谷とする）
static double ToothRadius(double angle, int teeth, double baseR, double toothH)
{
    double x = angle * teeth / (2.0 * DX_PI);
    double phase = x - floor(x); // 0.0〜1.0（負の角度でも正しく循環する）
    return baseR + (phase < 0.5 ? toothH : 0.0);
}

// 外側リングギア上の点の位置（自転のみ、公転しない）
static void OuterRingPoint(double t, double baseAngle, double& outX, double& outY, double& outAngle)
{
    double angle = baseAngle + OuterSpinRate * t;
    double radius = ToothRadius(angle, OuterTeeth, OuterBaseR, OuterToothH);
    outX = HubX + radius * cos(angle);
    outY = HubY + radius * sin(angle);
    outAngle = angle;
}

// 内側ピニオン上の点の位置（公転+内転がり拘束による自転）
// Phase2Start より前は θ=0 に固定（外周内側で静止＝組立フェーズの見た目）、
// Phase2End 以降は θ を Phase2Duration 分で頭打ちにして転動を止める（意匠拡大フェーズへ引き継ぐ）
static void InnerPinionPoint(double t, double localBase, double& outX, double& outY, double& outAngle)
{
    double elapsed = t - (double)Phase2Start;
    if (elapsed < 0.0) elapsed = 0.0;
    if (elapsed > (double)Phase2Duration) elapsed = (double)Phase2Duration;
    double theta = OrbitOmega * elapsed;
    double phi = -GearRatio * theta; // 転がり拘束

    double pcx = HubX + OrbitR * cos(theta);
    double pcy = HubY + OrbitR * sin(theta);
    double angle = localBase + phi;
    double radius = ToothRadius(angle, InnerTeeth, InnerBaseR, InnerToothH);

    outX = pcx + radius * cos(angle);
    outY = pcy + radius * sin(angle);
    outAngle = angle;
}

// ============================================================
//  弾幕：外側リングギア
// ============================================================
static void ShotOuterRing(sEnemyShotSet* pEnemyShotSet)
{
    // 一定間隔で1発ずつ円周上に配置し、リングを少しずつ描き上げる
    if (pEnemyShotSet->count % OuterSpawnInterval == 0 && pEnemyShotSet->param_i[0] < OuterPoints) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        int j = pEnemyShotSet->param_i[0]++;

        pEnemyShot->param_i[PI_INDEX] = j;
        pEnemyShot->param_d[PD_SPAWN_T] = (double)count; // 生成時点のグローバルカウントを記録
        pEnemyShot->kind = img_enemyShotSmallBall[3];
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count + pShot->param_d[PD_SPAWN_T]; // 現在のグローバルカウント相当
        double baseAngle = pShot->param_i[PI_INDEX] * (2.0 * DX_PI / OuterPoints);

        if (t < ReleaseTrigger) {
            double x, y, angle;
            OuterRingPoint(t, baseAngle, x, y, angle);
            pShot->x = x;
            pShot->y = y;
            pShot->muki = angle;
            bool isPeak = ToothRadius(angle, OuterTeeth, OuterBaseR, OuterToothH) > OuterBaseR + 0.5;
            pShot->kind = isPeak ? img_enemyShotMediumBall[3] : img_enemyShotSmallBall[3];
        }
        else {
            // 解放トリガーの瞬間の位置を一度だけ捕獲し、以後はそこから放射状に加速飛散
            if (pShot->param_i[PI_FROZEN] == 0) {
                double fx, fy, fangle;
                OuterRingPoint((double)ReleaseTrigger, baseAngle, fx, fy, fangle);
                pShot->param_d[PD_FX] = fx;
                pShot->param_d[PD_FY] = fy;
                pShot->param_d[PD_FANGLE] = fangle;
                pShot->param_i[PI_FROZEN] = 1;
                bool isPeak = ToothRadius(fangle, OuterTeeth, OuterBaseR, OuterToothH) > OuterBaseR + 0.5;
                pShot->kind = isPeak ? img_enemyShotMediumBall[3] : img_enemyShotSmallBall[3];
            }
            double dt = t - ReleaseTrigger;
            double dist = ReleaseAccel * dt * dt * 0.5;
            pShot->x = pShot->param_d[PD_FX] + dist * cos(pShot->param_d[PD_FANGLE]);
            pShot->y = pShot->param_d[PD_FY] + dist * sin(pShot->param_d[PD_FANGLE]);
            pShot->muki = pShot->param_d[PD_FANGLE];
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：内側ピニオン
// ============================================================
static void ShotInnerRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % InnerSpawnInterval == 0 && pEnemyShotSet->param_i[0] < InnerPoints) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        int i = pEnemyShotSet->param_i[0]++;

        pEnemyShot->param_i[PI_INDEX] = i;
        pEnemyShot->param_d[PD_SPAWN_T] = (double)count;
        pEnemyShot->kind = img_enemyShotSmallBall[5];
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count + pShot->param_d[PD_SPAWN_T];
        double localBase = pShot->param_i[PI_INDEX] * (2.0 * DX_PI / InnerPoints);

        if (t < ReleaseTrigger) {
            double x, y, angle;
            InnerPinionPoint(t, localBase, x, y, angle);
            pShot->x = x;
            pShot->y = y;
            pShot->muki = angle;
            bool isPeak = ToothRadius(angle, InnerTeeth, InnerBaseR, InnerToothH) > InnerBaseR + 0.5;
            pShot->kind = isPeak ? img_enemyShotMediumBall[5] : img_enemyShotSmallBall[5];
        }
        else {
            if (pShot->param_i[PI_FROZEN] == 0) {
                double fx, fy, fangle;
                InnerPinionPoint((double)ReleaseTrigger, localBase, fx, fy, fangle);
                pShot->param_d[PD_FX] = fx;
                pShot->param_d[PD_FY] = fy;
                pShot->param_d[PD_FANGLE] = atan2(fy - HubY, fx - HubX); // 中心基準の放射方向に飛ばす
                pShot->param_i[PI_FROZEN] = 1;
                bool isPeak = ToothRadius(fangle, InnerTeeth, InnerBaseR, InnerToothH) > InnerBaseR + 0.5;
                pShot->kind = isPeak ? img_enemyShotMediumBall[5] : img_enemyShotSmallBall[5];
            }
            double dt = t - ReleaseTrigger;
            double dist = ReleaseAccel * dt * dt * 0.5;
            pShot->x = pShot->param_d[PD_FX] + dist * cos(pShot->param_d[PD_FANGLE]);
            pShot->y = pShot->param_d[PD_FY] + dist * sin(pShot->param_d[PD_FANGLE]);
            pShot->muki = pShot->param_d[PD_FANGLE];
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：軌跡曲線（ハイポトロコイド／ペン先の軌跡）
// ============================================================
static void ShotTraceCurve(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ2の間、ペン先の現在位置に新規弾を1発ずつ置いて曲線を描き足す
    if (pEnemyShotSet->count % TraceSpawnInterval == 0 && count <= Phase2End) {
        double theta = OrbitOmega * (double)(count - Phase2Start);

        // ハイポトロコイド： x=(R-r)cosθ + d cos(kθ), y=(R-r)sinθ - d sin(kθ)  (k = GearRatio)
        double hx = HubX + OrbitR * cos(theta) + PenD * cos(GearRatio * theta);
        double hy = HubY + OrbitR * sin(theta) - PenD * sin(GearRatio * theta);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->param_d[PD_FX] = hx; // 描かれた時点の固定座標
        pEnemyShot->param_d[PD_FY] = hy;
        pEnemyShot->param_d[PD_FANGLE] = atan2(hy - HubY, hx - HubX); // 拡大フェーズ用の放射方向
        pEnemyShot->kind = img_enemyShotDiamond[8];
        pEnemyShot->x = hx;
        pEnemyShot->y = hy;
        pEnemyShot->muki = pEnemyShot->param_d[PD_FANGLE];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 生成直後（フェーズ2中）は静止＝曲線として描かれ続ける。
        // フェーズ3(Phase3Start)以降は同じ式のまま dt が伸び続けるので、
        // 自然に加速しながら外側へ拡大し、そのままフェーズ4の解放も兼ねる。
        if (count >= Phase3Start) {
            double dt = (double)count - (double)Phase3Start;
            double dist = BloomAccel * dt * dt * 0.5;
            pShot->x = pShot->param_d[PD_FX] + dist * cos(pShot->param_d[PD_FANGLE]);
            pShot->y = pShot->param_d[PD_FY] + dist * sin(pShot->param_d[PD_FANGLE]);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：自機狙いNway（ピニオン歯先/外側歯先/最終解放で共用）
// ============================================================
// pEnemyShotSet に事前設定するパラメータ：
//   param_i[0] = way数, param_i[1] = 弾のkind, param_i[2] = 効果音種別(0light/1medium/2heavy/3extreme)
//   param_d[0] = 全体の開き角[rad], param_d[1] = 速さ
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet)
{
    int N = pEnemyShotSet->param_i[0];
    int kind = pEnemyShotSet->param_i[1];
    int soundKind = pEnemyShotSet->param_i[2];
    double spread = pEnemyShotSet->param_d[0];
    double speed = pEnemyShotSet->param_d[1];

    if (pEnemyShotSet->count == 0) {
        int snd = sound_enemyShot_light;
        if (soundKind == 1) snd = sound_enemyShot_medium;
        else if (soundKind == 2) snd = sound_enemyShot_heavy;
        else if (soundKind == 3) snd = sound_enemyShot_extreme;
        if (CheckSoundMem(snd)) StopSoundMem(snd);
        PlaySoundMem(snd, DX_PLAYTYPE_BACK);

        double centerAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int k = 0; k < N; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double a = (N == 1) ? centerAngle
                : centerAngle - spread * 0.5 + spread * k / (double)(N - 1);
            pEnemyShot->muki = a;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = kind;
            pEnemyShot->param_d[0] = pEnemyShotSet->x; // 発射座標を保持
            pEnemyShot->param_d[1] = pEnemyShotSet->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 発射座標 + 速度*count の純粋な式（積算ではない直線移動）
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：噛み合い接触点の煌めき（対になった短い煌めき弾）
// ============================================================
// pEnemyShotSet->muki に基準角度（接線方向）を事前設定しておくこと
static void ShotRadialSparkle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int k = 0; k < 2; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->muki = pEnemyShotSet->muki + (k == 0 ? 0.0 : DX_PI);
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotLaser[6];
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：予告点滅（テレグラフ）
// ============================================================
static void ShotTelegraphBlink(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 外周の東西南北・歯先位置で点滅させ、解放の予告とする
        for (int k = 0; k < 4; k++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = k * (DX_PI * 0.5);
            double tipR = OuterBaseR + OuterToothH;
            pEnemyShot->param_d[0] = pEnemyShotSet->x + tipR * cos(angle);
            pEnemyShot->param_d[1] = pEnemyShotSet->y + tipR * sin(angle);
            pEnemyShot->kind = img_enemyShotLargeBall[6];
            pEnemyShot->x = pEnemyShot->param_d[0];
            pEnemyShot->y = pEnemyShot->param_d[1];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0];
        pShot->y = pShot->param_d[1];
        // 白/赤を数フレームごとに切り替えて点滅させる
        pShot->kind = ((pShot->count / 4) % 2 == 0) ? img_enemyShotLargeBall[6] : img_enemyShotLargeBall[0];
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Spirograph_Claude()
{
    if (count == 1) {
        enemy.x = HubX;
        enemy.y = HubY;
        enemy.maxHp = enemy.hp = 200;

        // 外側リングギア：組立開始
        sEnemyShotSet* outerSet = new sEnemyShotSet;
        outerSet->count = 0;
        outerSet->patternFunc = ShotOuterRing;
        outerSet->x = HubX;
        outerSet->y = HubY;
        outerSet->param_i[0] = 0;
        outerSet->pEnemyShotHead = new sEnemyShot;
        outerSet->pEnemyShotHead->prev = outerSet->pEnemyShotHead;
        outerSet->pEnemyShotHead->next = outerSet->pEnemyShotHead;
        outerSet->prev = enemyShotSetHead.prev;
        outerSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = outerSet;
        enemyShotSetHead.prev = outerSet;
    }

    if (count == Phase1OuterEnd) {
        // 内側ピニオン：組立開始
        sEnemyShotSet* innerSet = new sEnemyShotSet;
        innerSet->count = 0;
        innerSet->patternFunc = ShotInnerRing;
        innerSet->x = HubX;
        innerSet->y = HubY;
        innerSet->param_i[0] = 0;
        innerSet->pEnemyShotHead = new sEnemyShot;
        innerSet->pEnemyShotHead->prev = innerSet->pEnemyShotHead;
        innerSet->pEnemyShotHead->next = innerSet->pEnemyShotHead;
        innerSet->prev = enemyShotSetHead.prev;
        innerSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = innerSet;
        enemyShotSetHead.prev = innerSet;
    }

    if (count == Phase2Start) {
        // 軌跡曲線：描画開始
        sEnemyShotSet* traceSet = new sEnemyShotSet;
        traceSet->count = 0;
        traceSet->patternFunc = ShotTraceCurve;
        traceSet->x = HubX;
        traceSet->y = HubY;
        traceSet->pEnemyShotHead = new sEnemyShot;
        traceSet->pEnemyShotHead->prev = traceSet->pEnemyShotHead;
        traceSet->pEnemyShotHead->next = traceSet->pEnemyShotHead;
        traceSet->prev = enemyShotSetHead.prev;
        traceSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = traceSet;
        enemyShotSetHead.prev = traceSet;
    }

    // フェーズ2中：ピニオン先端の歯から自機狙い3wayを継続的にパルス
    if (count >= Phase2Start && count < ReleaseTrigger && (count - Phase2Start) % 24 == 0) {
        double x, y, angle;
        InnerPinionPoint((double)count, 0.0, x, y, angle); // 局所角0の歯（先頭歯）を狙点にする

        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotAimedFan;
        set->x = x;
        set->y = y;
        set->param_i[0] = 3;
        set->param_i[1] = img_enemyShotBullet[0];
        set->param_i[2] = 0; // light
        set->param_d[0] = DX_PI / 6.0;
        set->param_d[1] = 2.6;
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // 外側歯先からの牽制弾（やや低頻度）
    if (count >= Phase1OuterEnd && count < ReleaseTrigger && count % 40 == 0) {
        double x, y, angle;
        OuterRingPoint((double)count, 0.0, x, y, angle); // 局所角0の歯を狙点にする

        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotAimedFan;
        set->x = x;
        set->y = y;
        set->param_i[0] = 3;
        set->param_i[1] = img_enemyShotMediumOval[0];
        set->param_i[2] = 1; // medium
        set->param_d[0] = DX_PI / 5.0;
        set->param_d[1] = 2.2;
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // フェーズ2中：噛み合い接触点の煌めき
    if (count >= Phase2Start && count < Phase2End && (count - Phase2Start) % 16 == 0) {
        double theta = OrbitOmega * (double)(count - Phase2Start);
        double cx = HubX + OuterBaseR * cos(theta);
        double cy = HubY + OuterBaseR * sin(theta);

        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotRadialSparkle;
        set->x = cx;
        set->y = cy;
        set->muki = theta + DX_PI * 0.5; // 接触点の接線方向
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // フェーズ4：予告点滅
    if (count == TelegraphStart) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotTelegraphBlink;
        set->x = HubX;
        set->y = HubY;
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // フェーズ4：全弾解放と同時に中心から自機狙い5way
    if (count == ReleaseTrigger) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotAimedFan;
        set->x = HubX;
        set->y = HubY;
        set->param_i[0] = 5;
        set->param_i[1] = img_enemyShotLargeBall[0];
        set->param_i[2] = 3; // extreme
        set->param_d[0] = DX_PI / 2.5;
        set->param_d[1] = 2.4;
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}