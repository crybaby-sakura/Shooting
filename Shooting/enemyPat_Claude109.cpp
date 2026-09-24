// enemyPat_roseKaleidoscope.cpp
// バラ曲線モチーフ弾幕「万華鏡薔薇」
//
// r(θ,t) = a(t) * cos(k(t)*θ) を土台に、花弁数k(3〜7を往復)と
// 半径スケールa(呼吸)を時間変化させる「呼吸する花」弾幕。
// ・輪郭トレース弾: 曲線上の点から外向きに連続パルス発射
// ・花弁先端スプレー: 各花弁の先端から扇状バースト
// ・回転レーザースポーク: フェーズ2で中心から放射状に回転する十字レーザー

static constexpr double kPI = 3.14159265358979323846;

// 色パレット（花弁の色を虹色に巡回させる）
static int RoseColorKind(int colorIdx)
{
    static const int palette[] = {
        img_enemyShotSmallBall[0], // 赤
        img_enemyShotSmallBall[8], // 橙
        img_enemyShotSmallBall[1], // 黄
        img_enemyShotSmallBall[2], // 緑
        img_enemyShotSmallBall[3], // シアン
        img_enemyShotSmallBall[4], // 青
        img_enemyShotSmallBall[5], // マゼンタ
    };
    int idx = colorIdx % 7;
    if (idx < 0) idx += 7;
    return palette[idx];
}

// 現在のグローバルcountから花弁パラメータk・半径スケールaを計算
// 1800フレーム周期: 前半900F=k 3→7上昇、後半900F=k 7→3下降(フェーズ2)
static void CalcRoseParams(int globalCount, double* outK, double* outA)
{
    double phaseT = fmod((double)(globalCount - 1), 1800.0);
    double k;
    if (phaseT < 900.0) {
        k = 3.0 + 4.0 * (phaseT / 900.0);
    }
    else {
        k = 7.0 - 4.0 * ((phaseT - 900.0) / 900.0);
    }
    *outK = k;
    *outA = 130.0 + 20.0 * sin(2.0 * kPI * globalCount / 240.0); // 呼吸するように半径が脈動
}

static bool IsRosePhase2(int globalCount)
{
    double phaseT = fmod((double)(globalCount - 1), 1800.0);
    return phaseT >= 900.0;
}

