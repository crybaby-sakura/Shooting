// enemyPat_YozoraCarpet.cpp
// 6ボス 天川星羅　流符「夜空のカーペット」
//
// ※ img_enemyShot*, sound_enemyShot_*, DxLib関数(GetRand/DX_PI/PlaySoundMem等)は
//   enemyPat_sampleForAI.cpp と同様、既存の共通ヘッダ経由で利用可能という前提で
//   include文を最小限にしてあります。

// 色定数（0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
enum {
    COL_RED = 0, COL_YELLOW, COL_GREEN, COL_CYAN, COL_BLUE,
    COL_MAGENTA, COL_WHITE, COL_BLACK, COL_ORANGE
};

// ============================================================
// 素材選定メモ
// ------------------------------------------------------------
// ・菱形弾(4.5x2.5) … きらめく星の形に近いため「星」役として全面採用
// ・小玉/中楕円弾    … 星紋（同心円の波紋）のアクセントに使用
// ・短レーザー        … 流れ星の光跡表現に使用（禁止されていないため採用）
// ・色は シアン/青/白/マゼンタ を基調とし、夜空らしい寒色系でまとめた
// ============================================================


// ============================================================
// 弾幕①：降星弾
// 画面上方から絶え間なく星弾が降り注ぎ、夜空のカーペットが
// 少しずつ広がっていくイメージのベース密度パターン。
// ============================================================
static void ShotStarfall(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM = 5; // 1波あたりの星弾数

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < NUM; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double x0 = pEnemyShotSet->x + (GetRand(440) - 220);
            double y0 = pEnemyShotSet->y;
            double muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI; // 下向き±30度
            double speed = 1.2 + GetRand(80) / 100.0; // 1.2〜2.0

            // formula-driven: 初期位置と速度成分・揺れパラメータを保存
            pEnemyShot->param_d[0] = x0;
            pEnemyShot->param_d[1] = y0;
            pEnemyShot->param_d[2] = speed * cos(muki); // vx
            pEnemyShot->param_d[3] = speed * sin(muki); // vy
            pEnemyShot->param_d[4] = 0.6 + GetRand(60) / 100.0;    // 揺れ振幅
            pEnemyShot->param_d[5] = 0.8 + GetRand(120) / 100.0;   // 揺れ周波数係数
            pEnemyShot->param_d[6] = GetRand(628) / 100.0;         // 揺れ位相(0〜2π)

            pEnemyShot->x = x0;
            pEnemyShot->y = y0;
            pEnemyShot->muki = muki;

            // 白とシアンを基調に、時々マゼンタを混ぜて瞬きを演出
            int colorRoll = GetRand(9);
            int color = (colorRoll < 4) ? COL_WHITE : (colorRoll < 8) ? COL_CYAN : COL_MAGENTA;
            pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        double vx = pShot->param_d[2];
        double vy = pShot->param_d[3];
        double amp = pShot->param_d[4];
        double freq = pShot->param_d[5] * 0.05;
        double phase = pShot->param_d[6];

        // 直進しつつ左右にゆらゆら揺れる（きらめきながら落ちる星）
        double wobble = amp * sin(freq * t + phase);
        pShot->x = x0 + vx * t + wobble;
        pShot->y = y0 + vy * t;

        // 揺れを含めた実際の進行方向を muki に反映
        double dx = vx + amp * freq * cos(freq * t + phase);
        double dy = vy;
        pShot->muki = atan2(dy, dx);

        pShot = pShot->next;
    }
}


// ============================================================
// 弾幕②：星紋弾
// 同心円状の弾がゆっくり回転しながら広がり、夜空に波紋のような
// 星座模様を描く。呼び出し側は pEnemyShotSet->muki に初期回転
// オフセット、pEnemyShotSet->kind に色バリエーションを積んで渡す。
// ============================================================
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM = 36;        // 1輪あたりの弾数
    const double SPEED = 1.05; // 半径方向の広がる速さ
    const double SPIN = 0.006; // 回転角速度（ラジアン/フレーム）

    if (pEnemyShotSet->count == 0) {
        double baseAngle = pEnemyShotSet->muki;
        int color = (pEnemyShotSet->kind % 2 == 0) ? COL_CYAN : COL_BLUE;

        for (int i = 0; i < NUM; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double theta0 = baseAngle + 2.0 * DX_PI * i / NUM;

            pEnemyShot->param_d[0] = pEnemyShotSet->x; // 中心x
            pEnemyShot->param_d[1] = pEnemyShotSet->y; // 中心y
            pEnemyShot->param_d[2] = theta0;            // 初期角度

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = theta0;

            // 6発に1発だけ中楕円弾を混ぜて星座の主星のように目立たせる
            pEnemyShot->kind = (i % 6 == 0) ? img_enemyShotMediumOval[COL_WHITE]
                : img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double cx = pShot->param_d[0];
        double cy = pShot->param_d[1];
        double theta0 = pShot->param_d[2];

        double r = SPEED * t;
        double theta = theta0 + SPIN * t;

        pShot->x = cx + r * cos(theta);
        pShot->y = cy + r * sin(theta);

        // 螺旋の接線方向（進行方向）を muki に反映
        double dx = SPEED * cos(theta) - r * SPIN * sin(theta);
        double dy = SPEED * sin(theta) + r * SPIN * cos(theta);
        pShot->muki = atan2(dy, dx);

        pShot = pShot->next;
    }
}


