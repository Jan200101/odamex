// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//		DOOM selection menu, options, episode etc.
//		Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "dstrings.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "d_main.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "m_argv.h"
#include "m_swap.h"
#include "m_random.h"
#include "s_sound.h"
#include "doomstat.h"
#include "m_menu.h"
#include "v_text.h"
#include "st_stuff.h"
#include "cl_ctf.h"
#include "r_sky.h"

#include "gi.h"

extern patch_t* 	hu_font[HU_FONTSIZE];

// temp for screenblocks (0-9)
int 				screenSize;			

 // 1 = message to be printed
int 				messageToPrint;
// ...and here is the message string!
char*				messageString;			

// message x & y
int 				messx;					
int 				messy;
int 				messageLastMenuActive;

// timed message = no input from user
bool				messageNeedsInput;	   

void	(*messageRoutine)(int response);
void	CL_SendUserInfo(void); 
void	M_ChangeTeam (int choice);
void	CL_RememberSkin(void);

team_t playersteam;

#define SAVESTRINGSIZE	24

// we are going to be entering a savegame string
int 				genStringEnter;
int					genStringLen;	// [RH] Max # of chars that can be entered
void	(*genStringEnd)(int slot);
int 				saveSlot;		// which slot to save in
int 				saveCharIndex;	// which char we're editing
// old save description before edit
char				saveOldString[SAVESTRINGSIZE];	

bool 				menuactive;

#define SKULLXOFF	-32
#define LINEHEIGHT	16

extern bool st_firsttime;

extern bool			sendpause;
char				savegamestrings[10][SAVESTRINGSIZE];

char				endstring[160];

menustack_t			MenuStack[16];
int					MenuStackDepth;

short				itemOn; 			// menu item skull is on
short				skullAnimCounter;	// skull animation counter
short				whichSkull; 		// which skull to draw
bool				drawSkull;			// [RH] don't always draw skull

// graphic name of skulls
char				skullName[2][9] = {"M_SKULL1", "M_SKULL2"};

// current menudef
oldmenu_t *currentMenu;						  

//
// PROTOTYPES
//
void M_Options(int choice);
void M_EndGame(int choice);
void M_QuitDOOM(int choice);

