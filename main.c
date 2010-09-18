#include "eyebot.h"
#include "wabotcam.h"
#include <malloc.h>
#include <stdio.h>
#include "main.h"
#define TURNING 2
#define TARGET_MATCH 300
#define AVOID_MATCH 200

/* グローバル変数 */
// colimage pic;

/* init_servo: サーボに関する各数値を初期化する  */
void init_servo(struct ServoHandles *sh){
	sh->lw.stop = 128;      // 左車輪停止
	sh->lw.walk = 132;      // 左車輪徐行スピード
	sh->lw.run  = 138;      // 左車輪走行スピード
	sh->lw.backward = 125;  // 左車輪後退スピード
	sh->lw.offset = 1;      // 左車輪オフセット
	sh->lw.homing = 0;

	sh->rw.stop = 128;      // 右車輪停止
	sh->rw.walk = 124;      // 右車輪徐行スピード
	sh->rw.run  = 118;      // 右車輪走行スピード
	sh->rw.backward = 131;  // 右車輪後退スピード
	sh->rw.offset = 2;      // 右車輪オフセット
	sh->rw.homing = 0;

	sh->cam.center = 128;    // カメラ正面
	sh->cam.left = 191;      // カメラ左向き
	sh->cam.right = 64;      // カメラ右向き
	sh->cam.leftMax = 255;   // カメラ左向き最大値
	sh->cam.rightMax = 0;    // カメラ右向き最大値
	sh->cam.offset = 0;      // カメラ方向オフセット
	sh->cam.turn = 30;           // カメラ自動修整時の回転角度   (使用ステージ例：第５)
	sh->cam.left_center = 35;    // 目標物の左右位置の識別基準値 (使用ステージ例：第５)
	sh->cam.center_right = 45;   // 目標物の左右位置の識別基準値 (使用ステージ例：第５)

	sh->time.turn = 850;      // 旋回する時間(使用ステージ例：第２)
	sh->time.straight = 300;  // 直進する時間(使用ステージ例：第６)
	sh->time.adjustDirection = 100;  // Uターン後に方向を修正する時間(使用ステージ例：第７)

	sh->cameraHandle = SERVOInit( SERVO10 );    // カメラ
	sh->rightHandle = SERVOInit( SERVO11 ); // 右車輪
	sh->leftHandle = SERVOInit( SERVO12 );  // 左車輪
}

/* set_mode: モードの遷移順序を設定する。 */
void set_mode(struct Modes *modes){
	modes->SubMenu.nextMode	= Homing1stPylon;	// ここで指定したモードが最初のモードになる。
	modes->GoBy1stPylon.nextMode = GoByTime;
	modes->GoBy2ndPylon.nextMode = SubMenu;
	modes->LTurnByTime.nextMode = SubMenu;
	modes->RTurnByTime.nextMode = Homing2ndPylon;
	modes->GoByTime.nextMode 	= RTurnToTarget;

	//応用課題では以下のモードをループさせて8の字走行を実現させる
	modes->RTurnToTarget.nextMode = Homing2ndPylon;
	modes->LTurnToTarget.nextMode = Homing1stPylon;
	modes->Homing1stPylon.nextMode = AvoidLeft;
	modes->Homing2ndPylon.nextMode = AvoidRight;
	modes->AvoidLeft.nextMode 	= RTurnToTarget;
	modes->AvoidRight.nextMode	= LTurnToTarget;
}

