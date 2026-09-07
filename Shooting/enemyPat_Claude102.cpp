// enemyPat_meiwakuPopupRanbu.cpp
//
// 迷惑ポップアップ乱舞
// ポップアップ広告をモチーフにした4フェーズパターン。
// 専用スプライトを使わず、既存の弾種の「配置」と「挙動」だけでポップアップ広告っぽさを表現する。
//
//   フェーズ1(出現/count=1〜):画面四隅寄りに矩形の広告ウィンドウが4枚ポップアップし、
//                              各ウィンドウに偽の×ボタンと横スクロールバナーが常駐する。
//   フェーズ2(count〜360):     ×ボタンが一定周期で別の角へ逃げ回り、
//                              逃げる瞬間に自機を狙って反撃する。バナーは常時流れ続ける。
//   フェーズ3(count=360〜):    隙間を埋めるように子ポップアップが4枚追加で増殖する。
//   フェーズ4(count=560〜):    全ウィンドウが中央に統合された巨大な全画面広告の枠が出現し、
//                              count=620で全ての枠・巨大枠が一斉に中心から放射状に弾け飛び、
//                              同時に中央からリングバースト+自機狙い5wayが発射されて終幕する。
//
// 使用素材の選定理由:
//   ・img_enemyShotBullet   … ウィンドウの矩形枠線(細長い形状が枠っぽく見える)
//   ・img_enemyShotSmallBall… ×ボタンのバツ印(小玉を対角線状に並べる)
//   ・img_enemyShotMediumOval… バナー広告(横長の楕円が帯状バナーに見える)
//   ・img_enemyShotLargeBall … フィナーレのリングバースト(大玉で存在感を出す)
//
// パラメータ設計(sEnemyShotSet):
//   PatWindowFrame  : param_d[0..1]=中心xy, [2..3]=半幅半高, [4]=解放開始グローバルcount / param_i[0]=色
//   PatCloseButton  : param_d[0..1]=所属ウィンドウ中心xy, [2..3]=半幅半高
//   PatBannerScroll : param_d[0..1]=所属ウィンドウ中心xy, [2..3]=半幅半高, [4]=スクロール速度
//   PatAimedFan     : x,y=発射原点, muki=中心角, param_i[0]=way数, param_d[0]=開き角, param_d[1]=速度, param_i[1]=色
//   PatRingBurst    : x,y=発射原点, param_i[0]=発数, param_d[0]=速度, param_i[1]=色
//
// パラメータ設計(sEnemyShot、各パターン内で用途が異なる):
//   PatWindowFrame の弾 : param_d[0..1]=中心からの目標オフセットxy, param_d[2..3]=解放時の飛散方向(単位ベクトル)
//   PatCloseButton の弾: param_d[0..1]=ボタン中心からの相対オフセット(バツ印の形)
//   PatBannerScroll の弾: param_i[0]=行(0:上段/1:下段), param_d[0]=初期位相(0〜1)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

const int T = 800;
static int countT;

// ============================================================
// 矩形の周上の点を、周長に沿った媒介変数t(0〜1)から求める
// t=0で左上から時計回りに一周する
// ============================================================
static void RectPerimeterPoint(double halfW, double halfH, double t, double* outX, double* outY)
{
    double width = halfW * 2.0;
    double height = halfH * 2.0;
    double total = 2.0 * (width + height);
    double s = t * total;

    if (s < width) {
        *outX = -halfW + s;
        *outY = -halfH;
    }
    else if (s < width + height) {
        *outX = halfW;
        *outY = -halfH + (s - width);
    }
    else if (s < width + height + width) {
        *outX = halfW - (s - width - height);
        *outY = halfH;
    }
    else {
        *outX = -halfW;
        *outY = halfH - (s - width - height - width);
    }
}

