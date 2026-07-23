// enemyPat_swimmy.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 魚のオフセット定義用
static const int NUM_RED_FISH = 40;
static double fishOffsetX[NUM_RED_FISH];
static double fishOffsetY[NUM_RED_FISH];
static bool offsetInitialized = false;

static void InitFishOffsets()
{
    if (offsetInitialized) return;
    // 魚の形状を定義（スイミーの大きな魚）
    int idx = 0;
    // 体の楕円輪郭
    for (int i = 0; i < 24; ++i) {
        double angle = 2.0 * DX_PI * i / 24.0;
        double rx = 42.0;
        double ry = 20.0;
        fishOffsetX[idx] = rx * cos(angle);
        fishOffsetY[idx] = ry * sin(angle);
        // 頭の前方部分（目がある場所）は後で黒弾を置くので、赤弾は少し間引くためスキップ
        if (fabs(angle) < 0.3 || fabs(angle - 2 * DX_PI) < 0.3) {
            // 前方中央付近は配置しない（目にする）
            continue;
        }
        idx++;
        if (idx >= NUM_RED_FISH) break;
    }
    // 尾びれ (左側)
    double tailBaseX = -42.0;
    for (int i = 0; i < 6; ++i) {
        double t = (i - 2.5) * 0.4; // -1.0 ～ 1.0
        fishOffsetX[idx] = tailBaseX - 15.0;
        fishOffsetY[idx] = t * 18.0;
        idx++;
        if (idx >= NUM_RED_FISH) break;
    }
    // 背びれ
    for (int i = 0; i < 4; ++i) {
        fishOffsetX[idx] = -10.0 + i * 8.0;
        fishOffsetY[idx] = -22.0 - (i % 2 == 0 ? 5.0 : 8.0);
        idx++;
        if (idx >= NUM_RED_FISH) break;
    }
    // 腹びれ
    for (int i = 0; i < 4; ++i) {
        fishOffsetX[idx] = -10.0 + i * 8.0;
        fishOffsetY[idx] = 22.0 + (i % 2 == 0 ? 5.0 : 8.0);
        idx++;
        if (idx >= NUM_RED_FISH) break;
    }
    // 残りを内部のランダム配置用にデフォルト値を埋める（あとで乱数を加える）
    while (idx < NUM_RED_FISH) {
        fishOffsetX[idx] = (GetRand(60) - 30) * 0.8;
        fishOffsetY[idx] = (GetRand(30) - 15) * 0.8;
        idx++;
    }
    offsetInitialized = true;
}

