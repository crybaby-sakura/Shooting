// EnemyPat_TooChaotic_DeepSeek.cpp
// 超次元崩壊輪廻（ハイパーカオス・サムサーラ）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>

// 位相反転波のグローバルフラグ（EnemyPat_TooChaotic_DeepSeek と各パターン関数で共有）
static bool g_phaseWave = false;
static int  g_phaseWaveTimer = 0;

//---------------------------------------------------------
// 分裂カオス弾幕パターン
//---------------------------------------------------------
static void Pattern_SplitChaos(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 初期弾：1発だけ生成
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki + (GetRand(30) - 15) * DX_PI / 180.0;
        p->speed = 1.5;
        p->kind = img_enemyShotSmallBall[0];          // 小玉:赤
        p->param_i[0] = 20;                            // 分裂までのフレーム数
        p->param_i[1] = 0;                             // 世代
        p->param_i[3] = (g_phaseWave ? 1 : 0);         // 位相反転済みフラグ
        if (g_phaseWave) {
            p->muki += DX_PI / 2.0;
            p->speed *= 1.5;
        }
        // 双方向リンク挿入
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    // 全弾の移動と分裂判定
    std::vector<sEnemyShot*> toSplit;
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->param_i[1] < 6 &&                // 5世代まで
            pShot->count >= pShot->param_i[0]) {    // 分裂タイミング
            toSplit.push_back(pShot);
        }
        pShot = pShot->next;
    }

    // 分裂処理
    for (auto* parent : toSplit) {
        for (int i = 0; i < 2; ++i) {
            sEnemyShot* child = new sEnemyShot;
            child->x = parent->x;
            child->y = parent->y;
            // ±40°のカオス的な角度変化
            double delta = (GetRand(80) - 40) * DX_PI / 180.0;
            child->muki = parent->muki + (i == 0 ? delta : -delta);
            child->speed = parent->speed * (0.9 + GetRand(20) / 100.0);

            // 次回分裂時間（親のカウントを基準に未来へ設定）
            child->param_i[0] = parent->count + GetRand(20);
            child->param_i[1] = parent->param_i[1] + 1;   // 世代+1

            // 世代ごとに色変更
            int color = child->param_i[1] % 9;
            child->kind = img_enemyShotSmallBall[color];

            // 位相反転波の影響（生成時）
            if (g_phaseWave) {
                child->muki += DX_PI / 2.0;
                child->speed *= 1.5;
                child->param_i[3] = 1;
            }
            else {
                child->param_i[3] = 0;
            }

            // 親の直後に挿入
            child->prev = parent->prev;
            child->next = parent;
            parent->prev->next = child;
            parent->prev = child;
        }
        // 親をリストから削除
        parent->prev->next = parent->next;
        parent->next->prev = parent->prev;
        delete parent;
    }

    // 既存弾への位相反転波の影響（1回だけ適用）
    if (g_phaseWave) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[3] == 0) {
                pShot->muki += DX_PI / 2.0;
                pShot->speed *= 1.5;
                pShot->param_i[3] = 1;
            }
            pShot = pShot->next;
        }
    }
}

//---------------------------------------------------------
// 量子観測弾幕パターン
//---------------------------------------------------------
static void Pattern_Quantum(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme))
            StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + GetRand(100) - 50;
            p->y = pSet->y + GetRand(40) - 20;
            p->muki = GetRand(360) * DX_PI / 180.0;
            p->speed = 1.0;
            p->kind = img_enemyShotScale[3];        // 鱗弾:シアン
            p->margin = 200;
            p->param_i[0] = 0;                        // テレポートロックフラグ
            p->param_i[3] = (g_phaseWave ? 1 : 0);
            if (g_phaseWave) {
                p->muki += DX_PI / 2.0;
                p->speed *= 1.5;
            }
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    bool shooting = (key[KEY_INPUT_V] != 0);   // ショットキーの判定

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // ショット中＆未テレポート → プレイヤー近傍へ瞬間移動
        if (shooting && pShot->param_i[0] == 0) {
            while (true) {
                pShot->x = player.x + GetRand(120) - 60;
                pShot->y = player.y + GetRand(120) - 60;
                if (hypot(pShot->y - player.y, pShot->x - player.x) > 50) break;
            }
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 0.5;
            pShot->param_i[0] = 1;               // 連続テレポート防止
        }
        if (!shooting) {
            pShot->param_i[0] = 0;               // ロック解除
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // 位相反転波の適用（既存弾）
    if (g_phaseWave) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[3] == 0) {
                pShot->muki += DX_PI / 2.0;
                pShot->speed *= 1.5;
                pShot->param_i[3] = 1;
            }
            pShot = pShot->next;
        }
    }
}

