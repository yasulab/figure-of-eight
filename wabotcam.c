#include <malloc.h>
#include "eyebot.h"
#include "wabotcam.h"

/* RGB�l����HUE�l���Z�o���� (0 �` 255)
 * �܂��A�O���[�o���ϐ� definehue �ɒl��ݒ肷��
 * �D�F�ȂǐF���l���Z�o�ł��Ȃ��ꍇ -> definehue = 0
 * �F���l���Z�o�ł����ꍇ -> definehue = 1
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

/* RGB�l����SAT�l���Z�o���� (0 �` 255) */
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

/* RGB�l����INT�l���Z�o���� (0 �` 255) */
__inline__ BYTE rgb2int(BYTE r, BYTE g, BYTE b){
	return MAX(r, MAX(g, b));
}

/* �F���l hue ���F����� color �ɂ����ėL�����ǂ��� */
__inline__ int isHueArea(bitset *color, int hue){
	return bitset_get(color, hue);
}


/* C �ɂ��Ȉ� bitset �̎��� (thanks to mizuno)
 * ���̃��C�u�����ł͐F�����̕ێ��Ɏg�p����
 * 256bit �� bitset ��p�ӂ��A0 �` 255 �̐F���l���e�r�b�g�Ɋ֘A�t����
 * 
 * 000001111111111000000000.......0000
 * ���̏ꍇ�A�ԐF�ł��邱�Ƃ𔻕ʂ��邽�߂̐F�����ɂȂ��Ă���
 */

/* bitset �̏����� */
bitset *bitset_init(int length){
  bitset *bs;
  bs = (bitset *)malloc(((length + (BS_NBIT-1)) / BS_NBIT) * sizeof(long));
  return bs;
}
/* bitset �� pos �r�b�g�ڂ̒l���擾 (pos >= 0) */
__inline__ int bitset_get(bitset *bs, int pos){
  return (bs[pos >> BS_SHIFT] & 1LL << (pos & BS_MASK)) != 0;
}
/* bitset �� pos �r�b�g�ڂɒl��ݒ� (pos >= 0) */
__inline__ void bitset_put(bitset *bs, int pos, int val){
  if (val) {
    bs[pos >> BS_SHIFT] |= 1LL << (pos & BS_MASK);
  }
  else {
    bs[pos >> BS_SHIFT] &= ~(1LL << (pos & BS_MASK));
  }
}


/* �F���e�[�u���̎��� 
 * bitset ���g�����Ƃň�̐F�ɂ��� 256 bit �ŐF������ێ��ł���
 * ���� bitset ��z��Ŏ����Ƃŕ����̐F�̐F������ێ��ł��� */

/* �F���e�[�u���̏����� 
 * num �ɂ͌��݂̐F��������B����͏��������߂Ȃ̂� 1 �ɂȂ�B
 * num �͑��ł��g���̂ŕ֋X�I�ɌĂяo���悤�ɂ��Ă���B
 */
bitset **colordata_init(int *num){
	bitset **color;
	color = (bitset **)malloc(sizeof(bitset *) * 1);
	color[0] = bitset_init(256);
	*num = 1;
	return color;
}

/* �F���e�[�u���ɐV�����F�����p�̃f�[�^�̈���m�ۂ��� */
bitset **colordata_add(bitset **color, int *num){
	int p = *num;

	p++;
	color = (bitset **)realloc(color, sizeof(bitset *) * p);
	color[p - 1] = bitset_init(256);
	
	*num = p;
	return color;
}

/* �F���e�[�u������� */
void colordata_free(bitset **color, int num){
	int c;
	for (c = 0; c < num; c++){
		free(color[c]);
	}
	free(color);
}


/* �F���e�[�u���̃Z�b�g�A�b�v�v���O����
 * �J��������F���l���擾���A�F���e�[�u���ɏ����i�[���Ă���
 * number �ɂ͎擾�����F��
 * bitset ** ���F���e�[�u���ƂȂ�
 */
bitset **COLORSetup(int *number){
	int x, y, c, key;
	int max, hue, huegraph[256];
	colimage buf;

	bitset **bs;		// �F���e�[�u���ւ̃|�C���^
	int num;			// �F��

	/* �F���e�[�u���̏����� */ 
	bs = colordata_init(&num);

	do{
		/* �q�X�g�O�����p�̃o�b�t�@ */
		for (c = 0; c < 256; c++){
			huegraph[c] = 0;
		}	

		do{	
			LCDMenu(" ", " ", " ", "CAM");
			LCDSetPos(0, 10); LCDPrintf("# %2d", num);
			
			/* �B�e�҂� */
			CAMGetColFrame(&buf, 0);
			while (KEYRead() != KEY4){
				LCDPutColorGraphic(&buf);
				CAMGetColFrame(&buf, 0);
			}
			AUBeep();
	
			/* �B�e�����摜��F���l�ɕϊ� */
			for (y = 1; y < 61; y++){
				for (x = 1; x < 81; x++){
					hue = rgb2hue(buf[y][x][0], buf[y][x][1], buf[y][x][2]);
					
					/* �F���l�������Ă���΁A�q�X�g�O�����ɒǉ� */
					if (definehue){
						huegraph[hue]++;
					}
				}
			}

			/* �����ꖇ�A�����F�ɂ��ĎB�邩�ǂ���
			 * MORE: �����ꖇ�B��
			 *   OK: �q�X�g�O�����̏�����
			 */
			LCDMenu(" ", " ", "MORE", " OK");
			key = 0;
			while ((key = KEYRead()) != KEY3 && key != KEY4)
				;

		}while(key == KEY3);

		/* �q�X�g�O�����̏���
		 * �q�X�g�O�������̍ő�l�����臒l�ɖ����Ȃ����̂� 0 ��
		 * ���������̂� 1 �Ƃ��āAbitset �ɐݒ肵�Ă���
		 * 
		 * ���݂�臒l�͍ő�l�� 5 % 
		 */

		/* �ő�̐F���l���擾 */
		max = -1;
		for (c = 0; c < 256; c++){
			if (max < huegraph[c]){
				max = huegraph[c];
				LCDSetPos(6, 0); LCDPrintf("MAX %3d:%4d", c, max);
			}
		}
		/* 臒l (�v�f���ł������F���l�̗v�f���� 5% �ȉ�) */
		max = max / 20;
		for (c = 0; c < 256; c++){
			bitset_put(bs[num - 1], c, max <= huegraph[c]);
		}

		/* �q�X�g�O�����̏������ʂ̊m�F - LCD ��ɕ\�� */
		LCDClear();
		for (c = 0; c < 256; c++){
			LCDLine(c % 128, 10 + (c / 128) * 30, 
					c % 128, 10 + (c / 128) * 30 - bitset_get(bs[num - 1], c) * 5, 1);
		}

		/* ����� OK ���ǂ����A�܂����̐F�ɂ��Ă����W���邩�ǂ���
		 *   RE: ���݂̐F����蒼��
		 *  NXT: ���̐F�̎��W�Ɉڂ�
		 * DONE: �F�̎��W���I��
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

/* �F���e�[�u���Ɋi�[����Ă��邷�ׂĂ̐F�̏��� LCD �ɕ\������
 * num �ɂ͕\������F�����w�肷��
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

/* �F���e�[�u�����J������ */
void COLORRelease(bitset **bs, int num){
	colordata_free(bs, num);
}

