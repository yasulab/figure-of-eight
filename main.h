#ifndef __BEEPER_H__
#define __BEEPER_H__
#include "eyebot.h"

#define FIRST_COLOR 0   // 1回目に取得した色
#define SECOND_COLOR 1  // 2回目に取得した色
#define NUM_MATCH 500   // 色相一致の判断基準(範囲：0<X<4800)
#define NEAR_MATCH 3000 // 目標物の色で画面が一杯になる判断基準
#define HIT 1	// imageは8bitの構造体なので255が上限値
#define MISS 0

/* 車輪に関するデータ構造体 */
struct Wheel{
    int stop;      // 車輪停止
    int walk;      // 車輪徐行スピード
    int run;       // 車輪走行スピード
    int backward;  // 車輪後退スピード
    int offset;    // 車輪オフセット
    int homing;
};

/* カメラに関するデータ構造体 */
struct Camera{
    int center;    // カメラ正面
    int left;      // カメラ左向き
    int right;     // カメラ右向き
    int leftMax;   // カメラ左向き最大値
    int rightMax;  // カメラ右向き最大値
    int offset;    // カメラ方向オフセット
    int turn;           // カメラ自動修整時の回転角度   (使用ステージ例：第５)
    int left_center;    // 目標物の左右位置の識別基準値 (使用ステージ例：第５)
    int center_right;   // 目標物の左右位置の識別基準値 (使用ステージ例：第５)
};

/* 時間に関するデータ構造体 */
struct Time{
    int turn;      		// 旋回する時間(使用ステージ例：第２)
    int straight;  		// 直進する時間(使用ステージ例：第６)
    int adjustDirection;// Uターン後の方向を調整する時間(使用ステージ例：第７)
};

/* 使用するサーボをまとめた構造体 */
struct ServoHandles{
	ServoHandle cameraHandle;
	ServoHandle rightHandle;
	ServoHandle leftHandle;
	struct Wheel rw;
	struct Wheel lw;
	struct Camera cam;
	struct Time time;
};

/* 各モードに関するデータ構造体 */
struct Mode{
	int nextMode;
};

/* 各モードをまとめた構造体 */
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

/* イメージデータに関する構造体 */
struct ImageData{
	image imageFlag;
	int targetWindow;
	int match;
	int missMatch;
};

/* モードの列挙型宣言 */
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
