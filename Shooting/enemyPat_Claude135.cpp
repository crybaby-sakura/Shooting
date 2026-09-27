#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =====================================================================
// バックドラフト弾幕
//   Phase1 酸欠(吸気収束) → Phase2 着火の予告 → Phase3 爆轟(全方位爆発)
//   → Phase4 残り火の二次爆発、という一連の流れを、既存弾素材の色と
//   速度カーブの組み合わせだけで表現する。専用グラフィックは使わない。
// =====================================================================

// ---------------------------------------------------------------
// 新しい弾を弾セットの連結リストへ追加するヘルパ。
// 呼び出し側は戻り値に x,y,muki,speed,kind 等を設定すること。
// ---------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ---------------------------------------------------------------
// Phase1: 酸欠(吸気) — 画面周辺から発生させた弾を、発生源
// (param_d[0], param_d[1]) へ向けてゆっくり収束させる。
// くすぶる炎のイメージで 青・シアン・黒 の鱗弾/小玉を使用する。
// ---------------------------------------------------------------
static void ShotBackdraft_Suction(sEnemyShotSet* pEnemyShotSet)
{
    const double cx = pEnemyShotSet->param_d[0];
    const double cy = pEnemyShotSet->param_d[1];
    const int SPAWN_FRAMES = 80; // この期間だけ弾を発生させ続ける

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < SPAWN_FRAMES && pEnemyShotSet->count % 3 == 0) {
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            // 画面(480x480)の外周付近からランダムに発生させる
            switch (GetRand(3)) {
            case 0:  pEnemyShot->x = GetRand(480); pEnemyShot->y = -20.0;       break;
            case 1:  pEnemyShot->x = GetRand(480); pEnemyShot->y = 500.0;       break;
            case 2:  pEnemyShot->x = -20.0;        pEnemyShot->y = GetRand(480); break;
            default: pEnemyShot->x = 500.0;        pEnemyShot->y = GetRand(480); break;
            }
            pEnemyShot->muki = atan2(cy - pEnemyShot->y, cx - pEnemyShot->x);
            pEnemyShot->speed = 2.0 + GetRand(150) / 100.0; // 2.0〜3.5、吸い込まれる速度
            // 酸欠でくすぶる炎: 青・シアン・黒を中心に鱗弾と小玉を使い分ける
            static const int colorTable[3] = { 4, 3, 7 };
            int color = colorTable[GetRand(2)];
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotScale[color] : img_enemyShotSmallBall[color];
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// Phase2: 着火の予告 — 発生源に弾を静止したまま並べて一瞬だけ
// 溜めを作り、予告音の後にそのまま外側へ加速させて爆発の先陣を
// 切らせる。白・黄の大玉/中楕円弾でフラッシュのように見せる。
// ---------------------------------------------------------------
static void ShotBackdraft_Ignition(sEnemyShotSet* pEnemyShotSet)
{
    const double cx = pEnemyShotSet->param_d[0];
    const double cy = pEnemyShotSet->param_d[1];
    const int IGNITE_DELAY = 24; // 静止している予告フレーム数
    const int RING_COUNT = 20;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < RING_COUNT; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            double angle = DX_PI * 2.0 * i / RING_COUNT;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = angle; // 点火後に飛ぶ方向をあらかじめ入れておく
            pEnemyShot->speed = 0.0;  // 予告中は静止
            pEnemyShot->param_i[0] = IGNITE_DELAY;
            pEnemyShot->param_d[0] = 4.5 + GetRand(150) / 100.0; // 点火後の速度(4.5〜6.0)
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotLargeBall[6] : img_enemyShotMediumOval[1]; // 白/黄
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] > 0) {
            pShot->param_i[0]--;
        }
        else {
            pShot->speed = pShot->param_d[0];
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// Phase3: 爆轟 — 発生源から全方位へ、温度分布を模した色分けの
// 多重リングと衝撃波を表す短レーザーを一斉に放射する。
// 内側(先頭)ほど高温(白/黄)で速く、外側ほど低温(赤/橙)で遅い。
// ---------------------------------------------------------------
static void ShotBackdraft_Blast(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y;

        // 衝撃波の先端を8方向の短レーザーで表現
        const int LASER_COUNT = 8;
        for (int i = 0; i < LASER_COUNT; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            double angle = DX_PI * 2.0 * i / LASER_COUNT + DX_PI / LASER_COUNT;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 7.0;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 白
        }

        // 多重リング: 内側=高温・高速、外側=低温・低速
        struct RingDef { int count; double speedMin; double speedRange; int kindGroup; int color; };
        // kindGroup: 0=中玉 1=大玉 2=菱形弾 3=銃弾
        const RingDef rings[4] = {
            { 28, 5.0, 1.2, 0, 6 }, // 中玉・白(最高温、先頭)
            { 32, 4.0, 1.0, 1, 1 }, // 大玉・黄
            { 40, 3.0, 0.8, 2, 8 }, // 菱形弾・橙
            { 48, 1.8, 0.7, 3, 0 }, // 銃弾・赤(外周のくすぶり)
        };

        for (int r = 0; r < 4; r++) {
            const RingDef& def = rings[r];
            double baseOffset = r * 0.18; // リングごとに角度を少しずらして隙間を埋める
            for (int i = 0; i < def.count; i++) {
                sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
                double angle = DX_PI * 2.0 * i / def.count + baseOffset;
                pEnemyShot->x = cx;
                pEnemyShot->y = cy;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = def.speedMin + GetRand(100) / 100.0 * def.speedRange;
                switch (def.kindGroup) {
                case 0:  pEnemyShot->kind = img_enemyShotMediumBall[def.color]; break;
                case 1:  pEnemyShot->kind = img_enemyShotLargeBall[def.color];  break;
                case 2:  pEnemyShot->kind = img_enemyShotDiamond[def.color];    break;
                default: pEnemyShot->kind = img_enemyShotBullet[def.color];    break;
                }
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

// ---------------------------------------------------------------
// Phase4: 残り火の二次爆発 — 未燃焼ガスが局所的に再着火する様子を、
// 発生源付近のランダムな点で起こる小規模な放射爆発で表現する。
// ---------------------------------------------------------------
static void ShotBackdraft_Secondary(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y;
        const int SHOT_COUNT = 18 + GetRand(10); // 18〜28発

        for (int i = 0; i < SHOT_COUNT; i++) {
            sEnemyShot* pEnemyShot = AddShot(pEnemyShotSet);
            double angle = DX_PI * 2.0 * i / SHOT_COUNT + (GetRand(20) - 10) / 100.0;
            pEnemyShot->x = cx;
            pEnemyShot->y = cy;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.8 + GetRand(150) / 100.0; // 1.8〜3.3
            // 赤・橙を中心に、鱗弾/小玉/中玉を混在させる
            int color = (GetRand(1) == 0) ? 0 : 8; // 赤 or 橙
            switch (GetRand(2)) {
            case 0:  pEnemyShot->kind = img_enemyShotScale[color];     break;
            case 1:  pEnemyShot->kind = img_enemyShotSmallBall[color]; break;
            default: pEnemyShot->kind = img_enemyShotMediumBall[color]; break;
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

// ---------------------------------------------------------------
// 新しい弾セットを生成し、enemyShotSetHead に連結するヘルパ。
// ---------------------------------------------------------------
static sEnemyShotSet* SpawnShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// =====================================================================
// 敵本体: バックドラフト弾幕
// Phase1(吸気収束) → Phase2(着火予告) → Phase3(全方位爆発)
// → Phase4(残り火×3) を1周期(420フレーム)として繰り返す。
// =====================================================================
void EnemyPat_Backdraft_Claude()
{
    static double epicenterX, epicenterY; // この周期の爆心(発生源)
    const int CYCLE = 420;
    int t = (count - 1) % CYCLE; // 周期内での経過フレーム(0始まり)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // ゆったりと左右に揺れるだけの単純な移動
        enemy.x = 240.0 + sin(count / 90.0) * 60.0;
    }

    if (t == 0) {
        // 爆心を敵の少し下に設定し、この周期の間保持する
        epicenterX = enemy.x;
        epicenterY = enemy.y + 120.0;
        sEnemyShotSet* pSet = SpawnShotSet(ShotBackdraft_Suction, epicenterX, epicenterY);
        pSet->param_d[0] = epicenterX;
        pSet->param_d[1] = epicenterY;
    }
    else if (t == 190) {
        sEnemyShotSet* pSet = SpawnShotSet(ShotBackdraft_Ignition, epicenterX, epicenterY);
        pSet->param_d[0] = epicenterX;
        pSet->param_d[1] = epicenterY;
    }
    else if (t == 214) { // 190 + IGNITE_DELAY(24) で本爆発
        SpawnShotSet(ShotBackdraft_Blast, epicenterX, epicenterY);
    }
    else if (t == 260 || t == 285 || t == 312) {
        // 爆心付近のランダムな点で残り火が再着火する
        double angle = DX_PI * 2.0 * GetRand(359) / 360.0;
        double radius = 60.0 + GetRand(120);
        double sx = epicenterX + cos(angle) * radius;
        double sy = epicenterY + sin(angle) * radius;
        SpawnShotSet(ShotBackdraft_Secondary, sx, sy);
    }
}