int main(){
	bitset **color;
	int num;
	int key;
	int endFlag = FALSE;
	int getColorFlag = FALSE;
	CAMInit(NORMAL);        // カメラの初期化
	CAMMode(AUTOBRIGHTNESS);

	while(endFlag != TRUE){
		LCDClear();
		LCDPrintf("\n");
		LCDPrintf("[Menu Display]\n");
		LCDPrintf("GET: set color\n");
		LCDPrintf("RUN: run EyeBot\n");
		LCDPrintf("END: end program\n");
		LCDPrintf("Number = %d", num);
		/* 照合に使用する色を指定していた場合, "RUN"コマンドが使用可能 */
		/* if(getColorFlag == TRUE) */	LCDMenu( "GET", " ", "RUN", "End" );
		/*else LCDMenu( "GET", " ", " ", "End" );	*/
		key = KEYGet();
		AUBeep();

		/* メインメニュー画面 */
		switch(key){
		case KEY1:  // 色相取得メニューに遷移
			LCDClear();
			color = COLORSetup(&num);	/* 画面一杯の単一色を撮影し、colorに格納する。複数個取得することも可能 */
			COLORInfo(color, num);      /* bs に格納されているすべての色についての情報を LCD に表示 */
			getColorFlag = TRUE;
			break;

		case KEY2:  // 空き
			break;

		case KEY3:  // EyeBotを走行状態に遷移
			/*if(getColorFlag == TRUE)*/ run(color);
			break;

		case KEY4:  // プログラムの終了状態に遷移
			endFlag = TRUE;
			break;

		default:
			LCDPrintf("\nError:default\n");
		}
	}

	// 解放処理
	CAMRelease();
	if(getColorFlag == TRUE)COLORRelease(color, num);   // bsを開放し、収集した色相情報をすべて破棄します。
	return 0;
}

/* run(): 設定されたモードに基づいて、EyeBotを走らせる関数 */
void run(bitset **color){
	int currentMode = SubMenu;
	int key = KEY1;
	int endFlag;
	struct ImageData imageData;
	struct ServoHandles sh;
	struct Modes modes;
	init_servo(&sh);	/* サーボの初期化 */
	set_mode(&modes);	/* モードの設定 */

	while ( key != KEY4 && KEYRead() != KEY4 ) {
		/* 試しに入れてみる */
		//OSShowTime();
		//OSSetTime(0, 0, 0);
		/********************/
		switch(currentMode){

		/* SubMenu：サブメニュー画面 & 各サーボの初期化*/
		case SubMenu:
			key = sub_menu(&sh, &currentMode);
			if(key == KEY1) currentMode = modes.SubMenu.nextMode;
			else if(key == KEY2) showModeRoot(&modes);
			else if(key == KEY3) adjustOffset(&sh);
			break;

			/* GoBy1stPylon：カメラ右向き & １番目に取得した色相を発見するまで直進 */
		case GoBy1stPylon:
			getImage(color, FIRST_COLOR, &imageData);
			endFlag = goby_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.GoBy1stPylon.nextMode;
			}
			break;

			/* GoBy2ndPylon：カメラ右向き & ２番目に取得した色相を発見するまで直進 */
		case GoBy2ndPylon:
			getImage(color, SECOND_COLOR, &imageData);
			endFlag = goby_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.GoBy2ndPylon.nextMode;
			}
			break;

			/* GoByTime：一定時間(sh.time.straight)の間、直進 */
		case GoByTime:
			endFlag = goby_time(&sh, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.GoByTime.nextMode;
			}
			break;

			/* RTurnByTime：一定時間(sh.time.turn)の間、右旋回 */
		case RTurnByTime:
			endFlag = turnby_time(&sh, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.RTurnByTime.nextMode;
			}
			break;

			/* LTurnByTime：一定時間(sh.time.turn)の間、左旋回 */
		case LTurnByTime:
			endFlag = turnby_time(&sh, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.LTurnByTime.nextMode;
			}
			break;

			/* RTurnToTarget: 正面に、２番目に取得した色相を発見するまで右旋回 */
		case RTurnToTarget:
			getImage(color, SECOND_COLOR, &imageData);
			endFlag = turnto_target(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.RTurnByTime.nextMode;
			}
			break;

			/* LTurnToTarget: 正面に、１番目に取得した色のパイロンを発見するまで左旋回 */
			/* !!!CAUTION!!! 取得した画像を照合するまでに時間がかかりすぎて、旋回しすぎる。 */
		case LTurnToTarget:
			getImage(color, FIRST_COLOR, &imageData);
			endFlag = turnto_target(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.LTurnToTarget.nextMode;
			}
			break;

			/* Homing1stPylon: 1番目に取得した色のパイロンの横隣りに向かって進む */
		case Homing1stPylon:
			getImage(color, FIRST_COLOR, &imageData);
			// endFlag = homing_pylon(&sh, &imageData, &currentMode);
			endFlag = homing_pylon2(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.Homing1stPylon.nextMode;
			}
			break;

			/* Homing2ndPylon: 2番目に取得した色のパイロンの横隣りに向かって進む, */
		case Homing2ndPylon:
			getImage(color, SECOND_COLOR, &imageData);
			// endFlag = homing_pylon(&sh, &imageData, &currentMode);
			endFlag = homing_pylon2(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.Homing2ndPylon.nextMode;
			}
			break;

			/* AvoidLeft: 1番目に取得した色が画面から無くなるまで直進 */
		case AvoidLeft:
			getImage(color, FIRST_COLOR, &imageData);
			endFlag = avoid_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.AvoidLeft.nextMode;
			}
			break;

			/* AvoidRight: 2番目に取得した色が画面から無くなるまで直進 */
		case AvoidRight:
			getImage(color, SECOND_COLOR, &imageData);
			endFlag = avoid_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE)	currentMode = modes.AvoidRight.nextMode;
			AUBeep();
			break;

			/* 例外Mode: 予期しないモードに遷移したことを表示 */
		default:
			LCDClear();
			LCDPrintf("unexpected Mode\n");
			showModeName(&currentMode);
		}
	}

	// 解放処理
	SERVORelease( sh.cameraHandle );   // カメラ解放
	SERVORelease( sh.rightHandle );    // 右車輪解放
	SERVORelease( sh.leftHandle );     // 左車輪解放
}

