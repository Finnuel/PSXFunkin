#ifndef _AUDIO_H
#define _AUDIO_H

#include "psx.h"

//XA enumerations
typedef enum
{
	XA_Menu,   //MENU.XA
	XA_Week1A, //WEEK1A.XA
	XA_Week1B, //WEEK1B.XA
	XA_Week2A, //WEEK2A.XA
	XA_Week2B, //WEEK2B.XA
	XA_Week3A, //WEEK3A.XA
	XA_Week3B, //WEEK3B.XA
	XA_Week4A, //WEEK4A.XA
	XA_Week4B, //WEEK4B.XA
	XA_Week5A, //WEEK5A.XA
	XA_Week5B, //WEEK5B.XA
	XA_Week6A, //WEEK6A.XA
	XA_Week6B, //WEEK6B.XA
	XA_Week7A, //WEEK7A.XA
	XA_Week7B, //WEEK7B.XA
	XA_KapiA,  //KAPIA.XA
	XA_KapiB,  //KAPIB.XA
	XA_ClwnA,  //CLWNA.XA
	XA_ClwnB,  //CLWNB.XA
	
	XA_Max,
} XA_File;

typedef enum
{
	//MENU.XA
	XA_GettinFreaky, //Gettin' Freaky
	XA_GameOver,     //Game Over
	//WEEK1A.XA
	XA_Bopeebo, //Bopeebo
	XA_Fresh,   //Fresh
	//WEEK1B.XA
	XA_Dadbattle, //DadBattle
	XA_Tutorial,  //Tutorial
	//WEEK2A.XA
	XA_Spookeez, //Spookeez
	XA_South,    //South
	//WEEK2B.XA
	XA_Monster, //Monster
	//WEEK3A.XA
	XA_Pico,   //Pico
	XA_Philly, //Philly
	//WEEK3B.XA
	XA_Blammed, //Blammed
	//WEEK4A.XA
	XA_SatinPanties, //Satin Panties
	XA_High,         //High
	//WEEK4B.XA
	XA_MILF, //M.I.L.F
	XA_Test, //Test
	//WEEK5A.XA
	XA_Cocoa,  //Cocoa
	XA_Eggnog, //Eggnog
	//WEEK5B.XA
	XA_WinterHorrorland, //Winter Horrorland
	//WEEK6A.XA
	XA_Senpai, //Senpai
	XA_Roses,  //Roses
	//WEEK6B.XA
	XA_Thorns, //Thorns
	//WEEK7A.XA
	XA_Ugh,  //Ugh
	XA_Guns, //Guns
	//WEEK7B.XA
	XA_Stress, //Stress
	//KAPIA.XA
	XA_Wocky,     //Wocky
	XA_Beathoven, //Beathoven
	//KAPIB.XA
	XA_Hairball, //Hairball
	XA_Nyaw,     //Nyaw
	//CLWNA.XA
	XA_ImprobableOutset, //Improbable Outset
	XA_Madness,          //Madness
	//CLWNB.XA
	XA_Hellclown,   //Hellclown
	XA_Expurgation, //Expurgation
} XA_Track;

//Audio functions
void Audio_Init();
void Audio_GetXAFile(CdlFILE *file, XA_Track track);
void Audio_PlayXA_File(CdlFILE *file, u8 volume, u8 channel, boolean loop);
void Audio_PlayXA_Track(XA_Track track, u8 volume, u8 channel, boolean loop);
void Audio_PlayXA(const char *path, u8 volume, u8 channel, boolean loop);
void Audio_PauseXA();
void Audio_StopXA();
void Audio_ChannelXA(u8 channel);
s32 Audio_TellXA_Sector();
s32 Audio_TellXA_Milli();
boolean Audio_PlayingXA();
void Audio_WaitPlayXA();
void Audio_ProcessXA();

#endif
