// enemyPat_Tmp.cpp
// 弾幕：斑鳩風 極性シールド弾幕
//
// 仕様:
//  ・赤ボス(enemy.x/y)は赤中玉、青ボス(enemy.x2/y2)は青中玉を撃つ。
//    どちらか片方だけなら回避可能だが、両方同時だと実質回避不可能。
//  ・0秒後に sound_enemyCharge、1秒後に sound_enemyShot_extreme とともに
//    自機を取り囲むように赤小玉 img_enemyShotSmallBall[0] が配置される。
//    小玉の margin は 999.0。
//  ・赤小玉に触れた赤い敵弾、青小玉に触れた青い敵弾は消滅する。
//  ・3秒後、6秒後…にも sound_enemyCharge が鳴り、その1秒後に
//    小玉が赤⇔青(img_enemyShotSmallBall[4])へと反転する。以降3秒おきに繰り返し。
//  ・img_enemyShotSmallBall[0] / [4] / sound_enemyCharge / sound_enemyShot_extreme は
//    このパターン専用(敵弾には中玉 img_enemyShotMediumBall を使用する)。

static const int COLOR_RED = 0;
static const int COLOR_BLUE = 4;

// param_i[1] に入れる弾の役割
static const int SHOT_TYPE_BULLET = 1; // ボスが撃った敵弾(同色のシールド小玉で消える)
static const int SHOT_TYPE_SHIELD = 2; // 自機を取り囲む極性シールド小玉

static const double SHIELD_RADIUS = 40.0;       // シールド小玉の公転半径
static const int    SHIELD_NUM = 16;         // シールド小玉の数
static const double SPIN_SPEED = 0.01;       // 公転速度(rad/フレーム)
static const double ERASE_RANGE_SQ = 12.0 * 12.0;  // 小玉と敵弾の消滅判定距離(2乗)

// ----------------------------------------------------------------
// ボス弾(中玉)。pEnemyShotSet->kind に COLOR_RED / COLOR_BLUE を入れて使用。
// 弾発射元は毎フレーム自ボスの座標に追従する。
// ----------------------------------------------------------------
static void ShotColor(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射元をボスの現在位置に追従させる
    if (pEnemyShotSet->kind == COLOR_RED) {
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
    }
    else {
        pEnemyShotSet->x = enemy.x2;
        pEnemyShotSet->y = enemy.y2 + 10.0;
    }

    // 30フレームごとに自機狙い5-way
    if (pEnemyShotSet->count % 30 == 0) {
        double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        const int N = 25;
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base + (i - N / 2) * 0.14 / 2;
            pEnemyShot->speed = 2.6;
            pEnemyShot->kind = img_enemyShotMediumBall[pEnemyShotSet->kind];
            pEnemyShot->param_i[0] = pEnemyShotSet->kind; // 色(0:赤 4:青)
            pEnemyShot->param_i[1] = SHOT_TYPE_BULLET;    // 消去対象の弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 等速直線移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ----------------------------------------------------------------
// 極性シールド小玉。自機の周囲を公転し、同色の敵弾を消す。
// pEnemyShotSet->kind に現在の極性 COLOR_RED / COLOR_BLUE を保持する。
// ----------------------------------------------------------------
static void ShotShield(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ:自機を取り囲むように配置
    if (pEnemyShotSet->param_i[2] == 0) {
        pEnemyShotSet->param_i[2] = 1;

        for (int i = 0; i < SHIELD_NUM; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->kind];
            pEnemyShot->param_i[0] = pEnemyShotSet->kind; // 色
            pEnemyShot->param_i[1] = SHOT_TYPE_SHIELD;
            pEnemyShot->margin = 999.0;
            pEnemyShot->param_d[1] = SHIELD_RADIUS;                       // 公転半径
            pEnemyShot->param_d[2] = DX_PI * 2.0 * i / SHIELD_NUM;        // 公転位相

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 自機の周囲をゆっくり公転
    double spin = pEnemyShotSet->count * SPIN_SPEED;
    sEnemyShot* pBall = pEnemyShotSet->pEnemyShotHead->next;
    while (pBall != pEnemyShotSet->pEnemyShotHead) {
        pBall->x = player.x + pBall->param_d[1] * cos(pBall->param_d[2] + spin);
        pBall->y = player.y + pBall->param_d[1] * sin(pBall->param_d[2] + spin);
        pBall = pBall->next;
    }

    // 同色の敵弾との当たり判定(2フレームおきで十分)
    if (pEnemyShotSet->count % 2 != 0) return;

    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet != pEnemyShotSet) {
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next; // 消滅するかもしれないので先に保存

                if (pShot->param_i[1] == SHOT_TYPE_BULLET &&
                    pShot->param_i[0] == pEnemyShotSet->kind) {

                    // どれかの小玉に触れたら消滅
                    sEnemyShot* pB = pEnemyShotSet->pEnemyShotHead->next;
                    while (pB != pEnemyShotSet->pEnemyShotHead) {
                        double dx = pShot->x - pB->x;
                        double dy = pShot->y - pB->y;
                        if (dx * dx + dy * dy < ERASE_RANGE_SQ) {
                            // リストから切り離して消滅
                            pShot->prev->next = pShot->next;
                            pShot->next->prev = pShot->prev;
                            delete pShot;
                            break;
                        }
                        pB = pB->next;
                    }
                }
                pShot = pNext;
            }
        }
        pSet = pSet->next;
    }
}