//---------------------------------------------------------
// 時空逆流弾幕パターン（裂け目を疑似的に表現）
//---------------------------------------------------------
static void Pattern_ReverseTime(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 全方位8方向弾
        for (int i = 0; i < 8; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = i * 2.0 * DX_PI / 8.0;
            p->speed = 1.5;
            p->kind = img_enemyShotLaser[5];       // 菱形弾:マゼンタ
            p->margin = 240;
            p->param_i[0] = 120;                       // 初回逆流タイミング
            p->param_i[3] = (g_phaseWave ? 1 : 0);
            if (g_phaseWave) {
                p->muki += DX_PI / 2.0;
                p->speed *= 1.5;
            }
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 逆流：向き反転＋加速
        if (pShot->count == pShot->param_i[0]) {
            pShot->muki += DX_PI;
            pShot->speed *= 1.5;
            pShot->param_i[0] = pShot->count + 60 + GetRand(60); // 次回逆流
        }
        pShot = pShot->next;
    }

    // 位相反転波の適用
    if (g_phaseWave) {
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[3] == 0) {
                pShot->muki += DX_PI / 2.0;
                pShot->speed *= 1.5;
                pShot->param_i[3] = 1;
            }
            pShot = pShot->next;
        }
    }
}

//---------------------------------------------------------
// 敵本体パターン（超次元崩壊輪廻）
//---------------------------------------------------------
void EnemyPat_TooChaotic_DeepSeek()
{
    static int muki_x = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki_x = 1;
        g_phaseWave = false;
        g_phaseWaveTimer = 0;
    }
    else {
        // 横移動＋縦方向の揺らぎ（画面の歪みを演出）
        enemy.x += 0.8 * muki_x;
        if (enemy.x > 400.0 || enemy.x < 80.0) muki_x *= -1;
        enemy.y = 40.0 + 20.0 * sin(count * 0.03);
    }

    // ----- 位相反転波（30秒周期） -----
    if (count % 600 == 599) {
        g_phaseWave = true;
        g_phaseWaveTimer = 30;                // 約0.5秒間継続
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (g_phaseWaveTimer > 0) {
        g_phaseWaveTimer--;
        if (g_phaseWaveTimer == 0) g_phaseWave = false;
    }

    // ----- 弾幕セット生成用ラムダ -----
    auto spawnSet = [](void(*pattern)(sEnemyShotSet*), double x, double y, double muki, int kind) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = pattern;
        pSet->x = x;
        pSet->y = y;
        pSet->muki = muki;
        pSet->kind = kind;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルな弾幕セットリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    };

    // ----- 各弾幕の定期発射 -----
    if (count % 120 == 1) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x);
        spawnSet(Pattern_SplitChaos, enemy.x, enemy.y, muki, 0);
        if (CheckSoundMem(sound_enemyShot_heavy))
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (count % 180 == 60) {
        spawnSet(Pattern_Quantum, enemy.x, enemy.y, 0.0, 0);
    }
    if (count % 240 == 120) {
        spawnSet(Pattern_ReverseTime, enemy.x, enemy.y, 0.0, 0);
    }
    // 追加の分裂弾幕（位置をずらして同時展開）
    if (count % 300 == 200) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x) + DX_PI / 4.0;
        spawnSet(Pattern_SplitChaos, enemy.x + 30.0, enemy.y, muki, 1);
    }
    if (count % 300 == 250) {
        double muki = atan2(player.y - enemy.y, player.x - enemy.x) - DX_PI / 4.0;
        spawnSet(Pattern_SplitChaos, enemy.x - 30.0, enemy.y, muki, 2);
    }
}