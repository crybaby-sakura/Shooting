// enemyPat_popup.cpp
// 弾幕：ポップアップ広告
//
// 使用素材:
//   img_enemyShotMediumBall : 広告ウィンドウの枠
//   img_enemyShotSmallBall  : 広告本文・展開弾
//   img_enemyShotDiamond    : 閉じる「×」
// 効果音:
//   sound_enemyShot_medium
//   sound_enemyShot_light
//   sound_enemyShot_heavy
//
// 前提:
//   count, pEnemyShotSet->count, pEnemyShot->count の加算はメインルーチン側。
//   画面外へ出た弾の消去もメインルーチン側。
//   GetRand(x) は 0..x の整数を返す。

static void AddPopupShot(
    sEnemyShotSet* set,
    double x, double y, int kind,
    double angle, double speed,
    int role, double a, double b, double c)
{
    sEnemyShot* shot = new sEnemyShot;

    shot->x = x;
    shot->y = y;
    shot->muki = angle;
    shot->speed = speed;
    shot->kind = kind;

    shot->param_i[0] = role;
    shot->param_d[0] = a;
    shot->param_d[1] = b;
    shot->param_d[2] = c;

    shot->prev = set->pEnemyShotHead->prev;
    shot->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = shot;
    set->pEnemyShotHead->prev = shot;
}

static void SpawnPopup(sEnemyShotSet* set)
{
    const double w = 145.0;
    const double h = 92.0;

    // 枠：上・下
    for (int i = 0; i < 15; i++) {
        double tx = -w * 0.5 + w * i / 14.0;

        AddPopupShot(set,
            tx, -h * 0.5, img_enemyShotMediumBall[1],
            0.0, 0.0, 0, tx, -h * 0.5, 0.0);

        AddPopupShot(set,
            tx, h * 0.5, img_enemyShotMediumBall[1],
            0.0, 0.0, 0, tx, h * 0.5, 0.0);
    }

    // 枠：左・右
    for (int i = 1; i < 8; i++) {
        double ty = -h * 0.5 + h * i / 8.0;

        AddPopupShot(set,
            -w * 0.5, ty, img_enemyShotMediumBall[1],
            0.0, 0.0, 0, -w * 0.5, ty, 0.0);

        AddPopupShot(set,
            w * 0.5, ty, img_enemyShotMediumBall[1],
            0.0, 0.0, 0, w * 0.5, ty, 0.0);
    }

    // 閉じる「×」を右上へ配置
    const double cx = w * 0.5 - 15.0;
    const double cy = -h * 0.5 + 15.0;

    for (int i = 0; i < 3; i++) {
        double d = -9.0 + i * 9.0;
        AddPopupShot(set,
            cx + d, cy + d, img_enemyShotDiamond[5],
            0.0, 0.0, 1, d, d, 0.0);

        AddPopupShot(set,
            cx + d, cy - d, img_enemyShotDiamond[5],
            0.0, 0.0, 1, d, -d, 0.0);
    }

    // 広告本文を模した短い横線
    for (int row = 0; row < 4; row++) {
        int len = (row == 0) ? 8 : 6;
        double yy = -22.0 + row * 13.0;
        double span = (len - 1) * 11.0;

        for (int i = 0; i < len; i++) {
            double xx = -span * 0.5 + i * 11.0;

            AddPopupShot(set,
                xx, yy, img_enemyShotSmallBall[7],
                0.0, 0.0, 2, xx, yy, (double)row);
        }
    }

    // 「今すぐクリック」ボタンを模した横長配置
    for (int i = 0; i < 7; i++) {
        double xx = -33.0 + i * 11.0;
        AddPopupShot(set,
            xx, 30.0, img_enemyShotSmallBall[8],
            0.0, 0.0, 2, xx, 30.0, 4.0);
    }

    // 各弾の基準位置をセット座標へ変換
    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        shot->param_d[3] = set->x;
        shot = shot->next;
    }
}

