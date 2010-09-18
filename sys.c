#include "eyebot.h"
#include "wabotcam.h"
#include <malloc.h>
#include <stdio.h>
#include "main.h"

/* showLCDClear: LCDをclearする */
void showLCDClear(){
	LCDClear();
	LCDMenu( " ", " ", " ", "Back" );
}

/* showMatchNum: 一致した色と一致しなかった色の数を返す関数 */
void showMatchNum(struct ImageData *imageData){
	LCDPrintf("Match = %d\n", imageData->match);
	LCDPrintf("Miss  = %d\n", imageData->missMatch);
}

/* showModeName: 引数に与えられたモードの名前を返す関数 */
void showModeName(int *mode){
	switch(*mode){
	case SubMenu:
		LCDPrintf("SubMenu\n");
		break;
	case GoBy1stPylon:
		LCDPrintf("GoBy1stPylon\n");
		break;
	case GoBy2ndPylon:
		LCDPrintf("GoBy2ndPylon\n");
		break;
	case LTurnByTime:
		LCDPrintf("LTurnByTime\n");
		break;
	case RTurnByTime:
		LCDPrintf("RTurnByTime\n");
		break;
	case GoByTime:
		LCDPrintf("GoByTime\n");
		break;
	case RTurnToTarget:
		LCDPrintf("RTurnToTarget\n");
		break;
	case LTurnToTarget:
		LCDPrintf("LTurnToTarget\n");
		break;
	case Homing1stPylon:
		LCDPrintf("Homing1stPylon\n");
		break;
	case Homing2ndPylon:
		LCDPrintf("Homing2ndPylon\n");
		break;
	case AvoidLeft:
		LCDPrintf("AvoidLeft\n");
		break;
	case AvoidRight:
		LCDPrintf("AvoidRight\n");
		break;
	default:
		/* 例外処理 */
		LCDPrintf("unexpected mode name\n");
	}
}

/* showModeRoot: 引数で与えられたモードの遷移順序を表示する(未実装) */
void showModeRoot(struct Modes *modes){
	int currentMode = SubMenu;
	int key;
	int modeFlag[ModeLength] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
	int loopFlag = FALSE;
	LCDClear();
	LCDPrintf("Current Mode Root\n");
	LCDPrintf("SubMenu ");
	while(currentMode!= SubMenu){
		switch(currentMode){
		case GoBy1stPylon:
			LCDPrintf("-> GoBy1stPylon ");
			currentMode= modes->GoBy1stPylon.nextMode;
			if(modeFlag[GoBy1stPylon] == TRUE) loopFlag = TRUE;
			modeFlag[GoBy1stPylon] = TRUE;
			break;

		case GoBy2ndPylon:
			LCDPrintf("-> GoBy2ndPylon ");
			currentMode= modes->GoBy2ndPylon.nextMode;
			if(modeFlag[GoBy2ndPylon] == TRUE) loopFlag = TRUE;
			modeFlag[GoBy2ndPylon] = TRUE;
			break;

		case LTurnByTime:
			LCDPrintf("-> LTurnByTime ");
			currentMode= modes->LTurnByTime.nextMode;
			if(modeFlag[LTurnByTime] == TRUE) loopFlag = TRUE;
			modeFlag[LTurnByTime] = TRUE;
			break;

		case RTurnByTime:
			LCDPrintf("-> RTurnByTime ");
			currentMode= modes->RTurnByTime.nextMode;
			if(modeFlag[RTurnByTime] == TRUE) loopFlag = TRUE;
			modeFlag[RTurnByTime] = TRUE;
			break;

		case GoByTime:
			LCDPrintf("-> GoByTime ");
			currentMode= modes->GoByTime.nextMode;
			if(modeFlag[GoByTime] == TRUE) loopFlag = TRUE;
			modeFlag[GoByTime] = TRUE;
			break;

		case LTurnToTarget:
			LCDPrintf("-> LTurnToTarget ");
			currentMode= modes->LTurnToTarget.nextMode;
			if(modeFlag[LTurnToTarget] == TRUE) loopFlag = TRUE;
			modeFlag[LTurnToTarget] = TRUE;
			break;

		case RTurnToTarget:
			LCDPrintf("-> RTurnToTarget ");
			currentMode= modes->RTurnToTarget.nextMode;
			if(modeFlag[RTurnToTarget] == TRUE) loopFlag = TRUE;
			modeFlag[RTurnToTarget] = TRUE;
			break;

		case Homing1stPylon:
			LCDPrintf("-> Homing1stPylon ");
			currentMode= modes->Homing1stPylon.nextMode;
			if(modeFlag[Homing1stPylon] == TRUE) loopFlag = TRUE;
			modeFlag[Homing1stPylon] = TRUE;
			break;

		case Homing2ndPylon:
			LCDPrintf("-> Homing2ndPylon ");
			currentMode= modes->Homing2ndPylon.nextMode;
			if(modeFlag[Homing2ndPylon] == TRUE) loopFlag = TRUE;
			modeFlag[Homing2ndPylon] = TRUE;
			break;

		case AvoidLeft:
			LCDPrintf("-> AvoidLeft ");
			currentMode= modes->AvoidLeft.nextMode;
			if(modeFlag[AvoidLeft] == TRUE) loopFlag = TRUE;
			modeFlag[AvoidLeft] = TRUE;
			break;

		case AvoidRight:
			LCDPrintf("-> AvoidRight ");
			currentMode= modes->AvoidRight.nextMode;
			if(modeFlag[AvoidRight] == TRUE) loopFlag = TRUE;
			modeFlag[AvoidRight] = TRUE;
			break;

		default:
			LCDPrintf("\nError: currentMode=%d\n", currentMode);
			break;
		}
		if(loopFlag == TRUE){
			LCDPrintf("->...loop.\n");
			break;
		}
	}

	if(loopFlag == FALSE) LCDPrintf("-> 0\n");
	LCDPrintf("\nPress any key.");
	key = KEYGet();
}