void M_ChangeDetail(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_DrawMainMenu(void);
void M_DrawOptions(void);
void M_DrawSound(void);

void M_DrawSaveLoadBorder(int x,int y, int len);
void M_SetupNextMenu(oldmenu_t *menudef);
void M_DrawEmptyCell(oldmenu_t *menu,int item);
void M_DrawSelCell(oldmenu_t *menu,int item);
int  M_StringHeight(char *string);
void M_StartControlPanel(void);
void M_StartMessage(char *string,void (*routine)(int),bool input);
void M_StopMessage(void);
void M_ClearMenus (void);

// [RH] For player setup menu.
static void M_PlayerSetup (int choice);
static void M_PlayerSetupTicker (void);
static void M_PlayerSetupDrawer (void);
static void M_EditPlayerName (int choice);
//static void M_EditPlayerTeam (int choice);
static void M_PlayerTeamChanged (int choice);
static void M_PlayerNameChanged (int choice);
static void M_SlidePlayerRed (int choice);
static void M_SlidePlayerGreen (int choice);
static void M_SlidePlayerBlue (int choice);
static void M_ChangeGender (int choice);
static void M_ChangeSkin (int choice);
static void M_ChangeAutoAim (int choice);
bool M_DemoNoPlay;

static DCanvas *FireScreen;

//
// DOOM MENU
//
enum main_t
{
	options = 0,					// [RH] Moved
	playersetup,					// [RH] Player setup
	quitdoom,
	main_end
}main_e;

oldmenuitem_t MainMenu[]=
{
	{1,"M_OPTION",M_Options,'O'},	// [RH] Moved
	{1,"M_PSETUP",M_PlayerSetup,'P'},	// [RH] Player setup
	{1,"M_QUITG",M_QuitDOOM,'Q'}
};

oldmenu_t MainDef =
{
	main_end,
	MainMenu,
	M_DrawMainMenu,
	97,108,
	0
};

//
// [RH] Player Setup Menu
//
byte FireRemap[256];

enum psetup_t
{
	playername,
	playerteam,
	playerred,
	playergreen,
	playerblue,
	playersex,
	playerskin,
	playeraim,
	psetup_end
} psetup_e;

oldmenuitem_t PlayerSetupMenu[] =
{
	{ 1,"", M_EditPlayerName, 'N' },
	{ 1,"", M_ChangeTeam, 'T' },
	{ 2,"", M_SlidePlayerRed, 'R' },
	{ 2,"", M_SlidePlayerGreen, 'G' },
	{ 2,"", M_SlidePlayerBlue, 'B' },
	{ 2,"", M_ChangeGender, 'E' },
	{ 2,"", M_ChangeSkin, 'S' },
	{ 2,"", M_ChangeAutoAim, 'A' }
};

oldmenu_t PSetupDef = {
	psetup_end,
	PlayerSetupMenu,
	M_PlayerSetupDrawer,
	48,	47,
	playername
};

//
// OPTIONS MENU
//
// [RH] This menu is now handled in m_options.c
//
bool OptionsActive;

oldmenu_t OptionsDef =
{
	0,
	NULL,
	NULL,
	0,0,
	0
};

// [RH] Most menus can now be accessed directly
// through console commands.
BEGIN_COMMAND (menu_main)
{
	M_StartControlPanel ();
	M_SetupNextMenu (&MainDef);
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
}
END_COMMAND (menu_main)

BEGIN_COMMAND (menu_endgame)
{	// F7
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_EndGame(0);
}
END_COMMAND (menu_endgame)

BEGIN_COMMAND (menu_quit)
{	// F10
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_StartControlPanel ();
	M_QuitDOOM(0);
}
END_COMMAND (menu_quit)
								
BEGIN_COMMAND (menu_options)
{
	M_StartControlPanel ();
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_Options(0);
}
END_COMMAND (menu_options)

BEGIN_COMMAND (menu_player)
{
	M_StartControlPanel ();
	S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	M_PlayerSetup(0);
}
END_COMMAND (menu_player)

BEGIN_COMMAND (bumpgamma)
{
	// [RH] Gamma correction tables are now generated
	// on the fly for *any* gamma level.
	// Q: What are reasonable limits to use here?

	float newgamma = Gamma + 0.1;

	if (newgamma > 3.0)
		newgamma = 1.0;

	Gamma.Set (newgamma);
	Printf (PRINT_HIGH, "Gamma correction level %g\n", Gamma.value());
}
END_COMMAND (bumpgamma)

//
// Draw border for the savegame description
// [RH] Width of the border is variable
//
void M_DrawSaveLoadBorder (int x, int y, int len)
{
	int i;
		
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName ("M_LSLEFT",PU_CACHE), x-8, y+7);
		
	for (i = 0; i < len; i++)
	{
		screen->DrawPatchClean ((patch_t *)W_CacheLumpName ("M_LSCNTR",PU_CACHE), x, y+7);
		x += 8;
	}

	screen->DrawPatchClean ((patch_t *)W_CacheLumpName ("M_LSRGHT",PU_CACHE), x, y+7);
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu (void)
{
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_DOOM",PU_CACHE), 94, 42);
}

//
// M_Options
//
void M_DrawOptions(void)
{
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_OPTTTL",PU_CACHE), 108, 15);
}

void M_Options(int choice)
{
	//M_SetupNextMenu(&OptionsDef);
	OptionsActive = M_StartOptionsMenu();
}

//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
	if (toupper(ch) != 'Y')
		return;
				
	currentMenu->lastOn = itemOn;
	M_ClearMenus ();
	D_StartTitle ();
}

void M_EndGame(int choice)
{
	choice = 0;
	if (!usergame)
	{
		S_Sound (CHAN_VOICE, "player/male/grunt1", 1, ATTN_NONE);
		return;
	}
		
	if (netgame)
	{
		M_StartMessage(NETEND,NULL,false);
		return;
	}
		
	M_StartMessage(ENDGAME,M_EndGameResponse,true);
}

//
// M_QuitDOOM
//

void M_QuitResponse(int ch)
{
	if (toupper(ch) != 'Y')
		return;

	if (!netgame)
	{
		if (gameinfo.quitSounds)
		{
			S_Sound (CHAN_VOICE, gameinfo.quitSounds[(gametic>>2)&7],
				1, ATTN_SURROUND);
			I_WaitVBL (105);
		}
	}
	exit (0);
}

void M_QuitDOOM (int choice)
{
  // We pick index 0 which is language sensitive,
  //  or one at random, between 1 and maximum number.
  if (language != english )
	sprintf(endstring,"%s\n\n%s", endmsg[0], DOSY );
  else
	sprintf(endstring,"%s\n\n%s", endmsg[ (gametic%(NUM_QUITMESSAGES-2))+1 ], DOSY);
  
  M_StartMessage(endstring,M_QuitResponse,true);
}




