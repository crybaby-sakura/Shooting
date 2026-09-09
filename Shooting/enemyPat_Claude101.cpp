// enemyPat_JuusouMisshuuJin.cpp
// 「重装密集陣」— ファランクス(重装歩兵の密集陣形)をモチーフにした4フェーズ弾幕
// 専用素材は使わず、既存の中楕円弾/大玉/短レーザーの組み合わせで
// 「盾の壁」と「槍衾」を表現する。
//
// フェーズ構成(1サイクル = PHX_CYCLE_LEN フレームで無限ループ):
//   1. 陣形構築  : 盾弾(中楕円+大玉)と槍弾(短レーザー)が左右から集結し矩形グリッドを組む
//   2. 槍衾前進  : 陣形全体が左右に揺れながら自機側へじわじわ前進、槍の穂先から周期的に自機狙い3way
//   3. 分陣包囲  : 陣形が左右2隊に分裂し、自機を挟むように弧を描いて回り込む。自機狙いは3way→5wayへ強化
//   4. 総突撃    : 赤白点滅で予告後、全弾が分陣位置から自機方向へ加速して刺突。中央から自機狙い5way+リングも追加

// ============================================================
//  定数
// ============================================================
static const int    PHX_ROWS = 4;      // 盾の段数(0=最後尾 … PHX_ROWS-1=最前列)
static const int    PHX_COLS = 9;      // 列数
static const double PHX_SPACING_X = 32.0;
static const double PHX_SPACING_Y = 24.0;

static const int PHX_BUILD_LEN = 90;   // フェーズ1: 陣形構築
static const int PHX_ADVANCE_LEN = 180; // フェーズ2: 槍衾前進
static const int PHX_SPLIT_LEN = 180;   // フェーズ3: 分陣包囲
static const int PHX_CHARGE_TELEGRAPH = 30; // フェーズ4前半: 赤白点滅予告
static const int PHX_CHARGE_THRUST = 50;    // フェーズ4後半: 刺突

static const int PHX_B1 = PHX_BUILD_LEN;                 // 90   構築→前進
static const int PHX_B2 = PHX_B1 + PHX_ADVANCE_LEN;       // 270  前進→分陣
static const int PHX_B3 = PHX_B2 + PHX_SPLIT_LEN;         // 450  分陣→総突撃
static const int PHX_CHARGE_LEN = PHX_CHARGE_TELEGRAPH + PHX_CHARGE_THRUST; // 80
static const int PHX_CYCLE_LEN = PHX_B3 + PHX_CHARGE_LEN; // 530  1サイクルの全長

// ============================================================
//  補助関数
// ============================================================
static double Clamp01(double t)
{
    if (t < 0.0) return 0.0;
    if (t > 1.0) return 1.0;
    return t;
}

static double EaseOutCubic(double t)
{
    double u = 1.0 - t;
    return 1.0 - u * u * u;
}

// 列cの、陣形中心から見た相対Xオフセット
static double PhxRelX(int col)
{
    return (col - (PHX_COLS - 1) / 2.0) * PHX_SPACING_X;
}

// 段rowの、陣形中心から見た相対Yオフセット(row==PHX_ROWSは槍の定位置＝最前列のさらに前)
static double PhxEffRelY(int row)
{
    if (row >= PHX_ROWS) return (PHX_ROWS - 1) * PHX_SPACING_Y + 14.0;
    return row * PHX_SPACING_Y;
}

// フェーズ1〜3(localCount < PHX_B3)における陣形内(row,col)の弾の位置を計算する。
// フェーズ4(総突撃)の挙動はShotPhalanxFormation内で個別に(凍結位置を使って)計算するため対象外。
static void PhxComputePos(int localCount, int row, int col, double* outX, double* outY)
{
    if (localCount < PHX_B1) {
        // ---- フェーズ1: 陣形構築(左右から集結) ----
        double t = Clamp01(localCount / (double)PHX_B1);
        double ease = EaseOutCubic(t);

        double targetX = 240.0 + PhxRelX(col);
        double targetY = 100.0 + PhxEffRelY(row);

        double startX = (col < PHX_COLS / 2) ? -60.0 : 540.0; // 左半分は左端から、右半分は右端から
        double startY = 60.0; // 敵本体付近から降りてくる

        *outX = startX + (targetX - startX) * ease;
        *outY = startY + (targetY - startY) * ease;
    }
    else if (localCount < PHX_B2) {
        // ---- フェーズ2: 槍衾前進(陣形は剛体のまま左右に揺れつつ前進) ----
        double lt = (double)(localCount - PHX_B1);
        double fcx = 240.0 + 40.0 * sin(lt / 50.0);
        double fcy = 100.0 + lt * 0.35;

        double y = PhxEffRelY(row);
        if (row >= PHX_ROWS) {
            y += 10.0 * sin(lt / 12.0); // 槍の穂先だけ突き出しを繰り返す(パルス)
        }
        *outX = fcx + PhxRelX(col);
        *outY = fcy + y;
    }
    else {
        // ---- フェーズ3: 分陣包囲(左右2隊に分かれ自機を挟み込む) ----
        double localT = (double)(localCount - PHX_B2);
        double ease = EaseOutCubic(Clamp01(localT / (double)PHX_SPLIT_LEN));

        double startCx = 240.0 + 40.0 * sin((PHX_ADVANCE_LEN - 1) / 50.0);
        double startCy = 100.0 + (PHX_ADVANCE_LEN - 1) * 0.35;

        bool isLeft = (col < PHX_COLS / 2);
        double targetCx = isLeft ? (player.x - 110.0) : (player.x + 110.0);
        double targetCy = player.y - 60.0;

        double wingCx = startCx + (targetCx - startCx) * ease;
        double wingCy = startCy + (targetCy - startCy) * ease - 40.0 * sin(ease * DX_PI); // 弧を描くように旋回

        double y = PhxEffRelY(row);
        if (row >= PHX_ROWS) {
            y += (10.0 + localT * 0.05) * sin(localT / 10.0); // 包囲が進むほど穂先の突き出しが激しくなる
        }
        *outX = wingCx + PhxRelX(col);
        *outY = wingCy + y;
    }
}

