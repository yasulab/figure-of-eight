#ifndef __BEEPER_H__
#define __BEEPER_H__
#include "eyebot.h"

#define FIRST_COLOR 0   // 1��ڂɎ擾�����F
#define SECOND_COLOR 1  // 2��ڂɎ擾�����F
#define NUM_MATCH 500   // �F����v�̔��f�(�͈́F0<X<4800)
#define NEAR_MATCH 3000 // �ڕW���̐F�ŉ�ʂ���t�ɂȂ锻�f�
#define HIT 1	// image��8bit�̍\���̂Ȃ̂�255������l
#define MISS 0

/* �ԗւɊւ���f�[�^�\���� */
struct Wheel{
    int stop;      // �ԗ֒�~
    int walk;      // �ԗ֏��s�X�s�[�h
    int run;       // �ԗ֑��s�X�s�[�h
    int backward;  // �ԗ֌�ރX�s�[�h
    int offset;    // �ԗփI�t�Z�b�g
    int homing;
};

/* �J�����Ɋւ���f�[�^�\���� */
struct Camera{
    int center;    // �J��������
    int left;      // �J����������
    int right;     // �J�����E����
    int leftMax;   // �J�����������ő�l
    int rightMax;  // �J�����E�����ő�l
    int offset;    // �J���������I�t�Z�b�g
    int turn;           // �J���������C�����̉�]�p�x   (�g�p�X�e�[�W��F��T)
    int left_center;    // �ڕW���̍��E�ʒu�̎��ʊ�l (�g�p�X�e�[�W��F��T)
    int center_right;   // �ڕW���̍��E�ʒu�̎��ʊ�l (�g�p�X�e�[�W��F��T)
};

/* ���ԂɊւ���f�[�^�\���� */
struct Time{
    int turn;      		// ���񂷂鎞��(�g�p�X�e�[�W��F��Q)
    int straight;  		// ���i���鎞��(�g�p�X�e�[�W��F��U)
    int adjustDirection;// U�^�[����̕����𒲐����鎞��(�g�p�X�e�[�W��F��V)
};

/* �g�p����T�[�{���܂Ƃ߂��\���� */
struct ServoHandles{
	ServoHandle cameraHandle;
	ServoHandle rightHandle;
	ServoHandle leftHandle;
	struct Wheel rw;
	struct Wheel lw;
	struct Camera cam;
	struct Time time;
};

/* �e���[�h�Ɋւ���f�[�^�\���� */
struct Mode{
	int nextMode;
};

/* �e���[�h���܂Ƃ߂��\���� */
struct Modes{
    struct Mode SubMenu;
    struct Mode GoBy1stPylon;
    struct Mode GoBy2ndPylon;
    struct Mode LTurnByTime;
    struct Mode RTurnByTime;
    struct Mode GoByTime;
    struct Mode RTurnToTarget;
    struct Mode LTurnToTarget;
    struct Mode Homing1stPylon;
    struct Mode Homing2ndPylon;
    struct Mode AvoidLeft;
    struct Mode AvoidRight;
};

/* �C���[�W�f�[�^�Ɋւ���\���� */
struct ImageData{
	image imageFlag;
	int targetWindow;
	int match;
	int missMatch;
};

/* ���[�h�̗񋓌^�錾 */
enum ModeEnum{ SubMenu, GoBy1stPylon, GoBy2ndPylon, RTurnByTime, LTurnByTime, GoByTime,
				RTurnToTarget, LTurnToTarget, Homing1stPylon, Homing2ndPylon,
				AvoidRight, AvoidLeft, ModeLength };

/* main.c */
void init_servo(struct ServoHandles *);
void set_mode(struct Modes *);
void run(bitset **color);
void getImage(bitset **color, int, struct ImageData *);
void getImage2(bitset **color, int, struct ImageData *);

/* sys.c */
void showLCDClear(void);
void showMatchNum(struct ImageData *);
void showTargetPlace(struct ImageData *);
void showModeRoot();
void showModeName(int *);
void adjustOffset(struct ServoHandles *);

/* mode.c */
int sub_menu(struct ServoHandles *, int *);
int goby_pylon(struct ServoHandles *,  struct ImageData *, int *);
int goby_time(struct ServoHandles *, int *);
int turnby_time(struct ServoHandles *, int *);
int turnto_target(struct ServoHandles *, struct ImageData *, int *);
int homing_pylon(struct ServoHandles *, struct ImageData *, int *);
int homing_pylon2(struct ServoHandles *, struct ImageData *, int *);
int avoid_pylon(struct ServoHandles *, struct ImageData *, int *);

#endif