// -----------------------------------------------------
//		Player Setup Menu code
// -----------------------------------------------------

void M_DrawSlider (int x, int y, float min, float max, float cur);

static char *genders[3] = { "male", "female", "cyborg" };
static state_t *PlayerState;
static int PlayerTics;

EXTERN_CVAR (name)
EXTERN_CVAR (team)

void M_PlayerSetup (int choice)
{
	choice = 0;
	strcpy (savegamestrings[0], name.cstring());
//	strcpy (savegamestrings[1], team.cstring());	if (t = 1) // [Toke - Teams]
	M_DemoNoPlay = true;
	if (demoplayback)
		G_CheckDemoStatus ();
	M_SetupNextMenu (&PSetupDef);
	PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	PlayerTics = PlayerState->tics;
	if (FireScreen == NULL)
		FireScreen = new DCanvas (72, 72+5, 8);
}

static void M_PlayerSetupTicker (void)
{
	// Based on code in f_finale.c
	if (--PlayerTics > 0)
		return;

	if (PlayerState->tics == -1 || PlayerState->nextstate == S_NULL) {
		PlayerState = &states[mobjinfo[MT_PLAYER].seestate];
	} else {
		PlayerState = &states[PlayerState->nextstate];
	}
	PlayerTics = PlayerState->tics;
}

static void M_PlayerSetupDrawer (void)
{
	// Draw title
	{
		patch_t *patch = (patch_t *)W_CacheLumpName ("M_PSTTL", PU_CACHE);

		screen->DrawPatchClean (patch,
			160 - (patch->width() >> 1),
			PSetupDef.y - (patch->height() * 3));
	}

	// Draw player name box
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y, "Name");
	M_DrawSaveLoadBorder (PSetupDef.x + 56, PSetupDef.y, MAXPLAYERNAME+1);
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x + 56, PSetupDef.y, savegamestrings[0]);

	// Draw player team box