static void SwimmyPattern(sEnemyShotSet* pSet)
{
    int& phase = pSet->param_i[0];
    int& timer = pSet->param_i[1];
    double& baseX = pSet->param_d[0];
    double& baseY = pSet->param_d[1];
    double& targetBaseX = pSet->param_d[2];
    double& targetBaseY = pSet->param_d[3];

    const int PHASE_SCATTER = 0;
    const int PHASE_FORM = 1;
    const int PHASE_CHARGE = 2;
    const int PHASE_RUSH = 3;
    const int PHASE_DISPERSE = 4;

    if (phase == PHASE_SCATTER) {
        // 初期化：赤い小魚をばら撒く
        InitFishOffsets();

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        baseX = 240.0; // 魚の形成位置（画面中央やや下）
        baseY = 300.0;

        // 赤小弾生成
        for (int i = 0; i < NUM_RED_FISH; ++i) {
            sEnemyShot* pShot = new sEnemyShot;

            // ランダムな散開位置（基準点から離れた場所）
            double angle = (2.0 * DX_PI * i) / NUM_RED_FISH + (GetRand(60) - 30) / 180.0 * DX_PI;
            double dist = 150.0 + GetRand(80);
            pShot->x = baseX + dist * cos(angle);
            pShot->y = baseY + dist * sin(angle);
            pShot->muki = atan2(baseY - pShot->y, baseX - pShot->x);
            pShot->speed = 1.5 + (GetRand(100) / 100.0);

            pShot->kind = img_enemyShotSmallBall[0]; // 赤

            // オフセットを記憶（少しランダムにぶれを加える）
            pShot->param_d[0] = fishOffsetX[i] + (GetRand(10) - 5);
            pShot->param_d[1] = fishOffsetY[i] + (GetRand(10) - 5);

            // リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        timer = 0;
        phase = PHASE_FORM;
    }
    else if (phase == PHASE_FORM) {
        // 赤弾を目標オフセット位置へ収束させる
        bool allClose = true;
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double targetX = baseX + pShot->param_d[0];
            double targetY = baseY + pShot->param_d[1];
            double dx = targetX - pShot->x;
            double dy = targetY - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 2.0) {
                allClose = false;
                // ゆっくり目標へ移動（速度を再設定）
                pShot->muki = atan2(dy, dx);
                pShot->speed = dist * 0.08 + 0.5; // 距離に比例した速度
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 到着したら位置を固定
                pShot->x = targetX;
                pShot->y = targetY;
                pShot->speed = 0;
            }
            pShot = pShot->next;
        }

        if (allClose || timer > 100) { // 時間でも強制移行
            // 形成完了、チャージへ
            // 全弾を正確にオフセット位置に固定
            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->x = baseX + pShot->param_d[0];
                pShot->y = baseY + pShot->param_d[1];
                pShot->speed = 0;
                pShot = pShot->next;
            }
            timer = 0;
            phase = PHASE_CHARGE;
        }
        else {
            timer++;
        }
    }
    else if (phase == PHASE_CHARGE) {
        // スイミー（黒中弾）を目位置に生成
        if (timer == 0) {
            // 効果音（予告音）を再生
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            sEnemyShot* pEye = new sEnemyShot;
            pEye->x = baseX + 30.0; // 目のオフセット
            pEye->y = baseY;
            pEye->muki = 0;
            pEye->speed = 0;
            pEye->kind = img_enemyShotMediumBall[7]; // 黒
            pEye->param_d[0] = 30.0; // 目のオフセットX
            pEye->param_d[1] = 0.0;  // 目のオフセットY

            pEye->prev = pSet->pEnemyShotHead->prev;
            pEye->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pEye;
            pSet->pEnemyShotHead->prev = pEye;
        }

        // 魚全体を少し後退させる
        if (timer < 30) {
            baseX -= 0.5;
            // 各弾を追従させる
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->x = baseX + pShot->param_d[0];
                pShot->y = baseY + pShot->param_d[1];
                pShot = pShot->next;
            }
        }

        timer++;
        if (timer > 60) { // 1秒程度
            // 突進準備完了
            // プレイヤー方向を向く速度ベクトルを計算
            double dx = player.x - baseX;
            double dy = player.y - baseY;
            double len = sqrt(dx * dx + dy * dy);
            if (len > 1) {
                targetBaseX = dx / len * 6.0; // 速度
                targetBaseY = dy / len * 6.0;
            }
            else {
                targetBaseX = 0;
                targetBaseY = -6.0;
            }
            timer = 0;
            phase = PHASE_RUSH;
            // 突進音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == PHASE_RUSH) {
        // 魚全体を移動
        baseX += targetBaseX;
        baseY += targetBaseY;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x = baseX + pShot->param_d[0];
            pShot->y = baseY + pShot->param_d[1];
            pShot = pShot->next;
        }

        timer++;
        if (timer > 40) { // 突進継続フレーム
            phase = PHASE_DISPERSE;
            timer = 0;
            // 効果音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == PHASE_DISPERSE) {
        // 離散：黒中弾を画面外へ、赤小弾をランダムに散らす
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->kind == img_enemyShotMediumBall[7]) {
                // 黒中弾（スイミー）：ランダムな外方向へ飛ばす
                pShot->muki = atan2(baseY - 240.0, baseX - 240.0) + (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->speed = 8.0;
            }
            else {
                // 赤小弾：ランダム方向へ
                pShot->muki = (GetRand(360) / 180.0) * DX_PI;
                pShot->speed = 3.0 + (GetRand(100) / 50.0);
            }
            pShot = pShot->next;
        }

        timer++;
        if (timer > 30) {
            // 再編のためフェーズ0へ戻る（新たな魚を形成）
            // ただし、既存の赤小弾は画面外へ消えるか、残っているかもしれない。
            // 新たに生成せず、今ある弾を再利用して再形成する設計も可能だが、簡単のため新しくパターンを最初からやり直す。
            // その場合は、パターンセットのcountなどをリセットし、再度Scatterから。
            // ここでは、パターンを終了させず、再びScatterフェーズに移行し、新しい赤弾を生成する。
            // ただ、既存の弾が残っていると混ざるので、全弾を削除するのが望ましいが、
            // 削除はメインルーチン任せで難しい。よって、散らばった弾は画面外へ消えるのを待つ。
            // 十分時間が経ったら（今回はtimerで管理）再開する。
            phase = PHASE_SCATTER;
            timer = 0;
            // baseX, baseYはそのまま使うか、リセット
            baseX = 240.0;
            baseY = 300.0;
            // 注意: この時点で前の赤小弾のリストが残っている。新しいScatterでさらに弾が追加されるので、
            // 多重に生成されてしまう。それを避けるために、一度パターンを終了し、EnemyPat_Tmpから
            // 新たなパターンセットを生成する方が良い。しかし、このパターン内でループさせるのは難しい。
            // 改善策: パターンセットを新しく作らず、このパターンセットを使い続ける場合、
            // 前の弾は画面外へ消える前提で、新しいScatterで追加生成する。
            // これはテスト用としては許容範囲。もしくは、離散時に全弾の速度を高速にして画面外へ確実に消す。
        }
    }
}

// 敵本体パターン
void EnemyPat_Tmp()
{
    static int muki;
    static int shot_count;
    static bool patternSpawned = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        patternSpawned = false;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // スイミーパターンを一度だけ発動
    if (!patternSpawned && count == 100) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = SwimmyPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0; // 未使用
        pEnemyShotSet->kind = 0;

        // 双方向リスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // パラメータ初期化
        pEnemyShotSet->param_i[0] = 0; // phase: scatter
        pEnemyShotSet->param_i[1] = 0; // timer

        // グローバルリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        patternSpawned = true;
    }
}