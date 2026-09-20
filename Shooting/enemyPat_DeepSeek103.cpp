// enemyPat_Tmp.cpp
// 回転バー「クルクルリン・スティック」パターン

// 回転バーのパターン関数
// 小玉を直線状に並べた棒を回転させ、棒に空いた隙間をすり抜ける弾幕。
// pEnemyShotSet->param_d[0] : 回転速度（度/フレーム）
// pEnemyShotSet->param_d[1] : セグメント間の隙間サイズ（px）
// pEnemyShotSet->param_d[2] : 初期角度オフセット（度）
static void RotatingBarPattern(sEnemyShotSet* pSet)
{
    // 中心をボスの現在位置に追従させる
    pSet->x = enemy.x;
    pSet->y = enemy.y;

    // 初回フレームのみ弾を生成
    if (pSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 棒のパラメータ
        const int shotsPerSegment = 8;      // 各セグメントの弾数
        const double spacing = 4.0;         // 弾の間隔（px）
        const double gapSize = pSet->param_d[1]; // セグメント間の隙間（px）
        const double startDistance = 20.0;  // 中心から最初の弾までの距離（ボス本体と重ならないように）

        // 右側（正）の距離を生成（3セグメント分）
        const int N = 5;
        double distances[shotsPerSegment * N];
        int idx = 0;
        double current = startDistance;
        for (int seg = 0; seg < N; seg++) {
            for (int i = 0; i < shotsPerSegment; i++) {
                distances[idx++] = current;
                current += spacing;
            }
            current += gapSize; // セグメント間の隙間
        }

        // 左右対称に弾を生成
        for (int side = 0; side < 2; side++) {
            double sign = (side == 0) ? 1.0 : -1.0;
            for (int i = 0; i < shotsPerSegment * N; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->count = 0;
                // 小玉を使用、色は pSet->kind % 8 で指定
                pShot->kind = img_enemyShotSmallBall[pSet->kind % 8];
                // 距離（符号付き）を保存
                pShot->param_d[0] = sign * distances[i];
                // 初期角度オフセットを保存
                pShot->param_d[1] = pSet->param_d[2];
                pShot->margin = 480;

                // リストに追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 毎フレーム、回転角度を更新し各弾の位置を再計算
    double angleDeg = pSet->count * pSet->param_d[0]; // 基本角度
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 個々のオフセットを加えた角度（度）
        double totalAngle = angleDeg + pShot->param_d[1];
        double angleRad = totalAngle * DX_PI / 180.0;
        double dist = pShot->param_d[0];
        // 中心座標から回転後の位置を計算
        pShot->x = pSet->x + dist * cos(angleRad);
        pShot->y = pSet->y + dist * sin(angleRad);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KuruKuruKururin_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ボスを画面中央に配置
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.5 * muki;
        if (enemy.x < 60 || enemy.x > 420) muki *= -1;
    }

    // ---- 第1段階：回転バー1本（45度/秒、隙間20px） ----
    if (count == 60) { // 1秒後
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = RotatingBarPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0; // 赤
        pSet->param_d[0] = 0.75; // 45度/秒（60fps想定）
        pSet->param_d[1] = 20.0; // 隙間20px
        pSet->param_d[2] = 0.0;  // オフセット0度

        // 弾リスト初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ---- 第2段階：6秒後に2本目のバーを90度オフセットで追加（回転速度アップ、隙間15px） ----
    if (count == 360) { // 6秒後
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = RotatingBarPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 4; // 青
        pSet->param_d[0] = 0.9;  // 54度/秒
        pSet->param_d[1] = 15.0; // 隙間15px
        pSet->param_d[2] = 90.0; // 90度オフセット（十字回転）

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ---- 第3段階：11秒後に3本目のバーを追加（さらに高速、隙間10px） ----
    if (count == 660) { // 11秒後
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = RotatingBarPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 6; // 白
        pSet->param_d[0] = 1.1;  // 66度/秒
        pSet->param_d[1] = 10.0; // 隙間10px
        pSet->param_d[2] = 45.0; // 45度オフセット（斜め方向のバー）

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}