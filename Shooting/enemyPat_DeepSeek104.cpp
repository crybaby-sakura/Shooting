// enemyPat_tmp.cpp
// ブラジリアンワックスをモチーフにした弾幕パターン

// 色インデックス定数
enum {
    COLOR_RED = 0,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_WHITE,
    COLOR_BLACK,
    COLOR_ORANGE
};

// ショット生成ヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = 240;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 線形移動（各フレームで呼ぶ）
static void MoveLinear(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//-----------------------------------------------
// パターン1: V字ライン形成
//-----------------------------------------------
static void PatternVLine(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 頂点 (240, 340)、左右端点 (100,40), (380,40)
        const double vx = 240.0, vy = 340.0;
        const double lx = 100.0, ly = 40.0;
        const double rx = 380.0, ry = 40.0;

        const int numPerArm = 12;

        // 左腕
        for (int i = 0; i < numPerArm; ++i) {
            double t = (double)i / (numPerArm - 1);
            double x = lx + (vx - lx) * t;
            double y = ly + (vy - ly) * t;
            AddShot(pSet, x, y, DX_PI / 2.0, 0.5, img_enemyShotSmallBall[COLOR_MAGENTA]);
        }
        // 右腕
        for (int i = 0; i < numPerArm; ++i) {
            double t = (double)i / (numPerArm - 1);
            double x = rx + (vx - rx) * t;
            double y = ry + (vy - ry) * t;
            AddShot(pSet, x, y, DX_PI / 2.0, 0.5, img_enemyShotSmallBall[COLOR_MAGENTA]);
        }

        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    MoveLinear(pSet);
}

//-----------------------------------------------
// パターン2: ワックス充填（オレンジ大玉）
//-----------------------------------------------
static void PatternWaxFill(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const double vx = 240.0, vy = 340.0;
        const double leftSlope = (100.0 - vx) / (40.0 - vy); // dx/dy
        const double rightSlope = (380.0 - vx) / (40.0 - vy);

        const int numBalls = 10;
        for (int i = 0; i < numBalls; ++i) {
            double y = 100.0 + GetRand(200); // 100～300
            double halfWidth = (vy - y) * leftSlope; // 左腕とのx差（正）
            double minX = vx - halfWidth;
            double maxX = vx + halfWidth;
            double x = minX + GetRand((int)(maxX - minX));
            double speed = 0.3 + GetRand(20) / 100.0; // 0.3～0.5
            double muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI; // ほぼ下向き
            AddShot(pSet, x, y, muki, speed, img_enemyShotLargeBall[COLOR_ORANGE]);
        }

        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    MoveLinear(pSet);
}

//-----------------------------------------------
// パターン3: ストリップ貼付（白い横長弾）
//-----------------------------------------------
static void PatternStripApply(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int numStrips = 3;
        const int numPerStrip = 20;
        const double stripY[3] = { 150.0, 200.0, 250.0 };

        for (int s = 0; s < numStrips; ++s) {
            for (int i = 0; i < numPerStrip; ++i) {
                double x = -20.0 + i * 15.0; // 左端から間隔15で並べる
                double y = stripY[s];
                // 右方向へ高速移動
                AddShot(pSet, x, y, 0.0, 6.0 / 2, img_enemyShotBullet[COLOR_WHITE]);
            }
        }

        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    MoveLinear(pSet);
}

//-----------------------------------------------
// パターン4: リップオフ（剥がし）
//-----------------------------------------------
static void PatternRipOff(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 白い縦列（布）を下から上へ
        const int numCol = 20;
        const double colX = 240.0;
        for (int i = 0; i < numCol; ++i) {
            double y = 480.0 + i * 15.0; // 画面下外から
            AddShot(pSet, colX, y, -DX_PI / 2.0, 4.0, img_enemyShotBullet[COLOR_WHITE]);
        }

        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 剥がれに伴うワックス片（黄色小玉）と痛み（赤弾）
    if (pSet->count >= 0 && pSet->count < 30 && pSet->count % 2 == 0) {
        double stripY = 480.0 - 4.0 * pSet->count; // 現在の布のY位置
        // ワックス片：左右へ扇状に飛散
        for (int i = 0; i < 6; ++i) {
            double angle = (GetRand(120) - 60) / 180.0 * DX_PI; // -60°～60°
            if (i % 2 == 0) angle = DX_PI - angle; // 左右対称に
            double speed = 2.0 + GetRand(100) / 100.0; // 2.0～3.0
            AddShot(pSet, 240.0, stripY, angle, speed, img_enemyShotSmallBall[COLOR_YELLOW]);
        }

        // 赤い針弾：自機狙い
        double aim = atan2(player.y - stripY, player.x - 240.0);
        AddShot(pSet, 240.0, stripY, aim, 3.5, img_enemyShotBullet[COLOR_RED]);
    }

    MoveLinear(pSet);
}

//-----------------------------------------------
// パターン5: スムースフィニッシュ（波形）
//-----------------------------------------------
static void PatternSmoothFinish(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 左右から蛇行する黄色小玉
        const int numWaves = 8;
        for (int i = 0; i < numWaves; ++i) {
            double baseY = 100.0 + i * 30.0;
            // 左側から
            for (int j = 0; j < 12; ++j) {
                double x = -20.0 - j * 10.0;
                double y = baseY;
                double muki = 0.0; // 右向き
                double speed = 3.0 + (i % 3) * 0.5;
                AddShot(pSet, x, y, muki, speed, img_enemyShotSmallBall[COLOR_YELLOW]);
            }
            // 右側から
            for (int j = 0; j < 12; ++j) {
                double x = 500.0 + j * 10.0;
                double y = baseY;
                double muki = DX_PI; // 左向き
                double speed = 3.0 + (i % 3) * 0.5;
                AddShot(pSet, x, y, muki, speed, img_enemyShotSmallBall[COLOR_YELLOW]);
            }
        }

        // 自機狙いの赤弾を数発
        for (int i = 0; i < 10; ++i) {
            double aim = atan2(player.y - 240.0, player.x - 240.0);
            AddShot(pSet, 240.0, 300.0, aim + (GetRand(20) - 10) / 180.0 * DX_PI, 3.0, img_enemyShotBullet[COLOR_RED]);
        }

        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 波形移動を適用（黄色小玉のみ正弦波で揺らす）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 黄色小玉の場合、y座標に正弦波を加える
        if (pShot->kind == img_enemyShotSmallBall[COLOR_YELLOW]) {
            // 元の移動は線形なので、それに加えて揺らす（簡易的に）
            pShot->y += sin((pShot->x + pSet->count * 2.0) * 0.05) * 1.5;
        }
        // 通常の線形移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//-----------------------------------------------
// ショットセット生成ヘルパー
//-----------------------------------------------
static void CreateShotSet(void (*patternFunc)(sEnemyShotSet*), double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

//-----------------------------------------------
// 敵本体パターン
//-----------------------------------------------
void EnemyPat_BrazilianWax_DeepSeek()
{
    static int muki;          // 敵の左右移動方向
    static bool initialized;  // 初期化フラグ

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        initialized = true;
    }

    // 敵の水平移動（サンプルと同様）
    if (initialized && count > 1) {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    const int T = 400;
    int countT = count % T;

    // タイムラインに沿ってショットセットを生成
    if (countT == 30) {
        // V字ライン
        CreateShotSet(PatternVLine, enemy.x, enemy.y, 0.0, 0);
    }
    else if (countT == 90) {
        // ワックス充填
        CreateShotSet(PatternWaxFill, enemy.x, enemy.y + 10.0, 0.0, 1);
    }
    else if (countT == 150) {
        // ストリップ貼付
        CreateShotSet(PatternStripApply, enemy.x, enemy.y + 10.0, 0.0, 2);
    }
    else if (countT == 210) {
        // リップオフ
        CreateShotSet(PatternRipOff, enemy.x, enemy.y + 10.0, 0.0, 3);
    }
    else if (countT == 300) {
        // スムースフィニッシュ
        CreateShotSet(PatternSmoothFinish, enemy.x, enemy.y + 10.0, 0.0, 4);
    }
}