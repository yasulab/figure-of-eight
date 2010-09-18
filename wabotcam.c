#include <malloc.h>
#include "eyebot.h"
#include "wabotcam.h"

/* RGB値からHUE値を算出する (0 ～ 255)
 * また、グローバル変数 definehue に値を設定する
 * 灰色など色相値を算出できない場合 -> definehue = 0
 * 色相値が算出できた場合 -> definehue = 1
 */
BYTE rgb2hue(BYTE r, BYTE g, BYTE b){
	int max, min, delta;
	int tr;
	
	max = MAX(r, MAX(g, b));
	min = MIN(r, MIN(g, b));
	delta = (max - min);
	
	if (delta != 0) {
		definehue = 1;
	}
	else {
		definehue = 0;
		return 0;
	}
	
	if (max == r){
		//return (int)(255.0 * (1.0 * (g - b) / delta) / 6.0);  // for PC
		// fix for negative value (thanks to mizuno)
		tr = (425 * ((1000 * (g - b)) / delta)) / 10000;			// for Eyebot
		return tr &= 0xff;
	}
	else if (max == g){
		// return (int)(255.0 * (2.0 + 1.0 * (b - r) / delta) / 6.0); // for PC
		return (425 * (2000 + (1000 * (b - r)) / delta)) / 10000;	// for Eyebot
	}
	else {
		//return (int)(255.0 * (4.0 + 1.0 * (r - g) / delta) / 6.0);  // for PC
		return (425 * (4000 + (1000 * (r - g)) / delta)) / 10000;	// for Eyebot
	}
}

/* RGB値からSAT値を算出する (0 ～ 255) */
BYTE rgb2sat(BYTE r, BYTE g, BYTE b){
	int max, min;
	
	max = MAX(r, MAX(g, b));
	min = MIN(r, MIN(g, b));
	
	if (max == 0) {
		return 0;
	}
	else{
		return ((2550000 * (max - min)) / max) / 10000;
	}
}

/* RGB値からINT値を算出する (0 ～ 255) */
__inline__ BYTE rgb2int(BYTE r, BYTE g, BYTE b){
	return MAX(r, MAX(g, b));
}

/* 色相値 hue が色相情報 color において有効かどうか */
__inline__ int isHueArea(bitset *color, int hue){
	return bitset_get(color, hue);
}


/* C による簡易 bitset の実装 (thanks to mizuno)
 * このライブラリでは色相情報の保持に使用する
 * 256bit の bitset を用意し、0 ～ 255 の色相値を各ビットに関連付ける
 * 
 * 000001111111111000000000.......0000
 * この場合、赤色であることを判別するための色相情報になっている
 */

/* bitset の初期化 */
bitset *bitset_init(int length){
  bitset *bs;
  bs = (bitset *)malloc(((length + (BS_NBIT-1)) / BS_NBIT) * sizeof(long));
  return bs;
}
/* bitset の pos ビット目の値を取得 (pos >= 0) */
__inline__ int bitset_get(bitset *bs, int pos){
  return (bs[pos >> BS_SHIFT] & 1LL << (pos & BS_MASK)) != 0;
}
/* bitset の pos ビット目に値を設定 (pos >= 0) */
__inline__ void bitset_put(bitset *bs, int pos, int val){
  if (val) {
    bs[pos >> BS_SHIFT] |= 1LL << (pos & BS_MASK);
  }
  else {
    bs[pos >> BS_SHIFT] &= ~(1LL << (pos & BS_MASK));
  }
}


/* 色相テーブルの実装 
 * bitset を使うことで一つの色について 256 bit で色相情報を保持できる
 * この bitset を配列で持つことで複数の色の色相情報を保持できる */

/* 色相テーブルの初期化 
 * num には現在の色数が入る。これは初期化命令なので 1 になる。
 * num は他でも使うので便宜的に呼び出すようにしている。
 */
bitset **colordata_init(int *num){
	bitset **color;
	color = (bitset **)malloc(sizeof(bitset *) * 1);
	color[0] = bitset_init(256);
	*num = 1;
	return color;
}

/* 色相テーブルに新しい色相情報用のデータ領域を確保する */
bitset **colordata_add(bitset **color, int *num){
	int p = *num;

	p++;
	color = (bitset **)realloc(color, sizeof(bitset *) * p);
	color[p - 1] = bitset_init(256);
	
	*num = p;
	return color;
}

