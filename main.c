#include "eyebot.h"
#include "wabotcam.h"
#include <malloc.h>
#include <stdio.h>
#include "main.h"
#define TURNING 2
#define TARGET_MATCH 300
#define AVOID_MATCH 200

/* �O���[�o���ϐ� */
// colimage pic;

/* init_servo: �T�[�{�Ɋւ���e���l������������  */
void init_servo(struct ServoHandles *sh){
	sh->lw.stop = 128;      // ���ԗ֒�~
	sh->lw.walk = 132;      // ���ԗ֏��s�X�s�[�h
	sh->lw.run  = 138;      // ���ԗ֑��s�X�s�[�h
	sh->lw.backward = 125;  // ���ԗ֌�ރX�s�[�h
	sh->lw.offset = 1;      // ���ԗփI�t�Z�b�g
	sh->lw.homing = 0;

	sh->rw.stop = 128;      // �E�ԗ֒�~
	sh->rw.walk = 124;      // �E�ԗ֏��s�X�s�[�h
	sh->rw.run  = 118;      // �E�ԗ֑��s�X�s�[�h
	sh->rw.backward = 131;  // �E�ԗ֌�ރX�s�[�h
	sh->rw.offset = 2;      // �E�ԗփI�t�Z�b�g
	sh->rw.homing = 0;

	sh->cam.center = 128;    // �J��������
	sh->cam.left = 191;      // �J����������
	sh->cam.right = 64;      // �J�����E����
	sh->cam.leftMax = 255;   // �J�����������ő�l
	sh->cam.rightMax = 0;    // �J�����E�����ő�l
	sh->cam.offset = 0;      // �J���������I�t�Z�b�g
	sh->cam.turn = 30;           // �J���������C�����̉�]�p�x   (�g�p�X�e�[�W��F��T)
	sh->cam.left_center = 35;    // �ڕW���̍��E�ʒu�̎��ʊ�l (�g�p�X�e�[�W��F��T)
	sh->cam.center_right = 45;   // �ڕW���̍��E�ʒu�̎��ʊ�l (�g�p�X�e�[�W��F��T)

	sh->time.turn = 850;      // ���񂷂鎞��(�g�p�X�e�[�W��F��Q)
	sh->time.straight = 300;  // ���i���鎞��(�g�p�X�e�[�W��F��U)
	sh->time.adjustDirection = 100;  // U�^�[����ɕ������C�����鎞��(�g�p�X�e�[�W��F��V)

	sh->cameraHandle = SERVOInit( SERVO10 );    // �J����
	sh->rightHandle = SERVOInit( SERVO11 ); // �E�ԗ�
	sh->leftHandle = SERVOInit( SERVO12 );  // ���ԗ�
}

/* set_mode: ���[�h�̑J�ڏ�����ݒ肷��B */
void set_mode(struct Modes *modes){
	modes->SubMenu.nextMode	= Homing1stPylon;	// �����Ŏw�肵�����[�h���ŏ��̃��[�h�ɂȂ�B
	modes->GoBy1stPylon.nextMode = GoByTime;
	modes->GoBy2ndPylon.nextMode = SubMenu;
	modes->LTurnByTime.nextMode = SubMenu;
	modes->RTurnByTime.nextMode = Homing2ndPylon;
	modes->GoByTime.nextMode 	= RTurnToTarget;

	//���p�ۑ�ł͈ȉ��̃��[�h�����[�v������8�̎����s������������
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
	CAMInit(NORMAL);        // �J�����̏�����
	CAMMode(AUTOBRIGHTNESS);

	while(endFlag != TRUE){
		LCDClear();
		LCDPrintf("\n");
		LCDPrintf("[Menu Display]\n");
		LCDPrintf("GET: set color\n");
		LCDPrintf("RUN: run EyeBot\n");
		LCDPrintf("END: end program\n");
		LCDPrintf("Number = %d", num);
		/* �ƍ��Ɏg�p����F���w�肵�Ă����ꍇ, "RUN"�R�}���h���g�p�\ */
		/* if(getColorFlag == TRUE) */	LCDMenu( "GET", " ", "RUN", "End" );
		/*else LCDMenu( "GET", " ", " ", "End" );	*/
		key = KEYGet();
		AUBeep();

		/* ���C�����j���[��� */
		switch(key){
		case KEY1:  // �F���擾���j���[�ɑJ��
			LCDClear();
			color = COLORSetup(&num);	/* ��ʈ�t�̒P��F���B�e���Acolor�Ɋi�[����B�����擾���邱�Ƃ��\ */
			COLORInfo(color, num);      /* bs �Ɋi�[����Ă��邷�ׂĂ̐F�ɂ��Ă̏��� LCD �ɕ\�� */
			getColorFlag = TRUE;
			break;

		case KEY2:  // ��
			break;

		case KEY3:  // EyeBot�𑖍s��ԂɑJ��
			/*if(getColorFlag == TRUE)*/ run(color);
			break;

		case KEY4:  // �v���O�����̏I����ԂɑJ��
			endFlag = TRUE;
			break;

		default:
			LCDPrintf("\nError:default\n");
		}
	}

	// �������
	CAMRelease();
	if(getColorFlag == TRUE)COLORRelease(color, num);   // bs���J�����A���W�����F���������ׂĔj�����܂��B
	return 0;
}

