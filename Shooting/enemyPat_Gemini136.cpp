#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン1：水柱（重力で落下するアーチ状のメイン攻撃）
static void ShotFountainColumn(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 1回の生成につき数発の小玉を飛ばす
        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 角度を少しばらけさせる (-0.05 ~ 0.05 ラジアン)
            double angleOffset = (GetRand(100) - 50) / 1000.0;
            double a = pEnemyShotSet->muki + angleOffset;

            // 初速のばらつき (4.0 ~ 5.5)
            double s = 4.0 + GetRand(150) / 100.0;

            // 放物線を描かせるため、xy成分の速度を param_d に保持
            pEnemyShot->param_d[0] = s * cos(a); // vx
            pEnemyShot->param_d[1] = s * sin(a); // vy

            // 水しぶきの表現として、小玉のシアン(3)と青(4)をランダムに混ぜる
            pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[4];

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 下向きの重力加速度を毎フレーム加算
        pShot->param_d[1] += 0.15;

        // 座標の更新
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        // 描画の向きを実際の移動方向に合わせる
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);

        pShot = pShot->next;
    }
}

// 弾幕パターン2：水塊の落下と波紋（足場を制限するサブ攻撃）
static void ShotFountainDrop(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 真下へ
        pEnemyShot->speed = 2.0;        // ゆっくりと落下
        pEnemyShot->kind = img_enemyShotLargeBall[3]; // 大玉のシアン
        pEnemyShot->param_i[0] = 0;     // 判定フラグ (0: 大玉, 1: 波紋弾)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // --- 大玉の処理 ---
            pShot->y += pShot->speed;

            // 画面下部（y=450）に着水したら破裂して波紋を発生させる
            if (pShot->y > 450.0) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 波紋の発生（速度の違う2重リング）
                for (int j = 0; j < 2; j++) {
                    for (int i = 0; i < 20; i++) {
                        sEnemyShot* pRipple = new sEnemyShot;
                        pRipple->x = pShot->x;
                        pRipple->y = pShot->y;

                        // 内側と外側で角度を互い違いにする
                        pRipple->muki = (double)i * DX_PI * 2.0 / 20.0 + (j * DX_PI / 20.0);
                        pRipple->speed = 2.0 + j * 0.5; // 外側のリングを少し速くする
                        pRipple->kind = img_enemyShotDiamond[4]; // 菱形弾の青
                        pRipple->param_i[0] = 1; // 判定フラグを波紋弾(1)に

                        // 現在のリスト位置の手前に波紋弾を挿入
                        pRipple->prev = pShot->prev;
                        pRipple->next = pShot;
                        pShot->prev->next = pRipple;
                        pShot->prev = pRipple;
                    }
                }

                // 大玉自身の削除処理
                sEnemyShot* del = pShot;
                pShot = pShot->next; // 削除する前に次のポインタへ進めておく
                del->prev->next = del->next;
                del->next->prev = del->prev;
                delete del; // gv.hのカスタムdeleteによりメモリプールへ返却される
                continue;   // すでにpShotは次へ進んでいるためcontinue
            }
        }
        else {
            // --- 波紋弾の処理 ---
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン：フォンテーン・スイング
void EnemyPat_Fountain_Gemini()
{
    static int muki;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 噴水のアーチを画面内に綺麗に描かせるため、少し低めに配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右へのゆったりとした移動
        enemy.x += 1.5 * (double)muki;
        if (enemy.x < 100.0) muki = 1;
        if (enemy.x > 380.0) muki = -1;
    }

    // 首振りの角度計算
    // 周期120フレームで真上(-DX_PI/2)を中心に ±60度(DX_PI/3) 左右に揺れる
    double swingAngle = -DX_PI / 2.0 + sin(count * DX_PI / 60.0) * (DX_PI / 3.0);

    // 水柱（メイン攻撃）の発射: 4フレームに1回のペースで連続生成
    if (count % 4 == 1) {
        // 発射音がうるさくならないように 12フレームに1回だけ再生する
        if (count % 12 == 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFountainColumn;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = swingAngle; // 現在の首振り角度を渡す

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 大玉（足場制限）の発射: 60フレームに1回
    // (count % 120 が 30 または 90 のとき、つまり首振りがちょうど左右の端に達した瞬間に発射)
    if (count % 60 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFountainDrop;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}