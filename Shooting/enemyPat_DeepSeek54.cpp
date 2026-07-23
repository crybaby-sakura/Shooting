// enemyPat_Tmp.cpp
// ビールかけをモチーフにした弾幕パターン「祝勝の泡濁流」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  ヘルパー：弾をリスト末尾に追加する
// ------------------------------------------------------------
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int type)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[3] = type;  // 0:ビール粒  1:泡  2:炭酸気泡
    // その他パラメータ初期化
    for (int i = 0; i < 8; i++) {
        if (i != 3) p->param_i[i] = 0;
        p->param_d[i] = 0.0;
    }

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
//  弾幕パターン「祝勝の泡濁流」
// ------------------------------------------------------------
static void BeerPour(sEnemyShotSet* pEnemyShotSet)
{
    // サウンド：開始時に一回だけ重い効果音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // ----- 弾の生成（注ぐ→泡→破裂） -----
    const int PHASE1_END = 180;   // ビール粒を降らせる期間
    const int PHASE2_END = 360;   // 泡を発生させる期間

    // フェーズ1：ビール粒（琥珀色の小玉）の滝
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->count <= PHASE1_END) {
        // 注ぎ口は敵の少し上あたりに設定（x: 敵±30, y: 敵+10）
        double sx = enemy.x + GetRand(60) - 30.0;
        double sy = enemy.y + 10.0;
        // 下方向（DX_PI/2）±15度の散らばり
        double ang = DX_PI / 2.0 + (GetRand(30) - 15.0) * (DX_PI / 180.0);
        double spd = (200.0 + GetRand(100)) / 100.0;  // 2.0～3.0
        // 小玉・黄色(1)　＝　img_enemyShotSmallBall[1]
        AddShot(pEnemyShotSet, sx, sy, ang, spd, img_enemyShotSmallBall[1], 0);
    }

    // フェーズ2：泡（白い大玉）の湧き上がり
    if (pEnemyShotSet->count % 7 == 0 &&
        pEnemyShotSet->count > PHASE1_END &&
        pEnemyShotSet->count <= PHASE2_END) {
        double sx = GetRand(480);   // 画面下の広範囲
        double sy = 440.0;
        // 泡は垂直に近い上方向、ふわふわ感は後で正弦波を足す
        double ang = -DX_PI / 2.0 + (GetRand(20) - 10.0) * (DX_PI / 180.0);
        double spd = 0.0;   // 移動は手動で行うので速度0
        // 大玉・白色(6)　＝　img_enemyShotLargeBall[6]
        AddShot(pEnemyShotSet, sx, sy, ang, spd, img_enemyShotLargeBall[6], 1);

        // ふわふわ用のデータを最後に追加した弾に設定（pEnemyShotHead->prev）
        sEnemyShot* pNew = pEnemyShotSet->pEnemyShotHead->prev;
        pNew->param_d[0] = sx;               // 基準X座標
        pNew->param_i[4] = GetRand(360);      // 正弦波の位相（度）
    }

    // ----- 既存弾の更新 -----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 毎フレームの位置更新（速度と角度による）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        int type = pShot->param_i[3];

        // タイプ0：ビール粒（跳ね返り処理）
        if (type == 0) {
            // 画面下端で一度だけ跳ね返る（param_i[0]==0 は未跳ね返り）
            if (pShot->param_i[0] == 0 && pShot->y > 440.0) {
                pShot->muki = -pShot->muki;      // 上下反転
                pShot->speed *= 0.6;              // 減衰
                pShot->param_i[0] = 1;            // 跳ね返り済み
            }
        }
        // タイプ1：泡（手動で上昇＋正弦波の横揺れ）
        else if (type == 1) {
            // y座標：下から上へゆっくり移動（countは生成後の経過フレーム）
            pShot->y = 440.0 - 0.8 * pShot->count;
            // x座標：基準位置＋横揺れ
            double baseX = pShot->param_d[0];
            double phase = pShot->param_i[4];
            pShot->x = baseX + 30.0 * sin((pShot->count * 0.1 + phase) * DX_PI / 180.0);

            // 泡の破裂（生成から120フレーム経過で破裂）
            if (pShot->count >= 120) {
                // 炭酸気泡（シアンの小玉）を8方向にばら撒く
                for (int i = 0; i < 8; i++) {
                    double ang = (i * (2.0 * DX_PI) / 8.0) + (GetRand(30) - 15.0) * (DX_PI / 180.0);
                    double spd = 2.5 + GetRand(50) / 100.0;  // 2.5～3.0
                    // 小玉・シアン(3)　＝　img_enemyShotSmallBall[3]
                    AddShot(pEnemyShotSet, pShot->x, pShot->y, ang, spd, img_enemyShotSmallBall[3], 2);
                }

                // 破裂時の軽い効果音
                if (CheckSoundMem(sound_enemyShot_light) == 0) {
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }

                // 破裂した泡は画面外へ送り、消去されるのを待つ
                pShot->y = -100.0;
                pShot->speed = 0.0;
            }
        }
        // タイプ2：炭酸気泡（特に何もしない、等速直進）

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体パターン
// ------------------------------------------------------------
void EnemyPat_BeerSpray_DeepSeek()
{
    static int muki = 1;   // 左右移動の方向

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60フレーム目に「祝勝の泡濁流」を一回だけ発動
    if (count % 480 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = BeerPour;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // ダミーヘッドの作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 管理リストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}