//	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Team");	if (t = 1) // [Toke - Teams]
//	screen->DrawTextCleanMove (CR_RED, PSetupDef.x + 56, PSetupDef.y + LINEHEIGHT, savegamestrings[1]);


	// Draw cursor for either of the above
	if (genStringEnter)
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x + V_StringWidth(savegamestrings[saveSlot]) + 56, PSetupDef.y + ((saveSlot == 0) ? 0 : LINEHEIGHT), "_");

	// Draw player character
	{
		int x = 320 - 88 - 32, y = PSetupDef.y + LINEHEIGHT*3 - 14;

		x = (x-160)*CleanXfac+(screen->width>>1);
		y = (y-100)*CleanYfac+(screen->height>>1);
		if (!FireScreen)
		{
			screen->Clear (x, y, x + 72 * CleanXfac, y + 72 * CleanYfac, 34);
		}
		else
		{
			// [RH] The following fire code is based on the PTC fire demo
			int a, b;
			byte *from;
			int width, height, pitch;

			FireScreen->Lock ();

			width = FireScreen->width;
			height = FireScreen->height;
			pitch = FireScreen->pitch;

			from = FireScreen->buffer + (height - 3) * pitch;
			for (a = 0; a < width; a++, from++)
			{
				*from = *(from + (pitch << 1)) = M_Random();
			}

			from = FireScreen->buffer;
			for (b = 0; b < FireScreen->height - 4; b += 2)
			{
				byte *pixel = from;

				// special case: first pixel on line
				byte *p = pixel + (pitch << 1);
				unsigned int top = *p + *(p + width - 1) + *(p + 1);
				unsigned int bottom = *(pixel + (pitch << 2));
				unsigned int c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;
				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;
				pixel++;

				// main line loop
				for (a = 1; a < width-1; a++)
				{
					// sum top pixels
					p = pixel + (pitch << 1);
					top = *p + *(p - 1) + *(p + 1);

					// bottom pixel
					bottom = *(pixel + (pitch << 2));

					// combine pixels
					c1 = (top + bottom) >> 2;
					if (c1 > 1) c1--;

					// store pixels
					*pixel = c1;
					*(pixel + pitch) = (c1 + bottom) >> 1;		// interpolate

					// next pixel
					pixel++;
				}

				// special case: last pixel on line
				p = pixel + (pitch << 1);
				top = *p + *(p - 1) + *(p - width + 1);
				bottom = *(pixel + (pitch << 2));
				c1 = (top + bottom) >> 2;
				if (c1 > 1) c1--;
				*pixel = c1;
				*(pixel + pitch) = (c1 + bottom) >> 1;

				// next line
				from += pitch << 1;
			}

			y--;
			pitch = screen->pitch;
			switch (CleanXfac)
			{
			case 1:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to++, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
							*(to + pitch*c) = FireRemap[*from];
					}
				}
				break;

			case 2:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to += 2, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
						{
							*(to + pitch*c) = FireRemap[*from];
							*(to + pitch*c + 1) = FireRemap[*from];
						}
					}
				}
				break;

			case 3:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to += 3, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
						{
							*(to + pitch*c) = FireRemap[*from];
							*(to + pitch*c + 1) = FireRemap[*from];
							*(to + pitch*c + 2) = FireRemap[*from];
						}
					}
				}
				break;

			case 4:
			default:
				for (b = 0; b < FireScreen->height; b++)
				{
					byte *to = screen->buffer + y * screen->pitch + x;
					from = FireScreen->buffer + b * FireScreen->pitch;
					y += CleanYfac;

					for (a = 0; a < FireScreen->width; a++, to += 4, from++)
					{
						int c;
						for (c = CleanYfac; c; c--)
						{
							*(to + pitch*c) = FireRemap[*from];
							*(to + pitch*c + 1) = FireRemap[*from];
							*(to + pitch*c + 2) = FireRemap[*from];
							*(to + pitch*c + 3) = FireRemap[*from];
						}
					}
				}
				break;
			}
			FireScreen->Unlock ();
		}
	}
	{
		spriteframe_t *sprframe =
			&sprites[skins[consoleplayer().userinfo.skin].sprite].spriteframes[PlayerState->frame & FF_FRAMEMASK];

		V_ColorMap = translationtables + consoleplayer().id * 256;
		screen->DrawTranslatedPatchClean ((patch_t *)W_CacheLumpNum (sprframe->lump[0], PU_CACHE),
			320 - 52 - 32, PSetupDef.y + LINEHEIGHT*3 + 46);
	}
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName ("M_PBOX", PU_CACHE),
		320 - 88 - 32 + 36, PSetupDef.y + LINEHEIGHT*3 + 22);

	// Draw player color sliders
	//V_DrawTextCleanMove (CR_GREY, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Color");

	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*2, "Red");
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*3, "Green");
	screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*4, "Blue");

	{
		int x = V_StringWidth ("Green") + 8 + PSetupDef.x;
		int color = consoleplayer().userinfo.color;

		M_DrawSlider (x, PSetupDef.y + LINEHEIGHT*2, 0.0f, 255.0f, RPART(color));
		M_DrawSlider (x, PSetupDef.y + LINEHEIGHT*3, 0.0f, 255.0f, GPART(color));
		M_DrawSlider (x, PSetupDef.y + LINEHEIGHT*4, 0.0f, 255.0f, BPART(color));
	}

	// Draw team setting
	{
		int x = V_StringWidth ("Prefered Team") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT, "Prefered Team");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT, playersteam == TEAM_NONE ? "NONE" : team_names[playersteam]);
	}

	// Draw gender setting
	{
		int x = V_StringWidth ("Gender") + 8 + PSetupDef.x;
		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*5, "Gender");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*5, genders[consoleplayer().userinfo.gender]);
	}

	// Draw skin setting
	{
		if (!ctfmode && !teamplaymode) // [Toke - CTF] Dont allow skin selection if in CTF or Teamplay mode
		{
			int x = V_StringWidth ("Skin") + 8 + PSetupDef.x;
			screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*6, "Skin");
			screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*6, skins[consoleplayer().userinfo.skin].name);
		}
	}

	// Draw autoaim setting
	{
		int x = V_StringWidth ("Autoaim") + 8 + PSetupDef.x;
		float aim = autoaim;

		screen->DrawTextCleanMove (CR_RED, PSetupDef.x, PSetupDef.y + LINEHEIGHT*7, "Autoaim");
		screen->DrawTextCleanMove (CR_GREY, x, PSetupDef.y + LINEHEIGHT*7,
			aim == 0 ? "Never" :
			aim <= 0.25 ? "Very Low" :
			aim <= 0.5 ? "Low" :
			aim <= 1 ? "Medium" :
			aim <= 2 ? "High" :
			aim <= 3 ? "Very High" : "Always");
	}
}

