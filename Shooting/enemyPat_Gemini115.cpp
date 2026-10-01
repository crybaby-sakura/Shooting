#include <math.h>

// 弾幕名：狂蜂ノ繭（きょうほうのまゆ）

// --- 第1波・第3波：繭の形成と崩壊（全方位停滞弾＆一斉突撃） ---
static void ShotMayu(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    int t = pEnemyShotSet->count;

    // 第1波：繭の形成 (全方位停滞弾)
    // t=0〜180 の間、2フレーム毎に高密度で発射
    if (t < 180 && t % 2 == 0) {
        // 音が重なりすぎないよう適度に間引く
        if (t % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int ways = 10; // 1回の発射方向数

        // 2つの螺旋を逆回転で撃ち出して交差させ、網目を作る
        double angle_offset_right = t * 0.06;
        double angle_offset_left = -t * 0.06;

        // 速度を波打たせて停止距離にムラを作り、幾何学的な格子を形成する
        double initial_speed = 6.0 + sin(t * 0.15) * 2.5 + 0.03 * t;

        for (int i = 0; i < ways; i++) {
            // 右回転螺旋
            sEnemyShot* pShot1 = new sEnemyShot;
            pShot1->x = pEnemyShotSet->x;
            pShot1->y = pEnemyShotSet->y;
            pShot1->muki = angle_offset_right + (DX_PI * 2.0 / ways) * i;
            pShot1->speed = initial_speed;
            pShot1->kind = img_enemyShotSmallBall[4]; // 青の極小丸弾
            pShot1->param_i[0] = 0; // 状態: 0=減速中
            pShot1->margin = 480;

            pShot1->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot1->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot1;
            pEnemyShotSet->pEnemyShotHead->prev = pShot1;

            // 左回転螺旋
            sEnemyShot* pShot2 = new sEnemyShot;
            pShot2->x = pEnemyShotSet->x;
            pShot2->y = pEnemyShotSet->y;
            pShot2->muki = angle_offset_left + (DX_PI * 2.0 / ways) * i;
            pShot2->speed = initial_speed;
            pShot2->kind = img_enemyShotSmallBall[4]; // 青の極小丸弾
            pShot2->param_i[0] = 0; // 状態: 0=減速中
            pShot2->margin = 480;

            pShot2->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot2->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot2;
            pEnemyShotSet->pEnemyShotHead->prev = pShot2;
        }
    }

    // 第3波：発光演出 (警告音と共に青弾が赤へ変わる)
    if (t == 400-30) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 第3波：一斉誘導＆加速開始 (轟音と共に全弾が自機へ牙を剥く)
    if (t == 460-30) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // フェーズ0: 減速して停滞
            pShot->speed -= 0.15;
            if (pShot->speed <= 0.0) {
                pShot->speed = 0.0;
                pShot->param_i[0] = 1; // 停止状態へ移行
            }
        }
        else if (pShot->param_i[0] == 1) {
            // フェーズ1: 停止中
            if (t == 400-30) {
                pShot->kind = img_enemyShotSmallBall[0]; // 赤く発光して警告
            }
            if (t == 460-30) {
                // 現在の自機位置へ向けて角度を再計算
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->param_i[0] = 2; // 突撃状態へ移行
            }
        }
        else if (pShot->param_i[0] == 2) {
            // フェーズ2: 自機へ向かって徐々に超高速へ加速
            pShot->speed += 0.15;
            if (pShot->speed > 12.0) {
                pShot->speed = 12.0; // 最高速度
            }
        }

        // 座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// --- 第2波：超高速・2重交差針弾 ---
static void ShotNeedle(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    int t = pEnemyShotSet->count;

    // 停滞弾で身動きが取れない中、t=0〜180の間、20フレーム毎に発射
    if (t % 20 == 0 && t <= 180) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 自機への基本角度
        double angleToPlayer = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 左右2箇所から交差するように発射
        for (int i = 0; i < 2; i++) {
            // 発射位置を敵の左右へ少しずらす
            double offsetDir = angleToPlayer + (i == 0 ? DX_PI / 2.0 : -DX_PI / 2.0);
            double startX = pEnemyShotSet->x + cos(offsetDir) * 25.0;
            double startY = pEnemyShotSet->y + sin(offsetDir) * 25.0;

            // 自機の周囲(少し交差する先)を狙う
            // i=0(左から発射)は自機の右側を、i=1(右から発射)は自機の左側を狙う
            double targetX = player.x + cos(offsetDir);
            double targetY = player.y + sin(offsetDir);
            double aimAngle = atan2(targetY - startY, targetX - startX);

            // 速度差をつけて縦に連なる2連射にする
            for (int j = 0; j < 2; j++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = startX;
                pShot->y = startY;
                pShot->muki = aimAngle;
                pShot->speed = 9.0 + j * 1.5; // 超高速 (9.0 と 10.5)
                pShot->kind = img_enemyShotBullet[0]; // 赤の針弾(銃弾グラフィック)

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 針弾は直進のみ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// --- 敵本体のパターン ---
void EnemyPat_Inbachi_Gemini()
{
    if (count == 1) {
        // 出現位置と体力設定
        enemy.x = 240.0;
        enemy.y = -20.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 所定の位置までゆっくり降りてくる
    if (count < 90) {
        enemy.y += (80.0 - (-20.0)) / 90.0;
    }

    const int T = 600;
    int countT = (count - 100) % T;

    // count = 100：第1波(繭形成)開始
    if (countT == 100-100) {
        sEnemyShotSet* pSetMayu = new sEnemyShotSet;
        pSetMayu->count = 0;
        pSetMayu->patternFunc = ShotMayu;
        pSetMayu->x = enemy.x;
        pSetMayu->y = enemy.y;

        pSetMayu->pEnemyShotHead = new sEnemyShot;
        pSetMayu->pEnemyShotHead->prev = pSetMayu->pEnemyShotHead;
        pSetMayu->pEnemyShotHead->next = pSetMayu->pEnemyShotHead;

        pSetMayu->prev = enemyShotSetHead.prev;
        pSetMayu->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetMayu;
        enemyShotSetHead.prev = pSetMayu;
    }

    // count = 300：第2波(交差針弾)開始。停滞弾で身動きが取れない中で発動
    if (countT == 300-100) {
        sEnemyShotSet* pSetNeedle = new sEnemyShotSet;
        pSetNeedle->count = 0;
        pSetNeedle->patternFunc = ShotNeedle;
        pSetNeedle->x = enemy.x;
        pSetNeedle->y = enemy.y;

        pSetNeedle->pEnemyShotHead = new sEnemyShot;
        pSetNeedle->pEnemyShotHead->prev = pSetNeedle->pEnemyShotHead;
        pSetNeedle->pEnemyShotHead->next = pSetNeedle->pEnemyShotHead;

        pSetNeedle->prev = enemyShotSetHead.prev;
        pSetNeedle->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetNeedle;
        enemyShotSetHead.prev = pSetNeedle;
    }

    // 発射サイクル管理（弾幕が終わったら再度ループさせる場合）
    // 第1波の t=460 が突撃タイミングなので、count=100+460=560。
    // そこから弾が画面外に出るまでの余韻を持たせてリセットする。
    //if (count >= 750) {
    //    count = 99; // 次のフレームで100になり再度発射
    //}
}