/* run(): �ݒ肳�ꂽ���[�h�Ɋ�Â��āAEyeBot�𑖂点��֐� */
void run(bitset **color){
	int currentMode = SubMenu;
	int key = KEY1;
	int endFlag;
	struct ImageData imageData;
	struct ServoHandles sh;
	struct Modes modes;
	init_servo(&sh);	/* �T�[�{�̏����� */
	set_mode(&modes);	/* ���[�h�̐ݒ� */

	while ( key != KEY4 && KEYRead() != KEY4 ) {
		/* �����ɓ���Ă݂� */
		//OSShowTime();
		//OSSetTime(0, 0, 0);
		/********************/
		switch(currentMode){

		/* SubMenu�F�T�u���j���[��� & �e�T�[�{�̏�����*/
		case SubMenu:
			key = sub_menu(&sh, &currentMode);
			if(key == KEY1) currentMode = modes.SubMenu.nextMode;
			else if(key == KEY2) showModeRoot(&modes);
			else if(key == KEY3) adjustOffset(&sh);
			break;

			/* GoBy1stPylon�F�J�����E���� & �P�ԖڂɎ擾�����F���𔭌�����܂Œ��i */
		case GoBy1stPylon:
			getImage(color, FIRST_COLOR, &imageData);
			endFlag = goby_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.GoBy1stPylon.nextMode;
			}
			break;

			/* GoBy2ndPylon�F�J�����E���� & �Q�ԖڂɎ擾�����F���𔭌�����܂Œ��i */
		case GoBy2ndPylon:
			getImage(color, SECOND_COLOR, &imageData);
			endFlag = goby_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.GoBy2ndPylon.nextMode;
			}
			break;

			/* GoByTime�F��莞��(sh.time.straight)�̊ԁA���i */
		case GoByTime:
			endFlag = goby_time(&sh, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.GoByTime.nextMode;
			}
			break;

			/* RTurnByTime�F��莞��(sh.time.turn)�̊ԁA�E���� */
		case RTurnByTime:
			endFlag = turnby_time(&sh, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode= modes.RTurnByTime.nextMode;
			}
			break;

			/* LTurnByTime�F��莞��(sh.time.turn)�̊ԁA������ */
		case LTurnByTime:
			endFlag = turnby_time(&sh, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.LTurnByTime.nextMode;
			}
			break;

			/* RTurnToTarget: ���ʂɁA�Q�ԖڂɎ擾�����F���𔭌�����܂ŉE���� */
		case RTurnToTarget:
			getImage(color, SECOND_COLOR, &imageData);
			endFlag = turnto_target(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.RTurnByTime.nextMode;
			}
			break;

			/* LTurnToTarget: ���ʂɁA�P�ԖڂɎ擾�����F�̃p�C�����𔭌�����܂ō����� */
			/* !!!CAUTION!!! �擾�����摜���ƍ�����܂łɎ��Ԃ������肷���āA���񂵂�����B */
		case LTurnToTarget:
			getImage(color, FIRST_COLOR, &imageData);
			endFlag = turnto_target(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.LTurnToTarget.nextMode;
			}
			break;

			/* Homing1stPylon: 1�ԖڂɎ擾�����F�̃p�C�����̉��ׂ�Ɍ������Đi�� */
		case Homing1stPylon:
			getImage(color, FIRST_COLOR, &imageData);
			// endFlag = homing_pylon(&sh, &imageData, &currentMode);
			endFlag = homing_pylon2(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.Homing1stPylon.nextMode;
			}
			break;

			/* Homing2ndPylon: 2�ԖڂɎ擾�����F�̃p�C�����̉��ׂ�Ɍ������Đi��, */
		case Homing2ndPylon:
			getImage(color, SECOND_COLOR, &imageData);
			// endFlag = homing_pylon(&sh, &imageData, &currentMode);
			endFlag = homing_pylon2(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.Homing2ndPylon.nextMode;
			}
			break;

			/* AvoidLeft: 1�ԖڂɎ擾�����F����ʂ��疳���Ȃ�܂Œ��i */
		case AvoidLeft:
			getImage(color, FIRST_COLOR, &imageData);
			endFlag = avoid_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE){
				AUBeep();
				currentMode = modes.AvoidLeft.nextMode;
			}
			break;

			/* AvoidRight: 2�ԖڂɎ擾�����F����ʂ��疳���Ȃ�܂Œ��i */
		case AvoidRight:
			getImage(color, SECOND_COLOR, &imageData);
			endFlag = avoid_pylon(&sh, &imageData, &currentMode);
			if(endFlag == TRUE)	currentMode = modes.AvoidRight.nextMode;
			AUBeep();
			break;

			/* ��OMode: �\�����Ȃ����[�h�ɑJ�ڂ������Ƃ�\�� */
		default:
			LCDClear();
			LCDPrintf("unexpected Mode\n");
			showModeName(&currentMode);
		}
	}

	// �������
	SERVORelease( sh.cameraHandle );   // �J�������
	SERVORelease( sh.rightHandle );    // �E�ԗ։��
	SERVORelease( sh.leftHandle );     // ���ԗ։��
}

