// enemyPat_TarakoSupagetti.cpp
// たらこスパゲッティ弾幕「明太絡繰り(めんたいからくり)」
//
// 構成:
//   ① 麺ライン弾   … ShotNoodleStrand : ボスから伸びる、渦を巻きながらうねる麺の弾列。
//                                        フィナーレでは同じ弾がそのまま中心へ収束し、大爆発する。
//   ② 卵嚢バースト … ShotRoeBurst     : ボス周囲を漂う"卵嚢"が周期的に弾ける粒弾
//   ③ 胡椒弾       … ShotPepper       : 低頻度・高速で画面端から撃ち込まれる黒弾
//
// 注記:
//   当初案にあった「海苔帯（当たり判定なしの安全地帯）」は、本コードベースが
//   衝突判定付きの sEnemyShot 以外に演出専用のオブジェクトを描画する手段を
//   持たないため、卵嚢バーストの発生位置をボス周囲でゆっくり回転させることで
//   「密度の薄い回転レーン」を代替として再現している。
//
//   フィナーレの収束は、専用の弾を新たに撃つのではなく、その時点で画面上に
//   存在する麺ライン弾そのものの挙動を切り替えることで実現している
//   （＝画面上の弾が本当に「全て」中心へ収束する）。
//
//   img_enemyShotLaser は当たり判定が大きい(64x4)ため使用していない。
//   全ての乱数は GetRand(x) （0以上x以下のx+1通り）を使用し、リプレイ再現性を確保している。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int STRAND_NUM = 6;   // 麺ラインの本数
static const int CYCLE_LEN = 600; // 1周期のフレーム数（15秒 @60fps）でループする
static const int PHASE_MAIN_START = 200; // 卵嚢バースト・胡椒弾が本格化する境目
static const int PHASE_FINALE_START = 450; // フィナーレ（収束→大爆発）の開始点
static const int CONVERGE_FRAMES = 60;  // 収束にかけるフレーム数

// 麺ラインの角度を、ストランド番号と周期内時刻から計算する。
// 各ストランドは基準角度からずれた位相でゆっくり波打つ。
static double NoodleAngle(int strandIndex, int localCount)
{
    double theta0 = strandIndex * (2.0 * DX_PI / STRAND_NUM);
    double phi = strandIndex * (2.0 * DX_PI / STRAND_NUM);
    return theta0 + (25.0 * DX_PI / 180.0) * sin(2.0 * DX_PI * localCount / 240.0 * 2 + phi);
}

