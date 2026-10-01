// 竹やぶ焼けた - TakeyabuYaketa
// 使用素材選定:
// img_enemyShotLaser[2] 緑 : 竹本体。64x4の細長形状が竹に最適
// img_enemyShotSmallBall[1] 黄 / [8] 橙 : 火の粉。小さく明るくパチパチ感を出せる
// img_enemyShotScale[2] 緑 : 竹の葉。4x3のヒラヒラした形状
// img_enemyShotSmallBall[6] 白 : 燃えカスの灰
// sound_enemyShot_heavy : 竹林設置、sound_enemyShot_light : 火の粉噴き上げ

// ------------------------------------------------------------
// 竹林フィールド - 竹を立てて、時間差で爆ぜさせて葉と灰に変える
// ------------------------------------------------------------
static void ShotBambooField(sEnemyShotSet* pSet)
{
    const int BAMBOO_NUM = 22;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < BAMBOO_NUM; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 画面横に均等配置 + 少しランダムで自然な竹やぶに
            // GetRand(x)は 0〜x までなので 0〜20なら GetRand(20)
            double baseX = 15.0 + i * (450.0 / BAMBOO_NUM) + (double)(GetRand(20) - 10);
            pShot->x = baseX;
            pShot->y = pSet->y + (double)(GetRand(60) - 30);
            pShot->muki = DX_PI * 0.5 + (GetRand(20) - 10) / 180.0 * DX_PI; // 真下向き+少し傾き
            pShot->speed = 0.35 + GetRand(30) / 100.0; // 0.35〜0.65 超低速落下

            pShot->kind = img_enemyShotLaser[2]; // 緑レーザー = 竹

            pShot->param_d[0] = baseX; // 揺れの中心X
            pShot->param_d[1] = (double)GetRand(100); // 揺れ位相
            pShot->param_i[0] = i; // インデックス(爆発順番用)
            pShot->param_i[1] = 0; // 未爆発

            // 連結リストに登録
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next; // 削除に備えて先に保存

        if (p->kind == img_enemyShotLaser[2]) {
            // --- 竹本体の挙動 ---
            double sway = sin((pSet->count + p->param_d[1]) * 0.06) * 0.6;
            p->x = p->param_d[0] + sway;
            p->y += p->speed;
            p->muki = DX_PI * 0.5 + sin((pSet->count + p->param_d[1]) * 0.04) * 0.12;

            // 下から火が来る演出: yが高い(下にある)竹ほど早く、下から順に爆ぜる
            // 100F後からインデックス順+ランダムで爆発
            int explodeBase = 100 + p->param_i[0] * 7 + (int)(p->y * 0.25);
            if (pSet->count > explodeBase + GetRand(30)) {
                // 葉を6枚
                for (int k = 0; k < 6; k++) {
                    sEnemyShot* leaf = new sEnemyShot;
                    leaf->x = p->x;
                    leaf->y = p->y;
                    leaf->muki = (GetRand(360)) / 180.0 * DX_PI;
                    leaf->speed = 1.0 + GetRand(150) / 100.0;
                    leaf->kind = img_enemyShotScale[2]; // 緑鱗弾 = 葉

                    leaf->param_d[1] = (double)GetRand(100); // 揺れ位相
                    leaf->param_d[2] = (GetRand(100) - 50) / 100.0; // 横ドリフト
                    leaf->param_d[3] = 0.0; // 落下加速度蓄積

                    leaf->prev = pSet->pEnemyShotHead->prev;
                    leaf->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = leaf;
                    pSet->pEnemyShotHead->prev = leaf;
                }
                // 灰を3個
                for (int k = 0; k < 3; k++) {
                    sEnemyShot* ash = new sEnemyShot;
                    ash->x = p->x;
                    ash->y = p->y;
                    ash->muki = DX_PI * 0.5 + (GetRand(40) - 20) / 180.0 * DX_PI;
                    ash->speed = 0.4 + GetRand(80) / 100.0;
                    ash->kind = img_enemyShotSmallBall[6]; // 白小玉 = 灰

                    ash->param_d[1] = (double)GetRand(100);
                    ash->param_d[2] = 0.0;

                    ash->prev = pSet->pEnemyShotHead->prev;
                    ash->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = ash;
                    pSet->pEnemyShotHead->prev = ash;
                }

                // 竹本体を削除
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
        }
        else if (p->kind == img_enemyShotScale[2]) {
            // --- 葉のヒラヒラ落下 ---
            p->param_d[3] += 0.04; // 重力
            p->y += p->param_d[3] + p->speed * sin(p->muki) * 0.2;
            p->x += sin((pSet->count + p->param_d[1]) * 0.08) * 0.8 + p->param_d[2];
            p->muki += 0.07; // クルクル回る
        }
        else {
            // --- 灰のフワフワ落下 ---
            p->y += p->speed;
            p->x += sin((pSet->count + p->param_d[1]) * 0.05) * 0.5;
            p->speed *= 0.998; // だんだん遅くなる
        }

        p = pNext;
    }
}

// ------------------------------------------------------------
// 火の粉噴き上げ - 下から上にパチパチ上がる
// ------------------------------------------------------------
static void ShotFireSparks(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 14; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 画面下からランダムXで噴き上げ
            pShot->x = (double)GetRand(480); // 0〜480
            pShot->y = 480.0 + GetRand(20);
            pShot->muki = -DX_PI * 0.5 + (GetRand(60) - 30) / 180.0 * DX_PI; // 真上+ばらつき
            pShot->speed = 3.0 + GetRand(200) / 100.0; // 3.0〜5.0

            // 黄と橙を交互で火っぽく
            if (GetRand(1) == 0) pShot->kind = img_enemyShotSmallBall[1]; // 黄
            else pShot->kind = img_enemyShotSmallBall[8]; // 橙

            pShot->param_d[0] = (double)GetRand(100); // 揺れ位相
            pShot->param_d[1] = (GetRand(40) - 20) / 100.0; // 横ドリフト

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next;

        // パチパチ揺れながら上昇
        p->x += p->param_d[1] + sin((pSet->count + p->param_d[0]) * 0.15) * 0.7;
        p->y += p->speed * sin(p->muki);
        p->speed *= 0.985; // 上に行くほど失速

        // 画面外はメインで消されるが、念のため早めに消すとプール節約
        if (p->y < -10.0) {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
        }

        p = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_TakeyabuYaketa_MetaAI()
{
    static int dir;

    if (::count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        dir = 1;
    }
    else {
        // ゆっくり左右に揺れる、竹やぶの奥にいる雰囲気
        enemy.x += 0.6 * (double)dir;
        if (::count % 180 == 90) dir *= -1;
    }

    // 1回だけ竹林を立てる
    if (::count % 400 == 20) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBambooField;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 火の粉は定期的に噴き上げる
    if (::count >= 70 && ::count % 14 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFireSparks;
        pSet->x = enemy.x;
        pSet->y = 480.0;
        pSet->muki = -DX_PI * 0.5;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 予告音で燃え広がるタイミングを強調
    if (::count % 400 == 95) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}