// ----------------------------------------------------------------
// 敵本体のパターン
// ----------------------------------------------------------------
void EnemyPat_Ikaruga_Zai()
{
    static int muki;
    static int muki2;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;  enemy.y = 40.0;  // 赤ボス
        enemy.x2 = 360.0;  enemy.y2 = 40.0;  // 青ボス
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        muki2 = 1;

        // ===== 0秒後:予告音 =====
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // シールド小玉セットを生成(小玉の配置自体はShotShield内で行う)
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->kind = COLOR_RED; // 初期極性は赤
        pEnemyShotSet->patternFunc = ShotShield;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // ボス弾セット(赤・青 各1つ、撃ち続ける)
        for (int i = 0; i < 2; i++) {
            sEnemyShotSet* pBulletSet = new sEnemyShotSet;
            pBulletSet->count = 0;
            pBulletSet->kind = (i == 0) ? COLOR_RED : COLOR_BLUE;
            pBulletSet->patternFunc = ShotColor;
            pBulletSet->x = (i == 0) ? enemy.x : enemy.x2;
            pBulletSet->y = (i == 0) ? enemy.y : enemy.y2;

            pBulletSet->pEnemyShotHead = new sEnemyShot;
            pBulletSet->pEnemyShotHead->prev = pBulletSet->pEnemyShotHead;
            pBulletSet->pEnemyShotHead->next = pBulletSet->pEnemyShotHead;

            pBulletSet->prev = enemyShotSetHead.prev;
            pBulletSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pBulletSet;
            enemyShotSetHead.prev = pBulletSet;
        }
    }
    else {
        // 両ボスが左右に往復移動
        enemy.x += 0.98 * (double)muki;
        enemy.x2 += 0.98 * (double)muki2;
        if (enemy.x > 400) muki = -1;
        if (enemy.x < 40) muki = 1;
        if (enemy.x2 > 400) muki2 = -1;
        if (enemy.x2 < 40) muki2 = 1;

        // ===== 3秒後,6秒後,…:予告音 =====
        if (count % 180 == 1) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }

    // ===== 1秒後,4秒後,7秒後,…:シールド小玉の極性反転 =====
    if (count % 180 == 61) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // シールド小玉の色を赤⇔青で反転させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotShield) {
                int newColor = (pSet->kind == COLOR_RED) ? COLOR_BLUE : COLOR_RED;
                pSet->kind = newColor;

                sEnemyShot* pBall = pSet->pEnemyShotHead->next;
                while (pBall != pSet->pEnemyShotHead) {
                    pBall->kind = img_enemyShotSmallBall[newColor];
                    pBall->param_i[0] = newColor;
                    pBall = pBall->next;
                }
            }
            pSet = pSet->next;
        }
    }
}