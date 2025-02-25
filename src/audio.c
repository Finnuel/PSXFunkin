#include "psx.h"

#include "audio.h"
#include "io.h"
#include "main.h"

//XA state
#define XA_STATE_INIT    (1 << 0)
#define XA_STATE_PLAYING (1 << 1)
#define XA_STATE_LOOPS   (1 << 2)
#define XA_STATE_SEEKING (1 << 3)
static u8 xa_state, xa_resync, xa_volume, xa_channel;
static u32 xa_pos, xa_start, xa_end;

//XA files and tracks
#define XA_LENGTH(x) (((u64)(x) * 75) / 100 * IO_SECT_SIZE) //Centiseconds to sectors in bytes (w)

static CdlFILE xa_files[XA_Max];

typedef struct
{
	XA_File file;
	u32 length;
} XA_TrackDef;

static const XA_TrackDef xa_tracks[] = {
	//MENU.XA
	{XA_Menu, XA_LENGTH(11295)}, //XA_GettinFreaky
	{XA_Menu, XA_LENGTH(3840)},  //XA_GameOver
	//WEEK1A.XA
	{XA_Week1A, XA_LENGTH(7685)}, //XA_Bopeebo
	{XA_Week1A, XA_LENGTH(8000)}, //XA_Fresh
	//WEEK1B.XA
	{XA_Week1B, XA_LENGTH(8667)}, //XA_Dadbattle
	{XA_Week1B, XA_LENGTH(6800)}, //XA_Tutorial
	//WEEK2A.XA
	{XA_Week2A, XA_LENGTH(9923)}, //XA_Spookeez
	{XA_Week2A, XA_LENGTH(8880)}, //XA_South
	//WEEK2B.XA
	{XA_Week2B, XA_LENGTH(17778)}, //XA_Monster
	//WEEK3A.XA
	{XA_Week3A, XA_LENGTH(8400)},  //XA_Pico
	{XA_Week3A, XA_LENGTH(10000)}, //XA_Philly
	//WEEK3B.XA
	{XA_Week3B, XA_LENGTH(10700)}, //XA_Blammed
	//WEEK4A.XA
	{XA_Week4A, XA_LENGTH(9300)},  //XA_SatinPanties
	{XA_Week4A, XA_LENGTH(10300)}, //XA_High
	//WEEK4B.XA
	{XA_Week4B, XA_LENGTH(12300)}, //XA_MILF
	{XA_Week4B, XA_LENGTH(10300)}, //XA_Test
	//WEEK5A.XA
	{XA_Week5A, XA_LENGTH(11520)}, //XA_Cocoa
	{XA_Week5A, XA_LENGTH(9401)}, //XA_Eggnog
	//WEEK5B.XA
	{XA_Week5B, XA_LENGTH(13223)}, //XA_WinterHorrorland
	//WEEK6A.XA
	{XA_Week6A, XA_LENGTH(9800)}, //XA_Senpai
	{XA_Week6A, XA_LENGTH(8928)}, //XA_Roses
	//WEEK6B.XA
	{XA_Week6B, XA_LENGTH(10298)}, //XA_Thorns
	//WEEK7A.XA
	{XA_Week7A, XA_LENGTH(8493)},  //XA_Ugh
	{XA_Week7A, XA_LENGTH(13866)}, //XA_Guns
	//WEEK7B.XA
	{XA_Week7B, XA_LENGTH(12200)}, //XA_Stress
	//KAPIA.XA
	{XA_KapiA, XA_LENGTH(12138)}, //XA_Wocky
	{XA_KapiA, XA_LENGTH(12000)}, //XA_Beathoven
	//KAPIB.XA
	{XA_KapiB, XA_LENGTH(17159)}, //XA_Hairball
	{XA_KapiB, XA_LENGTH(15157)}, //XA_Nyaw
	//CLWNA.XA
	{XA_ClwnA, XA_LENGTH(10740)}, //XA_ImprobableOutset
	{XA_ClwnA, XA_LENGTH(15169)}, //XA_Madness
	//CLWNB.XA
	{XA_ClwnB, XA_LENGTH(21886)}, //XA_Hellclown
	{XA_ClwnB, XA_LENGTH(19607)}, //XA_Expurgation
};


//Internal XA functions
static u8 XA_BCD(u8 x)
{
	return x - 6 * (x >> 4);
}

static u32 XA_TellSector()
{
	u8 result[8];
	CdControlB(CdlGetlocP, NULL, result);
	return (XA_BCD(result[2]) * 75 * 60) + (XA_BCD(result[3]) * 75) + XA_BCD(result[4]);
}

