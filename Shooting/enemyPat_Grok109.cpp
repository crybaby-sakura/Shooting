// enemyPat_Tmp.cpp
// バラ曲線（ローズ曲線）をモチーフにした弾幕「開花ローズ」

// 弾幕パターン：開花ローズ（Blooming Rose）
// k=5 の5弁バラが中心から徐々に開き、回転しながら外側へ拡散する
static void ShotRoseBloom(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 開始時に予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 開花・生成フェーズ（約3秒間、徐々に半径を広げながら生成）
    // count はメインルーチンで毎フレーム+1される
    if (pEnemyShotSet->count < 180) {
        // 4フレームごとに1層生成（弾数を抑えつつ滑らかに）
        if (pEnemyShotSet->count % 6 == 0) {
            // 軽い発射音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            
            // 半径 a を徐々に大きくする（蕾→満開）
            double a = 15.0 + pEnemyShotSet->count * 0.85;
            if (a > 160.0) a = 160.0;

            // 全体をゆっくり回転させるオフセット
            double rot = pEnemyShotSet->count * 0.025;

            // バラ曲線のパラメータ（奇数なので5弁）
            const double k = 5.0;
            const int numPoints = 36;  // 曲線を滑らかに描く点数

            for (int i = 0; i < numPoints; i++) {
                double theta = (double)i / numPoints * 2.0 * DX_PI + rot;
                double r = a * abs(cos(k * theta));  // バラ曲線

                // 位置計算（rが負の場合も正しく反対側に配置される）
                double dx = r * cos(theta);
                double dy = r * sin(theta);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + dx;
                pEnemyShot->y = pEnemyShotSet->y + dy;

                // 中心から外側へ向かう向き（放射状に拡散）
                pEnemyShot->muki = atan2(dy, dx);

                // 外側ほど少し速くして「開いて散る」感じを出す
                pEnemyShot->speed = 0.55 + (a / 280.0);

                // 小玉を使用。色は角度に応じて変化させて花びら感を強調
                // 色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
                int colorIndex = ((int)((theta + rot) / (2.0 * DX_PI / k)) % 6 + 6) % 6;
                // 花びらごとに赤〜マゼンタ系を多めに（視覚的にバラらしく）
                if (colorIndex > 5) colorIndex = colorIndex % 6;  // 0-5に抑える
                pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];

                // リンクリストに追加（サンプルと同じ方式）
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 既存弾の移動（メインルーチンで画面外消去とcount++が行われる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // わずかに加速させて「散る」勢いを追加
        pShot->speed += 0.008;
        pShot = pShot->next;
    }
}

// 敵本体のパターン
// 新しく作成する場合、名前は void EnemyPat_RoseCurve_Grok() にすること。
void EnemyPat_RoseCurve_Grok()
{
    static int moveDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        moveDir = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.6 * (double)moveDir;
        if (enemy.x < 80.0 || enemy.x > 400.0) {
            moveDir *= -1;
        }
    }

    // パターン開始（少し待機してから）
    // 一度だけ大きな開花を発生させ、その後一定間隔で繰り返す
    if (count % 210 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRoseBloom;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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