/* getImage(): 取得した画像から色相情報を取り出し、imageDataに格納する関数
 * color=基準となる色の画像
 * colorNumber=照合する画像番号
 * imageData=取得した画像の分析結果を格納する構造体 */
void getImage(bitset **color, int colorNumber, struct ImageData *imageData){
	int i, j;
	int hitWindow[5] = {0,0,0,0,0};		// 画像を横に5つに分けたとき、各窓毎のHIT数
	int maxHitWindowNum;
	int count;
	colimage pic; // 毎回colimageの領域を探す演算が必要になるため, グローバル変数とした方がよい?

	CAMGetColFrame(&pic, 0);
	imageData->match = 0;
	imageData->missMatch = 0;
	// image *img = (image *) imageData->imageFlag;
	for (j = 1; j < imagecolumns - 1 ; j++) {
		for (i = 1; i < imagerows - 1 ; i++) {
			// RGB値から色相値を算出。算出できない場合、 definehue というグローバル変数を 0 にセット
			BYTE hue = rgb2hue(pic[i][j][0], pic[i][j][1], pic[i][j][2]);

			//色相値 hue が色相情報 bs において認識できていた場合
			if (definehue && isHueArea(color[colorNumber], hue)) {
				imageData->imageFlag[i][j] = HIT;
				// hitWindow[j%5]++;	// 演算量が多いかも・・・
				imageData->match++;    // 認識できた色相数をインクリメント
				// 認識できなかった場合
			}else{
				imageData->imageFlag[i][j] = MISS;
				imageData->missMatch++;
			}
		}
	}

	count=0;
	// 膨張処理(演算量を減らすため、先にimageData->imageFlag[i][j]==MISSであることを確認)
	for (j = 2; j < imagecolumns - 2; j++) {
		for (i = 2; i < imagerows - 2; i++) {
			/* 焦点を当てている画素はMISSだが、周りの半分以上がHITの場合 */
			if(	imageData->imageFlag[i][j] == MISS && (
					imageData->imageFlag[i-1][j-1] + imageData->imageFlag[i-1][j+0] + imageData->imageFlag[i-1][j+1] +
					imageData->imageFlag[i+0][j-1] + 				  imageData->imageFlag[i+0][j+1] +
					imageData->imageFlag[i+1][j-1] + imageData->imageFlag[i+1][j+0] + imageData->imageFlag[i+1][j+1]) >= 5
			){
				/* その画素はノイズで, 本来はHITであったと判断 */
				imageData->imageFlag[i][j] = HIT;
				count++;
			}
		}
	}

	/* 膨張処理が終わった後に,修正した画素一致数を計算しなおす */
	imageData->match += count;
	imageData->missMatch -= count;
	count = 0;

	// 縮小処理(演算量を減らすため、先にimageData->imageFlag[i][j]==HITであることを確認)
	// 縮小処理と同時にhitWindowを計算する? 余剰演算のコストが高いかもしれない。
	for (j = 2; j < imagecolumns - 2; j++) {
		for (i = 2; i < imagerows - 2; i++) {
			/* 焦点を当てている画素はHITだが, 周りの半分以上がHITの場合 */
			if(imageData->imageFlag[i][j] == HIT && (
					imageData->imageFlag[i-1][j-1] + imageData->imageFlag[i-1][j+0] +	imageData->imageFlag[i-1][j+1] +
					imageData->imageFlag[i+0][j-1] +					imageData->imageFlag[i+0][j+1] +
					imageData->imageFlag[i+1][j-1] + imageData->imageFlag[i+1][j+0] +	imageData->imageFlag[i+1][j+1]) <= 4
			){
				/* その画素はノイズで, 本来はMISSであったと判断 */
				imageData->imageFlag[i][j] = MISS;
				count++;
			}
		}
	}

	/* 縮小処理が終わった後に,修正した画素一致数を計算しなおす */
	imageData->match -= count;
	imageData->missMatch += count;

	// #define imagecolumns 82
	// #define imagerows 62

	// 画面ごとのHIT数をカウントする(if文を省略するため、ループアンローリングする)
	for (j=1; j<17 ; j++)
		for(i=1; i<61 ; i++)
			hitWindow[0] += imageData->imageFlag[i][j];
	for (j=17; j<33 ; j++)
		for(i=1; i<61 ; i++)
			hitWindow[1] += imageData->imageFlag[i][j];
	for (j=33; j<49 ; j++)
		for(i=1; i<61 ; i++)
			hitWindow[2] += imageData->imageFlag[i][j];
	for (j=49; j<65 ; j++)
		for(i=1; i<61 ; i++)
			hitWindow[3] += imageData->imageFlag[i][j];
	for (j=65; j<81 ; j++)
		for(i=1; i<61 ; i++)
			hitWindow[4] += imageData->imageFlag[i][j];

	/*	上部コードの元となったプログラム
	for (j=1; j<imagecolumns-1; j++) {
		for(i=1; i<imagerows-1; i++){
			if(j<16){
				hitWindow[0] += imageData->imageFlag[i][j];
			}else if(j<32){
				hitWindow[1] += imageData->imageFlag[i][j];
			}else if(j<48){
				hitWindow[2] += imageData->imageFlag[i][j];
			}else if(j<64){
				hitWindow[3] += imageData->imageFlag[i][j];
			}else{
				hitWindow[4] += imageData->imageFlag[i][j];
			}
		}
	}
	 */

	// HIT数が最も多い窓を識別する
	maxHitWindowNum = 0;
	for(i=1;i<5;i++){
		if(hitWindow[maxHitWindowNum] < hitWindow[i])	maxHitWindowNum = i;
	}
	imageData->targetWindow = maxHitWindowNum;	// 目標物の場所情報を格納
}