/* getImage(): �擾�����摜����F���������o���AimageData�Ɋi�[����֐�
 * color=��ƂȂ�F�̉摜
 * colorNumber=�ƍ�����摜�ԍ�
 * imageData=�擾�����摜�̕��͌��ʂ��i�[����\���� */
void getImage(bitset **color, int colorNumber, struct ImageData *imageData){
	int i, j;
	int hitWindow[5] = {0,0,0,0,0};		// �摜������5�ɕ������Ƃ��A�e������HIT��
	int maxHitWindowNum;
	int count;
	colimage pic; // ����colimage�̗̈��T�����Z���K�v�ɂȂ邽��, �O���[�o���ϐ��Ƃ��������悢?

	CAMGetColFrame(&pic, 0);
	imageData->match = 0;
	imageData->missMatch = 0;
	// image *img = (image *) imageData->imageFlag;
	for (j = 1; j < imagecolumns - 1 ; j++) {
		for (i = 1; i < imagerows - 1 ; i++) {
			// RGB�l����F���l���Z�o�B�Z�o�ł��Ȃ��ꍇ�A definehue �Ƃ����O���[�o���ϐ��� 0 �ɃZ�b�g
			BYTE hue = rgb2hue(pic[i][j][0], pic[i][j][1], pic[i][j][2]);

			//�F���l hue ���F����� bs �ɂ����ĔF���ł��Ă����ꍇ
			if (definehue && isHueArea(color[colorNumber], hue)) {
				imageData->imageFlag[i][j] = HIT;
				// hitWindow[j%5]++;	// ���Z�ʂ����������E�E�E
				imageData->match++;    // �F���ł����F�������C���N�������g
				// �F���ł��Ȃ������ꍇ
			}else{
				imageData->imageFlag[i][j] = MISS;
				imageData->missMatch++;
			}
		}
	}

	count=0;
	// �c������(���Z�ʂ����炷���߁A���imageData->imageFlag[i][j]==MISS�ł��邱�Ƃ��m�F)
	for (j = 2; j < imagecolumns - 2; j++) {
		for (i = 2; i < imagerows - 2; i++) {
			/* �œ_�𓖂ĂĂ����f��MISS�����A����̔����ȏオHIT�̏ꍇ */
			if(	imageData->imageFlag[i][j] == MISS && (
					imageData->imageFlag[i-1][j-1] + imageData->imageFlag[i-1][j+0] + imageData->imageFlag[i-1][j+1] +
					imageData->imageFlag[i+0][j-1] + 				  imageData->imageFlag[i+0][j+1] +
					imageData->imageFlag[i+1][j-1] + imageData->imageFlag[i+1][j+0] + imageData->imageFlag[i+1][j+1]) >= 5
			){
				/* ���̉�f�̓m�C�Y��, �{����HIT�ł������Ɣ��f */
				imageData->imageFlag[i][j] = HIT;
				count++;
			}
		}
	}

	/* �c���������I��������,�C��������f��v�����v�Z���Ȃ��� */
	imageData->match += count;
	imageData->missMatch -= count;
	count = 0;

	// �k������(���Z�ʂ����炷���߁A���imageData->imageFlag[i][j]==HIT�ł��邱�Ƃ��m�F)
	// �k�������Ɠ�����hitWindow���v�Z����? �]�艉�Z�̃R�X�g��������������Ȃ��B
	for (j = 2; j < imagecolumns - 2; j++) {
		for (i = 2; i < imagerows - 2; i++) {
			/* �œ_�𓖂ĂĂ����f��HIT����, ����̔����ȏオHIT�̏ꍇ */
			if(imageData->imageFlag[i][j] == HIT && (
					imageData->imageFlag[i-1][j-1] + imageData->imageFlag[i-1][j+0] +	imageData->imageFlag[i-1][j+1] +
					imageData->imageFlag[i+0][j-1] +					imageData->imageFlag[i+0][j+1] +
					imageData->imageFlag[i+1][j-1] + imageData->imageFlag[i+1][j+0] +	imageData->imageFlag[i+1][j+1]) <= 4
			){
				/* ���̉�f�̓m�C�Y��, �{����MISS�ł������Ɣ��f */
				imageData->imageFlag[i][j] = MISS;
				count++;
			}
		}
	}

	/* �k���������I��������,�C��������f��v�����v�Z���Ȃ��� */
	imageData->match -= count;
	imageData->missMatch += count;

	// #define imagecolumns 82
	// #define imagerows 62

	// ��ʂ��Ƃ�HIT�����J�E���g����(if�����ȗ����邽�߁A���[�v�A�����[�����O����)
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

	/*	�㕔�R�[�h�̌��ƂȂ����v���O����
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

	// HIT�����ł������������ʂ���
	maxHitWindowNum = 0;
	for(i=1;i<5;i++){
		if(hitWindow[maxHitWindowNum] < hitWindow[i])	maxHitWindowNum = i;
	}
	imageData->targetWindow = maxHitWindowNum;	// �ڕW���̏ꏊ�����i�[
}


// backup�p
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
			// RGB�l����F���l���Z�o�B�Z�o�ł��Ȃ��ꍇ�A definehue �Ƃ����O���[�o���ϐ��� 0 �ɃZ�b�g
			BYTE hue = rgb2hue(pic[i][j][0], pic[i][j][1], pic[i][j][2]);

			//�F���l hue ���F����� bs �ɂ����ĔF���ł��Ă����ꍇ
			if (definehue && isHueArea(color[colorNumber], hue)) {
				imageData->imageFlag[i][j] = 0;
				imageData->match++;    // �F���ł����F�������C���N�������g
				// �F���ł��Ȃ������ꍇ
			}else{
				imageData->imageFlag[i][j] = MISS;
				imageData->missMatch++;
			}
		}
	}

	// �c������
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

	// �k������
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

	// ��ʂ��Ƃ�HIT�����J�E���g����(if�����ȗ����邽�߁A���[�v�A�����[�����O����)
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

		/*	�㕔�R�[�h�̌��ƂȂ����v���O����
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

	// HIT�����ł������������ʂ���
	maxHitWindowNum = 0;
	for(i=0;i<5;i++){
		if(hitWindow[maxHitWindowNum] < hitWindow[i])	maxHitWindowNum = i;
	}
	imageData->targetWindow = maxHitWindowNum;	// �ڕW���̏ꏊ�����i�[
}
 // */