void M_ChangeTeam (int choice) // [Toke - Teams]
{
	team_t t = playersteam;

	if (t == TEAM_NONE)
		if (choice)
			t = TEAM_BLUE;
		else
			t = TEAM_GOLD;


	else if (t == TEAM_BLUE)
		if (choice)
			t = TEAM_RED;
		else
			t = TEAM_NONE;


	else if (t == TEAM_RED)
		if (choice)
			t = TEAM_GOLD;
		else
			t = TEAM_BLUE;


	else if (t == TEAM_GOLD)
		if (choice)
			t = TEAM_NONE;
		else
			t = TEAM_RED;

	if (ctfmode || teamplaymode)
	{
		char command[16] = "";
		
		if (t == TEAM_BLUE)
			sprintf (command, "skin BlueTeam");

		else if (t == TEAM_RED)
			sprintf (command, "skin RedTeam");

		else if (t == TEAM_GOLD)
			sprintf (command, "skin none");

		else 
			sprintf (command, "skin none");

		AddCommandString (command);
	}

	playersteam = t;
}

static void M_ChangeGender (int choice)
{
	int gender = consoleplayer().userinfo.gender;

	if (!choice)
		gender = (gender == 0) ? 2 : gender - 1;
	else
		gender = (gender == 2) ? 0 : gender + 1;

	cvar_t::cvar_set ("gender", genders[gender]);
}

static void M_ChangeSkin (int choice) // [Toke - Skins]
{
	unsigned skin = consoleplayer().userinfo.skin;

	if (!choice)
		skin = (skin == 0) ? numskins - 1 : skin - 1;
	else
		skin = (skin < numskins - 1) ? skin + 1 : 0;

	// denis - doing the following on the clientside is pointless and confusing

	if (!ctfmode && !teamplaymode) 
	{
		// prevent players from selecting team skins
		// toke - todo - do this serverside
		while (!stricmp(skins[skin].name, "blueteam") || 
                !stricmp (skins[skin].name, "redteam") || 
                !stricmp (skins[skin].name, "goldteam"))
		{
			skin++;

			if (skin == (numskins - 1))
				skin = 0;
		}
	}

	cvar_t::cvar_set ("skin", skins[skin].name);

	CL_RememberSkin(); 
}

static void M_ChangeAutoAim (int choice)
{
	static const float ranges[] = { 0, 0.25, 0.5, 1, 2, 3, 5000 };
	float aim = autoaim;
	int i;

	if (!choice) {
		// Select a lower autoaim

		for (i = 6; i >= 1; i--) {
			if (aim >= ranges[i]) {
				aim = ranges[i - 1];
				break;
			}
		}
	} else {
		// Select a higher autoaim

		for (i = 5; i >= 0; i--) {
			if (aim >= ranges[i]) {
				aim = ranges[i + 1];
				break;
			}
		}
	}

	autoaim.Set (aim);
}

static void M_EditPlayerName (int choice)
{
	// we are going to be intercepting all chars
	genStringEnter = 1;
	genStringEnd = M_PlayerNameChanged;
	genStringLen = MAXPLAYERNAME;
	
	saveSlot = 0;
	strcpy(saveOldString,savegamestrings[0]);
	if (!strcmp(savegamestrings[0],EMPTYSTRING))
		savegamestrings[0][0] = 0;
	saveCharIndex = strlen(savegamestrings[0]);
}

static void M_PlayerNameChanged (int choice)
{
	char command[SAVESTRINGSIZE+8];

	sprintf (command, "name \"%s\"", savegamestrings[0]);
	AddCommandString (command);
}

static void M_PlayerTeamChanged (int choice)
{
	char command[SAVESTRINGSIZE+8];

	sprintf (command, "team \"%s\"", savegamestrings[1]);
	AddCommandString (command);
}


static void SendNewColor (int red, int green, int blue)
{
	char command[24];

	sprintf (command, "color \"%02x %02x %02x\"", red, green, blue);
	AddCommandString (command);
}

static void M_SlidePlayerRed (int choice)
{
	int color = consoleplayer().userinfo.color;
	int red = RPART(color);

	if (choice == 0) {
		red -= 16;
		if (red < 0)
			red = 0;
	} else {
		red += 16;
		if (red > 255)
			red = 255;
	}

	SendNewColor (red, GPART(color), BPART(color));
}