// ============================================================
//  ① 麺ライン弾：渦を巻きながら伸びる → フィナーレで中心へ収束 → 大爆発
// ============================================================
static void ShotNoodleStrand(sEnemyShotSet* pEnemyShotSet)
{
    int strandIndex = pEnemyShotSet->param_i[0];
    int localCount = (count - 1) % CYCLE_LEN;
    int elapsedFinale = localCount - PHASE_FINALE_START; // フィナーレ開始からの経過フレーム（開始前は負）

    bool stillGrowing = elapsedFinale < 0;

    // 5フレームごとに新しい"節"を追加していく（麺が伸びていくイメージ）。フィナーレ中は追加しない。
    if (stillGrowing && pEnemyShotSet->count % 2 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        double angle = NoodleAngle(strandIndex, localCount);

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->speed = 2.35;          // 半径方向への伸び速度として流用
        pEnemyShot->param_d[0] = angle;    // 生成時の角度で固定する＝渦を巻く軌跡になる
        pEnemyShot->muki = angle;          // 進行方向（見た目の向き）＝放射角度そのもの
        pEnemyShot->kind = img_enemyShotBullet[1]; // 黄：たらこスパゲッティの麺の色
        pEnemyShot->param_i[1] = 0;        // 0:成長/収束中　1:大爆発後

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[1] == 0) {
            if (elapsedFinale < 0) {
                // --- 成長フェーズ：count のみから半径を決定する式駆動の動き ---
                double radius = pShot->speed * pShot->count;

                pShot->x = pEnemyShotSet->x + radius * cos(pShot->param_d[0]);
                pShot->y = pEnemyShotSet->y + radius * sin(pShot->param_d[0]);
            }
            else if (elapsedFinale < CONVERGE_FRAMES) {
                // --- 収束フェーズ：フィナーレ開始時点の半径を基準に、count のみから縮めていく ---
                // pShot->count は毎フレーム自動加算され続けるため、
                // 「pShot->count - elapsedFinale」は常にフィナーレ開始時点の値になり、
                // 収束の起点半径を追加のパラメータなしで一意に復元できる。
                int countAtFinaleStart = pShot->count - elapsedFinale;
                double radiusAtFinaleStart = pShot->speed * countAtFinaleStart;
                double t = (double)elapsedFinale / CONVERGE_FRAMES;

                double angle = pShot->param_d[0] + t * DX_PI; // 半回転しながら収束＝フォークで巻き取る演出
                double radius = radiusAtFinaleStart * (1.0 - t);

                pShot->x = pEnemyShotSet->x + radius * cos(angle);
                pShot->y = pEnemyShotSet->y + radius * sin(angle);
                pShot->muki = angle + DX_PI; // 中心へ向かう向き（見た目の回転用）
            }
            else {
                // --- 収束完了→大爆発へ切り替え（一度だけ）---
                double burstAngle = GetRand(3599) / 3600.0 * 2.0 * DX_PI;

                pShot->muki = burstAngle;
                pShot->speed = 2.2 + GetRand(150) / 100.0;
                pShot->kind = img_enemyShotMediumBall[0]; // 赤：弾ける瞬間の色替え
                pShot->param_i[1] = 1;
            }
        }
        else {
            // --- 大爆発フェーズ：単純な直線放射のため速度積分で問題ない ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  ② 卵嚢バースト：漂う"卵嚢"が周期的にプチッと弾ける
// ============================================================
static void ShotRoeBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int grainNum = 18 + GetRand(6); // 18~24粒（増量）

        for (int i = 0; i < grainNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double angle = GetRand(3599) / 3600.0 * 2.0 * DX_PI;
            double speed = 1.1 + GetRand(140) / 100.0;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;   // 進行方向＝放射角度（速度積分にも見た目にも使う）
            pEnemyShot->speed = speed;
            // 赤/橙をランダムに混ぜて、たらこの粒っぽい見た目にする
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾ける粒は寿命が短く画面外へすぐ抜けるため、単純な速度積分で問題ない
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  ③ 胡椒弾：画面端から撃ち込まれる低頻度・高速の黒弾
// ============================================================
static void ShotPepper(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;

        double edgeX, edgeY;
        switch (GetRand(3)) {
        case 0: edgeX = GetRand(480); edgeY = -20.0;         break;
        case 1: edgeX = GetRand(480); edgeY = 500.0;         break;
        case 2: edgeX = -20.0;        edgeY = GetRand(480);  break;
        default:edgeX = 500.0;        edgeY = GetRand(480);  break;
        }

        pEnemyShot->x = edgeX;
        pEnemyShot->y = edgeY;
        // 進行方向＝プレイヤー狙い±30度のブレ（速度積分にも見た目にも使う）
        pEnemyShot->muki = atan2(player.y - edgeY, player.x - edgeX)
            + (GetRand(60) - 30) / 180.0 * DX_PI;
        pEnemyShot->speed = 4.2 + GetRand(80) / 100.0;
        pEnemyShot->kind = img_enemyShotBullet[7]; // 黒：胡椒粒

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_TarakoSpaghetti_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 250 == 125) muki *= -1;
    }

    int localCount = (count - 1) % CYCLE_LEN;

    // --- ① 周期の頭で麺ラインを6本まとめて発生させる ---
    if (localCount == 0) {
        for (int i = 0; i < STRAND_NUM; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotNoodleStrand;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->param_i[0] = i;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // --- ② 卵嚢バースト：中盤に入ると頻度・同時発生数が上がる（たらこ増量）---
    bool inMain = localCount < PHASE_FINALE_START;
    int roeInterval = (localCount < PHASE_MAIN_START) ? 70 : 25;
    int roeBurstPoints = (localCount < PHASE_MAIN_START) ? 1 : 2;

    if (inMain && localCount % roeInterval == 0) {
        for (int i = 0; i < roeBurstPoints; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotRoeBurst;

            // ボスの周りをゆっくり回転する位置から発生させ、
            // 「回転レーンの外側は密度が薄い」という一時的な回避ルートを作る。
            // 中盤以降は2点を対称（π離れた位置）に配置して同時発生させる。
            double slideAngle = 2.0 * DX_PI * localCount / 300.0 + i * DX_PI;
            double slideRadius = 60.0 + 20.0 * sin(2.0 * DX_PI * localCount / 150.0);
            pEnemyShotSet->x = enemy.x + slideRadius * cos(slideAngle);
            pEnemyShotSet->y = enemy.y + slideRadius * sin(slideAngle);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }

    // --- ③ 胡椒弾：中盤以降、低頻度で撃ち込む ---
    if (localCount >= PHASE_MAIN_START && localCount < PHASE_FINALE_START && localCount % 25 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPepper;
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

    // --- ④ フィナーレの合図音（弾自体はShotNoodleStrand内で既存の麺弾がそのまま収束・爆発する）---
    if (localCount == PHASE_FINALE_START) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 「巻き取るぞ」の予告音
    }
    if (localCount == PHASE_FINALE_START + CONVERGE_FRAMES) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK); // 大爆発音
    }
}