// 弾幕：バラ曲線 輪郭トレース
// 曲線上の各点から、その点の角度方向へそのまま外向きに飛ばすことで
// 花が開いていくような軌跡になる。数フレームおきに位相をずらして連続発射。
static void ShotRoseContour(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        double k = pEnemyShotSet->param_d[0];
        double a = pEnemyShotSet->param_d[1];
        double phaseOffset = pEnemyShotSet->param_d[2];
        int colorIdx = pEnemyShotSet->kind;

        const int N = 24; // 1バーストあたりの弾数
        for (int i = 0; i < N; i++) {
            double theta = 2.0 * kPI * i / N + phaseOffset;
            double r = a * cos(k * theta);
            double angle = theta;
            if (r < 0.0) {
                // バラ曲線は負の半径で反対方向に折り返して描画される
                r = -r;
                angle += kPI;
            }

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + r * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + r * sin(angle);
            pEnemyShot->muki = angle; // 曲線の外向き方向へそのまま飛ばす
            pEnemyShot->speed = 1.6 + 0.9 * (r / (a + 0.001)); // 花弁の外側ほど速く
            pEnemyShot->kind = RoseColorKind(colorIdx + i / 4);

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：花弁先端スプレー
// 花弁の先端(おおよその位置)から扇状に弾を撃ち出す。輪郭トレースの隙間を埋める役割。
static void ShotRoseTip(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        double k = pEnemyShotSet->param_d[0];
        double a = pEnemyShotSet->param_d[1];
        int tips = (int)(k + 0.5);
        if (tips < 3) tips = 3;

        for (int t = 0; t < tips; t++) {
            double angle = 2.0 * kPI * t / tips;
            double baseX = pEnemyShotSet->x + a * cos(angle);
            double baseY = pEnemyShotSet->y + a * sin(angle);

            const int FAN = 5;
            for (int f = 0; f < FAN; f++) {
                double spreadAngle = angle + (f - (FAN - 1) / 2.0) * (10.0 * kPI / 180.0);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = baseX;
                pEnemyShot->y = baseY;
                pEnemyShot->muki = spreadAngle;
                pEnemyShot->speed = 2.6;
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白で先端を強調

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：回転レーザースポーク(フェーズ2専用)
// 中心から放射状に伸びるレーザーの列(スポーク)を複数本、回転させる。
// 位置は pShot->count からの純粋な式で計算する(速度積分ではなく毎フレーム再計算)。
static void ShotRoseLaserSpoke(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        const int SPOKES = 4;
        const int SEGMENTS = 6;
        double baseAngle0 = pEnemyShotSet->param_d[0];
        int rotDir = pEnemyShotSet->param_i[0];

        for (int s = 0; s < SPOKES; s++) {
            double baseAngle = baseAngle0 + 2.0 * kPI * s / SPOKES;
            for (int seg = 1; seg <= SEGMENTS; seg++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->param_d[0] = baseAngle;   // 初期角度
                pEnemyShot->param_d[1] = seg * 24.0;  // 中心からの固定距離
                pEnemyShot->param_i[0] = rotDir;      // 回転方向(+1/-1)
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle;
                pEnemyShot->speed = 0.0; // 移動は下のformulaで直接位置決定するため未使用
                pEnemyShot->kind = img_enemyShotLaser[4]; // 青

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    const double ANGULAR_SPEED = 0.015; // 1フレームあたりの回転角
    const int LIFETIME = 900;           // フェーズ2の長さに合わせて自動消滅させる

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count < LIFETIME) {
            double angle = pShot->param_d[0] + ANGULAR_SPEED * pShot->param_i[0] * pShot->count;
            double dist = pShot->param_d[1];
            pShot->x = pEnemyShotSet->x + dist * cos(angle);
            pShot->y = pEnemyShotSet->y + dist * sin(angle);
            pShot->muki = angle;
        }
        else {
            // 寿命が来たら画面外へ飛ばし、メインルーチンの画面外削除に任せる
            pShot->x = -1000.0;
            pShot->y = -1000.0;
        }
        pShot = pShot->next;
    }
}

// sEnemyShotSetを生成してグローバルリストに連結する共通処理
static sEnemyShotSet* CreateRoseShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// 敵本体のパターン
void EnemyPat_RoseCurve_Claude()
{
    static double swayDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200;
        swayDir = 1.0;
    }
    else {
        enemy.x += 0.3 * swayDir;
        if (count % 300 == 150) swayDir *= -1.0;
    }

    double k, a;
    CalcRoseParams(count, &k, &a);
    bool phase2 = IsRosePhase2(count);

    // 輪郭トレース: 6フレームごとに連続パルス発射(位相をずらして隙間を埋める)
    if (count % 6 == 1) {
        sEnemyShotSet* pSet = CreateRoseShotSet(ShotRoseContour, enemy.x, enemy.y);
        pSet->param_d[0] = k;
        pSet->param_d[1] = a;
        pSet->param_d[2] = (count / 6) * 0.15;
        pSet->kind = (int)(k * 2.0 + 0.5);
    }

    // 花弁先端スプレー: 20フレームごと
    if (count % 20 == 1) {
        sEnemyShotSet* pSet = CreateRoseShotSet(ShotRoseTip, enemy.x, enemy.y);
        pSet->param_d[0] = k;
        pSet->param_d[1] = a;
    }

    // 回転レーザースポーク: フェーズ2開始時に1回だけ生成
    if (phase2 && (count % 1800 == 901)) {
        sEnemyShotSet* pSet = CreateRoseShotSet(ShotRoseLaserSpoke, enemy.x, enemy.y);
        pSet->param_d[0] = GetRand(628) / 100.0;      // 初期角度をランダム化(0〜2π相当)
        pSet->param_i[0] = (GetRand(1) == 0) ? 1 : -1; // 回転方向をランダム化
    }
}