#include "eyebot.h"
#include "wabotcam.h"
#include <malloc.h>
#include <stdio.h>
#include "main.h"
#define TURNING 2
#define TARGET_MATCH 300
#define AVOID_MATCH 200
#define RH sh->rw.homing
#define LH sh->lw.homing
#define AVOID_ADJUST 4
int cameraAdjust = 0;
int targetFound = FALSE;
short int prevMatch[3] = {-1, -1, -1};
short int prevCount = 0;

/* sub_menu：サブメニュー画面を表示する。
 * サブメニュー画面では現在のモードの確認と、各SERVOHandleの調整ができる。 */
int sub_menu(struct ServoHandles *sh, int *mode){
	int key;
	SERVOSet(sh->cameraHandle, sh->cam.center + sh->cam.offset);
	SERVOSet(sh->rightHandle, sh->rw.stop + sh->rw.offset);
	SERVOSet(sh->leftHandle, sh->lw.stop + sh->lw.offset);
	LCDClear();
	LCDMenu( "1", "2", "3", "Back" );
	showModeName(mode);
	LCDPrintf("1: Start\n");
	LCDPrintf("2: Show Stage Root\n");
	LCDPrintf("3: Adjust Servo Offset\n");
	key = KEYGet();
	AUBeep();
	return key;
}

/* goby_pylon：カメラを右または左に向けて、１番目に取得した色相を発見するまで直進 */
int goby_pylon(struct ServoHandles *sh, struct ImageData *imageData, int *mode){
	SERVOSet(sh->cameraHandle, sh->cam.rightMax + sh->cam.offset);
	SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset);
	SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset);

	showLCDClear();
	showModeName(mode);
	showMatchNum(imageData);
	showTargetPlace(imageData);
	if(imageData->match > NUM_MATCH){
		AUBeep();
		return TRUE;
	}
	return FALSE;
}

/* goby_time：一定時間(sh.time.straight)の間、直進 */
int goby_time(struct ServoHandles *sh, int *mode){
	int osWaitTime;
	SERVOSet(sh->rightHandle, sh->rw.run + sh->rw.offset);
	SERVOSet(sh->leftHandle, sh->lw.run + sh->lw.offset);
	showLCDClear();
	showModeName(mode);
	osWaitTime = OSWait(sh->time.straight);
	return TRUE;
}

/* turnby_time：一定時間(sh.time.turn)の間、右または左に旋回 */
int turnby_time(struct ServoHandles *sh, int *mode) {
	int osWaitTime;
	if(*mode == RTurnByTime){
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset);
		SERVOSet(sh->leftHandle, sh->lw.run + sh->lw.offset);
	}else if(*mode == LTurnByTime){
		SERVOSet(sh->cameraHandle, sh->cam.leftMax + sh->cam.offset);
		SERVOSet(sh->rightHandle, sh->rw.run + sh->rw.offset);
		SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset);
	}else{	/* 例外時はサーボをストップ */
		SERVOSet(sh->cameraHandle, sh->cam.center + sh->cam.offset);
		SERVOSet(sh->rightHandle, sh->rw.stop + sh->rw.offset);
		SERVOSet(sh->leftHandle, sh->lw.stop + sh->lw.offset);
	}

	LCDClear();
	LCDMenu( " ", " ", " ", "Back" );
	showModeName(mode);
	osWaitTime = OSWait(sh->time.turn);
	return TRUE;
}

/* turnto_target: 正面に指定した色のパイロンを見つけるまでの間、左または右旋回
 * 指定した色は, getImage()でどの色を取得したかによって決まる */
int turnto_target(struct ServoHandles *sh, struct ImageData *imageData, int *mode){
	if(*mode == LTurnToTarget){
		SERVOSet(sh->cameraHandle, sh->cam.center + sh->cam.offset);
		SERVOSet(sh->rightHandle, sh->rw.run + sh->rw.offset);
		SERVOSet(sh->leftHandle, sh->lw.walk + TURNING + sh->lw.offset);
	}else if(*mode == RTurnToTarget){
		SERVOSet(sh->cameraHandle, sh->cam.center + sh->cam.offset);
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset - 2);
		SERVOSet(sh->leftHandle, sh->lw.run + TURNING + sh->lw.offset);
	}

	showLCDClear();
	showMatchNum(imageData);
	showModeName(mode);
	if(imageData->match > NUM_MATCH){
		AUBeep();
		return TRUE;
	}

	return FALSE;
}

