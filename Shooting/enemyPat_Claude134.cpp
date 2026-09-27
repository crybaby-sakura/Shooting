#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 禁忌「カゴメカゴメ」
//
// 東方紅魔郷本家：ライン状に並べた丸弾を網目模様に配置後、大玉をぶつけて
//                 ラインを崩し画面中に拡散させる、という演出。
// 東方紅魔郷 New Classic：緑と黄のレーザーの格子で自機を箱に閉じ込め、
//                         箱がゆっくり下降していく、という演出。
// 上記2つの資料をもとにした独自解釈：
//   「籠目」文様のように緑（横線）・黄（縦線）のレーザーで格子を編んで
//   ゆっくり降下させ、ボスの放つ大玉の飛来に合わせて中心から外側へ
//   波紋状に破断が伝播、各セグメントが丸弾となって放射状に四散する。
//
// 前提（要確認）：
//   ・ゲーム画面は480x480（enemyPat_sampleForAI.cppのコメントに準拠）
//   ・短レーザー(64.0x4.0)はmuki=0で横向き（長辺がX軸方向）に描画される想定
// ============================================================

// ---- 網の配置・タイミングパラメータ ----
static const int    NET_ORIGIN_X = 0;      // 格子の左端X
static const int    NET_ORIGIN_Y = 160;    // 格子の上端Y
static const int    NET_SEGMENT_LEN = 64;     // レーザー弾1本分の長さ（img_enemyShotLaserの長辺）
static const int    NET_H_LINE_COUNT = 5;     // 横線の本数
static const int    NET_H_SEG_COUNT = 7;     // 横線1本あたりのセグメント数
static const int    NET_V_LINE_COUNT = 8;     // 縦線の本数
static const int    NET_V_SEG_COUNT = 4;     // 縦線1本あたりのセグメント数
static const double NET_CENTER_X = 240.0;  // 網の中心（波紋の起点）
static const double NET_CENTER_Y = 288.0;
static const int    NET_BREAK_START = 200;    // 網完成後、中心セグメントが破断し始めるまでのフレーム数
static const double NET_RIPPLE_SPEED = 5.0;   // 波紋が中心から外へ伝わる速さ（px/frame）
static const double NET_BURST_SPEED = 3.5;   // 破断した破片が飛び散る基準速度

// ---- 全体の進行タイミング（EnemyPat_KagomeKagome_Claude内のcount基準） ----
static const int PHASE_NET_START = 30;   // 網の展開開始
static const int PHASE_BREAKER_START = 178;  // 大玉発射（網の中心が破断し始める頃に到達するよう逆算）
static const int PHASE_FINAL_BURST = 400;  // 網が完全に崩れ切った後の締めくくり弾幕
static const int PHASE_AIMED_SHOT_END = 360;  // 牽制弾の打ち止め（締めくくり弾幕の前に止める）

// sEnemyShotのparam使用法（このファイル内の網パターン専用）：
//   param_i[0] : 破断済みフラグ (0=未破断, 1=破断済み)
//   param_i[1] : 元の色番号（緑=2 / 黄=1）。破断後の丸弾の色に踏襲する
//   param_d[0] : 移動速度 vx（muki＝見た目の向きと分離して管理）
//   param_d[1] : 移動速度 vy
//   param_d[2] : 網の中心からの距離（波紋到達タイミングの算出に使用）