// ============================================================
// 弾幕③：織弾
// 画面端で跳ね返りながら斜めに走る弾が、絨毯を織るように画面
// 全体を埋めていく。位置は三角波の式で純粋に count から算出する
// ため、反射時にも誤差が蓄積しない。一定時間で自ら消える。
// ============================================================
static void ShotBounceWeave(sEnemyShotSet* pEnemyShotSet)
{
    const int NUM = 3;            // 1回に放つ弾数
    const double LIFETIME = 260;  // 生存フレーム数
    const double SCREEN_W = 480.0;
    const double SCREEN_H = 480.0;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < NUM; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double x0 = pEnemyShotSet->x;
            double y0 = pEnemyShotSet->y;
            double muki = pEnemyShotSet->muki + (GetRand(200) - 100) / 100.0; // 若干のばらつき
            double speed = 1.6 + GetRand(60) / 100.0;

            pEnemyShot->param_d[0] = x0;
            pEnemyShot->param_d[1] = y0;
            pEnemyShot->param_d[2] = speed * cos(muki); // vx
            pEnemyShot->param_d[3] = speed * sin(muki); // vy

            pEnemyShot->x = x0;
            pEnemyShot->y = y0;
            pEnemyShot->muki = muki;

            int color = (i % 2 == 0) ? COL_WHITE : COL_MAGENTA;
            pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 寿命が尽きた弾は自前でリストから外して削除する
        // （画面内で跳ね返り続けるため、メインルーチンの画面外判定では消えない）
        if (pShot->count > LIFETIME) {
            sEnemyShot* toDelete = pShot;
            pShot = pShot->next;
            toDelete->prev->next = toDelete->next;
            toDelete->next->prev = toDelete->prev;
            delete toDelete;
            continue;
        }

        double t = (double)pShot->count;
        double x0 = pShot->param_d[0];
        double y0 = pShot->param_d[1];
        double vx = pShot->param_d[2];
        double vy = pShot->param_d[3];

        // 三角波による反射：端に達すると折り返す位置を式だけで計算する
        auto reflect = [](double origin, double v, double tt, double size) {
            double raw = origin + v * tt;
            double period = 2.0 * size;
            double m = fmod(raw, period);
            if (m < 0) m += period;
            return (m <= size) ? m : (period - m);
        };

        pShot->x = reflect(x0, vx, t, SCREEN_W);
        pShot->y = reflect(y0, vy, t, SCREEN_H);

        // 反射による符号反転を考慮した実際の進行方向を muki に反映
        double periodX = 2.0 * SCREEN_W;
        double mx = fmod(x0 + vx * t, periodX); if (mx < 0) mx += periodX;
        double signX = (mx <= SCREEN_W) ? 1.0 : -1.0;

        double periodY = 2.0 * SCREEN_H;
        double my = fmod(y0 + vy * t, periodY); if (my < 0) my += periodY;
        double signY = (my <= SCREEN_H) ? 1.0 : -1.0;

        pShot->muki = atan2(vy * signY, vx * signX);

        pShot = pShot->next;
    }
}


// ============================================================
// 弾幕④：流れ星弾
// 短レーザーを使い、時折画面を斜めに切り裂く流星の光跡を演出。
// 直進のみのシンプルな formula-driven パターン。
// ============================================================
static void ShotShootingStar(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        double x0 = pEnemyShotSet->x;
        double y0 = pEnemyShotSet->y;
        double muki = pEnemyShotSet->muki;
        double speed = 6.0 + GetRand(150) / 100.0; // 流星らしい速さ

        pEnemyShot->param_d[0] = x0;
        pEnemyShot->param_d[1] = y0;
        pEnemyShot->param_d[2] = speed * cos(muki);
        pEnemyShot->param_d[3] = speed * sin(muki);

        pEnemyShot->x = x0;
        pEnemyShot->y = y0;
        pEnemyShot->muki = muki;
        pEnemyShot->kind = img_enemyShotLaser[COL_WHITE]; // 短レーザーで光跡を表現

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        pShot->x = pShot->param_d[0] + pShot->param_d[2] * t;
        pShot->y = pShot->param_d[1] + pShot->param_d[3] * t;
        // 直進のため muki は初期値のまま（進行方向と一致）
        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体：6ボス 天川星羅　流符「夜空のカーペット」
// ============================================================
void EnemyPat_NightCarpet_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200; // 仮の値。実際のバランスに合わせて調整してください

        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 上空をゆったり左右に漂う
        enemy.x = 240.0 + 130.0 * sin(count / 210.0);
        enemy.y = 90.0 + 14.0 * sin(count / 95.0);
    }

    // ① 降星弾：絶え間なく星が降り注ぐベース密度
    if (count % 18 == 1) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotStarfall;
        p->x = GetRand(480);  // 画面上方の乱数位置から発生
        p->y = -10.0;
        p->kind = 0;

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }

    // ② 星紋弾：波紋のような同心円弾を周期的に展開
    if (count % 55 == 1) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotRing;
        p->x = enemy.x;
        p->y = enemy.y;
        p->muki = (count / 55) * (DX_PI / 6.0); // 輪ごとに開始角をずらして重なりを回避
        p->kind = (count / 55) % 2;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }

    // ③ 織弾：画面を斜めに跳ね回る弾を絶え間なく追加し、絨毯の織り目を形成
    if (count % 30 == 1) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotBounceWeave;
        p->x = enemy.x;
        p->y = enemy.y + 15.0;
        p->muki = GetRand(628) / 100.0; // 0〜2πのランダムな初期方向
        p->kind = 0;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }

    // ④ 流れ星弾：時折、画面を斜めに横切る流星の光跡
    if (count % 100 == 1) {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotShootingStar;

        // 画面左右どちらかの外側から、斜め下方向へ
        bool fromLeft = (GetRand(1) == 0);
        p->x = fromLeft ? -20.0 : 500.0;
        p->y = GetRand(200);
        p->muki = fromLeft
            ? (DX_PI / 6.0 + GetRand(20) / 100.0)
            : (DX_PI - DX_PI / 6.0 - GetRand(20) / 100.0);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}