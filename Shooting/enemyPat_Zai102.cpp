// enemyPat_popupHell.cpp
// 弾幕：閉じろ!閉じろ!ポップアップ地獄
//
// 【概要】白弾で矩形の「ポップアップウィンドウ」を構成し、画面下部から湧かせて上昇させる。
//        右上の赤弾2個が「閉じるボタン(×)」で、自機弾を当てると窓が弾け飛んで消滅する。
//        放置すると老化して枠が剥がれ、最終的に全体が暴発する。
// 【フェーズ】
//   通常  : 閉じれば無害。放置で暴発。
//   バグ窓: 総閉鎖数8超で発生。閉じると大玉で撃ち返し、小型窓2つに分裂。
//   巨大窓: 総閉鎖数20で発生。周回×(耐久6)を4つ全部破壊して閉鎖。放置で全弾放出。
//
// 【使用素材】
//   小玉: 白(枠/タイトル) 赤(×) 黄/橙(広告文) シアン(タイトル文字)
//   中玉: 白(巨大窓枠) 赤(周回×) 黄/橙(巨大窓の広告文)
//   大玉: 黄(当選マーク) 赤(撃り返し)
//   SE  : 出現=light / 閉じた=medium / 暴発=heavy / 撃り返し・巨大出現=extreme / 巨大予告=charge

// ---- 弾の役割 (sEnemyShot::param_i[0]) ----
enum {
    ROLE_FRAME = 0,   // 窓の枠
    ROLE_TITLE = 1,   // タイトルバー
    ROLE_TEXT = 2,   // 広告文(飾り)
    ROLE_X = 3,   // 閉じるボタン(当たると即閉じ)
    ROLE_ORBIT = 4,   // 周回×ボタン(耐久あり。param_i[1]=被弾数, [2]=必要ヒット数)
    ROLE_FREE = 10,  // 自由飛行弾(窓から分離後)
};

// ---- ウィンドウの種類 (sEnemyShotSet::param_i[0]) ----
enum {
    PHASE_NORMAL = 0,
    PHASE_BUGGY = 1,
    PHASE_GIANT = 2,
};

static const int    MAX_WINDOWS = 10;    // 同時出現上限(弾プール保護)
static const double FIELD_W = 480.0;     // ゲーム画面 480x480

// ファイル内共有カウンタ
static int g_activeWindows = 0;          // 出現中の窓の数
static int g_totalClosed = 0;          // 閉じた総数(フェーズ進行用)
static int g_giantActive = 0;          // 巨大窓出現中
static int g_giantDone = 0;          // 巨大窓終了済み

