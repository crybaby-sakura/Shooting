// =============================================================
//  balloonCage.cpp
//  風船をモチーフにした弾幕：『バルーン・ケージ ～割れば散る、割らねば詰む～』
//  敵本体: void EnemyPat_Balloon_DeepSeek()
//  弾幕  : ShotBalloonCage()
//
//  使用素材（enemyPat_sampleForAI.cpp 準拠）
//    画像: img_enemyShotMediumBall[5]   … 風船（マゼンタ中玉）
//          img_enemyShotSmallBall[0]    … 破裂時にばら撒く弾（赤小玉）
//    効果音: sound_enemyCharge         … 風船展開の予告音
//            sound_enemyShot_light     … 風船出現
//            sound_enemyShot_medium    … 放射破裂
//            sound_enemyShot_heavy     … 時間切れ自機狙い破裂
//    グローバル: player, playerShotHead, enemy, enemyShotSetHead, count
//
//  仕様メモ
//    - count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//      画面外弾の消去はメインルーチンが行うのでここでは触らない。
//    - GetRand(x) は 0..x の x+1 種を返す点に注意（今回は未使用）。
// =============================================================

// -------------------------------------------------------------
//  風船ケージ弾幕
// -------------------------------------------------------------
//  sEnemyShotSet::param の使い方
//    param_i[0] : wave 番号
//    param_i[1] : 風船の総数
//    param_i[2] : 破裂済みの数
//    param_i[3] : 風船を一度生成したか (0/1)
//    param_d[0] : リング半径
//    param_d[1] : リング回転角
//    param_d[2] : リング中心 X
//    param_d[3] : リング中心 Y
//    param_d[4] : 回転速度 (rad/frame)
//    param_d[5] : 収縮速度 (px/frame)
//
//  sEnemyShot::param の使い方
//    param_i[0] : 0=風船 / 1=破裂弾
//    --- 風船の場合 ---
//    param_i[1] : HP
//    param_i[2] : 状態 0=生存 / 1=破裂処理中
//    param_i[3] : 破裂タイプ 0=放射 / 1=自機狙い
//    param_i[4] : 自動破裂までの残りフレーム
//    param_i[5] : 破裂弾を吐き出し済みかどうか
//    param_d[0] : リング上の角度
//    param_d[1] : ふわふわ揺れの位相
//    --- 破裂弾の場合 ---
//    param_i[1] : 破裂タイプ
//    param_i[2] : 停止残りフレーム
//    param_d[0] : 進行角度
//    param_d[1] : 停止後に出す速度
// -------------------------------------------------------------
static void ShotBalloonCage(sEnemyShotSet* pEnemyShotSet)
{
    // === 初期化（このセットが生まれて最初のフレームだけ） ===
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int wave = pEnemyShotSet->kind;
        int n = 18 + wave;   // 8, 9, 10, 11, 8, 9, ... と増える

        pEnemyShotSet->param_i[0] = wave;
        pEnemyShotSet->param_i[1] = n;
        pEnemyShotSet->param_i[2] = 0;
        pEnemyShotSet->param_i[3] = 0;
        pEnemyShotSet->param_d[0] = 180.0;                              // 半径
        pEnemyShotSet->param_d[1] = 0.0;                                // 回転角
        pEnemyShotSet->param_d[2] = player.x;                           // 中心X
        pEnemyShotSet->param_d[3] = player.y;                           // 中心Y
        // 波ごとに回転方向を反転（1deg/frame 程度）
        pEnemyShotSet->param_d[4] = ((wave % 2) ? -1.0 : 1.0) * (DX_PI / 180.0);
        pEnemyShotSet->param_d[5] = 20.0 / 60.0;                        // 20px/s

        // セット自身は画面外消去されないよう画面中心に固定
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
    }

    // === リング中心を自機にゆっくり追従させる ===
    //  風船が画面外に飛び出して消されないよう、中心は画面内にクランプする
    double cx = pEnemyShotSet->param_d[2];
    double cy = pEnemyShotSet->param_d[3];
    cx += (player.x - cx) * 0.02;
    cy += (player.y - cy) * 0.02;
    if (cx < 200.0) cx = 200.0;
    if (cx > 280.0) cx = 280.0;
    if (cy < 200.0) cy = 200.0;
    if (cy > 280.0) cy = 280.0;
    pEnemyShotSet->param_d[2] = cx;
    pEnemyShotSet->param_d[3] = cy;

    // === 風船の初期生成（1回だけ） ===
    if (pEnemyShotSet->param_i[3] == 0) {
        int n = pEnemyShotSet->param_i[1];
        double r = pEnemyShotSet->param_d[0];

        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            double a = (2.0 * DX_PI) * i / n;

            p->x = cx + r * cos(a);
            p->y = cy + r * sin(a);
            p->muki = a;
            p->speed = 0.0;

            // 風船の見た目：マゼンタの中玉
            p->kind = img_enemyShotMediumBall[5];

            p->param_i[0] = 0;                       // 種別：風船
            p->param_i[1] = 1;                       // HP
            p->param_i[2] = 0;                       // 生存
            p->param_i[3] = 0;                       // 破裂タイプ
            p->param_i[4] = 180;                     // 自動破裂まで 3 秒
            p->param_i[5] = 0;                       // 未破裂
            p->param_d[0] = a;                       // リング角度
            p->param_d[1] = (double)GetRand(100);    // ふわふわ位相

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        pEnemyShotSet->param_i[3] = 1;

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // === リングの回転と収縮 ===
    pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[4];
    if (pEnemyShotSet->param_d[0] > 40.0) {
        pEnemyShotSet->param_d[0] -= pEnemyShotSet->param_d[5];
    }
    double radius = pEnemyShotSet->param_d[0];
    double rot = pEnemyShotSet->param_d[1];

    // === 弾（風船／破裂弾）の毎フレーム更新 ===
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;   // 削除に備えて先に保存
        int type = pShot->param_i[0];

        // ------------------------------
        //  風船
        // ------------------------------
        if (type == 0) {
            if (pShot->param_i[2] == 0) {
                // --- 生存中：リング上に固定＋ふわふわ ---
                double a = pShot->param_d[0] + rot;
                double wob = sin((pShot->count + pShot->param_d[1]) * 0.1) * 5.0;
                pShot->x = cx + (radius + wob) * cos(a);
                pShot->y = cy + (radius + wob) * sin(a);
                pShot->muki = a;
                pShot->speed = 0.0;

                // --- 自機ショットとの当たり判定 ---
                bool hit = false;
                sPlayerShot* ps = playerShotHead.next;
                while (ps != &playerShotHead) {
                    double dx = ps->x - pShot->x;
                    double dy = ps->y - pShot->y;
                    if (dx * dx + dy * dy < 24.0 * 24.0) { hit = true; break; }
                    ps = ps->next;
                }

                // --- 自動破裂タイマー ---
                if (pShot->param_i[4] > 0) pShot->param_i[4]--;

                if (hit) {
                    pShot->param_i[2] = 1;
                    pShot->param_i[3] = 0;   // 放射破裂
                }
                else if (pShot->param_i[4] == 0) {
                    pShot->param_i[2] = 1;
                    pShot->param_i[3] = 1;   // 自機狙い破裂
                }
            }

            // --- 破裂処理（1フレームだけ） ---
            if (pShot->param_i[2] == 1 && pShot->param_i[5] == 0) {
                int    burstType = pShot->param_i[3];
                int    bullets;
                double baseAngle;

                if (burstType == 1) {
                    // 自機狙い 3-way
                    baseAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                    bullets = 3;
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
                else {
                    // リング外向きを基準にした放射
                    baseAngle = pShot->param_d[0] + rot;
                    bullets = 16;
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                double bx = pShot->x;
                double by = pShot->y;

                for (int i = 0; i < bullets; i++) {
                    sEnemyShot* b = new sEnemyShot;
                    b->x = bx;
                    b->y = by;

                    double a;
                    if (burstType == 1) {
                        // 自機狙い 3-way（±10°）
                        a = baseAngle + (i - 1) * (10.0 / 180.0 * DX_PI);
                    }
                    else {
                        // 16 方向均等
                        a = baseAngle + (2.0 * DX_PI) * i / bullets;
                    }

                    b->muki = a;
                    b->speed = 0.0;   // まずはその場に留まる
                    b->kind = img_enemyShotSmallBall[0]; // 赤小玉

                    b->param_i[0] = 1;                    // 種別：破裂弾
                    b->param_i[1] = burstType;
                    b->param_i[2] = 12;                   // 0.2 秒停止
                    b->param_d[0] = a;                    // 発射角度
                    b->param_d[1] = (burstType == 1) ? 3.0 : 2.7; // 停止後の速度

                    b->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    b->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = b;
                    pEnemyShotSet->pEnemyShotHead->prev = b;
                }

                pShot->param_i[5] = 1;
                pEnemyShotSet->param_i[2]++;

                // 破裂した風船自体はリストから抜いて消す
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = nullptr;
            }
        }
        // ------------------------------
        //  破裂弾
        // ------------------------------
        else if (type == 1) {
            if (pShot->param_i[2] > 0) {
                // その場で停止（タメ）
                pShot->param_i[2]--;
            }
            else {
                // 停止後に指定速度で飛ぶ
                pShot->speed = pShot->param_d[1];
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        pShot = nextShot;
    }
}

// =============================================================
//  敵本体パターン
// =============================================================
void EnemyPat_Balloon_DeepSeek()
{
    static int muki;
    static int wave;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 で固定
        muki = 1;
        wave = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 約 7 秒ごとに新しい風船ケージを発動（波ごとに数と回転方向が変わる）
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBalloonCage;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = wave++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}