static void M_SlidePlayerGreen (int choice)
{
	int color = consoleplayer().userinfo.color;
	int green = GPART(color);

	if (choice == 0) {
		green -= 16;
		if (green < 0)
			green = 0;
	} else {
		green += 16;
		if (green > 255)
			green = 255;
	}

	SendNewColor (RPART(color), green, BPART(color));
}

static void M_SlidePlayerBlue (int choice)
{
	int color = consoleplayer().userinfo.color;
	int blue = BPART(color);

	if (choice == 0) {
		blue -= 16;
		if (blue < 0)
			blue = 0;
	} else {
		blue += 16;
		if (blue > 255)
			blue = 255;
	}

	SendNewColor (RPART(color), GPART(color), blue);
}


//
//		Menu Functions
//
void M_DrawEmptyCell (oldmenu_t *menu, int item)
{
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_CELL1",PU_CACHE),
		menu->x - 10, menu->y+item*LINEHEIGHT - 1);
}

void M_DrawSelCell (oldmenu_t *menu, int item)
{
	screen->DrawPatchClean ((patch_t *)W_CacheLumpName("M_CELL2",PU_CACHE),
		menu->x - 10, menu->y+item*LINEHEIGHT - 1);
}


void M_StartMessage (char *string, void (*routine)(int), bool input)
{
	C_HideConsole ();
	messageLastMenuActive = menuactive;
	messageToPrint = 1;
	messageString = string;
	messageRoutine = routine;
	messageNeedsInput = input;
	menuactive = true;
	return;
}



void M_StopMessage (void)
{
	menuactive = messageLastMenuActive;
	messageToPrint = 0;
}


//
//		Find string height from hu_font chars
//
int M_StringHeight(char* string)
{
	int h;
	int height = hu_font[0]->height();
		
	h = height;
	while (*string)
		if ((*string++) == '\n')
			h += height;
				
	return h;
}



//
// CONTROL PANEL
//

