#ifndef _CHARACTER_H
#define _CHARACTER_H

#include "io.h"
#include "gfx.h"

#include "fixed.h"
#include "animation.h"

//Character enums
typedef enum
{
	CharAnim_Idle,
	CharAnim_Left,  CharAnim_LeftAlt,
	CharAnim_Down,  CharAnim_DownAlt,
	CharAnim_Up,    CharAnim_UpAlt,
	CharAnim_Right, CharAnim_RightAlt,
	
	CharAnim_Max //Max standard/shared animation
} CharAnim;

//Character structures
typedef struct
{
	u8 tex;
	u16 src[4];
	s16 off[2];
} CharFrame;

typedef struct Character
{
	//Character functions
	void (*tick)(struct Character*);
	void (*set_anim)(struct Character*, u8);
	void (*free)(struct Character*);
	
	//Position
	fixed_t x, y;
	
	//Stage information
	u8 health_i; //hud1.tim
	fixed_t focus_x, focus_y, focus_zoom;
	
	//Animation state
	Animatable animatable;
	fixed_t sing_end;
} Character;

//Character functions
void Character_Free(Character *this);
void Character_Init(Character *this, fixed_t x, fixed_t y);
void Character_Draw(Character *this, Gfx_Tex *tex, const CharFrame *cframe);

void Character_CheckStartSing(Character *this);
void Character_CheckEndSing(Character *this);

#endif