/* showTargetplace(): 目標物の位置を表示する  */
void showTargetPlace(struct ImageData *imageData){
	switch(imageData->targetWindow){
		case 0:
			LCDPrintf("target Left\n");
			break;
		case 1:
			LCDPrintf("target Center-L\n");
			break;
		case 2:
			LCDPrintf("target Center\n");
			break;
		case 3:
			LCDPrintf("target Center-R\n");
			break;
		case 4:
			LCDPrintf("target Right\n");
			break;
		default:
			LCDPrintf("target error\n");
	}
}


/* 各サーボのオフセットを調整する関数 */
void adjustOffset(struct ServoHandles *sh){
	int menuKey, adjustKey;
	int endFlag = FALSE;
	int adjustFlag = FALSE;
	while(endFlag != TRUE){
		LCDClear();
		LCDPrintf("Which sarvo offset do you want to adjust?\n");
		LCDPrintf("Left = %d\n",   sh->lw.offset);
		LCDPrintf("Right = %d\n",  sh->rw.offset);
		LCDPrintf("Camera = %d\n", sh->cam.offset);
		LCDMenu("Lft", "Rht", "Cam", "Back");
		menuKey = KEYGet();
		switch(menuKey){
		case KEY1:	// 左車輪Offset調整
			adjustFlag = FALSE;
			while(adjustFlag != TRUE){
				LCDClear();
				LCDPrintf("current left wheel offset = %d\n", sh->lw.offset);
				LCDPrintf("\nBest offset stops the wheel\n");
				LCDMenu("up", "dwn", "def", "Back");
				adjustKey = KEYGet();
				AUBeep();
				switch(adjustKey){
				case KEY1:		//	left offset increments
					sh->lw.offset++;
					break;

				case KEY2:		//	left offset decrements
					sh->lw.offset--;
					break;

				case KEY3:		//	left offset sets default
					sh->lw.offset = 0;
					break;

				case KEY4:		// set up end adjustFlag
					adjustFlag = TRUE;
					break;

				default:
					LCDPrintf("adjust error.\n");
				}
				// offsetを調整した結果を反映
				SERVOSet(sh->leftHandle, sh->lw.stop + sh->lw.offset);
			}

			break;

		case KEY2:	// 右車輪Offset調整
			adjustFlag = FALSE;
			while(adjustFlag != TRUE){
				LCDClear();
				LCDPrintf("current right wheel offset = %d\n", sh->rw.offset);
				LCDPrintf("\nBest offset stops the wheel.\n");
				LCDMenu("up", "dwn", "def", "Back");
				adjustKey = KEYGet();
				AUBeep();
				switch(adjustKey){
				case KEY1:		//	right offset increment
					sh->rw.offset++;
					break;

				case KEY2:		//	right offset decrement
					sh->rw.offset--;
					break;

				case KEY3:		//	right offset
					sh->rw.offset = 0;
					break;

				case KEY4:		// set up end adjustFlag
					adjustFlag = TRUE;
					break;

				default:
					LCDPrintf("adjust error.\n");
				}
				// offsetを調整した結果を反映
				SERVOSet(sh->rightHandle, sh->rw.stop + sh->rw.offset);
			}
			break;

		case KEY3:	// カメラOffset調整
			adjustFlag = FALSE;
			while(adjustFlag != TRUE){
				LCDClear();
				LCDPrintf("current camera offset = %d\n", sh->cam.offset);
				LCDPrintf("\nBest offset is the camera faces straight.\n");
				LCDMenu("up", "dwn", "def", "Back");
				adjustKey = KEYGet();
				AUBeep();
				switch(adjustKey){
				case KEY1:		//	camera offset increment
					sh->cam.offset++;
					break;

				case KEY2:		//	camera offset decrement
					sh->cam.offset--;
					break;

				case KEY3:		//	camera offset sets default
					sh->cam.offset = 0;
					break;

				case KEY4:		// set up end adjustFlag
					adjustFlag = TRUE;
					break;

				default:
					LCDPrintf("adjust error.\n");
				}
				// offsetを調整した結果を反映
				SERVOSet(sh->cameraHandle, sh->cam.center + sh->cam.offset);
			}
			break;

		case KEY4:	// 終了処理
			endFlag = TRUE;
			break;

		default:
			LCDPrintf("adjustOffsetSwitch has error.");
		}
	}
}