/* homing_pylon: 指定した色のパイロンの横隣りに向かって走行する。
 * 指定した色は, getImage()でどの色を取得したかによって決まる。
 * RH:右車輪修正値,	LH:左車輪修正値
 * !!! 方向修正アルゴリズムは要検討 !!! */
int homing_pylon(struct ServoHandles *sh, struct ImageData *imageData, int *mode){
	if(*mode == Homing1stPylon){
		SERVOSet(sh->cameraHandle, sh->cam.rightMax + sh->cam.offset + cameraAdjust);
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset - RH);
		SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset + LH);
	}else{	// mode == Homing2ndPylon
		SERVOSet(sh->cameraHandle, sh->cam.leftMax + sh->cam.offset - cameraAdjust);
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset - RH);
		SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset + LH);
	}

	/* 目標物を捕捉 */
	if(imageData->match > TARGET_MATCH){
		AUBeep();
		//cameraAdjust = 64;
		/* 目標物発見時に, 急旋回していたら直進に戻す */
		if(RH == 3 || LH == 3){
			RH = 0;
			LH = 0;
		}

		/* 中程度の旋回をしていたら, 旋回角度を抑える */
		// if(RH == 2)	RH--;
		// if(LH == 2) LH--;

		switch(imageData->targetWindow){

		/* 目標物の位置：左 */
		case 0:
			if(*mode == Homing2ndPylon){
				if(RH<2)RH++;
				else if(RH==2 && LH==0) LH--;
			}
			break;

			/* 目標物の位置：中央左 */
		case 1:
			//if(RH>0 && *mode == Homing2ndPylon) RH--;
			if(RH==0) RH--;
			break;

			/* 目標物の位置：中央 */
		case 2:
			RH = LH = 0;
			break;

			/* 目標物の位置：中央右 */
		case 3:
			//if(LH>0 && *mode == Homing1stPylon)	LH--;
			if(LH>0) LH--;
			break;


			/* 目標物の位置：右 */
		case 4:
			if(*mode == Homing1stPylon){
				if(LH<2)	LH++;
				else if(RH==2 && LH>0)	RH--;
			}
			break;

		default:
			LCDPrintf("target window error\n");
		}

		/* 目標物が視野に入っていない場合 & 方向修正をまだ行っていない場合 */
	}else if(targetFound == FALSE){
		/* 現在のモードによって振る舞いが異なる。
		 * 目標物は見失う理由は、前状態において目標物を発見してから次状態に遷移するまでの間も
		 * 旋回し続けているためである。よって、前状態の旋回方向とは逆の方向に旋回させると、
		 * 目標物を発見できる。*/
		if(*mode == Homing1stPylon){
			if(LH < 3)	LH++;
			/* LHを4にするよりはRHを-1にした方が動く距離が少なくて済むので、目標物を見失いにくい */
			else if(RH > -1) RH--;
			/* 許容できる方向修正の限界まで来たら、それを記録する(これ以降、方向修正は行わない) */
			else targetFound = TRUE;
		}else if(*mode == Homing2ndPylon){
			if(RH < 3)	RH++;
			else if(LH > -1) LH--;
			else targetFound = TRUE;
		}
	}

	showLCDClear();
	if(targetFound == TRUE)	LCDPrintf("targetFound true\n");
	if(targetFound == FALSE)LCDPrintf("targetFound false\n");
	//showModeName(mode);
	showMatchNum(imageData);
	LCDPrintf("L_homing = %d\n", LH);
	LCDPrintf("R_homing  = %d\n", RH);

	/* 目標物の場所を表示 */
	if(imageData->match < NEAR_MATCH){
		showTargetPlace(imageData);
		LCDPrintf("Target Far...\n");

		/* 目標物に接近したときに状態遷移をする */
	}else{
		AUBeep();
		LCDPrintf("Target Near!\n");
		/* 初期化 */
		LH = RH = 0;
		cameraAdjust = 0;
		targetFound = FALSE;
		return TRUE;
	}

	return FALSE;
}

