// enemyPat_KengouRanbu.cpp
//
// 剣豪乱舞 — 近接戦闘型ボス弾幕
// ボス本体が自機との間合いを積極的に詰め、
// 「間合い→抜刀→乱撃→残心」の4フェーズを無限ループする。
//
// フェーズ構成 (1サイクル = 540フレーム):
//   1. 間合い   (  1〜150 ) : 画面上部から接近しつつ足跡と警告壁を残す
//   2. 抜刀     (151〜240 ) : 停止し、三段斬りの弧状レーザーを発射
//   3. 乱撃     (241〜420 ) : 3回の踏み込みダッシュ、着地毎に自機狙い連発
//   4. 残心・納刀(421〜540 ) : 溜め→全方位＋自機狙いの大技→後退して1へ

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数群 (ファイルスコープの短い名前は避ける: CX/CY等はSDKと衝突する)
// ============================================================
static const double kBossSpawnX = 240.0; // 開始/後退先のx
static const double kBossSpawnY = 50.0;  // 開始/後退先のy
static const double kEngageY = 280.0;    // 交戦間合いの基準深度
static const double kBossMinX = 60.0;    // ボスの可動範囲(左)
static const double kBossMaxX = 420.0;   // ボスの可動範囲(右)

static const int kPhase1Len = 150; // 間合い
static const int kPhase2Len = 90;  // 抜刀
static const int kPhase3Len = 180; // 乱撃
static const int kPhase4Len = 120; // 残心・納刀
static const int kCycleLen = kPhase1Len + kPhase2Len + kPhase3Len + kPhase4Len; // 540

