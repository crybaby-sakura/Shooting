// enemyPat_nightSkyCarpet.cpp
// 流符「夜空のカーペット」

// ------------------------------------------------------------
// 弾をリスト末尾に追加するヘルパー
// ------------------------------------------------------------
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int color)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = color; // 爆発時の大玉の色として保持

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 弾幕：夜空のカーペット本体
//  [  0 -  59] 予告（チャージ音）
//  [ 60 - 419] カーペット展開：螺旋流弾を5方向から放ち、弾幕全体を回転させる
//  [420 - 479] 坍縮：展開した弾幕を中心へ吸い込む
//  [480 - 539] 爆発：中心の星を全方位へ吹き上げる
//  以降この周期をループ（回転方向は周期ごとに反転）
// ------------------------------------------------------------
static void ShotNightSkyCarpet(sEnemyShotSet* pEnemyShotSet)
{
    const double CX = pEnemyShotSet->x;                 // カーペットの中心
    const double CY = pEnemyShotSet->y;

    const int T_WARNING = 60-59;
    const int T_COLLAPSE = 420-60;
    const int T_ERUPT = 480-60;
    const int CYCLE = 540;

    const int t = pEnemyShotSet->count % CYCLE;         // 周期内の経過フレーム
    const int cycleNum = pEnemyShotSet->count / CYCLE;  // 何周期目か
    const double dir = (cycleNum % 2 == 0) ? 1.0 : -1.0; // 回転方向

    // ---- 予告音 ----
   

    // ---- カーペット展開 ----
    if (t == T_WARNING) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    if (t >= T_WARNING && t < T_COLLAPSE) {
        // 回転速度は展開とともに徐々に上がる
        pEnemyShotSet->param_d[1] = (0.045 + (t - T_WARNING) * 0.00006) * dir;

        // 螺旋流弾：5方向の腕が回転しながら星を流す
        if (t % 4 == 0) {
            // 綾織りのように青(4)/マゼンタ(5)を交互に切り替える
            int weave = ((t - T_WARNING) / 4) % 2;
            int color = ((cycleNum + weave) % 2 == 0) ? 4 : 5;

            for (int i = 0; i < 5; i++) {
                double ang = pEnemyShotSet->param_d[0] + i * (DX_PI * 2.0 / 5.0)
                    + (GetRand(8) - 4) / 100.0; // 小さな揺らぎ
                AddShot(pEnemyShotSet, CX, CY, ang, 1.3 + GetRand(70) / 100.0,
                    img_enemyShotMediumBall[color], color);
            }
            // 腕の基準角を進める（これが螺旋の元になる）
            pEnemyShotSet->param_d[0] += 0.13;
        }

        // 夜空のきらめき：白い小弾をランダムに散らす
        if (t % 14 == 0) {
            for (int i = 0; i < 2; i++) {
                AddShot(pEnemyShotSet,
                    CX + GetRand(400) - 200, CY + GetRand(280) - 140,
                    GetRand(360) / 180.0 * DX_PI, 0.6 + GetRand(40) / 100.0,
                    img_enemyShotSmallBall[6], 6);
            }
        }
    }

    if (t == T_COLLAPSE - 60) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 坍縮開始 ----
    if (t == T_COLLAPSE) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ---- 爆発開始：全弾を大玉化させてランダム方向へ吹き飛ばす ----
    if (t == T_ERUPT) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->muki = GetRand(360) / 180.0 * DX_PI;
            pShot->speed = 3.0 + GetRand(300) / 100.0;
            pShot->kind = img_enemyShotLargeBall[pShot->param_i[0]];
            if (GetRand(3) != 0) pShot->margin = -9999;
            pShot = pShot->next;
        }
    }

    // ---- 弾の移動（フェーズで挙動を切替）----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (t >= T_WARNING && t < T_COLLAPSE) {
            // カーペット：直進しつつ、全弾を中心の周りでゆっくり回転させる
            double rx = 0.012 * dir;
            double dx = pShot->x - CX;
            double dy = pShot->y - CY;
            pShot->x = CX + dx * cos(rx) - dy * sin(rx);
            pShot->y = CY + dx * sin(rx) + dy * cos(rx);
            pShot->muki += rx;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (t >= T_COLLAPSE && t < T_ERUPT) {
            // 坍縮：中心へ加速しながら吸い込まれる
            double dx = CX - pShot->x;
            double dy = CY - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            double sp = 2.5 + (t - T_COLLAPSE) * 0.09;
            pShot->muki = atan2(dy, dx);
            pShot->speed = sp;
            if (dist > sp) {
                pShot->x += dx / dist * sp;
                pShot->y += dy / dist * sp;
            }
        }
        else {
            // 予告中・爆発後：等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_NightCarpet_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    // ゆったりと遊泳する（スペル中も一定）
    enemy.x = 240.0 + 70.0 * sin(count * 0.012);
    enemy.y = 120.0 + 8.0 * sin(count * 0.027);

    // スペル開始：カーペットの中心を画面中央やや上に固定し、弾幕セットを一度だけ生成
    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNightSkyCarpet;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 150.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 99999;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_d[0] = 0.0; // 螺旋の基準角
        pEnemyShotSet->param_d[1] = 0.0; // 現在の回転速度

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}