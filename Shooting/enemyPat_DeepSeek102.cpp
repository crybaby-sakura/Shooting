// enemyPat_tmp.cpp
// ポップアップ広告風弾幕パターン

// 弾種別・色の定数
#define SHOT_SMALL_RED      img_enemyShotSmallBall[0]
#define SHOT_SMALL_YELLOW   img_enemyShotSmallBall[1]
#define SHOT_SMALL_BLUE     img_enemyShotSmallBall[4]
#define SHOT_MEDIUM_RED     img_enemyShotMediumBall[0]
#define SHOT_MEDIUM_YELLOW  img_enemyShotMediumBall[1]
#define SHOT_MEDIUM_BLUE    img_enemyShotMediumBall[4]
#define SHOT_BULLET_BLUE    img_enemyShotBullet[4]

// フォント定義（5x5 ドット）
static const char* font_W[5] = {
    "1...1",
    "1...1",
    "1...1",
    "1.1.1",
    ".1.1."
};
static const char* font_A[5] = {
    "..1..",
    ".1.1.",
    "1...1",
    "11111",
    "1...1"
};
static const char* font_R[5] = {
    "1111.",
    "1...1",
    "1111.",
    "1.1..",
    "1..1."
};
static const char* font_N[5] = {
    "1...1",
    "11..1",
    "1.1.1",
    "1..11",
    "1...1"
};
static const char* font_I[5] = {
    "11111",
    "..1..",
    "..1..",
    "..1..",
    "11111"
};
static const char* font_G[5] = {
    ".111.",
    "1...1",
    "1....",
    "1.111",
    ".1111"
};

