// enemyPat_kyokuseiBouheki.cpp
//
// 斑鳩のような弾幕：ボス1(赤, enemy.x/y)とボス2(青, enemy.x2/y2)が
// それぞれ自機狙い中心の弾を撃ち続ける。
// 赤い弾だけ、青い弾だけなら回避可能だが、両方が重なると回避不可能な密度になる。
// 自機の周りには一定周期(3秒おき)で色が入れ替わる小玉シールドが纏わりつき、
// シールドと同じ色の敵弾に触れると吸収して消滅させる。
// これにより実質的に「今シールドが持っている色」の弾だけを無効化しながら、
// 残ったもう一方の色を回避する、という斑鳩の極性システムに近い攻略になる。
//
// タイムライン：
//   0秒: 予告音(sound_enemyCharge)
//   1秒: シールド出現(赤, sound_enemyShot_extreme)
//   3秒: 予告音(sound_enemyCharge)
//   4秒: シールドが青に変化(sound_enemyShot_extreme)
//   以降、3秒おきに「予告→1秒後に色切り替え」を無限に繰り返す。
//
// 各ボスは「狙い撃ち（細い自機狙い連射）」「拡散（広い自機狙い＋速度ばらつき）」
// 「全方位（自機を狙わない輪）」の3種類の攻撃を一定周期で切り替える。
// 2体のボスは切り替えの位相をずらしてあり、常に同じ組み合わせにならないようにしている。
//
// img_enemyShotSmallBall[0]/[4] と sound_enemyCharge/sound_enemyShot_extreme は
// このシールド演出専用とし、他の用途では使用しない。
//
// ※60FPS想定（FPSが異なる場合はEnemyPat_Tmp内のFPS定数を調整してください）。

// 色定義（param_i[0]に格納。このファイル内のみで使用）
enum { PATCOLOR_RED = 0, PATCOLOR_BLUE = 1 };

// 弾幕：ボスの連射弾（自機狙いの扇 or 全方位の輪）
// param_d[0],[1] = 発射元座標　param_d[2] = 扇の角度(全方位時は未使用)
// param_d[3] = 基本速度　param_i[0] = 色　param_i[1] = way数
// param_i[2] = モード(0:自機狙い扇 1:全方位)　param_i[3] = 速度ばらつき(%、0で無効)
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pEnemyShot;
        double baseX = pEnemyShotSet->param_d[0];
        double baseY = pEnemyShotSet->param_d[1];
        double spread = pEnemyShotSet->param_d[2];
        double baseSpeed = pEnemyShotSet->param_d[3];
        int color = pEnemyShotSet->param_i[0];
        int way = pEnemyShotSet->param_i[1];
        int mode = pEnemyShotSet->param_i[2];
        int variancePercent = pEnemyShotSet->param_i[3];

        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double aimMuki = 0.0;
        if (mode == 0) {
            aimMuki = atan2(player.y - baseY, player.x - baseX);
        }

        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;

            double muki;
            if (mode == 0) {
                // 自機狙いの扇（wayが1本ならまっすぐ自機へ）
                muki = (way == 1) ? aimMuki : aimMuki + spread * (i - (way - 1) / 2.0) / (way - 1);
            }
            else {
                // 全方位（等間隔の輪）
                muki = DX_PI * 2.0 / way * i;
            }

            double speed = baseSpeed;
            if (variancePercent > 0) {
                // 速度に多少のばらつきを持たせて単調さをなくす（GetRandなのでリプレイ再現性あり）
                speed = baseSpeed * (100 + GetRand(variancePercent * 2) - variancePercent) / 100.0;
            }

            pEnemyShot->param_d[0] = baseX;   // 発射時のx（formula駆動の起点）
            pEnemyShot->param_d[1] = baseY;   // 発射時のy
            pEnemyShot->param_d[2] = muki;    // 向き
            pEnemyShot->param_d[3] = speed;   // 速さ
            pEnemyShot->param_i[0] = color;   // 色
            pEnemyShot->muki = muki;
            pEnemyShot->kind = (color == PATCOLOR_RED) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 位置はcountからのformulaで直接計算する（速度積分はしない）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double spawnX = pShot->param_d[0];
        double spawnY = pShot->param_d[1];
        double muki = pShot->param_d[2];
        double speed = pShot->param_d[3];

        pShot->x = spawnX + speed * cos(muki) * pShot->count;
        pShot->y = spawnY + speed * sin(muki) * pShot->count;

        pShot = pShot->next;
    }
}

