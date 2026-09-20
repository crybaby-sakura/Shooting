// enemyPat_Suika.cpp
#include <cmath>

// スイカ割り弾幕処理関数
static void ShotSuika(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 1. スイカの生成 (初期化処理) ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // スイカの中心位置（画面中央やや上）
        double cx = 240.0;
        double cy = 180.0;
        pEnemyShotSet->param_d[0] = cx;
        pEnemyShotSet->param_d[1] = cy;

        // A. スイカの「皮」（緑の大玉: 2番）を外円状に配置
        int outer_count = 12;
        for (int i = 0; i < outer_count; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / outer_count) * i;
            double radius = 38.0;

            pEnemyShot->x = cx + radius * cos(angle);
            pEnemyShot->y = cy + radius * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[2]; // 緑・大玉
            pEnemyShot->param_i[0] = 1; // 状態識別 1: 皮
            pEnemyShot->param_d[0] = angle;  // 現在の回転角度
            pEnemyShot->param_d[1] = radius; // 半径

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // B. スイカの「果肉」（赤の中玉: 0番）を内円状に配置
        int inner_count = 8*3;
        for (int i = 0; i < inner_count; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / inner_count) * i;
            double radius = 20.0;

            pEnemyShot->x = cx + radius * cos(angle);
            pEnemyShot->y = cy + radius * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤・中玉
            pEnemyShot->param_i[0] = 2; // 状態識別 2: 果肉
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = radius;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 中心点の果肉
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = cx;
        pEnemyShot->y = cy;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[0];
        pEnemyShot->param_i[0] = 2;
        pEnemyShot->param_d[0] = 0.0;
        pEnemyShot->param_d[1] = 0.0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    int t = pEnemyShotSet->count;
    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];

    // --- 2. スイカの旋回 & 自機誘導弾の発射 ---
    if (t < 120) {
        // プレイヤーをスイカの真下に誘導するため、左右からシアン小玉を交互に発射
        if (t >= 15 && t <= 90 && t % 3 == 0) {
            if (t % 15 == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // 左端から
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 30.0;
            pEnemyShot->y = 120.0;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x + (t - 50) * 2.0 - pEnemyShot->x);
            pEnemyShot->speed = 2.2 + 10;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン・小玉
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 右端から
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 450.0;
            pEnemyShot->y = 120.0;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - (t - 50) * 2.0 - pEnemyShot->x);
            pEnemyShot->speed = 2.2 + 10;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン・小玉
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 振り下ろし直前の予告音
        if (t == 95) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // --- 3. 棒による叩き割り（上からの連続レーザー） ---
    if (t == 120) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 画面上部からスイカの中心を貫く縦一列の棒（短レーザー）
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = -20.0 - (i * 28.0);
            pEnemyShot->muki = DX_PI / 2.0; // 真下
            pEnemyShot->speed = 15.0;
            pEnemyShot->kind = img_enemyShotLaser[1]; // 黄・短レーザー
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 4. スイカ破裂と「種」の飛び散り ---
    if (t == 130) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // スイカ内部から「種」（黒の菱形弾）をランダム方向に飛び散らせる
        int seed_count = 20*3;
        for (int i = 0; i < seed_count; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            // GetRand を使用して 0〜360度、および速度をばらつかせる
            pEnemyShot->muki = (GetRand(360) / 180.0) * DX_PI;
            pEnemyShot->speed = 1.5 + (GetRand(250) / 100.0);
            pEnemyShot->kind = img_enemyShotDiamond[7]; // 黒・菱形弾
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 全弾の毎フレーム更新処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1 || pShot->param_i[0] == 2) {
            // A. 未破裂状態（t < 130）: 低速回転
            if (t < 130) {
                pShot->param_d[0] += 0.015; // 角度を進める
                double r = pShot->param_d[1];
                double ang = pShot->param_d[0];
                pShot->x = cx + r * cos(ang);
                pShot->y = cy + r * sin(ang);
            }
            // B. 破裂の瞬間（t == 130）: 中心から外側に向かう角度と速度を設定して飛散
            else if (t == 130) {
                double ang = atan2(pShot->y - cy, pShot->x - cx);
                pShot->muki = ang;
                // 外側の皮(1)は速く、内側の果肉(2)はやや遅く飛ばす
                pShot->speed = (pShot->param_i[0] == 1) ? 3.8 : 2.5;
                pShot->param_i[0] = 0; // 通常の移動状態へ移行
            }
        }
        else {
            // 通常弾の移動処理
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_Suikawari_Gemini()
{
    static int muki;

    if (count == 1) {
        // 初期位置と基本設定
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 上部で左右に緩やかに往復移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) {
            muki *= -1;
        }
    }

    // 240フレーム（4秒）周期でスイカ割りセットを生成
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSuika;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾リストのセンチネルノード生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}