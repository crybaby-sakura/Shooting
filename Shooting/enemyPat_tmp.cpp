#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
//  禁忌「カゴメカゴメ」 (東方紅魔郷 Extra / 芙蘭朵露・スカーレット EX-4符)
//
//  ◆原作の再現ポイント
//   ・緑の魔法の檻(格子)が画面の端からゆっくりと形成されていく
//   ・黄色い大弾(鳥)が、自機狙いとランダム方向が混ざって放たれる
//   ・黄色い弾を撃つたび、緑の檻がその発射方向へ「にじんで変形」する
//
//  ◆素材の選定
//   ・檻    … 短レーザー(64x4)の緑[2]。長尺弾で格子状の「檻」を表現
//   ・鳥(大)… 大玉(20x20)の黄[1]。原作通りの主役弾
//   ・鳥(小)… 中玉(7x7)の黄[1]。同色で統一した自機狙いの補助弾
//   ・音    … 予告にsound_enemyCharge、檻にlight、黄色弾にheavy
// ============================================================================

// 黄色い弾の発射で檻がにじむ方向と強さ(毎フレーム減衰)
static double g_deformX = 0.0;
static double g_deformY = 0.0;

// 弾を生成してショットセットのリストに繋げる
static sEnemyShot* NewShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ショットセットを生成して敵弾リストに繋げる
static void CreateShotSet(void (*patternFunc)(sEnemyShotSet*), double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 弾発射による檻の「にじみ」を加算(上限でクランプして暴走を防ぐ)
static void AddDeform(double muki, double power)
{
    g_deformX += cos(muki) * power;
    g_deformY += sin(muki) * power;
    double mag = sqrt(g_deformX * g_deformX + g_deformY * g_deformY);
    if (mag > 30.0) {
        g_deformX *= 30.0 / mag;
        g_deformY *= 30.0 / mag;
    }
}

// ---------------------------------------------------------------------------
// 緑の魔法の檻:画面の端からレーザーが滑り込み、ゆっくり格子状に形成される。
// 黄色い弾が撃たれると、芙蘭に近い檻ほど発射方向へにじみ(変形し)、
// やがてゆっくりと元の形へ戻る。
// param_d[0],[1]=現在の基準位置 [2],[3]=変形オフセット [4],[5]=定位置(home)
// ---------------------------------------------------------------------------
static void ShotCage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 横棒:5段 x 8本(画面左右の端から交互に滑り込む)
        for (int r = 0; r < 5; r++) {
            for (int i = 0; i < 8; i++) {
                pEnemyShot = NewShot(pEnemyShotSet);
                pEnemyShot->kind = img_enemyShotLaser[2];   // 緑
                pEnemyShot->param_d[4] = 30.0 + i * 60.0;   // homeX
                pEnemyShot->param_d[5] = 60.0 + r * 80.0;   // homeY
                // 生成位置は画面の端の内側(画面外だと即消去されるため)
                pEnemyShot->param_d[0] = (i % 2 == 0) ? 5.0 : 475.0;
                pEnemyShot->param_d[1] = pEnemyShot->param_d[5];
                pEnemyShot->param_d[2] = 0.0;
                pEnemyShot->param_d[3] = 0.0;
                pEnemyShot->x = pEnemyShot->param_d[0];
                pEnemyShot->y = pEnemyShot->param_d[1];
                pEnemyShot->muki = (i % 2 == 0) ? 0.0 : DX_PI;
                pEnemyShot->speed = 0.0;
            }
        }
        // 縦棒:5列 x 8本(画面上下の端から交互に滑り込む)
        for (int c = 0; c < 5; c++) {
            for (int j = 0; j < 8; j++) {
                pEnemyShot = NewShot(pEnemyShotSet);
                pEnemyShot->kind = img_enemyShotLaser[2];   // 緑
                pEnemyShot->param_d[4] = 60.0 + c * 80.0;   // homeX
                pEnemyShot->param_d[5] = 30.0 + j * 60.0;   // homeY
                pEnemyShot->param_d[0] = pEnemyShot->param_d[4];
                pEnemyShot->param_d[1] = (j % 2 == 0) ? 5.0 : 475.0;
                pEnemyShot->param_d[2] = 0.0;
                pEnemyShot->param_d[3] = 0.0;
                pEnemyShot->x = pEnemyShot->param_d[0];
                pEnemyShot->y = pEnemyShot->param_d[1];
                pEnemyShot->muki = (j % 2 == 0) ? DX_PI / 2.0 : -DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // ゆっくりと定位置へ近づく = 檻の形成
        pShot->param_d[0] += (pShot->param_d[4] - pShot->param_d[0]) * 0.025;
        pShot->param_d[1] += (pShot->param_d[5] - pShot->param_d[1]) * 0.025;
        // 芙蘭(セット生成位置)に近いほど強く、弾の発射方向へにじむ
        double dx = pShot->x - pEnemyShotSet->x;
        double dy = pShot->y - pEnemyShotSet->y;
        double atten = 150.0 / (150.0 + sqrt(dx * dx + dy * dy));
        pShot->param_d[2] += (g_deformX * atten - pShot->param_d[2]) * 0.08;
        pShot->param_d[3] += (g_deformY * atten - pShot->param_d[3]) * 0.08;
        pShot->x = pShot->param_d[0] + pShot->param_d[2];
        pShot->y = pShot->param_d[1] + pShot->param_d[3];
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------------
// 黄色い弾(鳥):自機を狙う大弾とランダム方向の大弾を一度に放つ。
// その後しばらく20フレーム毎に自機狙いの黄中弾で追い込む。
// 1発撃つごとに檻がその方向へにじむ。
// ---------------------------------------------------------------------------
static void ShotYellowBird(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 自機を狙う大弾3発(小さく拡散)
        for (int i = 0; i < 3; i++) {
            double muki = base + (GetRand(40) - 20) / 100.0;
            pEnemyShot = NewShot(pEnemyShotSet);
            pEnemyShot->kind = img_enemyShotLargeBall[1];   // 黄の大玉
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 2.2 + GetRand(60) / 100.0;
            AddDeform(muki, 3.0);
        }
        // ランダム方向の大弾3発
        for (int i = 0; i < 3; i++) {
            double muki = GetRand(628) / 100.0;   // 0～6.28rad(GetRandは0～xまで返る)
            pEnemyShot = NewShot(pEnemyShotSet);
            pEnemyShot->kind = img_enemyShotLargeBall[1];   // 黄の大玉
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 2.0 + GetRand(100) / 100.0;
            AddDeform(muki, 3.0);
        }
    }
    else if (pEnemyShotSet->count <= 40 && pEnemyShotSet->count % 20 == 0) {
        // 自機狙いの黄中弾(次の大弾までの繋ぎ。発射期間を制限して暴発を防ぐ)
        double base = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double muki = base + (GetRand(21) - 10) / 100.0;
        pEnemyShot = NewShot(pEnemyShotSet);
        pEnemyShot->kind = img_enemyShotMediumBall[1];   // 黄の中玉
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = 3.2;
        AddDeform(muki, 1.0);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------------
// 敵本体:芙蘭は画面中央上をゆったり漂い、
// 「檻の形成 → 黄色い大弾の連射」を繰り返す。
// ---------------------------------------------------------------------------
void EnemyPat_Tmp()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと
        g_deformX = 0.0;
        g_deformY = 0.0;

        // スペルカード予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 緑の魔法の檻(画面端から形成される)
        CreateShotSet(ShotCage, 240.0, 90.0);
    }
    else {
        // ゆったりと漂う(「笼中鸟」の浮遊感)
        enemy.x = 240.0 + 55.0 * sin(count * 0.015);
        enemy.y = 90.0 + 18.0 * sin(count * 0.027);
    }

    // 檻が形になりかけた頃から、60フレーム毎に黄色い大弾(鳥)を放つ
    if (count >= 100 && count % 60 == 40) {
        CreateShotSet(ShotYellowBird, enemy.x, enemy.y + 10.0);
    }

    // 檻の変形量は時間と共に減衰していく
    g_deformX *= 0.92;
    g_deformY *= 0.92;
}