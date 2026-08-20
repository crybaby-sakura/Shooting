#include "DxLib.h"
#include "gv.h"
#include "enemyShot.h"
#include "imgSoundLoad.h"
#include "stateManager.h"
#include "player.h"
#include "tasController.h"


void enemyShotControl(){
	sEnemyShotSet *pEnemyShotSet;
	
	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		sEnemyShotSet* pNext = pEnemyShotSet->next;

		pEnemyShotSet->patternFunc(pEnemyShotSet);

		pEnemyShotSet = pNext;
	}
}

void enemyShotCalc()
{
	sEnemyShotSet *pEnemyShotSet, *pNextEnemyShotSet;
	sEnemyShot *pEnemyShot, *pNextEnemyShot;

	pEnemyShotSet = enemyShotSetHead.next;
	while(pEnemyShotSet != &enemyShotSetHead){
		pEnemyShotSet->count++;
		
		pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while(pEnemyShot != pEnemyShotSet->pEnemyShotHead){
			pEnemyShot->count++;
			
			pNextEnemyShot = pEnemyShot->next;

			if (pEnemyShot->x < -pEnemyShot->margin || pEnemyShot->x > 480.0 + pEnemyShot->margin //画面外の弾は削除
				|| pEnemyShot->y < -pEnemyShot->margin || pEnemyShot->y > 480.0 + pEnemyShot->margin) {
				pEnemyShot->prev->next = pEnemyShot->next;
				pEnemyShot->next->prev = pEnemyShot->prev;
				delete pEnemyShot;
			}
			
			pEnemyShot = pNextEnemyShot;
		}
		
		pNextEnemyShotSet = pEnemyShotSet->next;
		
		if(pEnemyShotSet->pEnemyShotHead->next==pEnemyShotSet->pEnemyShotHead && pEnemyShotSet->count > pEnemyShotSet->alive){ //空のセットは削除
			delete pEnemyShotSet->pEnemyShotHead;
			
			pEnemyShotSet->prev->next = pEnemyShotSet->next;
			pEnemyShotSet->next->prev = pEnemyShotSet->prev;
			delete pEnemyShotSet;
		}
		
		pEnemyShotSet = pNextEnemyShotSet;
	}
}

void enemyShotDisp()
{
	// TASモードの場合、あらかじめブレンドモードと色を設定しておく
	unsigned int tasColor = 0;
	if (g_isTasMode) {
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 196);
		tasColor = GetColor(255, 255, 255); // 色は好みに合わせて変更してください（ここでは赤）
	}
	
	sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;
	while (pEnemyShotSet != &enemyShotSetHead) {
		sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

			if (g_isTasMode) {
				// ==========================================
				// TASモード：敵弾画像の代わりに当たり判定を描画
				// ==========================================

				// ここでは「敵弾自身の当たり判定サイズ」を設定しています。
				double rx = imageData[pEnemyShot->kind].radiusX;
				double ry = imageData[pEnemyShot->kind].radiusY;

				// 【補足】
				// enemyShotHit() の計算と同様に、「自機の中心がここに入ったら被弾する」という
				// 実際の判定領域（プレイヤーの半径を加算したサイズ）を描画したい場合は、
				// 以下の2行のコメントアウトを外してください。
				// rx += imageData[img_player].radiusX;
				// ry += imageData[img_player].radiusY;

				if (!imageData[pEnemyShot->kind].rotatable) {
					// 回転しない弾は DxLib 標準の DrawOval で描画
					DrawOval((int)pEnemyShot->x, (int)pEnemyShot->y, (int)rx, (int)ry, tasColor, TRUE);
				}
				else {
					// 弾が回転する場合、DxLib に傾いた楕円を描画する標準関数がないため
					// 中心から頂点へ三角形(DrawTriangle)を敷き詰めて扇形で近似描画する
					int cx = (int)pEnemyShot->x;
					int cy = (int)pEnemyShot->y;
					double muki = pEnemyShot->muki;

					const int DIV = 32; // 分割数（数値を大きくするとより滑らかな円になります）
					double c = cos(muki);
					double s = sin(muki);

					// DX_PI は DxLib で定義されている円周率の定数
					for (int i = 0; i < DIV; i++) {
						double a1 = DX_PI * 2.0 * i / DIV;
						double a2 = DX_PI * 2.0 * (i + 1) / DIV;

						// ローカル座標での頂点
						double lx1 = rx * cos(a1);
						double ly1 = ry * sin(a1);
						double lx2 = rx * cos(a2);
						double ly2 = ry * sin(a2);

						// muki に合わせて回転させたワールド座標
						int px1 = cx + (int)(lx1 * c - ly1 * s);
						int py1 = cy + (int)(lx1 * s + ly1 * c);
						int px2 = cx + (int)(lx2 * c - ly2 * s);
						int py2 = cy + (int)(lx2 * s + ly2 * c);

						// 三角形を描画して塗りつぶす
						DrawTriangle(cx, cy, px1, py1, px2, py2, tasColor, TRUE);
					}
				}
			}
			else {
				// ==========================================
				// 通常モード：従来通りの画像描画
				// ==========================================
				if (!imageData[pEnemyShot->kind].rotatable) {
					DrawRotaGraph((int)pEnemyShot->x, (int)pEnemyShot->y,
						imageData[pEnemyShot->kind].mag, 0.0,
						imageData[pEnemyShot->kind].handle, TRUE, FALSE);
				}
				else {
					DrawRotaGraph((int)pEnemyShot->x, (int)pEnemyShot->y,
						imageData[pEnemyShot->kind].mag, pEnemyShot->muki,
						imageData[pEnemyShot->kind].handle, TRUE, FALSE);
				}
			}

			pEnemyShot = pEnemyShot->next;
		}

		pEnemyShotSet = pEnemyShotSet->next;
	}

	// 描画が終わったらブレンドモードを元に戻す
	if (g_isTasMode) {
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void enemyShotHit()
{
	sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;
	while (pEnemyShotSet != &enemyShotSetHead) {
		sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
		while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
			// プレイヤーとの相対座標
			double dx = player.x - pEnemyShot->x;
			double dy = player.y - pEnemyShot->y;

			// 弾の半径（画像に登録されたもの＋マージン）
			double rx = imageData[pEnemyShot->kind].radiusX + imageData[img_player].radiusX;
			double ry = imageData[pEnemyShot->kind].radiusY + imageData[img_player].radiusY;

			// 弾が回転する場合は逆回転してローカル座標系に変換
			if (imageData[pEnemyShot->kind].rotatable) {
				double c = cos(-pEnemyShot->muki);
				double s = sin(-pEnemyShot->muki);
				double lx = dx * c - dy * s;
				double ly = dx * s + dy * c;
				dx = lx;
				dy = ly;
			}

			// 楕円の内側かで簡易判定
			if ((dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) < 1.0) {
				if (CheckSoundMem(sound_playerDestroyed)) StopSoundMem(sound_playerDestroyed);
				PlaySoundMem(sound_playerDestroyed, DX_PLAYTYPE_BACK);

				if (!isMuteki) {
					StateManager::ChangeState(Joutai::Lose);
				}
			}

			pEnemyShot = pEnemyShot->next;
		}
		pEnemyShotSet = pEnemyShotSet->next;
	}
}
