#ifndef __WABOTCAM_H__
#define __WABOTCAM_H__
#include "eyebot.h"

/* for sendImageBinary */
#define FRAME		0	/* grayscale 82x62   */
#define COLFRAME	1	/* color     82x62   */
#define FRAMEMONO	2   /* grayscale 176x144 */
#define FRAMERGB	3   /* color     176x144 */

/* for rgb2hue */
int definehue;

#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)

/* for bitset */
typedef long bitset;
#define BS_SHIFT 5
#define BS_MASK 31
#define BS_NBIT 32


/* prototype declare */
BYTE rgb2hue(BYTE, BYTE, BYTE);
BYTE rgb2sat(BYTE, BYTE, BYTE);
__inline__ BYTE rgb2int(BYTE, BYTE, BYTE);
__inline__ int isHueArea(bitset *, int);

bitset *bitset_init(int);
__inline__ int bitset_get(bitset *, int);
__inline__ void bitset_put(bitset *, int, int);

bitset **colordata_init(int *);
bitset **colordata_add(bitset **, int *);
void colordata_free(bitset **, int);

bitset **COLORSetup(int *);
void COLORInfo(bitset **, int);
void COLORRelease(bitset **, int);
#endif
