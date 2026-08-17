// enemyPat_ShoriOchi.cpp
//
// 「処理落ち」演出をテーマにした弾幕パターン。
//
// コンセプト：
//   ゲーム内の「経過時間」そのものを歪ませるワープタイム関数 GetWarpedTime() を
//   全ての弾の位置計算式に共通で通すことで、画面内の弾群が同じ「時間の歪み」を
//   共有する。個々の弾自体は従来通り「発生時刻からの経過時間 × 速度」で直線移動
//   するだけの純粋な式であり、速度積分は一切行わない。
//
//   フェーズ2(擬似処理落ち)では、ワープ時間が「6フレーム凍結→2フレーム進行」を
//   繰り返すため、弾は見た目上ほとんど静止し、たまにガクッと進む「コマ送り」状態
//   になる。その間も過負荷(=処理落ちの原因)を表すリング弾をどんどん撒き続け、
//   水面下で"遅れ"が大きく蓄積していく。
//
//   フェーズ3(反動早送り)では、蓄積した遅れをイーズアウト曲線で一気に解消する。
//   瞬間的に実時間の数倍の速度で弾が押し寄せる「処理落ち明けの早送り感」を再現する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// フェーズ境界・パラメータ
// ============================================================
static constexpr int CYCLE_LEN = 600 - 60; // 1サイクル(以後ループ)
static constexpr int PHASE1_END = 150 - 60; // 通常速度フェーズ終了
static constexpr int PHASE2_END = 330 - 60; // 擬似処理落ちフェーズ終了
static constexpr int PHASE3_END = 390 - 60; // 反動早送りフェーズ終了(以後PHASE4)

// フェーズ2のコマ送り挙動：凍結フレーム数 / 進行フレーム数
static constexpr int STUTTER_FREEZE_FRAMES = 6;
static constexpr int STUTTER_MOVE_FRAMES = 2;
static constexpr int STUTTER_MICRO_CYCLE = STUTTER_FREEZE_FRAMES + STUTTER_MOVE_FRAMES;

// ============================================================
// ワープタイム：実フレームcountを受け取り、「歪んだ経過時間」を返す。
// 全ての弾がこの関数を通して動くため、画面内の弾全体が同じ時間の歪みを共有する。
// ============================================================
static double GetWarpedTime(int c)
{
    int cycleBase = (c / CYCLE_LEN) * CYCLE_LEN;
    int local = c - cycleBase;

    if (local < PHASE1_END) {
        // フェーズ1：通常速度(実時間そのまま)
        return (double)c;
    }
    else if (local < PHASE2_END) {
        // フェーズ2：擬似処理落ち。「凍結」区間と「進行」区間を繰り返す。
        int local2 = local - PHASE1_END;
        int numFull = local2 / STUTTER_MICRO_CYCLE;
        int rem = local2 % STUTTER_MICRO_CYCLE;
        int moved = numFull * STUTTER_MOVE_FRAMES;
        if (rem > STUTTER_FREEZE_FRAMES) {
            moved += (rem - STUTTER_FREEZE_FRAMES);
        }
        return (double)(cycleBase + PHASE1_END + moved);
    }
    else if (local < PHASE3_END) {
        // フェーズ3：反動早送り。フェーズ2終了時点の「遅れ」をイーズアウトで解消する。
        int local2AtEnd = PHASE2_END - PHASE1_END;
        int numFullAtEnd = local2AtEnd / STUTTER_MICRO_CYCLE;
        int remAtEnd = local2AtEnd % STUTTER_MICRO_CYCLE;
        int movedAtEnd = numFullAtEnd * STUTTER_MOVE_FRAMES;
        if (remAtEnd > STUTTER_FREEZE_FRAMES) {
            movedAtEnd += (remAtEnd - STUTTER_FREEZE_FRAMES);
        }
        double debtTime = (double)(cycleBase + PHASE1_END + movedAtEnd);
        double realTimeNow = (double)c;

        int phase3Elapsed = local - PHASE2_END;
        int phase3Duration = PHASE3_END - PHASE2_END;
        double progress = (double)phase3Elapsed / (double)phase3Duration;
        if (progress > 1.0) progress = 1.0;
        double eased = 1.0 - (1.0 - progress) * (1.0 - progress); // イーズアウト

        return debtTime + (realTimeNow - debtTime) * eased;
    }
    else {
        // フェーズ4：回復後、実時間で通常運転
        return (double)c;
    }
}

// 現在の実フレームcountがどのフェーズかを返す(0〜3)
static int GetPhase(int c)
{
    int local = c % CYCLE_LEN;
    if (local < PHASE1_END) return 0;
    if (local < PHASE2_END) return 1;
    if (local < PHASE3_END) return 2;
    return 3;
}

// ============================================================
// 弾幕パターン：ワープ時間ベースの直線弾
// 発生時刻からの「歪んだ経過時間」×速度で位置を式から直接算出する(速度積分なし)。
// param_d[0..1] = 発生位置(x,y)
// ============================================================
static void ShotWarpedLinear(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double spawnX = pShot->param_d[0];
        double spawnY = pShot->param_d[1];

        int spawnCount = count - pShot->count; // この弾が生成された瞬間の実count
        double elapsedWarped = GetWarpedTime(count) - GetWarpedTime(spawnCount);

        pShot->x = spawnX + pShot->speed * cos(pShot->muki) * elapsedWarped;
        pShot->y = spawnY + pShot->speed * sin(pShot->muki) * elapsedWarped;

        pShot = pShot->next;
    }
}

// ============================================================
// 生成ヘルパー
// ============================================================

