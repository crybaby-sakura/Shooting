// enemyPat_Tmp.cpp
// 弾幕:スイカ割り
// 緑の大玉でスイカの球体を作り、黄レーザーの「棒」で割ると
// 赤い果汁・緑の外皮リング(割れ目あり)・曲がる種弾が飛散するパターン

// ---- 状態 ----
enum {
    ST_MELON = 0,   // スイカ設置・回転
    ST_RAISE,       // 棒を構える
    ST_STRIKE,      // 棒を振り下ろす
    ST_BURST,       // 割裂(メイン弾幕)
    ST_FALL,        // 果汁の雫・半割れスイカ
    ST_IDLE         // 終了(後始末はメインルーチン)
};

// ---- 弾の役割(param_i[0]に格納) ----
enum {
    ROLE_MELON = 0, // スイカ外皮(自転中)   param_d[0]:公転角 [1]:半径 [2]:角速度
    ROLE_SEED_DOT,  // スイカの種(見た目・自転)
    ROLE_STICK,     // 棒(レーザー)
    ROLE_SHARD,     // 直進する破片(果汁・外皮)
    ROLE_SEED_SHOT, // 曲がる種弾            param_d[0]:曲がり量
    ROLE_DROP,      // 落下する果汁の雫
    ROLE_HALF       // 半割れスイカの部品    param_i[1]:部品番号
};

// スイカの設置位置
static const double MELON_X = 240.0;
static const double MELON_Y = 320.0;

// ---- ヘルパー:ショットセット生成 ----
static sEnemyShotSet* AddShotSet(void (*patternFunc)(sEnemyShotSet*))
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ---- ヘルパー:弾生成 ----
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->margin = 240;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ---- ヘルパー:弾削除(リンクリストから安全に取り外す) ----
static void DeleteShot(sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev->next = pEnemyShot->next;
    pEnemyShot->next->prev = pEnemyShot->prev;
    delete pEnemyShot;
}