// スムーズステップ(イーズイン・アウト)
static double EaseSmooth(double t)
{
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

// 新しいsEnemyShotSetを生成し、リストに連結して返す共通ヘルパー
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

// 新しいsEnemyShotを生成し、指定ShotSetのリストに連結して返す共通ヘルパー
static sEnemyShot* SpawnShot(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ============================================================
//  弾幕: 足跡 (フェーズ1の接近路・フェーズ4の後退路に残す低脅威弾)
// ============================================================
static void ShotFootprint(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = SpawnShot(pSet);
            double angle = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI; // 進行方向±60度に散る
            pShot->param_d[0] = pSet->x + GetRand(10) - 5;
            pShot->param_d[1] = pSet->y + GetRand(10) - 5;
            pShot->muki = angle;
            pShot->speed = 0.3 + GetRand(20) / 100.0; // 0.3〜0.5
            pShot->kind = img_enemyShotSmallBall[7]; // 黒(足跡)
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x = p->param_d[0] + p->speed * cos(p->muki) * p->count;
        p->y = p->param_d[1] + p->speed * sin(p->muki) * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕: 警告の壁 (フェーズ1で片側に張る、退避を促す誘導弾)
// ============================================================
static void ShotWarningWall(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        sEnemyShot* pShot = SpawnShot(pSet);
        pShot->param_d[0] = pSet->x;
        pShot->param_d[1] = pSet->y;
        pShot->muki = DX_PI / 2.0; // 下方向
        pShot->speed = 0.6;
        pShot->kind = img_enemyShotBullet[8]; // 橙(警告色)
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x = p->param_d[0];
        p->y = p->param_d[1] + p->speed * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕: 斬撃弧 (フェーズ2の三段斬り本体)
//  pSet->muki       : 弧の中心角
//  pSet->param_d[0] : 半開き角(rad)
//  pSet->param_d[1] : 安全回廊の中心角オフセット(rad)
//  pSet->param_d[2] : 安全回廊の幅(rad)
//  pSet->param_d[3] : 拡散速度
// ============================================================
static void ShotSlashArc(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int kBulletNum = 44;
        double centerAngle = pSet->muki;
        double halfSpan = pSet->param_d[0];
        double gapAngle = pSet->param_d[1];
        double gapWidth = pSet->param_d[2];
        double speed = pSet->param_d[3];

        for (int i = 0; i < kBulletNum; i++) {
            double a = centerAngle - halfSpan + (2.0 * halfSpan) * i / (kBulletNum - 1);

            // 安全回廊(centerAngle+gapAngle ± gapWidth/2)は間引く
            double diff = fabs(a - (centerAngle + gapAngle));
            while (diff > DX_PI) diff = fabs(diff - 2.0 * DX_PI);
            if (diff < gapWidth / 2.0) continue;

            sEnemyShot* pShot = SpawnShot(pSet);
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->muki = a;
            pShot->speed = speed;
            pShot->kind = img_enemyShotLaser[6]; // 白刃
            pShot->margin = 100;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double r = p->speed * p->count;
        p->x = p->param_d[0] + r * cos(p->muki);
        p->y = p->param_d[1] + r * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  弾幕: 刃の煌き (フェーズ3、ダッシュ中に残る軌跡)
// ============================================================
static void ShotWakeTrail(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pShot = SpawnShot(pSet);
            double angle = GetRand(359) / 180.0 * DX_PI;
            pShot->param_d[0] = pSet->x + GetRand(16) - 8;
            pShot->param_d[1] = pSet->y + GetRand(16) - 8;
            pShot->muki = angle;
            pShot->speed = 0.5 + GetRand(30) / 100.0;
            pShot->kind = img_enemyShotScale[6]; // 白(刃の煌き)
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x = p->param_d[0] + p->speed * cos(p->muki) * p->count;
        p->y = p->param_d[1] + p->speed * sin(p->muki) * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕: 自機狙いの扇 (フェーズ3の連続突き・フェーズ4の大技に共用)
//  pSet->muki       : 中心角(自機狙い)
//  pSet->param_i[0] : 弾数
//  pSet->param_d[0] : 全体の開き角(rad)
//  pSet->param_d[1] : 速度
// ============================================================
static void ShotAimedFan(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        int n = pSet->param_i[0];
        double spread = pSet->param_d[0];
        double speed = pSet->param_d[1];
        double baseAngle = pSet->muki;

        for (int i = 0; i < n; i++) {
            double a = (n == 1) ? baseAngle
                : baseAngle - spread / 2.0 + spread * i / (n - 1);
            sEnemyShot* pShot = SpawnShot(pSet);
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->muki = a;
            pShot->speed = speed;
            pShot->kind = img_enemyShotBullet[0]; // 赤
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x = p->param_d[0] + p->speed * cos(p->muki) * p->count;
        p->y = p->param_d[1] + p->speed * sin(p->muki) * p->count;
        p = p->next;
    }
}

// ============================================================
//  弾幕: 溜めの輪 (フェーズ4前半、ボスの周囲を緩やかに公転しながら拡大)
// ============================================================
static void ShotTelegraphRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int kNum = 12;
        for (int i = 0; i < kNum; i++) {
            sEnemyShot* pShot = SpawnShot(pSet);
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->param_d[2] = 2.0 * DX_PI * i / kNum; // 初期角
            pShot->kind = img_enemyShotDiamond[0]; // 赤(緊迫感)
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double angle = p->param_d[2] + 0.05 * p->count; // ゆっくり自転
        double radius = 15.0 + 0.25 * p->count;         // 半径が徐々に拡大
        p->x = p->param_d[0] + radius * cos(angle);
        p->y = p->param_d[1] + radius * sin(angle);
        p->muki = angle;
        p = p->next;
    }
}

// ============================================================
//  弾幕: 全方位バースト (フェーズ4、残心の一撃)
// ============================================================
static void ShotOmniBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int kNum = 48;
        for (int i = 0; i < kNum; i++) {
            double a = 2.0 * DX_PI * i / kNum;
            sEnemyShot* pShot = SpawnShot(pSet);
            pShot->param_d[0] = pSet->x;
            pShot->param_d[1] = pSet->y;
            pShot->muki = a;
            pShot->speed = 1.8;
            pShot->kind = img_enemyShotMediumBall[0]; // 赤(会心の一撃)
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double r = p->speed * p->count;
        p->x = p->param_d[0] + r * cos(p->muki);
        p->y = p->param_d[1] + r * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_CloseCombat_Claude()
{
    // ループ内のローカルフレーム (1〜kCycleLen で無限ループ)
    int local = (count - 1 - 30) % kCycleLen + 1;

    static double s_approachStartX, s_approachStartY;
    static double s_approachTargetX, s_approachTargetY;
    static int    s_warnSide; // -1:左側 / +1:右側
    static double s_dashStartX, s_dashStartY;
    static double s_dashTargetX, s_dashTargetY;
    static double s_retreatStartX, s_retreatStartY;

    if (count == 1) {
        enemy.x = kBossSpawnX;
        enemy.y = kBossSpawnY;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        s_approachStartX = enemy.x;
        s_approachStartY = enemy.y;
        s_approachTargetX = enemy.x;
        s_approachTargetY = enemy.y;
        s_warnSide = -1;
        s_dashStartX = enemy.x;
        s_dashStartY = enemy.y;
        s_dashTargetX = enemy.x;
        s_dashTargetY = enemy.y;
        s_retreatStartX = enemy.x;
        s_retreatStartY = enemy.y;
    }

    // ------------------------------------------------------
    // フェーズ1: 間合い (踏み込み予備動作)  local: 1〜150
    // ------------------------------------------------------
    if (local <= kPhase1Len) {
        int p1 = local;

        if (p1 == 1) {
            s_approachStartX = enemy.x;
            s_approachStartY = enemy.y;
            s_approachTargetX = player.x;
            if (s_approachTargetX < kBossMinX) s_approachTargetX = kBossMinX;
            if (s_approachTargetX > kBossMaxX) s_approachTargetX = kBossMaxX;
            s_approachTargetY = kEngageY;
            s_warnSide = (GetRand(1) == 0) ? -1 : 1;
        }

        double t = (double)(p1 - 1) / (double)(kPhase1Len - 1);
        double e = EaseSmooth(t);
        enemy.x = s_approachStartX + (s_approachTargetX - s_approachStartX) * e;
        enemy.y = s_approachStartY + (s_approachTargetY - s_approachStartY) * e;

        // 足跡: 接近が進むほど間隔を短縮し密度を上げる
        int footInterval = 16 - (int)(10.0 * t); // 16→6フレームへ短縮
        if (footInterval < 6) footInterval = 6;
        if (p1 % footInterval == 1) {
            SpawnShotSet(ShotFootprint, enemy.x, enemy.y);
        }

        // 警告の壁: 選ばれた側に一定間隔で降下弾を追加
        if (p1 % 12 == 1) {
            double wallX = (s_warnSide < 0) ? kBossMinX - 20.0 : kBossMaxX + 20.0;
            SpawnShotSet(ShotWarningWall, wallX, -10.0);
        }
    }
    // ------------------------------------------------------
    // フェーズ2: 抜刀 (三段斬り)  local: 151〜240
    // ------------------------------------------------------
    else if (local <= kPhase1Len + kPhase2Len) {
        int p2 = local - kPhase1Len;
        enemy.x = s_approachTargetX;
        enemy.y = s_approachTargetY;

        if (p2 == 1 || p2 == 31 || p2 == 61) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 三段斬り: 一閃(左斜め)→二閃(右斜め)→三閃(正面) を時間差で発射
        int slashTiming = 0;
        double slashOffset = 0.0;
        if (p2 == 6) { slashTiming = 1; slashOffset = -25.0 / 180.0 * DX_PI; }
        if (p2 == 36) { slashTiming = 1; slashOffset = 25.0 / 180.0 * DX_PI; }
        if (p2 == 66) { slashTiming = 1; slashOffset = 0.0; }

        if (slashTiming) {
            double aimAngle = atan2(player.y - enemy.y, player.x - enemy.x);
            sEnemyShotSet* pSet = SpawnShotSet(ShotSlashArc, enemy.x, enemy.y);
            pSet->muki = aimAngle + slashOffset;
            pSet->param_d[0] = 110.0 / 180.0 * DX_PI;        // 半開き角(合計220度)
            pSet->param_d[1] = (GetRand(200) - 100) / 180.0 * DX_PI; // 安全回廊オフセット
            pSet->param_d[2] = 28.0 / 180.0 * DX_PI;         // 安全回廊幅
            pSet->param_d[3] = 2.6;                          // 拡散速度
        }
    }
    // ------------------------------------------------------
    // フェーズ3: 乱撃 (3連ジグザグ踏み込み)  local: 241〜420
    // ------------------------------------------------------
    else if (local <= kPhase1Len + kPhase2Len + kPhase3Len) {
        int p3 = local - (kPhase1Len + kPhase2Len);
        int dashIndex = (p3 - 1) / 60;     // 0, 1, 2
        int sub = (p3 - 1) % 60 + 1;       // 1〜60

        if (sub == 1) {
            s_dashStartX = enemy.x;
            s_dashStartY = enemy.y;
            double offset = (dashIndex == 0) ? -90.0 : (dashIndex == 1) ? 90.0 : 0.0;
            s_dashTargetX = player.x + offset;
            if (s_dashTargetX < kBossMinX) s_dashTargetX = kBossMinX;
            if (s_dashTargetX > kBossMaxX) s_dashTargetX = kBossMaxX;
            s_dashTargetY = kEngageY - 20.0 + dashIndex * 10.0; // 徐々に収束
        }

        if (sub <= 20) {
            double t = (double)(sub - 1) / 19.0;
            double e = 1.0 - (1.0 - t) * (1.0 - t); // イーズアウト(急停止)
            enemy.x = s_dashStartX + (s_dashTargetX - s_dashStartX) * e;
            enemy.y = s_dashStartY + (s_dashTargetY - s_dashStartY) * e;

            if (sub % 2 == 1) {
                SpawnShotSet(ShotWakeTrail, enemy.x, enemy.y);
            }
        }
        else {
            enemy.x = s_dashTargetX;
            enemy.y = s_dashTargetY;
        }

        // 着地後、数フレーム間隔で自機狙いの扇を連続発射(単発で終わらせない)
        if (sub == 20 || sub == 28 || sub == 36 || sub == 44 || sub == 52) {
            int n = 3 + dashIndex * 2; // 3way→5way→7wayと段階的に強化
            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            sEnemyShotSet* pSet = SpawnShotSet(ShotAimedFan, enemy.x, enemy.y);
            pSet->muki = aim;
            pSet->param_i[0] = n;
            pSet->param_d[0] = 45.0 / 180.0 * DX_PI;
            pSet->param_d[1] = 2.4;
        }
    }
    // ------------------------------------------------------
    // フェーズ4: 残心・納刀  local: 421〜540
    // ------------------------------------------------------
    else {
        int p4 = local - (kPhase1Len + kPhase2Len + kPhase3Len);

        if (p4 <= 40) {
            // 溜め: 最後のダッシュ着地位置で静止し、輪が緩やかに拡大する
            enemy.x = s_dashTargetX;
            enemy.y = s_dashTargetY;
            if (p4 % 8 == 1) {
                SpawnShotSet(ShotTelegraphRing, enemy.x, enemy.y);
            }
        }
        else if (p4 == 41) {
            // 解放: 全方位バースト + 自機狙い5way を同時発射
            enemy.x = s_dashTargetX;
            enemy.y = s_dashTargetY;

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            SpawnShotSet(ShotOmniBurst, enemy.x, enemy.y);

            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            sEnemyShotSet* pSet = SpawnShotSet(ShotAimedFan, enemy.x, enemy.y);
            pSet->muki = aim;
            pSet->param_i[0] = 5;
            pSet->param_d[0] = 40.0 / 180.0 * DX_PI;
            pSet->param_d[1] = 3.2;

            s_retreatStartX = enemy.x;
            s_retreatStartY = enemy.y;
        }
        else {
            // 納刀: 開始地点まで後退しつつ足跡を残し、フェーズ1へ繋げる
            int sub = p4 - 41; // 1〜79
            double t = (double)sub / 79.0;
            double e = EaseSmooth(t);
            enemy.x = s_retreatStartX + (kBossSpawnX - s_retreatStartX) * e;
            enemy.y = s_retreatStartY + (kBossSpawnY - s_retreatStartY) * e;

            if (sub % 10 == 1) {
                SpawnShotSet(ShotFootprint, enemy.x, enemy.y);
            }
        }
    }
}