// enemyPat_Tmp.cpp
// 裏隠しボス「陰蜂」イメージの超弾幕パターン: 冥界狂想曲
//
// 構成 (敵HP 200 をフェーズ分けの基準にする):
//   フェーズ1 (hp > 133): 「冥界螺旋」回転二重螺旋 + 「裂空刺」予告付き高速自機狙い
//   フェーズ2 (66 < hp <= 133): 「冥界追尾」ゆっくり曲がる追尾弾 + 「冥界波」下からせり上がる弾幕墙
//   フェーズ3 (hp <= 66): 発狂。「滝筒洗衣机」多層回転リング + 「河豚刺身」ランダム高速直線弾 + 高速赤螺旋
//
// 注意:
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//     画面外の弾の消去はメインルーチンで行われるため、ここでは扱わない。
//   - GetRand(x) は 0〜x の x+1 種類を返す。
//   - 弾の移動は各パターン関数内で毎フレーム行う。

// 色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙

// ------------------------------------------------------------
// 弾を1発生成してリストに繋ぐ (生成した弾を返す)
// ------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int img)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = img;

    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// ------------------------------------------------------------
// 弾セットを生成してリストに繋ぐ
// param_i[0] を寿命(発射をやめるフレーム数)として使う
// ------------------------------------------------------------
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int color, int life)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = color;   // 弾の色として各パターン内で使用
    pEnemyShotSet->param_i[0] = life;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// ------------------------------------------------------------