static void ShotPopup(sEnemyShotSet* set)
{
    if (set->count == 0) {
        SpawnPopup(set);

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    const double phase = (double)set->count;

    // ポップアップ自体を少し揺らして、固定画像ではない嫌らしさを出す
    const double shakeX = 2.8 * sin(phase * 0.16);
    const double shakeY = 1.8 * sin(phase * 0.21 + 1.4);

    sEnemyShot* shot = set->pEnemyShotHead->next;

    while (shot != set->pEnemyShotHead) {
        const int role = shot->param_i[0];
        const double localX = shot->param_d[0];
        const double localY = shot->param_d[1];

        if (set->count < 72) {
            // 生成直後は広告枠が「開く」ように外側から収束
            const double k = 1.35 - 0.35 * (double)set->count / 72.0;

            shot->x = set->x + shakeX + localX * k;
            shot->y = set->y + shakeY + localY * k;
            shot->muki = 0.0;
            shot->speed = 0.0;
        }
        else if (set->count < 125) {
            // いったん広告として停滞
            shot->x = set->x + shakeX + localX;
            shot->y = set->y + shakeY + localY;
            shot->speed = 0.0;
        }
        else {
            const int t = set->count - 125;

            if (shot->param_i[0] < 3) {
                // 枠と×は広告閉鎖時に放射状へ飛び散る
                const double sx = set->x + localX;
                const double sy = set->y + localY;

                const double dx = localX;
                const double dy = localY;
                const double len = sqrt(dx * dx + dy * dy) + 0.001;

                shot->x = sx + dx / len * (double)t * 2.4;
                shot->y = sy + dy / len * (double)t * 2.4;
                shot->muki = atan2(dy, dx);
                shot->speed = 2.4;
            }
            else {
                // 本文は中心へ吸い込まれてから、一斉に外向きへ反転
                const double bx = set->x + localX;
                const double by = set->y + localY;

                if (t < 28) {
                    const double k = 1.0 - 0.9 * (double)t / 28.0;
                    shot->x = set->x + localX * k;
                    shot->y = set->y + localY * k;
                }
                else {
                    const double tt = (double)(t - 28);
                    const double len = sqrt(localX * localX + localY * localY) + 0.001;

                    shot->x = set->x + localX * (-0.15 + tt * 0.060);
                    shot->y = set->y + localY * (-0.15 + tt * 0.060);
                    shot->muki = atan2(localY, localX);
                    shot->speed = 3.6;
                }

                (void)bx;
                (void)by;
            }
        }

        shot = shot->next;
    }

    // 閉じた広告から追撃弾を大量放出
    //if (set->count >= 145 && set->count % 8 == 1) {
    //    const double base = set->muki;
    //    for (int i = 0; i < 18; i++) {
    //        const double a =
    //            base + DX_PI * 2.0 * (double)i / 18.0
    //            + 0.035 * (double)(set->count - 145);

    //        AddPopupShot(
    //            set,
    //            set->x, set->y,
    //            img_enemyShotSmallBall[i % 9],
    //            a, 2.2 + 0.12 * (i % 4),
    //            4, 0.0, 0.0, a);
    //    }

    //    if (CheckSoundMem(sound_enemyShot_light))
    //        StopSoundMem(sound_enemyShot_light);
    //    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    //}

    // 半周期ごとに「別の広告」を開くための向きを変える
    set->muki += 0.006;
}

void EnemyPat_PopUpAds_ChatGPT()
{
    static int moveDir;
    static int popupCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;

        moveDir = 1;
        popupCount = 0;
    }
    else {
        enemy.x += 0.82 * (double)moveDir;

        if (enemy.x < 75.0 || enemy.x > 405.0)
            moveDir *= -1;
    }

    // ボスの移動に合わせて広告発生位置を左右へ散らす
    if (count % 58 == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;

        set->count = 0;
        set->patternFunc = ShotPopup;

        // 画面内へ確実に収まる範囲で広告窓を出す
        const int xoff = GetRand(220) - 110;
        const int yoff = GetRand(145) + 70;

        set->x = enemy.x + xoff;
        if (set->x < 88.0)  set->x = 88.0;
        if (set->x > 392.0) set->x = 392.0;

        set->y = enemy.y + yoff;
        if (set->y < 95.0)  set->y = 95.0;
        if (set->y > 360.0) set->y = 360.0;

        set->muki =
            atan2(player.y - set->y, player.x - set->x)
            + ((double)GetRand(120) - 60.0) / 180.0 * DX_PI;

        set->kind = popupCount++;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // 終盤は広告の出現頻度をさらに上げる
    if (count > 600 && count % 71 == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;

        set->count = 0;
        set->patternFunc = ShotPopup;

        set->x = 70.0 + GetRand(340);
        set->y = 110.0 + GetRand(260) - 60;
        set->muki =
            atan2(player.y - set->y, player.x - set->x)
            + ((double)GetRand(90) - 45.0) / 180.0 * DX_PI;

        set->kind = popupCount++;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}
