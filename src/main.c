#include "main.h"

#include "mem.h"
#include "timer.h"
#include "io.h"
#include "gfx.h"
#include "audio.h"
#include "pad.h"

#include "menu.h"
#include "stage.h"

//Game loop
GameLoop gameloop;

//Error handler
char error_msg[0x200];

void ErrorLock()
{
	while (1)
	{
		FntPrint("A fatal error has occured\n~c700%s\n", error_msg);
		Gfx_Flip();
	}
}

//Entry point
u32 malloc_heap[0x100000 / sizeof(u32)];

int main()
{
	//Initialize system
	Mem_Init((void*)malloc_heap, sizeof(malloc_heap));
	
	IO_Init();
	Audio_Init();
	Pad_Init();
	Gfx_Init();
	
	Timer_Init();
	
	//Start game
	gameloop = GameLoop_Menu;
	Menu_Load(MenuPage_Opening);
	
	//Game loop
	while (1)
	{
		//Prepare frame
		Timer_Tick();
		Audio_ProcessXA();
		Pad_Update();
		
		#ifdef MEM_STAT
			//Memory stats
			size_t mem_used, mem_size, mem_max;
			Mem_GetStat(&mem_used, &mem_size, &mem_max);
			#ifndef MEM_BAR
				FntPrint("mem: %08X/%08X (max %08X)\n", mem_used, mem_size, mem_max);
			#endif
		#endif
		
		//Tick and draw game
		switch (gameloop)
		{
			case GameLoop_Menu:
				Menu_Tick();
				break;
			case GameLoop_Stage:
				Stage_Tick();
				break;
		}
		
		//Flip gfx buffers
		Gfx_Flip();
	}
	return 0;
}