// 弾幕：籠目状の網（緑＝横線／黄＝縦線の短レーザーで格子を編み、
//       波紋状に中心から外側へ破断して丸弾となり四散する）
static void ShotKagomeNet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 横線（緑）
        for (int row = 0; row < NET_H_LINE_COUNT; row++) {
            double y = NET_ORIGIN_Y + row * NET_SEGMENT_LEN;
            for (int col = 0; col < NET_H_SEG_COUNT; col++) {
                double x = NET_ORIGIN_X + col * NET_SEGMENT_LEN + NET_SEGMENT_LEN / 2.0;
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x;
                pEnemyShot->y = y;
                pEnemyShot->muki = 0.0; // 横向き（見た目の向き。移動方向とは別管理）
                pEnemyShot->speed = 0.0; // 移動はparam_d[0]/[1]で行うため未使用
                pEnemyShot->kind = img_enemyShotLaser[2]; // 緑
                pEnemyShot->param_i[0] = 0;
                pEnemyShot->param_i[1] = 2;
                pEnemyShot->param_d[0] = 0.0;
                pEnemyShot->param_d[1] = 0.3; // ゆっくり下降し、網が迫ってくる圧を出す
                double dx = x - NET_CENTER_X, dy = y - NET_CENTER_Y;
                pEnemyShot->param_d[2] = sqrt(dx * dx + dy * dy);
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 縦線（黄）
        for (int col = 0; col < NET_V_LINE_COUNT; col++) {
            double x = NET_ORIGIN_X + col * NET_SEGMENT_LEN;
            for (int seg = 0; seg < NET_V_SEG_COUNT; seg++) {
                double y = NET_ORIGIN_Y + seg * NET_SEGMENT_LEN + NET_SEGMENT_LEN / 2.0;
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x;
                pEnemyShot->y = y;
                pEnemyShot->muki = DX_PI / 2.0; // 縦向き
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotLaser[1]; // 黄
                pEnemyShot->param_i[0] = 0;
                pEnemyShot->param_i[1] = 1;
                pEnemyShot->param_d[0] = 0.0;
                pEnemyShot->param_d[1] = 0.3;
                double dx = x - NET_CENTER_X, dy = y - NET_CENTER_Y;
                pEnemyShot->param_d[2] = sqrt(dx * dx + dy * dy);
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 未破断：波紋が到達するタイミングを、中心からの距離で算出
            double breakFrame = NET_BREAK_START + pShot->param_d[2] / NET_RIPPLE_SPEED;
            if ((double)pEnemyShotSet->count >= breakFrame) {
                // 波紋がここまで到達：中心から外向きに弾け飛ぶ丸弾に変化
                pShot->param_i[0] = 1;
                double dx = pShot->x - NET_CENTER_X, dy = pShot->y - NET_CENTER_Y;
                double dist = sqrt(dx * dx + dy * dy);
                if (dist < 0.01) dist = 0.01;
                double burstSpeed = NET_BURST_SPEED + GetRand(150) / 100.0;
                pShot->param_d[0] = dx / dist * burstSpeed;
                pShot->param_d[1] = dy / dist * burstSpeed;
                pShot->muki = atan2(dy, dx); // 見た目も飛んでいく向きに合わせる
                pShot->kind = img_enemyShotSmallBall[pShot->param_i[1]]; // 元の色（緑/黄）を踏襲
            }
        }
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot = pShot->next;
    }
}

// 弾幕：網へ襲いかかる大玉（波紋のきっかけを視覚的に演出する）
static void ShotBreakerBall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int BALL_NUM = 5;
        for (int i = 0; i < BALL_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double targetX = NET_ORIGIN_X + (double)(i + 1) * (NET_H_SEG_COUNT * NET_SEGMENT_LEN) / (BALL_NUM + 1);
            double targetY = NET_CENTER_Y;
            pEnemyShot->muki = atan2(targetY - pEnemyShot->y, targetX - pEnemyShot->x);
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotLargeBall[8]; // 橙
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

// 弾幕：網の形成中、ボスが放つ牽制の狙い撃ち三方弾
static void ShotAimedThreeWay(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = -1; i <= 1; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * 0.15;
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤
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

// 弾幕：網の崩壊後、画面全体に広がる締めくくりの多重リング弾
static void ShotFinalBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int RING_NUM = 40;
        const int RING_COUNT = 4;
        for (int ring = 0; ring < RING_COUNT; ring++) {
            for (int i = 0; i < RING_NUM; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                double angle = DX_PI * 2.0 * i / RING_NUM + ring * (DX_PI * 2.0 / RING_NUM / (double)RING_COUNT);
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 1.8 + ring * 0.5;
                int colorIdx = (ring % 2 == 0) ? 2 : 1; // 緑と黄を交互に、網の色を踏襲
                pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン：禁忌「カゴメカゴメ」
void EnemyPat_KagomeKagome_Claude()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと。
        muki = 1;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }

    const int T = 500;
    int countT = count % T;

    // 牽制弾：網が形成されている間、20フレームごとに狙い撃ち三方弾
    if (countT % 20 == 1 && countT < PHASE_AIMED_SHOT_END) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAimedThreeWay;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 籠目の網の展開
    if (countT == PHASE_NET_START) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKagomeNet;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 網を打ち崩す大玉（波紋のきっかけを演出）
    if (countT == PHASE_BREAKER_START) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBreakerBall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 網が完全に崩れきった後の締めくくり弾幕
    if (countT == PHASE_FINAL_BURST) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFinalBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}