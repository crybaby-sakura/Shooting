// enemyPat_Tmp.cpp
//
// 弾幕パターン: 「くるくるロータリーゲート」
// くるくるくるりんの「回転する細長い棒」をイメージした弾幕。
//   - 3本の「棒」(小玉の密集した直線)が敵の出した位置を支点に回転する
//   - 回転速度は sin 波で「遅⇄速」を繰り返し、遅い瞬間を読んで隙間を抜ける
//   - 棒の寿命が尽きると遠心方向に飛び散って画面外へ消える
//   - 棒の両端の中玉がまき散らし弾を放出、別途 自機狙いの銃弾3連も飛んでくる
//
// 使用素材:
//   小玉(img_enemyShotSmallBall)   … 棒の本体(シアン)
//   中玉(img_enemyShotMediumBall)  … 棒の両端(白) / 放出弾(マゼンタ)
//   銃弾(img_enemyShotBullet)     … 自機狙いの針弾(赤)
//   効果音: sound_enemyCharge(予告) / sound_enemyShot_heavy(棒生成) / sound_enemyShot_light(針弾)
//
// 仕様メモ:
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//     画面外弾の自動消去はメインルーチン側で行われる前提。
//   - 棒の弾は寿命まで画面内に留まるため、寿命時に遠心放出して
//     「画面外→自動消去」のルートに乗せている。

// ---------------- チューニングパラメータ ----------------
static const int    GATE_INTERVAL = 180;                 // 回転棒ゲートの発生間隔
static const int    BAR_LIFE = 360;                 // 棒が回転し続けるフレーム数
static const double ROT_W0 = 0.02;                // 基本角速度
static const double ROT_AMP = 0.03;                // 角速度の揺らぎ振幅
static const double ROT_FREQ = (2.0 * DX_PI / 240.0); // 揺らぎの周期(約4秒)
static const int    BULLETS_HALF = 12;                  // 棒の片側の小玉の数
static const double BAR_STEP = 8.0;                 // 小玉の間隔
static const double END_RADIUS = 104.0;               // 両端の中玉の支点からの距離

// ---------------- 共通ヘルパ: 弾リスト末尾へ弾を追加 ----------------
static sEnemyShot* AddEnemyShot(sEnemyShotSet* pEnemyShotSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// ---------------- 棒を構成する1弾を追加(円運動用パラメータを設定) ----------------
// role: 0=棒本体 1=両端の中玉
static void AddBarShot(sEnemyShotSet* pEnemyShotSet, double r, double angleOffset,
    int role, int kind)
{
    // 初期角度は set の muki を「初期位相」として流用
    double th = pEnemyShotSet->muki + angleOffset;
    double x = pEnemyShotSet->x + r * cos(th);
    double y = pEnemyShotSet->y + r * sin(th);

    sEnemyShot* pEnemyShot = AddEnemyShot(pEnemyShotSet, x, y, th, 0.0, kind);
    pEnemyShot->param_i[0] = role;          // 役割
    pEnemyShot->param_d[0] = pEnemyShotSet->x; // 支点X
    pEnemyShot->param_d[1] = pEnemyShotSet->y; // 支点Y
    pEnemyShot->param_d[2] = r;             // 支点からの符号付き距離
    pEnemyShot->param_d[3] = angleOffset;   // 棒ごとの角度オフセット
}

// ---------------- 弾幕本体: 回転する3本の棒 ----------------
static void ShotRotaryGate(sEnemyShotSet* pEnemyShotSet)
{
    // --- 生成: 3本の棒を120度間隔で設置 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int k = 0; k < 3; k++) {
            double off = k * (2.0 * DX_PI / 3.0);

            // 棒本体: 小玉(シアン)を8px間隔で左右対称に並べる(隙間3pxでほぼ壁)
            for (int j = 1; j <= BULLETS_HALF; j++) {
                double r = j * BAR_STEP;
                AddBarShot(pEnemyShotSet, r, off, 0, img_enemyShotSmallBall[3]);
                AddBarShot(pEnemyShotSet, -r, off, 0, img_enemyShotSmallBall[3]);
            }
            // 両端: 中玉(白)
            AddBarShot(pEnemyShotSet, END_RADIUS, off, 1, img_enemyShotMediumBall[6]);
            AddBarShot(pEnemyShotSet, -END_RADIUS, off, 1, img_enemyShotMediumBall[6]);
        }
    }

    // --- 回転角の計算: ω(t) = W0 + AMP*sin(FREQ*t) を積分した閉形式 ---
    // angle(t) = W0*t + (AMP/FREQ)*(1 - cos(FREQ*t))  → 「遅⇄速(ときどき逆回転)」を表現
    double t = (double)pEnemyShotSet->count;
    double ang = pEnemyShotSet->muki
        + ROT_W0 * t
        + (ROT_AMP / ROT_FREQ) * (1.0 - cos(ROT_FREQ * t));

    // --- 全弾更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];

        if (role == 2) {
            // 放出済みの弾: 直進して画面外へ(自動消去に任せる)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 棒の弾: 支点周りの円運動
            double th = ang + pShot->param_d[3];
            pShot->x = pShot->param_d[0] + pShot->param_d[2] * cos(th);
            pShot->y = pShot->param_d[1] + pShot->param_d[2] * sin(th);
            pShot->muki = th;
            pShot->param_d[1] += 0.5;

            // 両端の中玉から、遠心方向へ中玉(マゼンタ)を放出
            if (role == 1
                && pEnemyShotSet->count >= 40
                && pEnemyShotSet->count < BAR_LIFE - 40
                && pEnemyShotSet->count % 10 == 0)
            {
                double em = atan2(pShot->y - pShot->param_d[1],
                    pShot->x - pShot->param_d[0]);
                sEnemyShot* pNew = AddEnemyShot(pEnemyShotSet, pShot->x, pShot->y,
                    em, 1.5, img_enemyShotMediumBall[5]);
                pNew->param_i[0] = 2; // 直進弾として扱う
            }

            // 寿命切れ: 棒を構成する全弾を遠心方向へ弾き飛ばす(=画面外へ消える)
            if (pShot->count >= BAR_LIFE) {
                pShot->param_i[0] = 2;
                pShot->muki = atan2(pShot->y - pShot->param_d[1],
                    pShot->x - pShot->param_d[0]);
                pShot->speed = 2.0;
            }
        }

        pShot = pShot->next;
    }
}

// ---------------- 弾幕: 自機狙いの針弾3連(銃弾・赤) ----------------
static void ShotNeedle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = -1; i <= 1; i++) {
            AddEnemyShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y,
                aim + i * 0.12, 1.7, img_enemyShotBullet[0]);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------- 敵本体のパターン ----------------
void EnemyPat_KuruKuruKururin_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        // y=110 は棒の先端(支点から104px)が画面外自動消去判定(y < -20)に入らないための最低位置
        enemy.x = 240.0;
        enemy.y = 110.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // ゆっくり左右へドリフト(0.5*240=120px の振幅 → 棒の先端も画面内に収まる)
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // ゲート発生の30フレーム前に予告音
    if (count >= 10 && count % GATE_INTERVAL == 10) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 回転棒ゲートの発生(敵のいた場所が支点となり、回転する壁が置き土産として残る)
    if (count >= 40 && count % GATE_INTERVAL == 40) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRotaryGate;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // muki は「棒の初期位相」として使用(ゲートごとに角度が変わる/再現性あり)
        pEnemyShotSet->muki = GetRand(6) * (DX_PI / 3.0);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 自機狙いの針弾3連(棒の隙間ルート選択を迫る妨害)
    if (count >= 70 && count % 90 == 70) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNeedle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}