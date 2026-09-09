// enemyPat_Tmp.cpp
// 弾幕名: くるくるヘリリン迷宮
// モチーフ: くるくるくるりん

static void ShotKururin(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->param_d の使い方
    // [0]: 現在の回転角度 angle (rad)
    // [1]: 回転速度の絶対値 speed
    // [2]: 回転方向 dir +1:時計回り / -1:反時計回り
    // [3]: 前回の停止からの累積回転量
    // pEnemyShotSet->param_i の使い方
    // [0]: 状態 0=時計回り回転中 / 1=反時計回り回転中 / 2=停止中(壁際)
    // [1]: 停止カウンタ
    // pEnemyShot->param_d[0]: 棒に沿ったオフセット along
    // pEnemyShot->param_d[1]: 棒に垂直なオフセット perp (プロペラ用)
    // pEnemyShot->param_i[0]: 0=ヘリリン本体(制御弾) / 1=排気(自由弾)

    const double STICK_SPACING = 14.0;
    const int STICK_HALF = 6; // -6..+6 で12本、中央は軸に譲る
    const double TIP_OFFSET = STICK_HALF * STICK_SPACING;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 初期化
        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_d[1] = 0.012; // 約0.7度/frame
        pEnemyShotSet->param_d[2] = 1.0;
        pEnemyShotSet->param_d[3] = 0.0;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;

        // 1. 軸 : 大玉 白(6) 半径20.0
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = 0;
            p->speed = 0;
            p->kind = img_enemyShotLargeBall[6]; // 6:白
            p->margin = 200.0;
            p->param_d[0] = 0.0;
            p->param_d[1] = 0.0;
            p->param_i[0] = 0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 2. 棒本体 : 中楕円弾 橙(8) 10.5x7.0 隙間なく並べて棒に見せる
        for (int i = -STICK_HALF; i <= STICK_HALF; ++i) {
            if (i == 0) continue; // 中心は軸
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = 0;
            p->speed = 0;
            p->kind = img_enemyShotMediumOval[8]; // 8:橙 木目っぽさ
            p->margin = 200.0;
            p->param_d[0] = i * STICK_SPACING; // along
            p->param_d[1] = 0.0; // perp
            p->param_i[0] = 0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // 3. プロペラ(羽根) : 菱形弾 赤と青で両端に2枚ずつ
        // 進行方向に対して垂直に付くイメージ
        const int ends[2] = { -STICK_HALF, STICK_HALF };
        const int colors[2] = { 0, 4 }; // 0:赤 4:青
        for (int e = 0; e < 2; ++e) {
            for (int s = -1; s <= 1; s += 2) { // 両側
                if (s == 0) continue;
                sEnemyShot* p = new sEnemyShot;
                p->x = pEnemyShotSet->x;
                p->y = pEnemyShotSet->y;
                p->muki = 0;
                p->speed = 0;
                p->kind = img_enemyShotDiamond[colors[e]]; // 4.5x2.5
                p->margin = 200.0;
                p->param_d[0] = ends[e] * STICK_SPACING; // along = 先端
                p->param_d[1] = s * 6.0; // perp = 少し横にずらす
                p->param_i[0] = 0;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }
    else {
        // --- 状態管理: 回転→停止→反転 (くるりんの壁際の切り返し) ---
        if (pEnemyShotSet->param_i[0] == 2) { // 停止中
            pEnemyShotSet->param_i[1]--;
            if (pEnemyShotSet->param_i[1] <= 0) {
                // 反転して再始動
                pEnemyShotSet->param_d[2] *= -1.0;
                pEnemyShotSet->param_i[0] = (pEnemyShotSet->param_d[2] > 0) ? 0 : 1;
                pEnemyShotSet->param_d[3] = 0.0;
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }
        else { // 回転中
            double dir = pEnemyShotSet->param_d[2];
            double spd = pEnemyShotSet->param_d[1];
            pEnemyShotSet->param_d[0] += dir * spd;
            pEnemyShotSet->param_d[3] += spd;

            // 約180度回ったら壁にぶつかったとみなして停止
            if (pEnemyShotSet->param_d[3] >= DX_PI) {
                pEnemyShotSet->param_i[0] = 2; // 停止へ
                pEnemyShotSet->param_i[1] = 28; // 28F停止
                pEnemyShotSet->param_d[3] = 0.0;        
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }

        double angle = pEnemyShotSet->param_d[0];
        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;
        double cosA = cos(angle);
        double sinA = sin(angle);
        double cosA90 = cos(angle + DX_PI / 2.0);
        double sinA90 = sin(angle + DX_PI / 2.0);

        // 既存弾の更新
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) { // ヘリリン本体は角度から位置を再計算
                double along = pShot->param_d[0];
                double perp = pShot->param_d[1];
                pShot->x = cx + along * cosA + perp * cosA90;
                pShot->y = cy + along * sinA + perp * sinA90;
                // 楕円と菱形は棒の向きに合わせる
                // 中楕円は棒方向、菱形は垂直方向
                if (perp == 0.0) {
                    pShot->muki = angle; // 棒と平行
                }
                else {
                    pShot->muki = angle + DX_PI / 2.0; // プロペラは垂直
                }
            }
            else { // 排気弾は自由落下
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        // 排気エフェクト: 先端から小玉をばら撒く (回転中のみ)
        if (pEnemyShotSet->param_i[0] != 2 && pEnemyShotSet->count % 7 == 0) {
            // 両端の先端位置を計算
            for (int side = -1; side <= 1; side += 2) {
                double tipAlong = side * TIP_OFFSET;
                double tipX = cx + tipAlong * cosA;
                double tipY = cy + tipAlong * sinA;

                // 外向きに発射 (棒の延長線上)
                double baseMuki = (side > 0) ? angle : angle + DX_PI;
                // GetRandは 0..x を返すので注意
                double spread = (GetRand(20) - 10) / 180.0 * DX_PI; // -10度～+10度
                double spd = (150 + GetRand(100)) / 100.0; // 1.5～2.5

                sEnemyShot* p = new sEnemyShot;
                p->x = tipX + 4.0 * cos(baseMuki); // 少し外側から出す
                p->y = tipY + 4.0 * sin(baseMuki);
                p->muki = baseMuki + spread;
                p->speed = spd;
                p->kind = img_enemyShotSmallBall[3]; // 3:シアン 小玉 2.5x2.5 排気っぽい
                p->margin = 20.0;
                p->param_i[0] = 1; // 自由弾
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }
}

void EnemyPat_KuruKuruKururin_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵本体は上部で微左右移動(おまけ)
        enemy.x += sin(count / 60.0) * 0.3;
    }

    double xs[7] = { 240.0, 80.0, 400.0, 160.0, 320.0, 160.0, 320.0 };
    double ys[7] = { 240.0, 240.0, 240.0, 400.0, 400.0, 80.0, 80.0 };

    const int T = 140;
    if (count % T == 1 && count / T < 7) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKururin;
        pSet->x = xs[count / T];
        pSet->y = ys[count / T];
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}