/* 視野から目標物がいなくなるまで直進する。(旋回時にパイロンに衝突しないため) */
int avoidCount = 0;
int avoid_pylon(struct ServoHandles *sh, struct ImageData *imageData, int *mode){
	/* 前状態で既にパイロンを捕捉しているため、カメラの状態を変更しない */
	SERVOSet(sh->rightHandle, sh->rw.walk - AVOID_ADJUST + sh->rw.offset);
	SERVOSet(sh->leftHandle, sh->lw.walk + AVOID_ADJUST + sh->lw.offset);

	showLCDClear();
	showMatchNum(imageData);
	showModeName(mode);
	showTargetPlace(imageData);

	/* 目標物を見失ったとき状態遷移する */
	if(imageData->match <= AVOID_MATCH){
		AUBeep();
		avoidCount++;
		if(avoidCount == 3){
			avoidCount = 0;
			return TRUE;
		}
	}
	return FALSE;
}



/* homing_pylon: 指定した色のパイロンの横隣りに向かって走行する。
 * 指定した色は, getImage()でどの色を取得したかによって決まる。
 * RH:右車輪修正値,	LH:左車輪修正値 */
#define TEST 3
int homing_pylon2(struct ServoHandles *sh, struct ImageData *imageData, int *mode){
	if(*mode == Homing1stPylon){
		SERVOSet(sh->cameraHandle, sh->cam.right + sh->cam.offset - cameraAdjust);
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset - RH - TEST);
		SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset + LH + TEST);
	}else{
		SERVOSet(sh->cameraHandle, sh->cam.left + sh->cam.offset + cameraAdjust);
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset - RH - TEST);
		SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset + LH + TEST);
	}



	/* 目標物を捕捉 */
	if(imageData->match > TARGET_MATCH){
		AUBeep();
		targetFound = TRUE;
		cameraAdjust = 64;

		/* 目標物を発見したら直進に戻す */
		RH = 0;
		LH = 0;

		if(prevCount == 2){
			prevMatch[0] = prevMatch[1];
			prevMatch[1] = prevMatch[2];
			prevMatch[2] = imageData->match;
		}else if(prevCount == 1){
			prevMatch[2] = imageData->match;
			prevCount++;
		}else{
			prevMatch[1] = imageData->match;
			prevCount++;
		}


		/* 目標物が遠くにあった場合 */
		if(imageData->match < 2000){
			showTargetPlace(imageData);
			LCDPrintf("Target Far...\n");
		}else if(imageData->match <3000){
			if(prevCount==2){
				/* 目標物を通り過ぎてしまった場合 */
				if(prevMatch[0] < prevMatch[1] && prevMatch[1] > prevMatch[2]){
					goto NEXT_MODE;		// 次の状態に遷移する
				}
			}


			/* 目標物に接近したときに状態遷移をする */
		}else{
			NEXT_MODE:
			AUBeep();
			LCDPrintf("Target Near!\n");
			/* 初期化 */
			LH = RH = 0;
			cameraAdjust = 0;
			targetFound = FALSE;
			prevCount = 0;
			prevMatch[0] = -1;
			prevMatch[1] = -1;
			prevMatch[2] = -1;
			return TRUE;
		}


		/* 目標物が視野に入っていない場合 */
	}else if(targetFound == FALSE){
		/* 現在のモードによって振る舞いが異なる。
		 * 目標物は見失う理由は、前状態において目標物を発見してから次状態に遷移するまでの間も
		 * 旋回し続けているためである。よって、前状態の旋回方向とは逆の方向に旋回させると、
		 * 目標物を発見できる。*/
		if(*mode == Homing1stPylon){
			if(LH < 2)	LH++;
		}else if(*mode == Homing2ndPylon){
			if(RH < 2)	RH++;
		}
	}

	showLCDClear();
	showModeName(mode);
	showMatchNum(imageData);
	LCDPrintf("L_homing = %d\n", LH);
	LCDPrintf("R_homing  = %d\n", RH);
	return FALSE;
}
