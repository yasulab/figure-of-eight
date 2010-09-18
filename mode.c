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

/* sub_menu�F�T�u���j���[��ʂ�\������B
 * �T�u���j���[��ʂł͌��݂̃��[�h�̊m�F�ƁA�eSERVOHandle�̒������ł���B */
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

/* goby_pylon�F�J�������E�܂��͍��Ɍ����āA�P�ԖڂɎ擾�����F���𔭌�����܂Œ��i */
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

/* goby_time�F��莞��(sh.time.straight)�̊ԁA���i */
int goby_time(struct ServoHandles *sh, int *mode){
	int osWaitTime;
	SERVOSet(sh->rightHandle, sh->rw.run + sh->rw.offset);
	SERVOSet(sh->leftHandle, sh->lw.run + sh->lw.offset);
	showLCDClear();
	showModeName(mode);
	osWaitTime = OSWait(sh->time.straight);
	return TRUE;
}

/* turnby_time�F��莞��(sh.time.turn)�̊ԁA�E�܂��͍��ɐ��� */
int turnby_time(struct ServoHandles *sh, int *mode) {
	int osWaitTime;
	if(*mode == RTurnByTime){
		SERVOSet(sh->rightHandle, sh->rw.walk + sh->rw.offset);
		SERVOSet(sh->leftHandle, sh->lw.run + sh->lw.offset);
	}else if(*mode == LTurnByTime){
		SERVOSet(sh->cameraHandle, sh->cam.leftMax + sh->cam.offset);
		SERVOSet(sh->rightHandle, sh->rw.run + sh->rw.offset);
		SERVOSet(sh->leftHandle, sh->lw.walk + sh->lw.offset);
	}else{	/* ��O���̓T�[�{���X�g�b�v */
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

/* turnto_target: ���ʂɎw�肵���F�̃p�C������������܂ł̊ԁA���܂��͉E����
 * �w�肵���F��, getImage()�łǂ̐F���擾�������ɂ���Č��܂� */
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

/* homing_pylon: �w�肵���F�̃p�C�����̉��ׂ�Ɍ������đ��s����B
 * �w�肵���F��, getImage()�łǂ̐F���擾�������ɂ���Č��܂�B
 * RH:�E�ԗ֏C���l,	LH:���ԗ֏C���l
 * !!! �����C���A���S���Y���͗v���� !!! */
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

	/* �ڕW����ߑ� */
	if(imageData->match > TARGET_MATCH){
		AUBeep();
		//cameraAdjust = 64;
		/* �ڕW����������, �}���񂵂Ă����璼�i�ɖ߂� */
		if(RH == 3 || LH == 3){
			RH = 0;
			LH = 0;
		}

		/* �����x�̐�������Ă�����, ����p�x��}���� */
		// if(RH == 2)	RH--;
		// if(LH == 2) LH--;

		switch(imageData->targetWindow){

		/* �ڕW���̈ʒu�F�� */
		case 0:
			if(*mode == Homing2ndPylon){
				if(RH<2)RH++;
				else if(RH==2 && LH==0) LH--;
			}
			break;

			/* �ڕW���̈ʒu�F������ */
		case 1:
			//if(RH>0 && *mode == Homing2ndPylon) RH--;
			if(RH==0) RH--;
			break;

			/* �ڕW���̈ʒu�F���� */
		case 2:
			RH = LH = 0;
			break;

			/* �ڕW���̈ʒu�F�����E */
		case 3:
			//if(LH>0 && *mode == Homing1stPylon)	LH--;
			if(LH>0) LH--;
			break;


			/* �ڕW���̈ʒu�F�E */
		case 4:
			if(*mode == Homing1stPylon){
				if(LH<2)	LH++;
				else if(RH==2 && LH>0)	RH--;
			}
			break;

		default:
			LCDPrintf("target window error\n");
		}

		/* �ڕW��������ɓ����Ă��Ȃ��ꍇ & �����C�����܂��s���Ă��Ȃ��ꍇ */
	}else if(targetFound == FALSE){
		/* ���݂̃��[�h�ɂ���ĐU�镑�����قȂ�B
		 * �ڕW���͌��������R�́A�O��Ԃɂ����ĖڕW���𔭌����Ă��玟��ԂɑJ�ڂ���܂ł̊Ԃ�
		 * ���񂵑����Ă��邽�߂ł���B����āA�O��Ԃ̐�������Ƃ͋t�̕����ɐ��񂳂���ƁA
		 * �ڕW���𔭌��ł���B*/
		if(*mode == Homing1stPylon){
			if(LH < 3)	LH++;
			/* LH��4�ɂ������RH��-1�ɂ��������������������Ȃ��čςނ̂ŁA�ڕW�����������ɂ��� */
			else if(RH > -1) RH--;
			/* ���e�ł�������C���̌��E�܂ŗ�����A������L�^����(����ȍ~�A�����C���͍s��Ȃ�) */
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

	/* �ڕW���̏ꏊ��\�� */
	if(imageData->match < NEAR_MATCH){
		showTargetPlace(imageData);
		LCDPrintf("Target Far...\n");

		/* �ڕW���ɐڋ߂����Ƃ��ɏ�ԑJ�ڂ����� */
	}else{
		AUBeep();
		LCDPrintf("Target Near!\n");
		/* ������ */
		LH = RH = 0;
		cameraAdjust = 0;
		targetFound = FALSE;
		return TRUE;
	}

	return FALSE;
}

/* ���삩��ڕW�������Ȃ��Ȃ�܂Œ��i����B(���񎞂Ƀp�C�����ɏՓ˂��Ȃ�����) */
int avoidCount = 0;
int avoid_pylon(struct ServoHandles *sh, struct ImageData *imageData, int *mode){
	/* �O��ԂŊ��Ƀp�C������ߑ����Ă��邽�߁A�J�����̏�Ԃ�ύX���Ȃ� */
	SERVOSet(sh->rightHandle, sh->rw.walk - AVOID_ADJUST + sh->rw.offset);
	SERVOSet(sh->leftHandle, sh->lw.walk + AVOID_ADJUST + sh->lw.offset);

	showLCDClear();
	showMatchNum(imageData);
	showModeName(mode);
	showTargetPlace(imageData);

	/* �ڕW�������������Ƃ���ԑJ�ڂ��� */
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



/* homing_pylon: �w�肵���F�̃p�C�����̉��ׂ�Ɍ������đ��s����B
 * �w�肵���F��, getImage()�łǂ̐F���擾�������ɂ���Č��܂�B
 * RH:�E�ԗ֏C���l,	LH:���ԗ֏C���l */
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



	/* �ڕW����ߑ� */
	if(imageData->match > TARGET_MATCH){
		AUBeep();
		targetFound = TRUE;
		cameraAdjust = 64;

		/* �ڕW���𔭌������璼�i�ɖ߂� */
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


		/* �ڕW���������ɂ������ꍇ */
		if(imageData->match < 2000){
			showTargetPlace(imageData);
			LCDPrintf("Target Far...\n");
		}else if(imageData->match <3000){
			if(prevCount==2){
				/* �ڕW����ʂ�߂��Ă��܂����ꍇ */
				if(prevMatch[0] < prevMatch[1] && prevMatch[1] > prevMatch[2]){
					goto NEXT_MODE;		// ���̏�ԂɑJ�ڂ���
				}
			}


			/* �ڕW���ɐڋ߂����Ƃ��ɏ�ԑJ�ڂ����� */
		}else{
			NEXT_MODE:
			AUBeep();
			LCDPrintf("Target Near!\n");
			/* ������ */
			LH = RH = 0;
			cameraAdjust = 0;
			targetFound = FALSE;
			prevCount = 0;
			prevMatch[0] = -1;
			prevMatch[1] = -1;
			prevMatch[2] = -1;
			return TRUE;
		}


		/* �ڕW��������ɓ����Ă��Ȃ��ꍇ */
	}else if(targetFound == FALSE){
		/* ���݂̃��[�h�ɂ���ĐU�镑�����قȂ�B
		 * �ڕW���͌��������R�́A�O��Ԃɂ����ĖڕW���𔭌����Ă��玟��ԂɑJ�ڂ���܂ł̊Ԃ�
		 * ���񂵑����Ă��邽�߂ł���B����āA�O��Ԃ̐�������Ƃ͋t�̕����ɐ��񂳂���ƁA
		 * �ڕW���𔭌��ł���B*/
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