// ============================================================
//  弾幕：陣形本体(盾+槍) — 45発(9列 x (4段の盾+1段の槍))
// ============================================================
static void ShotPhalanxFormation(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int row = 0; row <= PHX_ROWS; row++) {      // 0..PHX_ROWS-1:盾, PHX_ROWS:槍
            for (int col = 0; col < PHX_COLS; col++) {
                pEnemyShot = new sEnemyShot;

                pEnemyShot->param_i[0] = row;
                pEnemyShot->param_i[1] = col;
                pEnemyShot->param_i[2] = 0; // 総突撃フェーズでの凍結位置キャプチャ済みフラグ

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = DX_PI / 2.0;

                // 弾の種類と半径一覧: 中楕円弾(10.5x7.0)、大玉(20.0x20.0)、短レーザー(64.0x4.0)
                // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
                if (row < PHX_ROWS - 1) {
                    pEnemyShot->kind = img_enemyShotMediumOval[4]; // 後方の隊列(青)
                }
                else if (row == PHX_ROWS - 1) {
                    pEnemyShot->kind = img_enemyShotLargeBall[6];  // 最前列の大盾(白)
                }
                else {
                    pEnemyShot->kind = img_enemyShotLaser[0];      // 槍の穂先(赤)
                }
                pEnemyShot->margin = 480;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int row = pShot->param_i[0];
        int col = pShot->param_i[1];
        int lc = pShot->count;

        if (lc < PHX_B3) {
            // ---- 構築 / 前進 / 分陣(共通の位置計算式を使用) ----
            double x, y;
            PhxComputePos(lc, row, col, &x, &y);
            pShot->x = x;
            pShot->y = y;

            if (row >= PHX_ROWS) {
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x); // 穂先は常に自機を狙う
            }
            else {
                pShot->muki = DX_PI / 2.0; // 盾は前方(下方向)を向いたままでよい
            }
        }
        else {
            // ---- フェーズ4: 総突撃 ----
            int lt = lc - PHX_B3;

            if (pShot->param_i[2] == 0) {
                // 分陣終了時点の位置を一度だけ凍結キャプチャする
                double fx, fy;
                PhxComputePos(PHX_B3 - 1, row, col, &fx, &fy);
                pShot->param_d[0] = fx;
                pShot->param_d[1] = fy;
                pShot->param_d[2] = atan2(player.y - fy, player.x - fx);
                pShot->param_i[2] = 1;
            }

            double fx = pShot->param_d[0];
            double fy = pShot->param_d[1];
            double fmuki = pShot->param_d[2];

            if (lt < PHX_CHARGE_TELEGRAPH) {
                // 赤白点滅の予告(位置は凍結したまま静止)
                pShot->x = fx;
                pShot->y = fy;
                pShot->muki = fmuki;

                bool blinkOn = ((lt / 5) % 2 == 0);
                if (row >= PHX_ROWS) {
                    pShot->kind = blinkOn ? img_enemyShotLaser[0] : img_enemyShotLaser[6];
                }
                else if (row == PHX_ROWS - 1) {
                    pShot->kind = blinkOn ? img_enemyShotLargeBall[6] : img_enemyShotLargeBall[0];
                }
                else {
                    pShot->kind = blinkOn ? img_enemyShotMediumOval[4] : img_enemyShotMediumOval[8];
                }
            }
            else {
                // 刺突: 凍結位置から自機方向へ加速しながら直進(count駆動、速度積分はしない)
                double tt = (double)(lt - PHX_CHARGE_TELEGRAPH);
                double dist = 0.06 * tt * tt + 2.0 * tt;
                pShot->x = fx + dist * cos(fmuki);
                pShot->y = fy + dist * sin(fmuki);
                pShot->muki = fmuki;

                if (row >= PHX_ROWS) {
                    pShot->kind = img_enemyShotLaser[0];
                }
                else if (row == PHX_ROWS - 1) {
                    pShot->kind = img_enemyShotLargeBall[6];
                }
                else {
                    pShot->kind = img_enemyShotMediumOval[4];
                }
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：自機狙い扇状N-way(槍の穂先などから発射する汎用パターン)
// ============================================================
static void ShotAimedFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        int ways = pEnemyShotSet->param_i[0];
        double spread = pEnemyShotSet->param_d[0];
        double speed = pEnemyShotSet->param_d[1];

        for (int i = 0; i < ways; i++) {
            pEnemyShot = new sEnemyShot;

            double offset = (ways == 1) ? 0.0 : (-spread / 2.0 + spread * i / (double)(ways - 1));
            pEnemyShot->muki = pEnemyShotSet->muki + offset;
            pEnemyShot->speed = speed;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = pEnemyShotSet->kind;
            pEnemyShot->param_d[0] = pEnemyShot->x; // 発射原点(位置は式駆動で毎フレーム再計算)
            pEnemyShot->param_d[1] = pEnemyShot->y;
            pEnemyShot->margin = 480;

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
//  弾幕：全方位リングバースト(総突撃フェーズの追加演出用)
// ============================================================
static void ShotRadialRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        int n = pEnemyShotSet->param_i[0];
        double speed = pEnemyShotSet->param_d[1];

        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->muki = 2.0 * DX_PI * i / (double)n;
            pEnemyShot->speed = speed;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->kind = pEnemyShotSet->kind;
            pEnemyShot->param_d[0] = pEnemyShot->x;
            pEnemyShot->param_d[1] = pEnemyShot->y;
            pEnemyShot->margin = 480;

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
//  ShotSet生成ヘルパー
// ============================================================
static void SpawnAimedFan(double x, double y, int ways, double spreadRad, double speed, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotAimedFan;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = atan2(player.y - y, player.x - x);
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->param_i[0] = ways;
    pEnemyShotSet->param_d[0] = spreadRad;
    pEnemyShotSet->param_d[1] = speed;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

static void SpawnRadialRing(double x, double y, int n, double speed, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotRadialRing;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->param_i[0] = n;
    pEnemyShotSet->param_d[1] = speed;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  敵本体のパターン:重装密集陣(ファランクス)
// ============================================================
void EnemyPat_Phalanx_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
    }
    enemy.x = 240.0 + 10.0 * sin(count / 90.0); // ボス本体はゆるく左右に揺れるだけ

    int cf = (count - 1) % PHX_CYCLE_LEN; // 現サイクル内の経過フレーム(0始まり)

    // --- サイクル開始:陣形(盾+槍)45発をまとめて生成 ---
    if (cf == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPhalanxFormation;
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

    // --- フェーズ2(槍衾前進):9本の穂先すべてから30フレームごとに自機狙い3way ---
    if (cf >= PHX_B1 && cf < PHX_B2 && (cf - PHX_B1) % 30 == 0) {
        for (int col = 0; col < PHX_COLS; col++) {
            double tipX, tipY;
            PhxComputePos(cf, PHX_ROWS, col, &tipX, &tipY);
            SpawnAimedFan(tipX, tipY, 3, 40.0 * DX_PI / 180.0, 2.4, img_enemyShotBullet[0]);
        }
    }

    // --- フェーズ3(分陣包囲):穂先から自機狙い、包囲が進むほど3way→5way・間隔短縮で密度上昇 ---
    if (cf >= PHX_B2 && cf < PHX_B3) {
        int lt = cf - PHX_B2;
        bool secondHalf = (lt >= PHX_SPLIT_LEN / 2);
        int interval = secondHalf ? 18+5 : 26+5;
        int ways = secondHalf ? 5 : 3;

        if (lt % interval == 0) {
            for (int col = 0; col < PHX_COLS; col++) {
                double tipX, tipY;
                PhxComputePos(cf, PHX_ROWS, col, &tipX, &tipY);
                SpawnAimedFan(tipX, tipY, ways, 50.0 * DX_PI / 180.0, 2.6, img_enemyShotBullet[0]);
            }
        }
    }

    // --- フェーズ4予告開始の合図(詠唱音) ---
    if (cf == PHX_B3) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- フェーズ4(総突撃)トリガー瞬間:中央から自機狙い5way+全方位リングを追加発射 ---
    if (cf == PHX_B3 + PHX_CHARGE_TELEGRAPH) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        SpawnAimedFan(enemy.x, enemy.y, 5, 60.0 * DX_PI / 180.0, 3.0, img_enemyShotDiamond[0]);
        SpawnRadialRing(enemy.x, enemy.y, 24, 2.2, img_enemyShotSmallBall[6]);
    }
}