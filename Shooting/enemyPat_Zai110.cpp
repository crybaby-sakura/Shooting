// enemyPat_tmp.cpp
// 弾幕「投環・千環繚乱（せんかんりょうらん）」: 輪投げモチーフ
//
// 使用素材:
//   弾種: 中玉(7x7)=輪の縁、小玉(2.5x2.5)=ピン(輪投げの棒)
//   色:   0:赤 / 1:黄 / 6:白 (輪の色で縮小の速さ=難易度を示す)
//   音:   sound_enemyCharge=投げ予告 / sound_enemyShot_heavy=輪射出 / sound_enemyShot_light=ピン設置

// ---------------------------------------------------------
// 輪のスペック表 [type: 0=赤, 1=黄, 2=白]
//   speed : 輪全体の飛行速度
//   shrink: 1フレームあたりの半径縮小量
// （いずれも自機付近に届く頃に輪が小さくなるよう調整）
// ---------------------------------------------------------
static const double RingSpeed[3] = { 1.8, 2.2, 1.6 };
static const double RingShrink[3] = { 0.32, 0.39, 0.31 };
static const int   RingColor[3] = { 0, 1, 6 };   // 赤 / 黄 / 白

#define RING_RADIUS0   80.0   // 輪の初期半径
#define RING_SEGMENTS  16     // 輪を構成する弾の数
#define RING_ROT_SPD   0.02   // 輪の回転速度(rad/frame)

static void ShotRing(sEnemyShotSet* pEnemyShotSet);

// ---------------------------------------------------------
// 輪の射出（sEnemyShotSet を1つ作って登録するヘルパ）
// ---------------------------------------------------------
static void FireRing(int type, double muki, double speed)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotRing;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 20.0;
    pEnemyShotSet->muki = muki;          // 輪全体の進行方向（自機狙い）
    pEnemyShotSet->kind = type;          // 輪の色タイプ
    pEnemyShotSet->param_d[0] = speed;   // 輪全体の速度

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ---------------------------------------------------------
// 弾幕: 輪（中心が空洞のリング。回転しながら飛び、徐々に縮小する）
// ---------------------------------------------------------
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < RING_SEGMENTS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 初期位置は輪の縁上
            double a = i * 2.0 * DX_PI / RING_SEGMENTS;
            pEnemyShot->x = pEnemyShotSet->x + RING_RADIUS0 * cos(a);
            pEnemyShot->y = pEnemyShotSet->y + RING_RADIUS0 * sin(a);
            pEnemyShot->muki = 0.0;   // 位置は毎フレーム再計算するので未使用
            pEnemyShot->speed = 0.0;

            pEnemyShot->kind = img_enemyShotMediumBall[RingColor[pEnemyShotSet->kind % 3]];

            pEnemyShot->param_d[0] = a;   // 輪の縁上での初期角度

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 輪の中心位置（発射地点から等速直線移動） ----
    double cx = pEnemyShotSet->x
        + pEnemyShotSet->count * pEnemyShotSet->param_d[0] * cos(pEnemyShotSet->muki);
    double cy = pEnemyShotSet->y
        + pEnemyShotSet->count * pEnemyShotSet->param_d[0] * sin(pEnemyShotSet->muki);

    double radius = RING_RADIUS0 - RingShrink[pEnemyShotSet->kind % 3] * pEnemyShotSet->count;
    double rot = pEnemyShotSet->count * RING_ROT_SPD;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (radius <= 10.0) {
            // 輪が閉じきったら画面外へ退避（メインルーチンが消去する）
            pShot->x = -1000.0;
            pShot->y = -1000.0;
        }
        else {
            // 回転しながら縮小する輪の縁上に配置
            double a = pShot->param_d[0] + rot;
            pShot->x = cx + radius * cos(a);
            pShot->y = cy + radius * sin(a);
        }
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕: ピン（輪投げの棒。白い小玉の縦列がゆっくり降りてくる）
// ---------------------------------------------------------
static void ShotPins(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 4列 x 6個のピン。列の間隔120pxなので、その間が回避スペース
        for (int col = 0; col < 4; col++) {
            double px = 60.0 + col * 120.0;
            for (int row = 0; row < 6*10; row++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                pEnemyShot->x = px;
                pEnemyShot->y = 220.0 + row * 45.0/10;
                pEnemyShot->muki = 0.5 * DX_PI;  // 下方向へゆっくり
                pEnemyShot->speed = 0.45;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白の小玉＝ピン

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

// ---------------------------------------------------------
// 弾幕: ピンの設置（sEnemyShotSet を1つ作って登録するヘルパ）
// ---------------------------------------------------------
static void FirePins()
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotPins;
    pEnemyShotSet->x = 0.0;
    pEnemyShotSet->y = 0.0;
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

// =========================================================
// 敵本体のパターン
// 1サイクル360フレームを3フェーズで構成
//   [0〜119]   フェーズ1: 単輪射出（赤）
//   [120〜239] フェーズ2: 重ね投げ（赤→黄→白を0.4秒間隔、進路をずらす）
//   [240〜359] フェーズ3: ピン設置→白い輪をピンの間に通す
// =========================================================
void EnemyPat_RingToss_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;   // 輪の初期半径80が画面上にはみ出さないよう少し下げる
        enemy.maxHp = enemy.hp = 200; // 200で固定
        return;
    }

    // 左右にゆっくり揺れる移動
    enemy.x = 240.0 + sin(count * 0.015) * 130.0;

    if (enemy.hp <= 0) return;

    int cyc = count % 360;

    // 投げ予告音
    if (cyc == 5 || cyc == 125 || cyc == 245) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 発射時点での自機狙い角度
    double aim = atan2(player.y - (enemy.y + 20.0), player.x - enemy.x);

    // ---- フェーズ1: 単輪射出（赤） ----
    if (cyc == 40) {
        FireRing(0, aim, RingSpeed[0]);
    }

    // ---- フェーズ2: 重ね投げ（赤→黄→白、24フレーム=0.4秒間隔、進路を微妙にずらす） ----
    if (cyc == 140) FireRing(0, aim, RingSpeed[0]);
    if (cyc == 164) FireRing(1, aim + 0.10, RingSpeed[1]);
    if (cyc == 188) FireRing(2, aim - 0.10, RingSpeed[2]);

    // ---- フェーズ3: 輪とピン（ピンを設置→白い輪をピンの間に通す） ----
    if (cyc == 250) FirePins();
    if (cyc == 300) FireRing(2, aim, RingSpeed[2]);
}