// backup用
void getImage2(bitset **color, int colorNumber, struct ImageData *imageData){
	int i, j;
	int hitWindow[5] = {0,0,0,0,0};
	int maxHitWindowNum;
	colimage pic;
	CAMGetColFrame(&pic, 0);
	imageData->match = 0;
	imageData->missMatch = 0;
	for (j = 1; j < imagecolumns - 1 ; j++) {
		for (i = 1; i < imagerows - 1 ; i++) {
			// RGB値から色相値を算出。算出できない場合、 definehue というグローバル変数を 0 にセット
			BYTE hue = rgb2hue(pic[i][j][0], pic[i][j][1], pic[i][j][2]);

			//色相値 hue が色相情報 bs において認識できていた場合
			if (definehue && isHueArea(color[colorNumber], hue)) {
				imageData->imageFlag[i][j] = 0;
				imageData->match++;    // 認識できた色相数をインクリメント
				// 認識できなかった場合
			}else{
				imageData->imageFlag[i][j] = MISS;
				imageData->missMatch++;
			}
		}
	}

	// 膨張処理
	for (j = 2; j < imagecolumns - 2; j++) {
		for (i = 2; i < imagerows - 2; i++) {
			if(
					(imageData->imageFlag[i-1][j-1] + imageData->imageFlag[i-1][j+0] + imageData->imageFlag[i-1][j+1] +
							imageData->imageFlag[i+0][j-1] + 					 			  imageData->imageFlag[i+0][j+1] +
							imageData->imageFlag[i+1][j-1] + imageData->imageFlag[i+1][j+0] + imageData->imageFlag[i+1][j+1]) >= 5
							&& imageData->imageFlag[i][j] == MISS
			){
				imageData->imageFlag[i][j] = HIT;
				imageData->match++;
				imageData->missMatch--;
			}
		}
	}

	// 縮小処理
	for (j = 2; j < imagecolumns - 2; j++) {
		for (i = 2; i < imagerows - 2; i++) {
			if(
					(imageData->imageFlag[i-1][j-1] + imageData->imageFlag[i-1][j+0] +	imageData->imageFlag[i-1][j+1] +
							imageData->imageFlag[i+0][j-1] +									imageData->imageFlag[i+0][j+1] +
							imageData->imageFlag[i+1][j-1] + imageData->imageFlag[i+1][j+0] +	imageData->imageFlag[i+1][j+1]) <= 4
							&& imageData->imageFlag[i][j] == HIT
			){
				imageData->imageFlag[i][j] = MISS;
				imageData->match--;
				imageData->missMatch++;
			}
		}
	}

	// 画面ごとのHIT数をカウントする(if文を省略するため、ループアンローリングする)
		for (j=1; j<17 ; j++)
			for(i=1; i<61 ; i++)
				hitWindow[0] += imageData->imageFlag[i][j];
		for (j=17; j<33 ; j++)
			for(i=1; i<61 ; i++)
				hitWindow[1] += imageData->imageFlag[i][j];
		for (j=33; j<49 ; j++)
			for(i=1; i<61 ; i++)
				hitWindow[2] += imageData->imageFlag[i][j];
		for (j=49; j<65 ; j++)
			for(i=1; i<61 ; i++)
				hitWindow[3] += imageData->imageFlag[i][j];
		for (j=65; j<81 ; j++)
			for(i=1; i<61 ; i++)
				hitWindow[4] += imageData->imageFlag[i][j];

		/*	上部コードの元となったプログラム
		for (j=1; j<imagecolumns-1; j++) {
			for(i=1; i<imagerows-1; i++){
				if(j<16){
					hitWindow[0] += imageData->imageFlag[i][j];
				}else if(j<32){
					hitWindow[1] += imageData->imageFlag[i][j];
				}else if(j<48){
					hitWindow[2] += imageData->imageFlag[i][j];
				}else if(j<64){
					hitWindow[3] += imageData->imageFlag[i][j];
				}else{
					hitWindow[4] += imageData->imageFlag[i][j];
				}
			}
		}
		 */

	// HIT数が最も多い窓を識別する
	maxHitWindowNum = 0;
	for(i=0;i<5;i++){
		if(hitWindow[maxHitWindowNum] < hitWindow[i])	maxHitWindowNum = i;
	}
	imageData->targetWindow = maxHitWindowNum;	// 目標物の場所情報を格納
}
 // */