/* 色相テーブルを解放 */
void colordata_free(bitset **color, int num){
	int c;
	for (c = 0; c < num; c++){
		free(color[c]);
	}
	free(color);
}


/* 色相テーブルのセットアッププログラム
 * カメラから色相値を取得し、色相テーブルに順次格納していく
 * number には取得した色数
 * bitset ** が色相テーブルとなる
 */
bitset **COLORSetup(int *number){
	int x, y, c, key;
	int max, hue, huegraph[256];
	colimage buf;

	bitset **bs;		// 色相テーブルへのポインタ
	int num;			// 色数

	/* 色相テーブルの初期化 */ 
	bs = colordata_init(&num);

	do{
		/* ヒストグラム用のバッファ */
		for (c = 0; c < 256; c++){
			huegraph[c] = 0;
		}	

		do{	
			LCDMenu(" ", " ", " ", "CAM");
			LCDSetPos(0, 10); LCDPrintf("# %2d", num);
			
			/* 撮影待ち */
			CAMGetColFrame(&buf, 0);
			while (KEYRead() != KEY4){
				LCDPutColorGraphic(&buf);
				CAMGetColFrame(&buf, 0);
			}
			AUBeep();
	
			/* 撮影した画像を色相値に変換 */
			for (y = 1; y < 61; y++){
				for (x = 1; x < 81; x++){
					hue = rgb2hue(buf[y][x][0], buf[y][x][1], buf[y][x][2]);
					
					/* 色相値が得られていれば、ヒストグラムに追加 */
					if (definehue){
						huegraph[hue]++;
					}
				}
			}

			/* もう一枚、同じ色について撮るかどうか
			 * MORE: もう一枚撮る
			 *   OK: ヒストグラムの処理へ
			 */
			LCDMenu(" ", " ", "MORE", " OK");
			key = 0;
			while ((key = KEYRead()) != KEY3 && key != KEY4)
				;

		}while(key == KEY3);

		/* ヒストグラムの処理
		 * ヒストグラム中の最大値を基準に閾値に満たないものを 0 に
		 * 満たすものを 1 として、bitset に設定していく
		 * 
		 * 現在の閾値は最大値の 5 % 
		 */

		/* 最大の色相値を取得 */
		max = -1;
		for (c = 0; c < 256; c++){
			if (max < huegraph[c]){
				max = huegraph[c];
				LCDSetPos(6, 0); LCDPrintf("MAX %3d:%4d", c, max);
			}
		}
		/* 閾値 (要素が最も多い色相値の要素数の 5% 以下) */
		max = max / 20;
		for (c = 0; c < 256; c++){
			bitset_put(bs[num - 1], c, max <= huegraph[c]);
		}

		/* ヒストグラムの処理結果の確認 - LCD 上に表示 */
		LCDClear();
		for (c = 0; c < 256; c++){
			LCDLine(c % 128, 10 + (c / 128) * 30, 
					c % 128, 10 + (c / 128) * 30 - bitset_get(bs[num - 1], c) * 5, 1);
		}

		/* これで OK かどうか、また他の色についても収集するかどうか
		 *   RE: 現在の色を取り直す
		 *  NXT: 次の色の収集に移る
		 * DONE: 色の収集を終了
		 */
		LCDMenu(" ", "RE", "NXT", "DONE");
		key = 0;
		while ((key = KEYRead()) != KEY2 && key != KEY3 && key != KEY4)
			;
		
		if (key == KEY3){
			bs = colordata_add(bs, &num);
		}
		LCDSetPos(6, 0); LCDPrintf("            ");

	}while(key != KEY4);
	
	LCDClear();

	*number = num;
	return bs;
}

/* 色相テーブルに格納されているすべての色の情報を LCD に表示する
 * num には表示する色数を指定する
 */
void COLORInfo(bitset **bs, int num){
	int i, c;
	for (i = 0; i < num; i++){
		for (c = 0; c < 256; c++){
			LCDLine(c % 128, 5 + (i * 6) + (c / 128) * 2, 
					c % 128, 5 + (i * 6) + (c / 128) * 2 - bitset_get(bs[i], c), 1);
		}
	}
}

/* 色相テーブルを開放する */
void COLORRelease(bitset **bs, int num){
	colordata_free(bs, num);
}