// 弾幕1: 冥界螺旋 ─ ボス位置から回転する二重螺旋
// set->param_d[0]: 現在の基準角度 / param_d[1]: 回転速度
// ------------------------------------------------------------
static void ShotNetherSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count > pSet->param_i[0]) return; // 寿命切れ

    // 発射源はボスに追従させる
    pSet->x = enemy.x;
    pSet->y = enemy.y + 20.0;

    if (pSet->count % 4 == 0) {
        pSet->param_d[0] += pSet->param_d[1];
        double m = pSet->param_d[0];
        AddShot(pSet, pSet->x, pSet->y, m, 2.6, img_enemyShotMediumBall[pSet->kind]);
        AddShot(pSet, pSet->x, pSet->y, m + DX_PI, 2.6, img_enemyShotMediumBall[pSet->kind]);

        // 効果音は間引いて鳴らす
        if (pSet->count % 24 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // 全弾直進
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕2: 裂空刺 ─ 予告音の後、自機目がけて狭範囲に高速弾を突き刺す
// ------------------------------------------------------------
static void ShotRiftThorn(sEnemyShotSet* pSet)
{
    if (pSet->count > pSet->param_i[0]) return;

    if (pSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 80フレーム周期で発射 (初回は40フレーム後)
    if (pSet->count >= 40 && (pSet->count - 40) % 80 == 0) {
        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = 0; i < 7; i++) {
            // 7発の扇 + わずかなランダムブレ
            double m = aim + (i - 3) * 0.09 + (GetRand(6) - 3) / 100.0;
            AddShot(pSet, pSet->x, pSet->y, m, 7.5, img_enemyShotDiamond[pSet->kind]);
        }
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 全弾直進
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕3: 河豚刺身 ─ ランダムな角度ブレを持つ高速直線弾が交差する
// ------------------------------------------------------------
static void ShotFuguSashimi(sEnemyShotSet* pSet)
{
    if (pSet->count > pSet->param_i[0]) return;

    if (pSet->count % 30 == 0) {
        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = 0; i < 5; i++) {
            // ±0.20rad のランダムブレ + ランダム弾速で交差する「刺身」を作る
            double m = aim + (GetRand(40) - 20) / 100.0;
            double sp = 8.0 + GetRand(150) / 100.0;
            AddShot(pSet, pSet->x, pSet->y, m, sp, img_enemyShotBullet[pSet->kind]);
        }
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 全弾直進
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕4: 冥界追尾 ─ 発射後にゆっくり自機へ曲がってくる弾
// 弾側 param_d[0]: 追尾旋回レート
// ------------------------------------------------------------
static void ShotHoming(sEnemyShotSet* pSet)
{
    if (pSet->count > pSet->param_i[0]) return;

    if (pSet->count % 12 == 0) {
        // 自機方向 + ランダムブレで発射
        double m = atan2(player.y - pSet->y, player.x - pSet->x) + (GetRand(80) - 40) / 100.0;
        sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, m, 3.0, img_enemyShotSmallBall[pSet->kind]);
        pShot->param_d[0] = 0.03; // 旋回レート
        if (pSet->count % 60 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // 全弾: 自機方向へわずかに曲がりながら移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double aim = atan2(player.y - pShot->y, player.x - pShot->x);
        double diff = aim - pShot->muki;
        while (diff > DX_PI) diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        double turn = pShot->param_d[0];
        if (diff > turn) diff = turn;
        if (diff < -turn) diff = -turn;
        pShot->muki += diff;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕5: 冥界波 ─ 画面下からせり上がる弾幕墙 (ランダムに隙間あり)
// ------------------------------------------------------------
static void ShotNetherWall(sEnemyShotSet* pSet)
{
    if (pSet->count > pSet->param_i[0]) return;

    if (pSet->count % 160 == 1) {
        for (int i = 0; i < 20; i++) {
            if (GetRand(6) == 0) continue; // ランダムに1カ所スキップして隙間を作る
            AddShot(pSet, 12.0 + i * 24.0, 500.0, -DX_PI / 2.0, 1.5, img_enemyShotMediumOval[pSet->kind]);
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 全弾直進 (上昇)
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕6: 滝筒洗衣机(発狂) ─ 多層リングが回転しながら画面を埋める
// set->param_d[0]: 基準角度 / param_d[1]: 回転速度(符号で方向)
// ------------------------------------------------------------
static void ShotWashingMachine(sEnemyShotSet* pSet)
{
    if (pSet->count > pSet->param_i[0]) return;

    // 発射源はボスに追従
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count % 18 == 0) {
        // 一定間隔で回転方向を反転させ、読み合わせを崩す
        if (pSet->count % 360 == 0) pSet->param_d[1] *= -1.0;
        pSet->param_d[0] += pSet->param_d[1];

        const int n = 22;
        double base = pSet->param_d[0];
        for (int i = 0; i < n; i++) {
            double m = base + i * (2.0 * DX_PI / n);
            AddShot(pSet, pSet->x, pSet->y, m, 3.4, img_enemyShotSmallBall[pSet->kind]);
        }
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 全弾直進
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Inbachi_Zai()
{
    static int phase;      // 1:螺旋 2:追尾+波 3:発狂
    static int dir;        // 移動方向
    static int spiralCnt;  // 各弾幕セットの定期補充タイマー
    static int burstCnt;
    static int sashimiCnt;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        phase = 1;
        dir = 1;
        spiralCnt = burstCnt = sashimiCnt = 0;

        // --- フェーズ1開始: 冥界螺旋(シアン) + 裂空刺(赤) ---
        sEnemyShotSet* p = CreateShotSet(ShotNetherSpiral, enemy.x, enemy.y + 20.0, 0.0, 3 /*シアン*/, 720);
        p->param_d[0] = GetRand(360) / 180.0 * DX_PI;
        p->param_d[1] = 0.11;

        CreateShotSet(ShotRiftThorn, enemy.x, enemy.y + 20.0, 0.0, 0 /*赤*/, 720);
    }
    else {
        // ボス移動: フェーズが進むほど速く左右に振れる
        double spd = (phase >= 3) ? 1.7 : (phase == 2) ? 1.2 : 0.9;
        enemy.x += spd * dir;
        if (enemy.x < 50.0) { enemy.x = 50.0;  dir = 1; }
        if (enemy.x > 430.0) { enemy.x = 430.0; dir = -1; }
    }

    // --- フェーズ遷移判定 ---
    int next = 1;
    if (enemy.hp <= 66)      next = 3;
    else if (enemy.hp <= 133) next = 2;

    if (next > phase) {
        phase = next;

        if (phase == 2) {
            // 「冥界」展開の予告
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 冥界追尾(緑): 画面上部中央から
            CreateShotSet(ShotHoming, 240.0, 40.0, 0.0, 2 /*緑*/, 600);

            // 冥界波(黄): 下からせり上がる壁
            CreateShotSet(ShotNetherWall, 240.0, 500.0, 0.0, 1 /*黄*/, 600);
        }
        if (phase == 3) {
            // 発狂: 極重音と共に洗衣机始動
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // 滝筒洗衣机(マゼンタ): 以降最後まで継続
            sEnemyShotSet* p = CreateShotSet(ShotWashingMachine, enemy.x, enemy.y + 10.0, 0.0, 5 /*マゼンタ*/, 3600);
            p->param_d[0] = GetRand(360) / 180.0 * DX_PI;
            p->param_d[1] = 0.13;
        }
    }

    // --- 定期補充 ---
    // 裂空刺(赤)は全フェーズで補充し続ける
    if (++burstCnt >= 200) {
        burstCnt = 0;
        CreateShotSet(ShotRiftThorn, enemy.x, enemy.y + 20.0, 0.0, 0 /*赤*/, 400);
    }

    // フェーズ2以降: 河豚刺身(マゼンタ)を追加
    if (phase >= 2) {
        if (++sashimiCnt >= 240) {
            sashimiCnt = 0;
            CreateShotSet(ShotFuguSashimi, enemy.x, enemy.y + 10.0, 0.0, 5 /*マゼンタ*/, 360);
        }
    }

    // フェーズ3: 高速赤螺旋で追い打ち
    if (phase == 3) {
        if (++spiralCnt >= 300) {
            spiralCnt = 0;
            sEnemyShotSet* p = CreateShotSet(ShotNetherSpiral, enemy.x, enemy.y + 20.0, 0.0, 0 /*赤*/, 720);
            p->param_d[0] = GetRand(360) / 180.0 * DX_PI;
            p->param_d[1] = 0.23; // フェーズ1の約2倍の回転速度
        }
    }
}