// 弾を追加するヘルパー
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int param0 = 0)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = param0;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// メインのポップアップ広告パターン
static void PopupAdPattern(sEnemyShotSet* pSet)
{
    // ポップアップの中心座標
    double cx = pSet->x;
    double cy = pSet->y;
    double hw = 130.0; // 横幅の半分
    double hh = 90.0;  // 縦幅の半分

    // 初回生成：枠・文字・ボタンなどを静止状態で配置
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 枠（上・下は赤、左・右は青）
        for (double x = cx - hw; x <= cx + hw; x += 10.0) {
            AddEnemyShot(pSet, x, cy - hh, 0, 0, SHOT_SMALL_RED, 1);  // 上辺
            AddEnemyShot(pSet, x, cy + hh, 0, 0, SHOT_SMALL_RED, 1);  // 下辺
        }
        for (double y = cy - hh; y <= cy + hh; y += 10.0) {
            AddEnemyShot(pSet, cx - hw, y, 0, 0, SHOT_SMALL_BLUE, 1); // 左辺
            AddEnemyShot(pSet, cx + hw, y, 0, 0, SHOT_SMALL_BLUE, 1); // 右辺
        }

        // 警告文字 "WARNING" を青い小粒でドット描画
        const char** letters[7] = { font_W, font_A, font_R, font_N, font_I, font_N, font_G };
        double startX = cx - 80.0;   // 文字列の開始X
        double startY = cy - 30.0;   // 文字列の上端Y
        double step = 6.0;           // ドット間隔
        for (int li = 0; li < 7; li++) {
            const char** letter = letters[li];
            for (int row = 0; row < 5; row++) {
                for (int col = 0; col < 5; col++) {
                    if (letter[row][col] == '1') {
                        double bx = startX + li * 5 * step + col * step;
                        double by = startY + row * step;
                        AddEnemyShot(pSet, bx, by, 0, 0, SHOT_SMALL_BLUE, 2);
                    }
                }
            }
        }

        // ×ボタン（右上、黄色中玉4つでX形）
        double xbx = cx + hw - 20.0;
        double xby = cy - hh + 20.0;
        AddEnemyShot(pSet, xbx - 8, xby - 8, 0, 0, SHOT_MEDIUM_YELLOW, 3);
        AddEnemyShot(pSet, xbx + 8, xby - 8, 0, 0, SHOT_MEDIUM_YELLOW, 3);
        AddEnemyShot(pSet, xbx - 8, xby + 8, 0, 0, SHOT_MEDIUM_YELLOW, 3);
        AddEnemyShot(pSet, xbx + 8, xby + 8, 0, 0, SHOT_MEDIUM_YELLOW, 3);
        AddEnemyShot(pSet, xbx + 0, xby + 0, 0, 0, SHOT_MEDIUM_YELLOW, 3);

        // OKボタン（左下、赤中玉の塊）
        double okX = cx - hw / 2.0;
        double okY = cy + hh - 25.0;
        pSet->param_d[0] = okX;
        pSet->param_d[1] = okY;
        for (double dx = -15.0; dx <= 15.0; dx += 10.0) {
            for (double dy = -5.0; dy <= 5.0; dy += 10.0) {
                AddEnemyShot(pSet, okX + dx, okY + dy, 0, 0, SHOT_MEDIUM_RED, 4);
            }
        }

        // キャンセルボタン（右下、青中玉の塊）
        double cancelX = cx + hw / 2.0;
        double cancelY = cy + hh - 25.0;
        pSet->param_d[2] = cancelX;
        pSet->param_d[3] = cancelY;
        for (double dx = -20.0; dx <= 20.0; dx += 10.0) {
            for (double dy = -5.0; dy <= 5.0; dy += 10.0) {
                AddEnemyShot(pSet, cancelX + dx, cancelY + dy, 0, 0, SHOT_MEDIUM_BLUE, 5);
            }
        }
    }

    // フェーズ処理（pSet->count はメインで毎フレーム加算される）
    if (pSet->count == 45) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 警告文字が自機狙い弾に変化
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 2) {
                p->speed = 180.0 / 100;
                p->muki = atan2(player.y - p->y, player.x - p->x);
            }
            p = p->next;
        }
    }

    if (pSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ×ボタンが分裂
        sEnemyShot* targets[16];
        int targetCount = 0;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead && targetCount < 16) {
            if (p->param_i[0] == 3) {
                targets[targetCount++] = p;
            }
            p = p->next;
        }
        for (int i = 0; i < targetCount; i++) {
            double sx = targets[i]->x;
            double sy = targets[i]->y;
            for (int j = 0; j < 8; j++) {
                double angle = j * (2.0 * DX_PI / 8.0);
                AddEnemyShot(pSet, sx, sy, angle, 150.0 / 100, SHOT_SMALL_RED, 6);
            }
            targets[i]->speed = 0; // 元の×は停止
        }
    }

    if (pSet->count == 120) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 枠が外側へ飛び散る
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {
                double dx = p->x - cx;
                double dy = p->y - cy;
                double len = sqrt(dx * dx + dy * dy);
                if (len > 0.1) {
                    p->muki = atan2(dy, dx);
                }
                p->speed = 120.0 / 100;
            }
            p = p->next;
        }
    }

    if (pSet->count == 150) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // OKボタンから扇形弾
        double okX = pSet->param_d[0];
        double okY = pSet->param_d[1];
        double baseAngle = atan2(player.y - okY, player.x - okX);
        for (int i = 0; i < 12; i++) {
            double angle = baseAngle + (i - 5.5) * 0.12;
            AddEnemyShot(pSet, okX, okY, angle, 160.0 / 100, SHOT_MEDIUM_RED, 6);
        }
    }

    if (pSet->count >= 180 && pSet->count <= 280 && (pSet->count % 10 == 0)) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // キャンセルボタンから低速追尾弾
        double cancelX = pSet->param_d[2];
        double cancelY = pSet->param_d[3];
        for (int i = 0; i < 2; i++) {
            double angle = atan2(player.y - cancelY, player.x - cancelX);
            AddEnemyShot(pSet, cancelX, cancelY, angle, 120.0 / 100, SHOT_SMALL_BLUE, 7);
        }
    }

    if (pSet->count == 300) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 巨大×を中央に出現させ、自機へ突進
        for (int i = -1; i <= 1; i += 2) {
            for (int j = -1; j <= 1; j += 2) {
                AddEnemyShot(pSet, cx + i * 40, cy + j * 40, 0, 0, SHOT_MEDIUM_YELLOW, 8);
                AddEnemyShot(pSet, cx + i * 20, cy + j * 20, 0, 0, SHOT_MEDIUM_YELLOW, 8);
            }
        }
        AddEnemyShot(pSet, cx, cy, 0, 0, SHOT_MEDIUM_YELLOW, 8);
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 8) {
                p->speed = 200.0 / 100;
                p->muki = atan2(player.y - p->y, player.x - p->x);
            }
            p = p->next;
        }
    }

    if (pSet->count == 360) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 巨大×が爆発（全方位＋自機狙い針弾）
        sEnemyShot* targets[64];
        int targetCount = 0;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead && targetCount < 64) {
            if (p->param_i[0] == 8) {
                targets[targetCount++] = p;
            }
            p = p->next;
        }
        for (int i = 0; i < targetCount; i++) {
            double sx = targets[i]->x;
            double sy = targets[i]->y;
            for (int j = 0; j < 12; j++) {
                double angle = j * (2.0 * DX_PI / 12.0);
                AddEnemyShot(pSet, sx, sy, angle, 150.0 / 100, SHOT_SMALL_RED, 6);
            }
            // 自機狙いの針弾
            double aim = atan2(player.y - sy, player.x - sx);
            AddEnemyShot(pSet, sx, sy, aim, 220.0 / 100, SHOT_BULLET_BLUE, 6);
            targets[i]->speed = 0;
        }
    }

    // 全弾の移動と、追尾弾（param_i[0]==7）のホーミング
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 7) {
            p->muki = atan2(player.y - p->y, player.x - p->x);
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 敵本体のパターン
void EnemyPat_PopUpAds_DeepSeek()
{
    static int muki;
    static int popupCount;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        popupCount = 0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ポップアップ広告を発生させるタイミング
    if (count % 300 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = PopupAdPattern;

        // 出現位置を少しずらす
        if (popupCount == 0) {
            pSet->x = 240.0;
            pSet->y = 240.0;
        }
        else if (popupCount == 1) {
            pSet->x = 160.0;
            pSet->y = 170.0;
        }
        else {
            pSet->x = 320.0;
            pSet->y = 170.0;
        }
        popupCount++;
        popupCount %= 3;

        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}