static void XA_SetVolume(u8 x)
{
	//Set CD mix volume
	CdlATV cd_vol;
	xa_volume = cd_vol.val0 = cd_vol.val1 = cd_vol.val2 = cd_vol.val3 = x;
	CdMix(&cd_vol);
}

static void XA_Init()
{
	u8 param[4];
	
	//Set XA state
	if (xa_state & XA_STATE_INIT)
		return;
	xa_state = XA_STATE_INIT;
	xa_resync = 0;
	
	//Set CD mix flag
	SpuCommonAttr spu_attr;
	spu_attr.mask = SPU_COMMON_CDMIX | SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
	spu_attr.cd.mix = SPU_ON;
	spu_attr.cd.volume.left = spu_attr.cd.volume.right = 0x6000; //Lame magic number
	SpuSetCommonAttr(&spu_attr);
	
	//Set initial volume
	XA_SetVolume(0);
	
	//Prepare CD drive for XA reading
	param[0] = CdlModeRT | CdlModeSF | CdlModeSize1;
	
	CdControlB(CdlSetmode, param, NULL);
	CdControlF(CdlPause, NULL);
}

static void XA_Quit()
{
	//Set XA state
	if (!(xa_state & XA_STATE_INIT))
		return;
	xa_state = 0;
	
	//Stop playing XA
	XA_SetVolume(0);
	CdControlB(CdlPause, NULL, NULL);
}

static void XA_Play(u32 start)
{
	//Play at given position
	CdlLOC cd_loc;
	CdIntToPos(start, &cd_loc);
	CdControlF(CdlReadS, (u8*)&cd_loc);
}

static void XA_Pause()
{
	//Set XA state
	if (!(xa_state & XA_STATE_PLAYING))
		return;
	xa_state &= ~XA_STATE_PLAYING;
	
	//Pause playback
	CdControlB(CdlPause, NULL, NULL);
}

static void XA_SetFilter(u8 channel)
{
	//Change CD filter
	CdlFILTER filter;
	filter.file = 1;
	xa_channel = filter.chan = channel;
	CdControlF(CdlSetfilter, (u8*)&filter);
}

//Audio functions
void Audio_Init()
{
	//Initialize sound system
	SsInit();
	SsSetSerialVol(SS_SERIAL_A, 0x7F, 0x7F);
	
	//Set XA state
	xa_state = 0;
	
	//Get file positions
	const char **pathp = (const char*[]){
		"\\MUSIC\\MENU.XA;1",   //XA_Menu
		"\\MUSIC\\WEEK1A.XA;1", //XA_Week1A
		"\\MUSIC\\WEEK1B.XA;1", //XA_Week1B
		"\\MUSIC\\WEEK2A.XA;1", //XA_Week2A
		"\\MUSIC\\WEEK2B.XA;1", //XA_Week2B
		"\\MUSIC\\WEEK3A.XA;1", //XA_Week3A
		"\\MUSIC\\WEEK3B.XA;1", //XA_Week3B
		"\\MUSIC\\WEEK4A.XA;1", //XA_Week4A
		"\\MUSIC\\WEEK4B.XA;1", //XA_Week4B
		"\\MUSIC\\WEEK5A.XA;1", //XA_Week5A
		"\\MUSIC\\WEEK5B.XA;1", //XA_Week5B
		"\\MUSIC\\WEEK6A.XA;1", //XA_Week6A
		"\\MUSIC\\WEEK6B.XA;1", //XA_Week6B
		"\\MUSIC\\WEEK7A.XA;1", //XA_Week7A
		"\\MUSIC\\WEEK7B.XA;1", //XA_Week7B
		"\\MUSIC\\KAPIA.XA;1",  //XA_KapiA
		"\\MUSIC\\KAPIB.XA;1",  //XA_KapiB
		"\\MUSIC\\CLWNA.XA;1",  //XA_ClwnA
		"\\MUSIC\\CLWNB.XA;1",  //XA_ClwnB
	};
	CdlFILE *filep = xa_files;
	for (u8 i = 0; i < XA_Max; i++)
		IO_FindFile(filep++, *pathp++);
}

void Audio_GetXAFile(CdlFILE *file, XA_Track track)
{
	const XA_TrackDef *track_def = &xa_tracks[track];
	file->pos = xa_files[track_def->file].pos;
	file->size = track_def->length;
}