// ShotBurst用のsEnemyShotSetを1つ生成してenemyShotSetHeadに繋ぐ
static void SpawnBurst(double x, double y, int color, int way, int mode, double spread, double speed, int variancePercent)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotBurst;
    pEnemyShotSet->param_d[0] = x;
    pEnemyShotSet->param_d[1] = y;
    pEnemyShotSet->param_d[2] = spread;
    pEnemyShotSet->param_d[3] = speed;
    pEnemyShotSet->param_i[0] = color;
    pEnemyShotSet->param_i[1] = way;
    pEnemyShotSet->param_i[2] = mode;
    pEnemyShotSet->param_i[3] = variancePercent;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 弾幕：自機を取り囲む色替わり小玉シールド
// param_i[0] = 現在の色（EnemyPat_Tmp側から書き換えられる）
static void ShotShield(sEnemyShotSet* pEnemyShotSet)
{
    const int    SHIELD_NUM = 8;
    const double SHIELD_RADIUS = 26.0;
    const double SHIELD_ROTATE_SPEED = 0.01;

    if (pEnemyShotSet->count == 0) {
        // 初回生成：自機を取り囲むように8発を等間隔配置
        sEnemyShot* pEnemyShot;
        for (int i = 0; i < SHIELD_NUM; i++) {
            pEnemyShot = new sEnemyShot;

            double baseAngle = DX_PI * 2.0 / SHIELD_NUM * i;
            pEnemyShot->param_d[0] = baseAngle;                   // 周回の基準角度
            pEnemyShot->margin = 999.9;
            pEnemyShot->param_i[0] = pEnemyShotSet->param_i[0];   // 現在の色を継承
            pEnemyShot->kind = (pEnemyShot->param_i[0] == PATCOLOR_RED) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 位置更新（自機中心＋回転角。countのformulaで駆動）と色の同期
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pShot->param_d[0] + SHIELD_ROTATE_SPEED * pShot->count;
        pShot->x = player.x + SHIELD_RADIUS * cos(angle);
        pShot->y = player.y + SHIELD_RADIUS * sin(angle);
        pShot->muki = angle;

        // シールド全体の色が切り替わったら追従して塗り替える
        if (pShot->param_i[0] != pEnemyShotSet->param_i[0]) {
            pShot->param_i[0] = pEnemyShotSet->param_i[0];
            pShot->kind = (pShot->param_i[0] == PATCOLOR_RED) ? img_enemyShotSmallBall[0] : img_enemyShotSmallBall[4];
        }

        pShot = pShot->next;
    }

    // 吸収判定：現在のシールド色と同じ色の敵弾が小玉のmargin内に入ったら消滅させる
    int shieldColor = pEnemyShotSet->param_i[0];
    sEnemyShotSet* pOtherSet = enemyShotSetHead.next;
    while (pOtherSet != &enemyShotSetHead) {
        if (pOtherSet == pEnemyShotSet) {
            pOtherSet = pOtherSet->next;
            continue;
        }

        sEnemyShot* pTarget = pOtherSet->pEnemyShotHead->next;
        while (pTarget != pOtherSet->pEnemyShotHead) {
            sEnemyShot* pTargetNext = pTarget->next; // 削除前に次を保存しておく

            if (pTarget->param_i[0] == shieldColor) {
                sEnemyShot* pBall = pEnemyShotSet->pEnemyShotHead->next;
                while (pBall != pEnemyShotSet->pEnemyShotHead) {
                    double dx = pTarget->x - pBall->x;
                    double dy = pTarget->y - pBall->y;
                    double dist = sqrt(dx * dx + dy * dy);

                    if (dist <= 12.0) {
                        // 吸収：リストから外してプールへ返却
                        pTarget->prev->next = pTarget->next;
                        pTarget->next->prev = pTarget->prev;
                        delete pTarget;
                        break;
                    }
                    pBall = pBall->next;
                }
            }

            pTarget = pTargetNext;
        }

        pOtherSet = pOtherSet->next;
    }
}

// ボス1体分の攻撃をディスパッチする。
// 「狙い撃ち」「拡散」「全方位」を一定周期で切り替えることで単調さをなくす。
// phaseShiftをボスごとにずらすことで、2体の組み合わせが常に一定にならないようにする。
static void BossFire(double x, double y, int color, int phaseShift)
{
    const int CYCLE = 150; // 攻撃パターン一巡（2.5秒）
    const int SEG = CYCLE / 3;

    int t = (count + phaseShift) % CYCLE;
    int phase = t / SEG;
    int tInPhase = t % SEG;

    if (phase == 0) {
        // 狙い撃ちフェーズ：細い自機狙い5wayを短間隔で連射
        if (tInPhase % 10 == 0) {
            SpawnBurst(x, y, color, 5, 0, 24.0 / 180.0 * DX_PI, 2.6, 0);
        }
    }
    else if (phase == 1) {
        // 拡散フェーズ：広い自機狙い9wayを1回、速度にばらつきを持たせる
        if (tInPhase % 20 == 0) {
            SpawnBurst(x, y, color, 9, 0, 70.0 / 180.0 * DX_PI, 2.0, 20);
        }
    }
    else {
        // 全方位フェーズ：自機を狙わず輪を一発
        if (tInPhase == 0) {
            SpawnBurst(x, y, color, 14*10, 1, 0.0, 1.8/2, 0);
        }
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    // 60FPS想定（FPSが異なる場合はここを調整）
    const int FPS = 60;
    const int CYCLE = FPS * 3;       // シールドの色切り替え周期（3秒）
    const int FIRST_SWITCH = FPS;    // 予告から切り替えまでの間隔（1秒）

    static sEnemyShotSet* pShield = nullptr;

    if (count == 1) {
        // ゲーム画面は480x480。ボス1(赤)を左上、ボス2(青)を右上に配置。
        enemy.x = 130.0;
        enemy.y = 70.0;
        enemy.x2 = 350.0;
        enemy.y2 = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        pShield = nullptr;
    }
    else {
        // 左右にゆるやかにスイング（countのformulaで駆動。速度積分は行わない）
        enemy.x = 130.0 + 24.0 * sin(count / 90.0);
        enemy.x2 = 350.0 - 24.0 * sin(count / 90.0);
    }

    // ボス1（赤）とボス2（青）：位相をずらしつつ狙い撃ち/拡散/全方位を切り替えて発射
    BossFire(enemy.x, enemy.y + 12.0, PATCOLOR_RED, 0);
    BossFire(enemy.x2, enemy.y2 + 12.0, PATCOLOR_BLUE, 100);

    // シールドの予告音（0秒, 3秒, 6秒, ... の周期）
    if ((count - 1) % CYCLE == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // シールドの色切り替え（予告の1秒後。1秒, 4秒, 7秒, ... の周期）
    if (count >= 1 + FIRST_SWITCH && (count - (1 + FIRST_SWITCH)) % CYCLE == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int switchIndex = (count - (1 + FIRST_SWITCH)) / CYCLE;
        int newColor = (switchIndex % 2 == 0) ? PATCOLOR_RED : PATCOLOR_BLUE;

        if (pShield == nullptr) {
            // 初回：赤小玉のシールドを生成
            pShield = new sEnemyShotSet;
            pShield->count = 0;
            pShield->patternFunc = ShotShield;
            pShield->param_i[0] = newColor;

            pShield->pEnemyShotHead = new sEnemyShot;
            pShield->pEnemyShotHead->prev = pShield->pEnemyShotHead;
            pShield->pEnemyShotHead->next = pShield->pEnemyShotHead;

            pShield->prev = enemyShotSetHead.prev;
            pShield->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pShield;
            enemyShotSetHead.prev = pShield;
        }
        else {
            // 2回目以降：玉は生成し直さず色だけ切り替える
            pShield->param_i[0] = newColor;
        }
    }
}