// ============================================================
//  スイカ割りショットセットのパターン関数
// ============================================================
static void ShotSuikawari(sEnemyShotSet* pEnemyShotSet)
{
    const double cx = pEnemyShotSet->param_d[0];
    const double cy = pEnemyShotSet->param_d[1];
    const int c = pEnemyShotSet->count;

    // ---- 状態遷移 ----
    switch (pEnemyShotSet->param_i[0]) {

    case ST_MELON:
        // 大玉(緑)3リングで球体を構築(30フレームごとに1リング)
    {
        static const int ringR[3] = { 20, 38, 56 };
        static const int ringN[3] = { 8, 12, 16 };
        int ring = c / 30;
        if (ring < 3 && c % 30 == 0) {
            for (int i = 0; i < ringN[ring]; i++) {
                double ang = DX_PI * 2.0 * i / ringN[ring] - DX_PI / 2;
                sEnemyShot* p = AddShot(pEnemyShotSet, cx, cy, 0.0, 0.0, img_enemyShotLargeBall[2]);
                p->param_i[0] = ROLE_MELON;
                p->param_i[1] = i;              // 割れ目用の番号
                p->param_d[0] = ang;            // 公転角
                p->param_d[1] = (double)ringR[ring];
                p->param_d[2] = 0.008;          // 自転速度
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        // 種(見た目用)を3つ
        if (c == 95) {
            for (int i = 0; i < 3; i++) {
                double ang = DX_PI * 2.0 * i / 3 + 0.5;
                sEnemyShot* p = AddShot(pEnemyShotSet, cx + cos(ang) * 29.0, cy + sin(ang) * 29.0,
                    0.0, 0.0, img_enemyShotSmallBall[7]);
                p->param_i[0] = ROLE_SEED_DOT;
                p->param_d[0] = ang;
                p->param_d[1] = 29.0;
                p->param_d[2] = 0.008;
            }
        }
        // 棒を出現させて構え音 → 次の状態へ
        if (c == 110) {
            for (int i = 0; i < 3; i++) {
                // 黄レーザーを縦に3本連結して「棒」にする
                sEnemyShot* p = AddShot(pEnemyShotSet, cx, 240.0 - i * 60.0, DX_PI / 2, 0.0, img_enemyShotLaser[1]);
                p->param_i[0] = ROLE_STICK;
            }
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            pEnemyShotSet->param_i[0] = ST_RAISE;
        }
        break;
    }

    case ST_RAISE:
        // 一瞬溜めてから棒を振り下ろす
        if (c == 140) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == ROLE_STICK) pShot->speed = 7.0;
                pShot = pShot->next;
            }
            pEnemyShotSet->param_i[0] = ST_STRIKE;
        }
        break;

    case ST_STRIKE:
        // 棒の先端がスイカ中心に到達 → 割裂!
        if (c == 152) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                if (pShot->param_i[0] == ROLE_STICK) {
                    // 棒は赤い果汁弾に変化して四散
                    pShot->kind = img_enemyShotMediumBall[0];
                    pShot->param_i[0] = ROLE_SHARD;
                    pShot->muki = DX_PI / 2 + (GetRand(120) - 60) / 180.0 * DX_PI;
                    pShot->speed = 3.0 + GetRand(150) / 100.0;
                }
                else if (pShot->param_i[0] == ROLE_MELON) {
                    if (pShot->param_d[1] > 45.0 &&
                        (pShot->param_i[1] == 3 || pShot->param_i[1] == 4 ||
                            pShot->param_i[1] == 11 || pShot->param_i[1] == 12)) {
                        // 外周の一部を消して「割れ目」を作る
                        DeleteShot(pShot);
                    }
                    else {
                        // 外皮はリング状にゆっくり広がる
                        pShot->muki = pShot->param_d[0];
                        pShot->speed = 1.2;
                        pShot->param_i[0] = ROLE_SHARD;
                    }
                }
                else if (pShot->param_i[0] == ROLE_SEED_DOT) {
                    // 種も飛び出す(種弾に変化)
                    pShot->param_i[0] = ROLE_SEED_SHOT;
                    pShot->muki = pShot->param_d[0];
                    pShot->speed = 2.0;
                    pShot->param_d[0] = (GetRand(20) - 10) / 2000.0; // 曲がり量
                }
                pShot = next;
            }
            // 果汁(赤中玉)1波目:全8方向
            for (int i = 0; i < 8; i++) {
                double ang = DX_PI * 2.0 * i / 8;
                sEnemyShot* p = AddShot(pEnemyShotSet, cx, cy, ang, 2.5 + GetRand(100) / 100.0,
                    img_enemyShotMediumBall[0]);
                p->param_i[0] = ROLE_SHARD;
            }
            // 種弾(黒小玉):ランダム12方向、曲線軌道
            for (int i = 0; i < 12; i++) {
                double ang = GetRand(359) / 180.0 * DX_PI;
                sEnemyShot* p = AddShot(pEnemyShotSet, cx, cy, ang, 2.0 + GetRand(150) / 100.0,
                    img_enemyShotSmallBall[7]);
                p->param_i[0] = ROLE_SEED_SHOT;
                p->param_d[0] = (GetRand(20) - 10) / 2000.0;
            }
        }
        // 果汁2波目:8方向(1波目の隙間を狙う)
        if (c == 164) {
            for (int i = 0; i < 8; i++) {
                double ang = DX_PI * 2.0 * i / 8 + DX_PI / 8;
                sEnemyShot* p = AddShot(pEnemyShotSet, cx, cy, ang, 3.5 + GetRand(100) / 100.0,
                    img_enemyShotMediumBall[0]);
                p->param_i[0] = ROLE_SHARD;
            }
        }
        if (c == 170) pEnemyShotSet->param_i[0] = ST_FALL;
        break;

    case ST_FALL:
        // 果汁の雫が降り注ぐ
        if (c >= 170 && c <= 280 && c % 12 == 0) {
            sEnemyShot* p = AddShot(pEnemyShotSet,
                cx + GetRand(200) - 100, cy - 80 + GetRand(60) - 20,
                DX_PI / 2, 1.0 + GetRand(100) / 100.0,
                img_enemyShotSmallBall[(GetRand(1) == 0) ? 0 : 8]);
            p->param_i[0] = ROLE_DROP;
        }
        // 半割れスイカを片側に生成(外皮+果肉+種)
        if (c == 200) {
            for (int i = -1; i <= 1; i += 2) {
                double hx = cx + i * 130.0;
                double hy = cy - 70.0;
                for (int i = 0; i < 5; i++) { // 外皮(緑中玉の下半円)
                    double ang = DX_PI * i / 4.0;
                    sEnemyShot* p = AddShot(pEnemyShotSet, hx + cos(ang) * 25.0, hy + sin(ang) * 25.0,
                        DX_PI / 2, 0.5, img_enemyShotMediumBall[2]);
                    p->param_i[0] = ROLE_HALF;
                    p->param_i[1] = i;
                }
                for (int i = 0; i < 3; i++) { // 果肉(赤小玉)
                    double ang = DX_PI * (1.0 + i) / 4.0;
                    sEnemyShot* p = AddShot(pEnemyShotSet, hx + cos(ang) * 12.0, hy + sin(ang) * 12.0,
                        DX_PI / 2, 0.5, img_enemyShotSmallBall[0]);
                    p->param_i[0] = ROLE_HALF;
                    p->param_i[1] = 10 + i;
                }
                sEnemyShot* p = AddShot(pEnemyShotSet, hx, hy - 2.0, DX_PI / 2, 0.5, img_enemyShotSmallBall[7]);
                p->param_i[0] = ROLE_HALF;
                p->param_i[1] = 20;
            }
        }
        // 半割れスイカがはじけて小弾に
        if (c == 290) {
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                if (pShot->param_i[0] == ROLE_HALF) {
                    double px = pShot->x, py = pShot->y;
                    int color = (pShot->param_i[1] < 5) ? 2 : 0; // 外皮は緑、それ以外は赤
                    for (int k = 0; k < 2; k++) {
                        double ang = GetRand(359) / 180.0 * DX_PI;
                        sEnemyShot* pNew = AddShot(pEnemyShotSet, px, py, ang, 1.5 + GetRand(100) / 100.0,
                            img_enemyShotSmallBall[color]);
                        pNew->param_i[0] = ROLE_SHARD;
                    }
                    DeleteShot(pShot);
                }
                pShot = next;
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        if (c == 320) pEnemyShotSet->param_i[0] = ST_IDLE;
        break;

    case ST_IDLE:
    default:
        // 何もしない(画面外の弾はメインルーチンが消去する)
        break;
    }

    // ---- 共通の弾更新 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ROLE_MELON:   // スイカ本体:中心まわりを自転
        case ROLE_SEED_DOT:
            pShot->param_d[0] += pShot->param_d[2];
            pShot->x = cx + pShot->param_d[1] * cos(pShot->param_d[0]);
            pShot->y = cy + pShot->param_d[1] * sin(pShot->param_d[0]);
            break;
        case ROLE_SEED_SHOT: // 種弾:曲線軌道
            pShot->muki += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        case ROLE_DROP:      // 雫:揺れながら落下
            pShot->x += sin(pShot->count * 0.08) * 0.8;
            pShot->y += pShot->speed;
            break;
        default:             // ROLE_SHARD / ROLE_STICK / ROLE_HALF:直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Suikawari_Zai()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    // ボスは画面上部をゆっくり左右に揺れる
    enemy.x = 240.0 + sin(count * 0.02) * 60.0;

    // 約470フレーム(約7.8秒)周期でスイカ割りを繰り返す
    if (count % 420 == 20) {
        sEnemyShotSet* pEnemyShotSet = AddShotSet(ShotSuikawari);
        pEnemyShotSet->param_d[0] = MELON_X; // スイカ中心
        pEnemyShotSet->param_d[1] = MELON_Y;
        pEnemyShotSet->param_i[0] = ST_MELON;
    }
}