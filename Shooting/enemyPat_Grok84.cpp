// enemyPat_Tmp.cpp
// スピログラフモチーフ弾幕：二重歯車の軌跡（Spirograph Dual Gear）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 外側歯車パターン
// 円周上に等間隔配置した通常弾を回転させて「大きな歯車」を表現
// ------------------------------------------------------------
static void ShotOuterGear(sEnemyShotSet* pEnemyShotSet)
{
    const int TOOTH_NUM = 16 * 2;          // 歯の数
    const double OUTER_R = 130.0;      // 外側半径
    const double ROT_SPEED = 0.012;    // 回転速度（ラジアン/フレーム）

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 効果音（予告っぽく軽め）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < TOOTH_NUM; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double baseAng = 2.0 * DX_PI * i / TOOTH_NUM;

            // 中心・半径・基準角・回転速度をパラメータに保存
            pEnemyShot->param_d[0] = pEnemyShotSet->x;   // cx
            pEnemyShot->param_d[1] = pEnemyShotSet->y;   // cy
            pEnemyShot->param_d[2] = OUTER_R;            // radius
            pEnemyShot->param_d[3] = baseAng;            // base angle
            pEnemyShot->param_d[4] = ROT_SPEED;          // rot speed

            // 初期位置
            pEnemyShot->x = pEnemyShotSet->x + OUTER_R * cos(baseAng);
            pEnemyShot->y = pEnemyShotSet->y + OUTER_R * sin(baseAng);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // 小さな青玉で歯を表現（色4:青）
            pEnemyShot->kind = img_enemyShotSmallBall[4];

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：角度を進めて位置を更新（回転）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double cx = pShot->param_d[0];
        double cy = pShot->param_d[1];
        double r = pShot->param_d[2];
        double ang = pShot->param_d[3] + pShot->count * pShot->param_d[4];
        pShot->x = cx + r * cos(ang);
        pShot->y = cy + r * sin(ang);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 内側歯車＋軌跡弾パターン
// 内側歯車の位置を公転・自転で計算し、そこから弾を放射。
// 弾は外側回転の影響を受けて曲線（ハイポトロコイド風）を描く
// ------------------------------------------------------------
static void ShotInnerGear(sEnemyShotSet* pEnemyShotSet)
{
    // 内側歯車パラメータ
    const double OUTER_R = 130.0;   // 外側歯車半径（合わせる）
    const double INNER_R = 48.0;    // 内側歯車半径
    const double ORBIT_R = OUTER_R - INNER_R; // 公転半径
    const double ORBIT_SPEED = 0.018;   // 公転角速度
    const double SPIN_SPEED = 0.055;   // 自転角速度（公転の約3倍で歯が噛み合うイメージ）
    const int    SHOT_WAYS = 10;      // 放射方向数
    const double BULLET_SPD = 1.35;    // 基本弾速

    // 中心座標は shotSet 生成時に渡された値を使用
    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];

    // 現在の内側歯車の角度（公転・自転）
    double orbitAng = pEnemyShotSet->count * ORBIT_SPEED;
    double spinAng = pEnemyShotSet->count * SPIN_SPEED;

    // 内側歯車の中心位置
    double innerX = cx + ORBIT_R * cos(orbitAng);
    double innerY = cy + ORBIT_R * sin(orbitAng);

    // 一定間隔で弾を放射（見た目の密度調整）
    if (pEnemyShotSet->count % 5 == 0) {
        // 発射音
        if (pEnemyShotSet->count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < SHOT_WAYS; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 発射方向 = 自転角 + 放射オフセット
            double shotAng = spinAng + 2.0 * DX_PI * i / SHOT_WAYS;

            // 初期位置は内側歯車の縁付近
            double emitR = INNER_R * 0.85;
            pEnemyShot->x = innerX + emitR * cos(shotAng);
            pEnemyShot->y = innerY + emitR * sin(shotAng);

            // 基本速度ベクトル（放射方向）
            pEnemyShot->muki = shotAng;
            pEnemyShot->speed = BULLET_SPD;

            // 曲線を作るための追加パラメータ
            // param_d[0] : 外側回転の影響を受ける強さ
            // param_d[1] : 外側回転角速度
            // param_d[2] : 中心x
            // param_d[3] : 中心y
            pEnemyShot->param_d[0] = 0.55;          // 引きずり係数
            pEnemyShot->param_d[1] = 0.012;         // 外側と同じ回転速度
            pEnemyShot->param_d[2] = cx;
            pEnemyShot->param_d[3] = cy;

            // 弾種・色（赤〜マゼンタ系で内側を強調）
            // 偶数は中玉赤、奇数は菱形マゼンタで変化を付ける
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            }
            else {
                pEnemyShot->kind = img_enemyShotDiamond[5];    // マゼンタ
            }

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動更新（通常の直進 + 外側回転の接線成分を加えて曲線化）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 外側歯車の回転に引きずられる成分を追加
        // 中心からの相対位置に対して接線方向の微小速度を加算
        double dx = pShot->x - pShot->param_d[2];
        double dy = pShot->y - pShot->param_d[3];
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 1.0) {
            double drag = pShot->param_d[0] * pShot->param_d[1];
            // 接線方向（時計回り）
            pShot->x += -dy / dist * drag * dist * 0.15;
            pShot->y += dx / dist * drag * dist * 0.15;
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Spirograph_Grok()
{
    // 初回初期化
    if (count == 1) {
        // ゲーム画面は 480x480。中央やや上に配置
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定

        // ---- 外側歯車ショットセットを1つ生成（常駐） ----
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotOuterGear;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            // 中心座標を param にも保存（念のため）
            pSet->param_d[0] = enemy.x;
            pSet->param_d[1] = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // ---- 内側歯車ショットセットを1つ生成（常駐） ----
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotInnerGear;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            // 中心座標を param に渡す
            pSet->param_d[0] = enemy.x;
            pSet->param_d[1] = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // 敵本体は固定（歯車の中心として動かさない）
    // 必要ならここに軽い揺らぎを追加可能
}