//
// M_Responder
//
bool M_Responder (event_t* ev)
{
	int ch, ch2;
	int i;

	ch = ch2 = -1;

	// eat mouse events
	if(menuactive)
		if(ev->type == ev_mouse)
			return true;

	if (ev->type == ev_keydown)
	{
		ch = ev->data1; 		// scancode
		ch2 = ev->data2;		// ASCII
	}
	
	if (ch == -1 || headsupactive)
		return false;

	if (menuactive && OptionsActive) {
		M_OptResponder (ev);
		return true;
	}
	
	// Save Game string input
	// [RH] and Player Name string input
	if (genStringEnter)
	{
		switch(ch)
		{
		  case KEY_BACKSPACE:
			if (saveCharIndex > 0)
			{
				saveCharIndex--;
				savegamestrings[saveSlot][saveCharIndex] = 0;
			}
			break;

		  case KEY_ESCAPE:
			genStringEnter = 0;
			M_ClearMenus ();
			strcpy(&savegamestrings[saveSlot][0],saveOldString);
			break;
								
		  case KEY_ENTER:
			genStringEnter = 0;
			M_ClearMenus ();
			if (savegamestrings[saveSlot][0])
				genStringEnd(saveSlot);	// [RH] Function to call when enter is pressed
			break;
								
		  default:
			ch = toupper(ev->data3);	// [RH] Use user keymap
			if (ch != 32)
				if (ch-HU_FONTSTART < 0 || ch-HU_FONTSTART >= HU_FONTSIZE)
					break;
			if (ch >= 32 && ch <= 127 &&
				saveCharIndex < genStringLen &&
				V_StringWidth(savegamestrings[saveSlot]) <
				(genStringLen-1)*8)
			{
				savegamestrings[saveSlot][saveCharIndex++] = ch;
				savegamestrings[saveSlot][saveCharIndex] = 0;
			}
			break;
		}
		return true;
	}
	
	// Take care of any messages that need input
	if (messageToPrint)
	{
		if (messageNeedsInput &&
			!(ch2 == ' ' || toupper(ch2) == 'N' || toupper(ch2) == 'Y' || ch == KEY_ESCAPE))
			return true;
				
		menuactive = messageLastMenuActive;
		messageToPrint = 0;
		if (messageRoutine)
			messageRoutine(ch2);
						
		menuactive = false;
		S_Sound (CHAN_VOICE, "switches/exitbutn", 1, ATTN_NONE);
		return true;
	}
		
	// [RH] F-Keys are now just normal keys that can be bound,
	//		so they aren't checked here anymore.
	
	// If devparm is set, pressing F1 always takes a screenshot no matter
	// what it's bound to. (for those who don't bother to read the docs)
	if (devparm && ch == KEY_F1) {
		G_ScreenShot (NULL);
		return true;
	}

	// Pop-up menu?
	if (!menuactive)
	{
		if (ch == KEY_ESCAPE)
		{
			M_StartControlPanel ();
			M_SetupNextMenu (&MainDef);
			S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
			return true;
		}
		return false;
	}

	
	// Keys usable within menu
	switch (ch)
	{
	  case KEY_DOWNARROW:
		do
		{
			if (itemOn+1 > currentMenu->numitems-1)
				itemOn = 0;
			else 
			{	
				// [Toke - CTF]  Skip the skins item in CTF or Teamplay mode
				if ((ctfmode || teamplaymode) && currentMenu == &PSetupDef && itemOn == 5)
					itemOn = itemOn + 2;	
				else	itemOn++;
			}
			S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
		} while(currentMenu->menuitems[itemOn].status==-1);
		return true;
				
	  case KEY_UPARROW:
		do
		{
			if (!itemOn)
				itemOn = currentMenu->numitems-1;
			else 
			{	 
				// [Toke - CTF]  Skip the skins item in CTF or Teamplay mode
				if ((ctfmode || teamplaymode) && currentMenu == &PSetupDef && itemOn == 7)
					itemOn = itemOn - 2;	
				else itemOn--;
			}
			S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
		} while(currentMenu->menuitems[itemOn].status==-1);
		return true;

	  case KEY_LEFTARROW:
		if (currentMenu->menuitems[itemOn].routine &&
			currentMenu->menuitems[itemOn].status == 2)
		{
			S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			currentMenu->menuitems[itemOn].routine(0);
		}
		return true;
				
	  case KEY_RIGHTARROW:
		if (currentMenu->menuitems[itemOn].routine &&
			currentMenu->menuitems[itemOn].status == 2)
		{
			S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			currentMenu->menuitems[itemOn].routine(1);
		}
		return true;

	  case KEY_ENTER:
		if (currentMenu->menuitems[itemOn].routine &&
			currentMenu->menuitems[itemOn].status)
		{
			currentMenu->lastOn = itemOn;
			if (currentMenu->menuitems[itemOn].status == 2)
			{
				currentMenu->menuitems[itemOn].routine(1);		// right arrow
				S_Sound (CHAN_VOICE, "plats/pt1_mid", 1, ATTN_NONE);
			}
			else
			{
				currentMenu->menuitems[itemOn].routine(itemOn);
				S_Sound (CHAN_VOICE, "weapons/pistol", 1, ATTN_NONE);
			}
		}
		return true;
				
	  // [RH] Escape now moves back one menu instead of
	  //	  quitting the menu system. Thus, backspace
	  //	  is now ignored.
	  case KEY_ESCAPE:
		currentMenu->lastOn = itemOn;
		M_PopMenuStack ();
		return true;
		
	  default:
		if (ch2) {
			for (i = itemOn+1;i < currentMenu->numitems;i++)
				if (currentMenu->menuitems[i].alphaKey == toupper(ch2))
				{
					itemOn = i;
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
					return true;
				}
			for (i = 0;i <= itemOn;i++)
				if (currentMenu->menuitems[i].alphaKey == toupper(ch2))
				{
					itemOn = i;
					S_Sound (CHAN_VOICE, "plats/pt1_stop", 1, ATTN_NONE);
					return true;
				}
		}
		break;
		
	}

	// [RH] Menu now eats all keydown events while active
	if (ev->type == ev_keydown)
		return true;
	else
		return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
	playersteam = consoleplayer().userinfo.team;

	// intro might call this repeatedly
	if (menuactive)
		return;
	
	drawSkull = true;
	MenuStackDepth = 0;
	menuactive = 1;
	currentMenu = &MainDef;
	itemOn = currentMenu->lastOn;
	C_HideConsole ();				// [RH] Make sure console goes bye bye.
	OptionsActive = false;			// [RH] Make sure none of the options menus appear.
	I_PauseMouse ();				// [RH] Give the mouse back in windowed modes.
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
	int i, x, y, max;

	// Horiz. & Vertically center string and print it.
	if (messageToPrint)
	{
		screen->Dim ();
		st_firsttime = true;

		brokenlines_t *lines = V_BreakLines (320, messageString);
		y = 100;

		for (i = 0; lines[i].width != -1; i++)
			y -= hu_font[0]->height() / 2;

		for (i = 0; lines[i].width != -1; i++)
		{
			screen->DrawTextCleanMove (CR_RED, 160 - lines[i].width/2, y, lines[i].string);
			y += hu_font[0]->height();
		}

		V_FreeBrokenLines (lines);
	}
	else if (menuactive)
	{
		screen->Dim ();
		st_firsttime = true;

		if (OptionsActive)
		{
			M_OptDrawer ();
		}
		else
		{
			if (currentMenu->routine)
				currentMenu->routine(); 		// call Draw routine
		
			// DRAW MENU
			x = currentMenu->x;
			y = currentMenu->y;
			max = currentMenu->numitems;

			for (i = 0; i < max; i++)
			{
				if (currentMenu->menuitems[i].name[0])
					screen->DrawPatchClean ((patch_t *)W_CacheLumpName(currentMenu->menuitems[i].name ,PU_CACHE), x, y);
				y += LINEHEIGHT;
			}

			
			// DRAW SKULL
			if (drawSkull)
			{
				screen->DrawPatchClean ((patch_t *)W_CacheLumpName(skullName[whichSkull],PU_CACHE),
					x + SKULLXOFF, currentMenu->y - 5 + itemOn*LINEHEIGHT);
			}
		}
	}
}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
	if (FireScreen)
	{
		delete FireScreen;
		FireScreen = NULL;
	}
	MenuStackDepth = 0;
	menuactive = false;
	drawSkull = true;
	M_DemoNoPlay = false;
	C_HideConsole ();		// [RH] Hide the console if we can.
	if (gamestate != GS_FULLCONSOLE)
		I_ResumeMouse ();	// [RH] Recapture the mouse in windowed modes.

	consoleplayer().userinfo.team = playersteam; // [Toke - teams]

        // joek - update skies (otherwise won't update if stretched or not.)
        R_InitSkyMap ();
        
	if(playersteam < NUMTEAMS)
		cvar_t::cvar_set ("team", team_names[playersteam]);
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu (oldmenu_t *menudef)
{
	MenuStack[MenuStackDepth].menu.old = menudef;
	MenuStack[MenuStackDepth].isNewStyle = false;
	MenuStack[MenuStackDepth].drawSkull = drawSkull;
	MenuStackDepth++;

	currentMenu = menudef;
	itemOn = currentMenu->lastOn;
}