void Audio_PlayXA_File(CdlFILE *file, u8 volume, u8 channel, boolean loop)
{
	//Initialize XA system and stop previous song
	XA_Init();
	XA_SetVolume(0);
	
	//Set XA state
	xa_start = xa_pos = CdPosToInt(&file->pos);
	xa_end = xa_start + (file->size / IO_SECT_SIZE) - 1;
	xa_state = XA_STATE_INIT | XA_STATE_PLAYING | XA_STATE_SEEKING;
	xa_resync = 0;
	if (loop)
		xa_state |= XA_STATE_LOOPS;
	
	//Start seeking to XA and use parameters
	IO_SeekFile(file);
	XA_SetFilter(channel);
	XA_SetVolume(volume);
}

void Audio_PlayXA_Track(XA_Track track, u8 volume, u8 channel, boolean loop)
{
	//Get track information
	CdlFILE file;
	Audio_GetXAFile(&file, track);
	
	//Play track
	Audio_PlayXA_File(&file, volume, channel, loop);
}

void Audio_PlayXA(const char *path, u8 volume, u8 channel, boolean loop)
{
	//Search for and play file
	CdlFILE file;
	IO_FindFile(&file, path);
	Audio_PlayXA_File(&file, volume, channel, loop);
}

void Audio_PauseXA()
{
	//Pause playing XA file
	XA_Pause();
}

void Audio_StopXA()
{
	//Deinitialize XA system
	XA_Quit();
}

void Audio_ChannelXA(u8 channel)
{
	//Set XA filter to the given channel
	XA_SetFilter(channel);
}

s32 Audio_TellXA_Sector()
{
	//Get CD position
	return (s32)xa_pos - (s32)xa_start; //Meh casting
}

s32 Audio_TellXA_Milli()
{
	return ((s32)xa_pos - (s32)xa_start) * 1000 / 75; //1000 / (75 * speed (1x))
}

boolean Audio_PlayingXA()
{
	return (xa_state & XA_STATE_PLAYING) != 0;
}

void Audio_WaitPlayXA()
{
	while (1)
	{
		Audio_ProcessXA();
		if (Audio_PlayingXA())
			return;
		VSync(0);
	}
}

void Audio_ProcessXA()
{
	//Handle playing state
	if (xa_state & XA_STATE_PLAYING)
	{
		//Retrieve CD status
		CdControl(CdlNop, NULL, NULL);
		u8 cd_status = CdStatus();
		
		//Handle resync timer
		if (xa_resync != 0)
		{
			//Wait for resync timer
			if (--xa_resync != 0)
				return;
			
			//Check if we're in a proper state
			if (cd_status & CdlStatShellOpen)
				return;
			
			//Attempt to get CD drive active
			while (1)
			{
				CdControl(CdlNop, NULL, NULL);
				cd_status = CdStatus();
				if (cd_status & CdlStatStandby)
					break;
			}
			
			//Re-initialize XA system
			u8 prev_state = xa_state;
			XA_Init();
			xa_state = prev_state;
			
			XA_SetFilter(xa_channel);
			XA_SetVolume(xa_volume);
			
			//Get new CD status
			CdControl(CdlNop, NULL, NULL);
			cd_status = CdStatus();
		}
		
		//Check CD status for issues
		if (cd_status & CdlStatShellOpen)
		{
			//Seek just ahead of last reported valid position
			if (!(xa_state & XA_STATE_SEEKING))
			{
				xa_pos++;
				xa_state |= XA_STATE_SEEKING;
			}
			
			//Wait a moment before attempting the actual resync
			xa_resync = 60;
			return;
		}
		
		//Handle seeking state
		if (xa_state & XA_STATE_SEEKING)
		{
			//Check if CD is still seeking to the XA's beginning
			if (!(cd_status & CdlStatSeek))
			{
				//Stopped seeking
				xa_state &= ~XA_STATE_SEEKING;
				XA_Play(xa_pos);
			}
			else
			{
				//Still seeking
				return;
			}
		}
		
		//Get CD position
		u32 next_pos = XA_TellSector();
		if (next_pos > xa_pos)
			xa_pos = next_pos;
		
		//Check position
		if (xa_pos >= xa_end)
		{
			if (xa_state & XA_STATE_LOOPS)
			{
				//Reset XA playback
				CdlLOC cd_loc;
				CdIntToPos(xa_pos = xa_start, &cd_loc);
				CdControlB(CdlSeekL, (u8*)&cd_loc, NULL);
				xa_state |= XA_STATE_SEEKING;
			}
			else
			{
				//Stop XA playback
				Audio_StopXA();
			}
		}
	}
}