// ============================================================
// 自機狙いのn-way弾(ボタンの反撃/フィナーレの多way弾で共用)
// ============================================================
static void PatAimedFan(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        int wayCount = pSet->param_i[0];
        double spread = pSet->param_d[0];
        double speed = pSet->param_d[1];
        int colorIndex = pSet->param_i[1];

        for (int i = 0; i < wayCount; i++) {
            double offset = (wayCount == 1) ? 0.0 : spread * (i - (wayCount - 1) / 2.0);

            pShot = new sEnemyShot;
            pShot->kind = img_enemyShotBullet[colorIndex];
            pShot->muki = pSet->muki + offset;
            pShot->speed = speed;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    double originX = pSet->x;
    double originY = pSet->y;

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = originX + pShot->speed * cos(pShot->muki) * t;
        pShot->y = originY + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ============================================================
// 全方位リングバースト(フィナーレ用)
// ============================================================
static void PatRingBurst(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        int ringCount = pSet->param_i[0];
        double speed = pSet->param_d[0];
        int colorIndex = pSet->param_i[1];

        for (int i = 0; i < ringCount; i++) {
            double angle = DX_PI * 2.0 * i / ringCount;

            pShot = new sEnemyShot;
            pShot->kind = img_enemyShotLargeBall[colorIndex];
            pShot->muki = angle;
            pShot->speed = speed;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    double originX = pSet->x;
    double originY = pSet->y;

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = originX + pShot->speed * cos(pShot->muki) * t;
        pShot->y = originY + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ============================================================
// 広告ウィンドウの矩形枠線
// 出現時: 中心からease-outで外側へ広がって枠を形作る
// 解放後(グローバルcountがreleaseAtCountを超えたら): 枠の各点がそのまま外向きに加速飛散する
// ============================================================
static void PatWindowFrame(sEnemyShotSet* pSet)
{
    double centerX = pSet->param_d[0];
    double centerY = pSet->param_d[1];
    double halfW = pSet->param_d[2];
    double halfH = pSet->param_d[3];
    double releaseAtCount = pSet->param_d[4];
    int colorIndex = pSet->param_i[0];

    sEnemyShot* pShot;
    if (pSet->count == 0) {
        double perimeter = 2.0 * (2.0 * halfW + 2.0 * halfH);
        int borderCount = (int)(perimeter / 14.0);
        if (borderCount < 16) borderCount = 16;
        if (borderCount > 80) borderCount = 80;

        for (int i = 0; i < borderCount; i++) {
            double t = (double)i / borderCount;
            double dx, dy;
            RectPerimeterPoint(halfW, halfH, t, &dx, &dy);

            pShot = new sEnemyShot;
            pShot->kind = img_enemyShotBullet[colorIndex];
            pShot->param_d[0] = dx; // 中心から見た枠上の目標オフセット
            pShot->param_d[1] = dy;
            double dist = sqrt(dx * dx + dy * dy);
            pShot->param_d[2] = (dist > 0.0001) ? dx / dist : 1.0; // 解放時の飛散方向(単位ベクトル)
            pShot->param_d[3] = (dist > 0.0001) ? dy / dist : 0.0;
            pShot->muki = atan2(dy, dx);

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    const double growDuration = 40.0;
    const double burstSpeed = 2.4;
    double elapsedFromRelease = (double)countT - releaseAtCount; // count はグローバルなフレームカウンタ

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double dx = pShot->param_d[0];
        double dy = pShot->param_d[1];

        if (elapsedFromRelease < 0.0) {
            // 出現〜静止保持:中心から目標オフセットへease-outで広がる
            double growT = pShot->count / growDuration;
            if (growT > 1.0) growT = 1.0;
            double eased = 1.0 - (1.0 - growT) * (1.0 - growT);
            pShot->x = centerX + dx * eased;
            pShot->y = centerY + dy * eased;
        }
        else {
            // 解放:枠だった位置から外向きに加速しながら飛散する
            double dirX = pShot->param_d[2];
            double dirY = pShot->param_d[3];
            double travel = burstSpeed * elapsedFromRelease * (1.0 + elapsedFromRelease * 0.01);
            pShot->x = centerX + dx + dirX * travel;
            pShot->y = centerY + dy + dirY * travel;
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 偽の×閉じるボタン
// 一定周期でウィンドウの別の角へ瞬間移動し、移動の瞬間に自機を狙って反撃する
// ============================================================
static void PatCloseButton(sEnemyShotSet* pSet)
{
    double windowCenterX = pSet->param_d[0];
    double windowCenterY = pSet->param_d[1];
    double halfW = pSet->param_d[2];
    double halfH = pSet->param_d[3];
    const int jumpInterval = 70;
    const double inset = 16.0;

    sEnemyShot* pShot;
    if (pSet->count == 0) {
        // バツ印を対角2本 x 4発ずつ、計8発で表現
        for (int line = 0; line < 2; line++) {
            for (int i = 0; i < 4; i++) {
                pShot = new sEnemyShot;
                pShot->kind = img_enemyShotSmallBall[0]; // 赤
                double t = (i - 1.5) * 3.0;
                pShot->param_d[0] = t;
                pShot->param_d[1] = (line == 0) ? t : -t;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    int cornerIndex = (pSet->count / jumpInterval) % 4;
    double cornerOffsetX[4] = { halfW - inset, -(halfW - inset), halfW - inset, -(halfW - inset) };
    double cornerOffsetY[4] = { -(halfH - inset), -(halfH - inset), halfH - inset, halfH - inset };

    double buttonX = windowCenterX + cornerOffsetX[cornerIndex];
    double buttonY = windowCenterY + cornerOffsetY[cornerIndex];

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = buttonX + pShot->param_d[0];
        pShot->y = buttonY + pShot->param_d[1];
        if (countT == T - 1) pShot->margin = -999;
        pShot = pShot->next;
    }

    // 逃げる瞬間(=角が切り替わる瞬間)、逃げる前の位置から自機狙い2wayで反撃する
    if (pSet->count > 0 && pSet->count % jumpInterval == 0) {
        int prevCornerIndex = ((pSet->count / jumpInterval) - 1 + 4) % 4;
        double prevX = windowCenterX + cornerOffsetX[prevCornerIndex];
        double prevY = windowCenterY + cornerOffsetY[prevCornerIndex];

        sEnemyShotSet* pAimSet = new sEnemyShotSet;
        pAimSet->count = 0;
        pAimSet->patternFunc = PatAimedFan;
        pAimSet->x = prevX;
        pAimSet->y = prevY;
        pAimSet->muki = atan2(player.y - prevY, player.x - prevX);
        pAimSet->param_i[0] = 2;    // 2way
        pAimSet->param_d[0] = 0.18; // 開き角
        pAimSet->param_d[1] = 3.0;  // 速度
        pAimSet->param_i[1] = 0;    // 赤

        pAimSet->pEnemyShotHead = new sEnemyShot;
        pAimSet->pEnemyShotHead->prev = pAimSet->pEnemyShotHead;
        pAimSet->pEnemyShotHead->next = pAimSet->pEnemyShotHead;

        pAimSet->prev = enemyShotSetHead.prev;
        pAimSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pAimSet;
        enemyShotSetHead.prev = pAimSet;
    }
}

// ============================================================
// ウィンドウ内を横スクロールする帯状バナー広告(上下2段、互い違いに流れる)
// ============================================================
static void PatBannerScroll(sEnemyShotSet* pSet)
{
    const int perRow = 4;
    sEnemyShot* pShot;
    if (pSet->count == 0) {
        for (int row = 0; row < 2; row++) {
            for (int i = 0; i < perRow; i++) {
                pShot = new sEnemyShot;
                pShot->kind = (row == 0) ? img_enemyShotMediumOval[1] : img_enemyShotMediumOval[8]; // 黄/橙
                pShot->param_i[0] = row;
                pShot->param_d[0] = (double)i / perRow; // 初期位相

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    double centerX = pSet->param_d[0];
    double centerY = pSet->param_d[1];
    double halfW = pSet->param_d[2];
    double halfH = pSet->param_d[3];
    double width = halfW * 2.0;
    double speed = pSet->param_d[4];

    double left = centerX - halfW;
    double topRowY = centerY - halfH * 0.45;
    double bottomRowY = centerY + halfH * 0.45;

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double phase = pShot->param_d[0];
        double dir = (pShot->param_i[0] == 0) ? 1.0 : -1.0;
        double progress = phase + dir * speed * pSet->count / width;
        progress = progress - floor(progress); // 0〜1でループ(横スクロール)

        pShot->x = left + progress * width;
        pShot->y = (pShot->param_i[0] == 0) ? topRowY : bottomRowY;
        pShot->muki = (dir > 0) ? 0.0 : DX_PI;

        if (countT == T - 1) pShot->margin = -999;

        pShot = pShot->next;
    }
}

// ============================================================
// 広告ウィンドウ1枚(枠+×ボタン+バナー)をまとめて生成するヘルパー
// ============================================================
static void SpawnAdWindow(double centerX, double centerY, double halfW, double halfH, double releaseAtCount, int colorIndex)
{
    // 枠
    sEnemyShotSet* pFrame = new sEnemyShotSet;
    pFrame->count = 0;
    pFrame->patternFunc = PatWindowFrame;
    pFrame->x = centerX;
    pFrame->y = centerY;
    pFrame->param_d[0] = centerX;
    pFrame->param_d[1] = centerY;
    pFrame->param_d[2] = halfW;
    pFrame->param_d[3] = halfH;
    pFrame->param_d[4] = releaseAtCount;
    pFrame->param_i[0] = colorIndex;
    pFrame->pEnemyShotHead = new sEnemyShot;
    pFrame->pEnemyShotHead->prev = pFrame->pEnemyShotHead;
    pFrame->pEnemyShotHead->next = pFrame->pEnemyShotHead;
    pFrame->prev = enemyShotSetHead.prev;
    pFrame->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pFrame;
    enemyShotSetHead.prev = pFrame;

    // ×ボタン
    sEnemyShotSet* pButton = new sEnemyShotSet;
    pButton->count = 0;
    pButton->patternFunc = PatCloseButton;
    pButton->x = centerX;
    pButton->y = centerY;
    pButton->param_d[0] = centerX;
    pButton->param_d[1] = centerY;
    pButton->param_d[2] = halfW;
    pButton->param_d[3] = halfH;
    pButton->pEnemyShotHead = new sEnemyShot;
    pButton->pEnemyShotHead->prev = pButton->pEnemyShotHead;
    pButton->pEnemyShotHead->next = pButton->pEnemyShotHead;
    pButton->prev = enemyShotSetHead.prev;
    pButton->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pButton;
    enemyShotSetHead.prev = pButton;

    // バナー
    sEnemyShotSet* pBanner = new sEnemyShotSet;
    pBanner->count = 0;
    pBanner->patternFunc = PatBannerScroll;
    pBanner->x = centerX;
    pBanner->y = centerY;
    pBanner->param_d[0] = centerX;
    pBanner->param_d[1] = centerY;
    pBanner->param_d[2] = halfW;
    pBanner->param_d[3] = halfH;
    pBanner->param_d[4] = 1.2; // スクロール速度
    pBanner->pEnemyShotHead = new sEnemyShot;
    pBanner->pEnemyShotHead->prev = pBanner->pEnemyShotHead;
    pBanner->pEnemyShotHead->next = pBanner->pEnemyShotHead;
    pBanner->prev = enemyShotSetHead.prev;
    pBanner->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pBanner;
    enemyShotSetHead.prev = pBanner;
}

// ============================================================
// 敵本体のパターン:迷惑ポップアップ乱舞
// ============================================================
void EnemyPat_PopUpAds_Claude()
{
    const int childSpawnCount = 360;  // フェーズ3:子ポップアップ増殖
    const int giantFrameSpawn = 560;  // フェーズ4:全画面広告の枠が出現
    const int releaseCount = 620;     // フィナーレ:全ウィンドウが一斉に弾け飛ぶ

    if (count == 1) {
        // ゲーム画面は480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    countT = count % T;

    if (countT == 1) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // フェーズ1:画面四隅寄りに広告ウィンドウが4枚ポップアップ
        SpawnAdWindow(130.0, 150.0, 75.0, 55.0, (double)releaseCount, 6); // 白
        SpawnAdWindow(350.0, 150.0, 75.0, 55.0, (double)releaseCount, 6);
        SpawnAdWindow(130.0, 330.0, 75.0, 55.0, (double)releaseCount, 6);
        SpawnAdWindow(350.0, 330.0, 75.0, 55.0, (double)releaseCount, 6);
    }

    if (countT == childSpawnCount) {
        // フェーズ3:隙間(上下左右の通路)を埋めるように子ポップアップが増殖
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        SpawnAdWindow(240.0, 150.0, 45.0, 35.0, (double)releaseCount, 3); // シアン
        SpawnAdWindow(240.0, 330.0, 45.0, 35.0, (double)releaseCount, 3);
        SpawnAdWindow(130.0, 240.0, 45.0, 35.0, (double)releaseCount, 3);
        SpawnAdWindow(350.0, 240.0, 45.0, 35.0, (double)releaseCount, 3);
    }

    if (countT == giantFrameSpawn) {
        // フェーズ4:全ての広告が中央で1枚の全画面広告に統合される(枠のみ、×やバナーは付けない)
        sEnemyShotSet* pGiant = new sEnemyShotSet;
        pGiant->count = 0;
        pGiant->patternFunc = PatWindowFrame;
        pGiant->x = 240.0;
        pGiant->y = 240.0;
        pGiant->param_d[0] = 240.0;
        pGiant->param_d[1] = 240.0;
        pGiant->param_d[2] = 210.0;
        pGiant->param_d[3] = 210.0;
        pGiant->param_d[4] = (double)releaseCount;
        pGiant->param_i[0] = 8; // 橙
        pGiant->pEnemyShotHead = new sEnemyShot;
        pGiant->pEnemyShotHead->prev = pGiant->pEnemyShotHead;
        pGiant->pEnemyShotHead->next = pGiant->pEnemyShotHead;
        pGiant->prev = enemyShotSetHead.prev;
        pGiant->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pGiant;
        enemyShotSetHead.prev = pGiant;
    }

    if (countT == releaseCount) {
        // フィナーレ:「同意する」を押すまで消えない広告、を体現する一斉解放
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pRing = new sEnemyShotSet;
        pRing->count = 0;
        pRing->patternFunc = PatRingBurst;
        pRing->x = 240.0;
        pRing->y = 240.0;
        pRing->param_i[0] = 28;
        pRing->param_d[0] = 2.2;
        pRing->param_i[1] = 6; // 白
        pRing->pEnemyShotHead = new sEnemyShot;
        pRing->pEnemyShotHead->prev = pRing->pEnemyShotHead;
        pRing->pEnemyShotHead->next = pRing->pEnemyShotHead;
        pRing->prev = enemyShotSetHead.prev;
        pRing->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRing;
        enemyShotSetHead.prev = pRing;

        sEnemyShotSet* pFan = new sEnemyShotSet;
        pFan->count = 0;
        pFan->patternFunc = PatAimedFan;
        pFan->x = 240.0;
        pFan->y = 240.0;
        pFan->muki = atan2(player.y - 240.0, player.x - 240.0);
        pFan->param_i[0] = 5;
        pFan->param_d[0] = 0.16;
        pFan->param_d[1] = 3.4;
        pFan->param_i[1] = 0; // 赤
        pFan->pEnemyShotHead = new sEnemyShot;
        pFan->pEnemyShotHead->prev = pFan->pEnemyShotHead;
        pFan->pEnemyShotHead->next = pFan->pEnemyShotHead;
        pFan->prev = enemyShotSetHead.prev;
        pFan->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFan;
        enemyShotSetHead.prev = pFan;
    }
}