void M_PopMenuStack (void)
{
	M_DemoNoPlay = false;
	if (MenuStackDepth > 1) {
		I_PauseMouse ();
		MenuStackDepth -= 2;
		if (MenuStack[MenuStackDepth].isNewStyle) {
			OptionsActive = true;
			CurrentMenu = MenuStack[MenuStackDepth].menu.newmenu;
			CurrentItem = CurrentMenu->lastOn;
		} else {
			OptionsActive = false;
			currentMenu = MenuStack[MenuStackDepth].menu.old;
			itemOn = currentMenu->lastOn;
		}
		drawSkull = MenuStack[MenuStackDepth].drawSkull;
		MenuStackDepth++;
		S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
	} else {
		M_ClearMenus ();
		S_Sound (CHAN_VOICE, "switches/exitbutn", 1, ATTN_NONE);
	}
}


//
// M_Ticker
//
void M_Ticker (void)
{
	if (--skullAnimCounter <= 0)
	{
		whichSkull ^= 1;
		skullAnimCounter = 8;
	}
	if (currentMenu == &PSetupDef)
	{
		// [Toke - CTF] skip skins selection
		if (ctfmode || teamplaymode)
			if (itemOn == 6)
				itemOn = 5;	

		M_PlayerSetupTicker ();
	}
}


//
// M_Init
//
EXTERN_CVAR (screenblocks)

void M_Init (void)
{
	int i;

	currentMenu = &MainDef;
	OptionsActive = false;
	menuactive = 0;
	itemOn = currentMenu->lastOn;
	whichSkull = 0;
	skullAnimCounter = 10;
	drawSkull = true;
	screenSize = (int)screenblocks - 3;
	messageToPrint = 0;
	messageString = NULL;
	messageLastMenuActive = menuactive;

	M_OptInit ();

	// [RH] Build a palette translation table for the fire
	for (i = 0; i < 255; i++)
		FireRemap[i] = BestColor (DefaultPalette->basecolors, i, 0, 0, DefaultPalette->numcolors);
}