// 新しいEnemyShotSetを1つ作成し、リストへ登録する
static sEnemyShotSet* CreateWarpedShotSet(double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotWarpedLinear;
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

// setへ1発追加する
static void AddWarpedShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kindImg)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kindImg;
    pShot->param_d[0] = x; // 発生時のx(式の基準点)
    pShot->param_d[1] = y; // 発生時のy(式の基準点)

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 自機狙いのN-wayファンを1組生成する(フェーズ1・フェーズ4で使用)
static void SpawnAimedFan(double x, double y, int wayCount, double speed, int colorIdx)
{
    sEnemyShotSet* pSet = CreateWarpedShotSet(x, y);

    double baseMuki = atan2(player.y - y, player.x - x);
    double spreadStep = 0.06 * DX_PI;

    for (int i = 0; i < wayCount; i++) {
        double offset = (i - (wayCount - 1) / 2.0) * spreadStep;
        double spd = speed + (GetRand(20) - 10) / 100.0; // ±0.1のばらつき
        AddWarpedShot(pSet, x, y, baseMuki + offset, spd, img_enemyShotBullet[colorIdx]);
    }
}

// 過負荷を表す全方位リング弾を1組生成する(フェーズ2で使用)
static void SpawnOverloadRing(double x, double y, int wayCount, double speed, int colorIdx)
{
    sEnemyShotSet* pSet = CreateWarpedShotSet(x, y);

    double angleOffset = (count % 360) * DX_PI / 180.0; // リングごとに開始角をずらす
    for (int i = 0; i < wayCount; i++) {
        double muki = angleOffset + (2.0 * DX_PI * i) / wayCount;
        muki += (GetRand(10) - 5) / 100.0 * DX_PI; // わずかな乱れで単調さを避ける
        double spd = speed + (GetRand(20) - 10) / 100.0;
        AddWarpedShot(pSet, x, y, muki, spd, img_enemyShotDiamond[colorIdx]);
    }

    // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
}

// 反動早送りの大玉バーストを1組生成する(フェーズ3突入時に1回だけ使用)
static void SpawnCatchupBurst(double x, double y, int wayCount, double speed, int colorIdx)
{
    sEnemyShotSet* pSet = CreateWarpedShotSet(x, y);

    double baseMuki = atan2(player.y - y, player.x - x);
    double spreadStep = 0.09 * DX_PI;
    for (int i = 0; i < wayCount; i++) {
        double offset = (i - (wayCount - 1) / 2.0) * spreadStep;
        AddWarpedShot(pSet, x, y, baseMuki + offset, speed, img_enemyShotLargeBall[colorIdx]);
    }

    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Lag_Claude()
{
    static double swayAmplitude;
    static double swayFreq;
    static int spawnTimerPhase0;
    static int spawnTimerPhase1;
    static int spawnTimerPhase3;
    static bool catchupBurstFired;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        swayAmplitude = 90.0;
        swayFreq = 0.02;
        spawnTimerPhase0 = 0;
        spawnTimerPhase1 = 0;
        spawnTimerPhase3 = 0;
        catchupBurstFired = false;
    }

    // 敵本体もワープ時間で揺れる。弾だけでなくボス自身も処理落ちの影響を受ける演出。
    double warpedNow = GetWarpedTime(count);
    enemy.x = 240.0 + swayAmplitude * sin(warpedNow * swayFreq);

    int local = count % CYCLE_LEN;
    int phase = GetPhase(count);

    switch (phase) {
    case 0: {
        // フェーズ1：通常速度。自機狙いファンの間隔を徐々に短くして負荷(=弾数)を高めていく。
        int interval = 24 - (16 * local) / PHASE1_END; // 24F→8Fへ短縮
        if (interval < 8) interval = 8;
        spawnTimerPhase0++;
        if (spawnTimerPhase0 >= interval) {
            spawnTimerPhase0 = 0;
            SpawnAimedFan(enemy.x, enemy.y + 15.0, 3, 1.6, 1); // 黄、3way
        }
        if (local == PHASE1_END - 20) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;
    }
    case 1: {
        // フェーズ2：擬似処理落ち。過負荷を表す密なリング弾を、加速度的に間隔を詰めながら撒く。
        // 見た目はガクガクと停滞するが、水面下では"遅れ"がどんどん積み上がっていく。
        int elapsed = local - PHASE1_END;
        int span = PHASE2_END - PHASE1_END;
        int interval = 20 - (14 * elapsed) / span; // 20F→6Fへ短縮
        if (interval < 6) interval = 6;
        spawnTimerPhase1++;
        if (spawnTimerPhase1 >= interval) {
            spawnTimerPhase1 = 0;
            SpawnOverloadRing(enemy.x, enemy.y + 15.0, 28, 1.1, 0); // 赤、28way
        }
        break;
    }
    case 2: {
        // フェーズ3：反動早送り。フェーズ突入の瞬間に一度だけ大玉バーストを放ち、
        // 溜め込まれた弾群が一斉に加速して追いつく様を後押しする。
        if (!catchupBurstFired) {
            catchupBurstFired = true;
            SpawnCatchupBurst(enemy.x, enemy.y + 15.0, 5, 2.0, 8); // 橙、5way
        }
        break;
    }
    case 3: {
        // フェーズ4：回復。通常密度の自機狙い3wayに戻り、次サイクルへ備える。
        if (local == PHASE3_END) {
            catchupBurstFired = false;
        }
        spawnTimerPhase3++;
        if (spawnTimerPhase3 >= 26) {
            spawnTimerPhase3 = 0;
            SpawnAimedFan(enemy.x, enemy.y + 15.0, 3, 1.5, 6); // 白、3way
        }
        break;
    }
    }
}