// 効果音を頭出し再生
static void PlaySE(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 矩形の周上の座標 (t:0.0~1.0, 左上起点・時計回り)
static void PerimeterPos(double cx, double cy, double rw, double rh, double t, double* x, double* y)
{
    double w = 2.0 * rw, h = 2.0 * rh;
    double dist = (t - floor(t)) * 2.0 * (w + h);
    if (dist < w) {
        *x = cx - rw + dist;           *y = cy - rh;
    }
    else if (dist < w + h) {
        *x = cx + rw;                  *y = cy - rh + (dist - w);
    }
    else if (dist < 2.0 * w + h) {
        *x = cx + rw - (dist - w - h); *y = cy + rh;
    }
    else {
        *x = cx - rw;                  *y = cy + rh - (dist - 2.0 * w - h);
    }
}

// 窓に所属する弾を1個追加
static sEnemyShot* AddWindowShot(sEnemyShotSet* set, double x, double y, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->speed = 0.0;
    p->muki = 0.0;
    p->kind = kind;
    p->param_i[0] = role;

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
    return p;
}

static void ShotPopupWindow(sEnemyShotSet* pEnemyShotSet);   // 前方宣言

// 窓を構成する全弾を窓の中心から外へ弾け飛ばす
static void ScatterWindowShots(sEnemyShotSet* set, double speedBase, double speedRand)
{
    double cx = set->x, cy = set->y;
    sEnemyShot* ps = set->pEnemyShotHead->next;
    while (ps != set->pEnemyShotHead) {
        if (ps->param_i[0] != ROLE_FREE) {
            double dx = ps->x - cx, dy = ps->y - cy;
            double d = sqrt(dx * dx + dy * dy);
            if (d < 1.0) { dx = 0.0; dy = -1.0; }
            ps->muki = atan2(dy, dx) + (GetRand(20) - 10) * 0.01;
            ps->speed = speedBase + GetRand((int)(speedRand * 10.0)) / 10.0;
            ps->param_i[0] = ROLE_FREE;
        }
        ps = ps->next;
    }
}

// ポップアップウィンドウ(弾幕セット)を1個湧かせる
static void SpawnPopupWindow(double cx, double cy, double rw, double rh, int phase, int life, double vy)
{
    if (g_activeWindows >= MAX_WINDOWS) return;

    // 画面内に収める
    if (cx < rw + 8.0)           cx = rw + 8.0;
    if (cx > FIELD_W - rw - 8.0) cx = FIELD_W - rw - 8.0;
    if (cy < rh + 8.0)           cy = rh + 8.0;
    if (cy > FIELD_W - rh - 8.0) cy = FIELD_W - rh - 8.0;

    sEnemyShotSet* set = new sEnemyShotSet;
    set->count = 0;
    set->patternFunc = ShotPopupWindow;
    set->x = cx;
    set->y = cy;
    set->muki = 0.0;
    set->kind = 0;
    set->param_i[0] = phase;             // 種類
    set->param_i[3] = life;              // 放置で暴発するまでのフレーム数
    set->param_d[0] = vy;                // 上昇速度
    set->param_d[1] = rw;                // 半幅
    set->param_d[2] = rh;                // 半高

    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;

    g_activeWindows++;
}

// 窓が閉じた時の処理
static void CloseWindow(sEnemyShotSet* set)
{
    set->param_i[1] = 1;                 // closed
    g_activeWindows--;
    g_totalClosed++;
    PlaySE(sound_enemyShot_medium);      // 「チッ…」と窓が消える音

    if (set->param_i[0] == PHASE_GIANT) {
        // 巨大窓を閉じきった: 派手に弾け飛ぶ
        g_giantActive = 0;
        g_giantDone = 1;
        ScatterWindowShots(set, 2.5, 1.5);
        return;
    }

    // 通常窓: ゆっくり弾け飛ぶ
    ScatterWindowShots(set, 1.0, 1.0);

    // バグった窓: 閉じると撃り返してくる(閉じる位置取りが問われる)
    if (set->param_i[0] == PHASE_BUGGY && set->param_i[4] == 0) {
        set->param_i[4] = 1;
        sEnemyShot* p = AddWindowShot(set, set->x, set->y, img_enemyShotLargeBall[0], ROLE_FREE);
        p->muki = atan2(player.y - set->y, player.x - set->x);
        p->speed = 2.6;
        PlaySE(sound_enemyShot_extreme);

        // さらに同じ場所から小型ポップアップが2つ分裂湧き出す
        if (g_activeWindows + 2 <= MAX_WINDOWS) {
            SpawnPopupWindow(set->x - 26.0, set->y, 22.0, 18.0, PHASE_NORMAL, 300, -0.25);
            SpawnPopupWindow(set->x + 26.0, set->y, 22.0, 18.0, PHASE_NORMAL, 300, -0.25);
        }
    }
}

// 窓の弾を生成(初回のみ)
static void CreateWindowShots(sEnemyShotSet* set)
{
    double cx = set->x, cy = set->y;
    double rw = set->param_d[1], rh = set->param_d[2];

    if (set->param_i[0] == PHASE_GIANT) {
        // ===== 巨大ポップアップ =====
        double space = 26.0;
        int nx = (int)(2.0 * rw / space); if (nx < 1) nx = 1;
        int ny = (int)(2.0 * rh / space); if (ny < 1) ny = 1;
        // 枠(中玉・白)
        for (int i = 0; i <= nx; i++) {
            double x = cx - rw + (2.0 * rw / nx) * i;
            AddWindowShot(set, x, cy - rh, img_enemyShotMediumBall[6], ROLE_FRAME);
            AddWindowShot(set, x, cy + rh, img_enemyShotMediumBall[6], ROLE_FRAME);
        }
        for (int j = 1; j < ny; j++) {
            double y = cy - rh + (2.0 * rh / ny) * j;
            AddWindowShot(set, cx - rw, y, img_enemyShotMediumBall[6], ROLE_FRAME);
            AddWindowShot(set, cx + rw, y, img_enemyShotMediumBall[6], ROLE_FRAME);
        }
        // タイトルバー
        for (int i = 1; i < nx; i++) {
            AddWindowShot(set, cx - rw + (2.0 * rw / nx) * i, cy - rh + 20.0, img_enemyShotMediumBall[6], ROLE_TITLE);
        }
        // 中の広告文(中玉・黄/橙をランダムに並べる)
        for (int row = 0; row < 4; row++) {
            double y = cy - rh + 70.0 + row * 60.0;
            for (int i = 0; i < 12; i++) {
                if (GetRand(3) == 0) continue;   // 抜けて文字っぽく見せる
                double x = cx - rw + 40.0 + i * ((2.0 * rw - 80.0) / 11.0);
                AddWindowShot(set, x, y, img_enemyShotMediumBall[(GetRand(2) == 0) ? 1 : 8], ROLE_TEXT);
            }
        }
        // 中央の「当選!!」マーク(大玉+小玉)
        AddWindowShot(set, cx, cy + 40.0, img_enemyShotLargeBall[1], ROLE_TEXT);
        AddWindowShot(set, cx - 34.0, cy + 40.0, img_enemyShotSmallBall[1], ROLE_TEXT);
        AddWindowShot(set, cx + 34.0, cy + 40.0, img_enemyShotSmallBall[1], ROLE_TEXT);
        // 枠の周囲を高速で逃げ回る×ボタン(中玉・赤) ×4
        for (int i = 0; i < 4; i++) {
            sEnemyShot* p = AddWindowShot(set, cx, cy - rh, img_enemyShotMediumBall[0], ROLE_ORBIT);
            p->param_d[0] = 0.25 * i;    // 周上の位置t
            p->param_d[1] = 0.0016;      // 1フレームの進行量(約2.7px/フレーム)
            p->param_i[1] = 0;           // 被弾数
            p->param_i[2] = 6;           // 破壊に必要なヒット数
        }
    }
    else {
        // ===== 通常/ミニポップアップ =====
        double space = 13.0;
        int nx = (int)(2.0 * rw / space); if (nx < 2) nx = 2;
        int ny = (int)(2.0 * rh / space); if (ny < 2) ny = 2;
        // 枠(小玉・白)
        for (int i = 0; i <= nx; i++) {
            double x = cx - rw + (2.0 * rw / nx) * i;
            AddWindowShot(set, x, cy - rh, img_enemyShotSmallBall[6], ROLE_FRAME);
            AddWindowShot(set, x, cy + rh, img_enemyShotSmallBall[6], ROLE_FRAME);
        }
        for (int j = 1; j < ny; j++) {
            double y = cy - rh + (2.0 * rh / ny) * j;
            AddWindowShot(set, cx - rw, y, img_enemyShotSmallBall[6], ROLE_FRAME);
            AddWindowShot(set, cx + rw, y, img_enemyShotSmallBall[6], ROLE_FRAME);
        }
        // タイトルバー(上辺の内側1行)
        for (int i = 1; i < nx; i++) {
            AddWindowShot(set, cx - rw + (2.0 * rw / nx) * i, cy - rh + 6.0, img_enemyShotSmallBall[6], ROLE_TITLE);
        }
        // タイトルの「文字」っぽいシアン弾
        for (int i = 2; i < nx - 2; i += 3) {
            if (GetRand(4) == 0) continue;
            AddWindowShot(set, cx - rw + (2.0 * rw / nx) * i, cy - rh + 6.0, img_enemyShotSmallBall[3], ROLE_TITLE);
        }
        // 本文(小玉・黄/橙をランダムに並べる)
        int rows = (int)((2.0 * rh - 16.0) / 11.0); if (rows < 1) rows = 1;
        for (int r = 0; r < rows; r++) {
            double y = cy - rh + 14.0 + 11.0 * r;
            for (int i = 0; i < 10; i++) {
                if (GetRand(3) == 0) continue;
                double x = cx - rw + 6.0 + i * ((2.0 * rw - 12.0) / 9.0);
                AddWindowShot(set, x, y, img_enemyShotSmallBall[(GetRand(2) == 0) ? 1 : 8], ROLE_TEXT);
            }
        }
        // 閉じるボタン「×」(右上に赤小弾2個を斜め配置)
        AddWindowShot(set, cx + rw - 10.0, cy - rh + 3.0, img_enemyShotSmallBall[0], ROLE_X);
        AddWindowShot(set, cx + rw - 3.0, cy - rh + 10.0, img_enemyShotSmallBall[0], ROLE_X);
    }
}

// ============================================================
//  弾幕本体(セットごとに毎フレーム呼ばれる)
// ============================================================
static void ShotPopupWindow(sEnemyShotSet* pEnemyShotSet)
{
    // ---- 初期化(初回呼び出し時に弾生成。カウントに依存せずフラグで管理) ----
    if (pEnemyShotSet->param_i[15] == 0) {
        pEnemyShotSet->param_i[15] = 1;
        CreateWindowShots(pEnemyShotSet);
        if (pEnemyShotSet->param_i[0] != PHASE_GIANT) {
            PlaySE(sound_enemyShot_light);     // 「ポン!」と湧く音
        }
        return;
    }

    int    phase = pEnemyShotSet->param_i[0];
    int    closed = pEnemyShotSet->param_i[1];
    int    burst = pEnemyShotSet->param_i[2];
    int    life = pEnemyShotSet->param_i[3];
    double rw = pEnemyShotSet->param_d[1];
    double rh = pEnemyShotSet->param_d[2];

    // ---- 自機弾と×ボタンの判定 ----
    if (!closed && !burst) {
        int doClose = 0;
        sPlayerShot* pps = playerShotHead.next;
        while (pps != &playerShotHead) {
            sPlayerShot* ppsNext = pps->next;
            int consumed = 0;
            sEnemyShot* ps = pEnemyShotSet->pEnemyShotHead->next;
            while (ps != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* psNext = ps->next;
                int role = ps->param_i[0];
                if (role == ROLE_X || role == ROLE_ORBIT) {
                    double r = (role == ROLE_ORBIT) ? 14.0 : 9.0;
                    double dx = pps->x - ps->x;
                    double dy = pps->y - ps->y;
                    if (dx * dx + dy * dy <= r * r) {
                        consumed = 1;
                        if (role == ROLE_X) {
                            doClose = 1;               // 通常窓は一撃で閉じる
                        }
                        else {
                            ps->param_i[1]++;          // 巨大窓の×は耐える
                            if (ps->param_i[1] >= ps->param_i[2]) {
                                ps->prev->next = ps->next;   // ×ボタン破壊
                                ps->next->prev = ps->prev;
                                delete ps;
                            }
                        }
                        break;
                    }
                }
                ps = psNext;
            }
            if (consumed) {
                // ×に当てた自機弾は消費する
                pps->prev->next = pps->next;
                pps->next->prev = pps->prev;
                delete pps;
            }
            if (doClose) break;
            pps = ppsNext;
        }

        // 巨大窓: 周回×を全部破壊したら閉鎖
        if (!doClose && phase == PHASE_GIANT) {
            int remain = 0;
            sEnemyShot* ps = pEnemyShotSet->pEnemyShotHead->next;
            while (ps != pEnemyShotSet->pEnemyShotHead) {
                if (ps->param_i[0] == ROLE_ORBIT) remain++;
                ps = ps->next;
            }
            if (remain == 0) doClose = 1;
        }

        if (doClose) {
            CloseWindow(pEnemyShotSet);
            closed = 1;
        }
    }

    // ---- 放置タイムアウト: 窓が暴発して全体が弾け飛ぶ ----
    if (!closed && !burst && pEnemyShotSet->count >= life) {
        pEnemyShotSet->param_i[2] = 1;
        burst = 1;
        g_activeWindows--;
        PlaySE(sound_enemyShot_heavy);
        ScatterWindowShots(pEnemyShotSet, (phase == PHASE_GIANT) ? 2.2 : 1.8, 1.2);
    }

    // ---- 老化: 放置気味なら枠が1個ずつ剥がれて飛んでいく ----
    if (!closed && !burst && pEnemyShotSet->count > life * 55 / 100 && pEnemyShotSet->count % 8 == 0) {
        sEnemyShot* ps = pEnemyShotSet->pEnemyShotHead->next;
        while (ps != pEnemyShotSet->pEnemyShotHead) {
            if (ps->param_i[0] == ROLE_FRAME) {
                double dx = ps->x - pEnemyShotSet->x;
                double dy = ps->y - pEnemyShotSet->y;
                double d = sqrt(dx * dx + dy * dy);
                if (d < 1.0) { dx = 0.0; dy = -1.0; }
                ps->muki = atan2(dy, dx);
                ps->speed = 1.2 + GetRand(8) / 10.0;
                ps->param_i[0] = ROLE_FREE;
                break;
            }
            ps = ps->next;
        }
    }

    // ---- 移動 ----
    double vy = pEnemyShotSet->param_d[0];
    pEnemyShotSet->y += vy;    // 窓の中心も上昇(基準位置)
    sEnemyShot* ps = pEnemyShotSet->pEnemyShotHead->next;
    while (ps != pEnemyShotSet->pEnemyShotHead) {
        int role = ps->param_i[0];
        if (role == ROLE_FREE) {
            // 分離した弾は自力で飛ぶ(画面外はメインルーチンが消去)
            ps->x += ps->speed * cos(ps->muki);
            ps->y += ps->speed * sin(ps->muki);
        }
        else if (role == ROLE_ORBIT) {
            // 巨大窓の×ボタンは枠の周囲を周回する
            ps->param_d[0] += ps->param_d[1];
            PerimeterPos(pEnemyShotSet->x, pEnemyShotSet->y, rw, rh, ps->param_d[0], &ps->x, &ps->y);
        }
        else {
            // 窓に付随する弾は窓と一緒にゆっくり上昇
            ps->y += vy;
        }
        ps = ps->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_PopUpAds_Zai()
{
    static int spawnTimer;
    static int giantPending;
    static int giantTimer;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        spawnTimer = 60;
        giantPending = 0;
        giantTimer = 0;
        g_activeWindows = 0;
        g_totalClosed = 0;
        g_giantActive = 0;
        g_giantDone = 0;
    }
    else {
        // 敵は画面上部をゆっくり往復
        enemy.x = 240.0 + 110.0 * sin(count * 0.012);
        enemy.y = 40.0;
    }

    // ---- 巨大ポップアップの予告・出現 ----
    if (!g_giantDone && !g_giantActive && !giantPending && g_totalClosed >= 20) {
        giantPending = 1;
        giantTimer = 90;
        PlaySE(sound_enemyCharge);      // 予告音
    }
    if (giantPending) {
        giantTimer--;
        if (giantTimer <= 0) {
            giantPending = 0;
            g_giantActive = 1;
            PlaySE(sound_enemyShot_extreme);
            SpawnPopupWindow(240.0, 250.0, 215.0, 205.0, PHASE_GIANT, 960, 0.0);
        }
    }

    // ---- 通常ポップアップの出現 ----
    if (!g_giantActive) {
        spawnTimer--;
        if (spawnTimer <= 0) {
            int phase = (g_totalClosed >= 8) ? PHASE_BUGGY : PHASE_NORMAL;
            double cx = 100.0 + GetRand(280);   // 100~380
            double cy = 260.0 + GetRand(150);   // 260~410
            double rw = 30.0 + GetRand(30);     // 半幅 30~60
            double rh = 24.0 + GetRand(24);     // 半高 24~48
            int    life = 420 + GetRand(120);
            SpawnPopupWindow(cx, cy, rw, rh, phase, life, -0.25);
            spawnTimer = (phase == PHASE_BUGGY) ? 130 : 100;
        }
    }
}