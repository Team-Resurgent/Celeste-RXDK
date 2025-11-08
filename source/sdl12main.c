#define  _NO_CRT_STDIO_INLINE

#include <SDL.h>
#include <SDL_mixer.h>

#if SDL_MAJOR_VERSION >= 2
#include "sdl20compat.inc.c"
#endif
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#ifdef _3DS
#include <3ds.h>
#endif
#include "celeste.h"

static void ErrLog(char* fmt, ...) {

#ifdef _3DS
	/*FILE* f = fopen("sdmc:/ccleste.txt", "a");
	if (!f) return;
	fprintf(f, "%li \t", (long int)time(NULL));*/
	FILE* f = stdout; //bottom screen console
#else
	//FILE* f = stderr;
#endif
	/*
	va_list ap;
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);

	if (f != stderr && f != stdout) fclose(f);
	*/
}


SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;

SDL_Surface* screen = NULL;
SDL_Rect screen_dstRect;
//SDL_Surface* screenReal = NULL;
SDL_Surface* gfx = NULL;
SDL_Surface* font = NULL;
Mix_Chunk* snd[64] = { NULL };
Mix_Music* mus[6] = { NULL };

#define PICO8_W 128
#define PICO8_H 128

#ifdef _3DS
static const int scale = 2;
#else
//HCF
static int scale = 1;  // 3;  // 2;  //4;
#endif

static const SDL_Color base_palette[16] = {
	{0x00, 0x00, 0x00},
	{0x1d, 0x2b, 0x53},
	{0x7e, 0x25, 0x53},
	{0x00, 0x87, 0x51},
	{0xab, 0x52, 0x36},
	{0x5f, 0x57, 0x4f},
	{0xc2, 0xc3, 0xc7},
	{0xff, 0xf1, 0xe8},
	{0xff, 0x00, 0x4d},
	{0xff, 0xa3, 0x00},
	{0xff, 0xec, 0x27},
	{0x00, 0xe4, 0x36},
	{0x29, 0xad, 0xff},
	{0x83, 0x76, 0x9c},
	{0xff, 0x77, 0xa8},
	{0xff, 0xcc, 0xaa}
};
static SDL_Color palette[16];

static inline Uint32 getcolor(char idx) {
	SDL_Color c = palette[idx % 16];
	return SDL_MapRGB(screen->format, c.r, c.g, c.b);
}

static void ResetPalette(void) {
	//SDL_SetPalette(surf, SDL_PHYSPAL|SDL_LOGPAL, (SDL_Color*)base_palette, 0, 16);
	//memcpy(screen->format->palette->colors, base_palette, 16*sizeof(SDL_Color));
	memcpy(palette, base_palette, sizeof palette);
}

static char* GetDataPath(char* path, int n, const char* fname) {
#ifdef _3DS
	snprintf(path, n, "romfs:/%s", fname);
#else

#ifdef _WIN32 || defined(__XBOX__)
	char pathsep = '\\';
#else
	char pathsep = '/';
#endif //_WIN32

#ifdef __XBOX__
	sprintf(path, "D:\\Media\\data%c%s", pathsep, fname);
#else
	snprintf(path, n, "data%c%s", pathsep, fname);
#endif // __XBOX__

#endif //_3DS
	return path;
}

static Uint32 getpixel(SDL_Surface* surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	/* Here p is the address to the pixel we want to retrieve */
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp) {
	case 1:
		return *p;

	case 2:
		return *(Uint16*)p;

	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		return *(Uint32*)p;
	}
	return 0;
}

static void loadbmpscale(char* filename, SDL_Surface** s) {
	SDL_Surface* surf = *s;
	if (surf) SDL_FreeSurface(surf), surf = *s = NULL;

	char tmpath[4096];
	SDL_Surface* bmp = SDL_LoadBMP(GetDataPath(tmpath, sizeof tmpath, filename));
	if (!bmp) {
		ErrLog("error loading bmp '%s': %s\n", filename, SDL_GetError());
		return;
	}

	int w = bmp->w, h = bmp->h;

	surf = SDL_CreateRGBSurface(SDL_SWSURFACE, w * scale, h * scale, 8, 0, 0, 0, 0);
	assert(surf != NULL);
	unsigned char* data = surf->pixels;
	/*memcpy((_S)->format->palette->colors, base_palette, 16*sizeof(SDL_Color));*/
	for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
		unsigned char pix = getpixel(bmp, x, y);
		for (int i = 0; i < scale; i++) for (int j = 0; j < scale; j++) {
			data[x * scale + i + (y * scale + j) * w * scale] = pix;
		}
	}
	SDL_FreeSurface(bmp);
	SDL_SetPalette(surf, SDL_PHYSPAL | SDL_LOGPAL, (SDL_Color*)base_palette, 0, 16);
	SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0);

	*s = surf;
}

#define LOGLOAD(w) SDL_Log("loading %s...", w)
#define LOGDONE() SDL_Log("done\n")

static void LoadData(void) {
	LOGLOAD("gfx.bmp");
	loadbmpscale("gfx.bmp", &gfx);
	LOGDONE();

	LOGLOAD("font.bmp");
	loadbmpscale("font.bmp", &font);
	LOGDONE();

	static const char sndids[] = { 0,1,2,3,4,5,6,7,8,9,13,14,15,16,23,35,37,38,40,50,51,54,55 };
	for (int iid = 0; iid < sizeof sndids; iid++) {
		int id = sndids[iid];
		char fname[20];
		sprintf(fname, "snd%i.wav", id);
		char path[4096];
		LOGLOAD(fname);
		GetDataPath(path, sizeof path, fname);
		snd[id] = Mix_LoadWAV(path);
		if (!snd[id]) {
			ErrLog("snd%i: Mix_LoadWAV: %s\n", id, Mix_GetError());
		}
		LOGDONE();
	}
	static const char musids[] = { 0,10,20,30,40 };
	for (int iid = 0; iid < sizeof musids; iid++) {
		int id = musids[iid];
		char fname[20];

		sprintf(fname, "mus%i.ogg", id);

		LOGLOAD(fname);
		char path[4096];
		GetDataPath(path, sizeof path, fname);
		mus[id / 10] = Mix_LoadMUS(path);

		if (!mus[id / 10]) {
			ErrLog("mus%i: Mix_LoadMUS: %s\n", id, Mix_GetError());
			return -1;
		}
		LOGDONE();
	}
}
#include "tilemap.h"

static Uint16 buttons_state = 0;

#define SDL_CHECK(r) do {                               \
	if (!(r)) {                                           \
		ErrLog("%s:%i, fatal error: `%s` -> %s\n", \
		        __FILE__, __LINE__, #r, SDL_GetError());    \
		exit(2);                                            \
	}                                                     \
} while(0)

static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static void p8_print(const char* str, int x, int y, int col);

//on-screen display (for info, such as loading a state, toggling screenshake, toggling fullscreen, etc)
static char osd_text[200] = "";
static int osd_timer = 0;
static void OSDset(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(osd_text, sizeof osd_text, fmt, ap);
	osd_text[sizeof osd_text - 1] = '\0'; //make sure to add NUL terminator in case of truncation
	//printf("%s\n", osd_text);
	osd_timer = 30;
	va_end(ap);
}
static void OSDdraw(void) {
	if (osd_timer > 0) {
		--osd_timer;
		//HCF
		//const int x = 64 + 4;
		const int x = 4;
		const int y = 120 + (osd_timer < 10 ? 10 - osd_timer : 0); //disappear by going below the screen
		p8_rectfill(x - 2, y - 2, x + 4 * strlen(osd_text), y + 6, 6); //outline
		p8_rectfill(x - 1, y - 1, x + 4 * strlen(osd_text) - 1, y + 5, 0);
		p8_print(osd_text, x, y, 7);
	}
}

static void DrawPausedBanner(void) {
	const char* t = "paused";
	const int tw = 4 * (int)strlen(t);     // p8_print is 4px per char
	const int x0 = (PICO8_W - tw) / 2;
	const int y0 = 8;

	p8_rectfill(x0 - 1, y0 - 1, x0 + tw + 1, y0 + 6 + 1, 6);
	p8_rectfill(x0, y0, x0 + tw, y0 + 6, 0);
	p8_print(t, x0 + 1, y0 + 1, 7);
}

static Mix_Music* current_music = NULL;
static _Bool enable_screenshake = 1;
static _Bool paused = 0;
static _Bool running = 1;
static void* initial_game_state = NULL;
static void* game_state = NULL;
static Mix_Music* game_state_music = NULL;
static void mainLoop(void);
static FILE* TAS = NULL;

SDL_Joystick* joystick = NULL;

// TODO: Clean up
int SCREEN_WIDTH = 1920;
int SCREEN_HEIGHT = 1080;

// === scaling & pause menu state ===
enum { SCALE_INT = 0, SCALE_ASPECT = 1, SCALE_STRETCH = 2 };
static int screen_scaling_mode = SCALE_INT;   // runtime-selected scaling

static _Bool pause_menu_active = 0;
static int pause_menu_index = 0;              // 0: Scaling row, 1: Apply, 2: Resume
static int pause_scaling_choice = 0;          // temp choice while paused
static const char* scaling_names[] = { "integer", "aspect", "stretched" };

SDL_Rect get_integer_scale_rectangle() {
	// Calculate the maximum integer scale that fits within the target resolution
	int scale_x = SCREEN_WIDTH / (PICO8_W * scale);
	int scale_y = SCREEN_HEIGHT / (PICO8_H * scale);
	int xscale = min(scale_x, scale_y);

	// Calculate the scaled dimensions
	int scaled_width = (PICO8_W * scale) * xscale;
	int scaled_height = (PICO8_H * scale) * xscale;

	// Calculate padding to center the image
	int pad_x = (SCREEN_WIDTH - scaled_width) / 2;
	int pad_y = (SCREEN_HEIGHT - scaled_height) / 2;

	SDL_Rect dstRect = { pad_x, pad_y, scaled_width, scaled_height };
	return dstRect;
}

SDL_Rect get_streteched_aspect_rectangle() {
	// uniform scale that fits inside the window
	float sx = (float)SCREEN_WIDTH / (PICO8_W * scale);
	float sy = (float)SCREEN_HEIGHT / (PICO8_H * scale);
	float s = (sx < sy) ? sx : sy;

	int w = (int)((PICO8_W * scale) * s + 0.5f);
	int h = (int)((PICO8_H * scale) * s + 0.5f);

	int x = (SCREEN_WIDTH - w) / 2;
	int y = (SCREEN_HEIGHT - h) / 2;

	SDL_Rect dstRect = { x, y, w, h };
	return dstRect;
}


SDL_Rect get_streteched_rectangle() {
	SDL_Rect dstRect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	return dstRect;
}

static void RecomputeScreenDstRect(void) {
	switch (screen_scaling_mode) {
	case SCALE_INT:     screen_dstRect = get_integer_scale_rectangle(); break;
	case SCALE_ASPECT:  screen_dstRect = get_streteched_aspect_rectangle(); break;
	case SCALE_STRETCH: screen_dstRect = get_streteched_rectangle(); break;
	default:            screen_dstRect = get_integer_scale_rectangle(); break;
	}
}

int mainy() {   //int argc, char** argv) {
	SDL_CHECK(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == 0);
#if SDL_MAJOR_VERSION >= 2
	//HCF
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	/*
	SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	SDL_GameControllerAddMappingsFromRW(SDL_RWFromFile("D:\\gamecontrollerdb.txt", "rb"), 1);
	*/

	//HCF
	do
	{
		joystick = SDL_JoystickOpen(0);
		if (joystick == 0)
		{
			SDL_Delay(500);
		}
	} while (joystick == 0);
#endif

	int videoflag = SDL_SWSURFACE | SDL_HWPALETTE;
#
	//HCF
	//SDL_CHECK(screenReal = SDL_SetVideoMode(640, 480, 32, videoflag));
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // "nearest" / "0" / "1" / "2"

	// 1) Create window
	window = SDL_CreateWindow("Escalado SDL2",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCREEN_WIDTH, SCREEN_HEIGHT, 0);

	// 2) Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	// 3) Use renderer's real backbuffer size as the source of truth
	int rw = 0, rh = 0;
	if (renderer && SDL_GetRendererOutputSize(renderer, &rw, &rh) == 0) {
		SCREEN_WIDTH = rw;
		SCREEN_HEIGHT = rh;
	}
	else {
		SDL_GetWindowSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
		SDL_Log("GetRendererOutputSize failed (%s); using window size", SDL_GetError());
	}

	// 4) Now it's safe to compute rectangles and create textures that depend on size
	RecomputeScreenDstRect();
	//OSDset("window=%dx%d, renderer=%dx%d", SCREEN_WIDTH, SCREEN_HEIGHT, rw, rh);

	pause_scaling_choice = screen_scaling_mode;  // seed pause menu

	screen = SDL_CreateRGBSurface(
		0,               // flags
		PICO8_W * scale, // weight
		PICO8_H * scale, // height
		32,              // bit-depth
		0x00FF0000,      // Red
		0x0000FF00,      // Green
		0x000000FF,      // Blue
		0xFF000000       // Alfa
	);

	int mixflag = MIX_INIT_OGG;
	if (Mix_Init(mixflag) != mixflag) {
		ErrLog("Mix_Init: %s\n", Mix_GetError());
		return -1;
	}

	if (Mix_OpenAudio(22050, AUDIO_S16SYS, 1, 1024) < 0) {
		ErrLog("Mix_Init: %s\n", Mix_GetError());
		return -1;
	}

	ResetPalette();
	SDL_ShowCursor(0);

	//printf("game state size %gkb\n", Celeste_P8_get_state_size()/1024.);
	//printf("now loading...\n");

	{
		const unsigned char loading_bmp[] = {
			0x42,0x4d,0xca,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x82,0x00,
			0x00,0x00,0x6c,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x09,0x00,
			0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x48,0x00,
			0x00,0x00,0x23,0x2e,0x00,0x00,0x23,0x2e,0x00,0x00,0x02,0x00,
			0x00,0x00,0x02,0x00,0x00,0x00,0x42,0x47,0x52,0x73,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
			0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
			0x00,0x00,0x66,0x3e,0xf1,0x24,0xf0,0x00,0x00,0x00,0x49,0x44,
			0x92,0x24,0x90,0x00,0x00,0x00,0x49,0x3c,0x92,0x24,0x90,0x00,
			0x00,0x00,0x49,0x04,0x92,0x24,0x90,0x00,0x00,0x00,0x46,0x38,
			0xf0,0x3c,0xf0,0x00,0x00,0x00,0x40,0x00,0x12,0x00,0x00,0x00,
			0x00,0x00,0xc0,0x00,0x10,0x00,0x00,0x00,0x00,0x00
		};
		SDL_RWops* rw = SDL_RWFromConstMem(loading_bmp, sizeof loading_bmp);
		SDL_Surface* loading = SDL_LoadBMP_RW(rw, 1);
		if (!loading) goto skip_load;

		//HCF
		//SDL_Rect rc = { 64+60, 60 };
		SDL_Rect rc = { 60, 60 };
		SDL_BlitSurface(loading, NULL, screen, &rc);

		//HCF
		rc.x = 64;
		rc.y = 0;
		/*
		rc.x = 128;
		rc.y = 48;
		*/

		/*
		SDL_Rect roc;
		roc.x = 0;
		roc.y = 16;
		roc.w = screen->w;
		roc.h = screen->h - 16;
		SDL_BlitSurface(screen, &roc, screenReal, &rc);
		*/

		//SDL_FillRect(screenReal, NULL, SDL_MapRGB(screenReal->format, 0, 0, 0));
		//SDL_BlitSurface(screen, NULL, screenReal, &rc);
		//SDL_Flip(screenReal);

		// Convertir a textura
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, screen);
		SDL_RenderClear(renderer);
		// Renderizar la textura escalada
		SDL_RenderCopy(renderer, texture, NULL, &screen_dstRect);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);

		SDL_FreeSurface(loading);
	} skip_load:

	LoadData();

	int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);
	Celeste_P8_set_call_func(pico8emu);

	//for reset
	initial_game_state = SDL_malloc(Celeste_P8_get_state_size());
	if (initial_game_state) Celeste_P8_save_state(initial_game_state);

	if (TAS) {
		// a consistent seed for tas playback
		Celeste_P8_set_rndseed(8);
	}
	else {
		Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
	}

	Celeste_P8_init();

	//printf("ready\n");
	{
		//FILE* start_fullscreen_f = fopen("ccleste-start-fullscreen.txt", "r");
		//const char* start_fullscreen_v = getenv("CCLESTE_START_FULLSCREEN");
		//if (start_fullscreen_f || (start_fullscreen_v && *start_fullscreen_v)) {
			//SDL_WM_ToggleFullScreen(screenReal);
		//}
		//if (start_fullscreen_f) fclose(start_fullscreen_f);
	}

#ifdef _3DS
	while (aptMainLoop()) mainLoop();
#elif !defined(EMSCRIPTEN)
	while (running) mainLoop();
#else
#include <emscripten.h>
	//FIXME: this assumes that the display refreshes at 60Hz
	emscripten_set_main_loop(mainLoop, 0, 0);
	emscripten_set_main_loop_timing(EM_TIMING_RAF, 2);
	return 0;
#endif

	if (game_state) SDL_free(game_state);
	if (initial_game_state) SDL_free(initial_game_state);

	SDL_FreeSurface(gfx);
	SDL_FreeSurface(font);
	for (int i = 0; i < (sizeof snd) / (sizeof * snd); i++) {
		if (snd[i]) Mix_FreeChunk(snd[i]);
	}
	for (int i = 0; i < (sizeof mus) / (sizeof * mus); i++) {
		if (mus[i]) Mix_FreeMusic(mus[i]);
	}

	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
	return 0;
}

#if SDL_MAJOR_VERSION >= 2
/* These inputs aren't sent to the game. */
enum {
	PSEUDO_BTN_SAVE_STATE = 6,
	PSEUDO_BTN_LOAD_STATE = 7,
	PSEUDO_BTN_EXIT = 8,
	PSEUDO_BTN_PAUSE = 9,
};
static void ReadGamepadInput(Uint16* out_buttons);
#endif

static void mainLoop(void) {
	const Uint8* kbstate = SDL_GetKeyState(NULL);

	static int reset_input_timer = 0;
	//hold F9 (select+start+y) to reset
	if (initial_game_state != NULL
#ifdef _3DS
		&& kbstate[SDLK_LSHIFT] && kbstate[SDLK_ESCAPE] && kbstate[SDLK_F11]
#else
		&& kbstate[SDLK_F9]
#endif
		) {
		reset_input_timer++;
		if (reset_input_timer >= 30) {
			reset_input_timer = 0;
			//reset
			OSDset("reset");
			paused = 0;
			Celeste_P8_load_state(initial_game_state);
			Celeste_P8_set_rndseed((unsigned)(time(NULL) + SDL_GetTicks()));
			Mix_HaltChannel(-1);
			Mix_HaltMusic();
			Celeste_P8_init();
		}
	}
	else reset_input_timer = 0;

	Uint16 prev_buttons_state = buttons_state;
	buttons_state = 0;

#if SDL_MAJOR_VERSION >= 2
	//SDL_GameControllerUpdate();
	ReadGamepadInput(&buttons_state);

	//HCF QUITAR ESTO
	/*
	if (buttons_state != 0)
	{
		SDL_Rect rc;
		rc.x = 0;
		rc.y = 0;
		SDL_FillRect(screenReal, NULL, SDL_MapRGB(screenReal->format, 255, 0, 255));
		SDL_Flip(screenReal);
		SDL_Delay(2000);
	}
	*/


	if (!((prev_buttons_state >> PSEUDO_BTN_PAUSE) & 1)
		&& (buttons_state >> PSEUDO_BTN_PAUSE) & 1) {
		goto toggle_pause;
	}

	if (!((prev_buttons_state >> PSEUDO_BTN_EXIT) & 1)
		&& (buttons_state >> PSEUDO_BTN_EXIT) & 1) {
		goto press_exit;
	}

	if (!((prev_buttons_state >> PSEUDO_BTN_SAVE_STATE) & 1)
		&& (buttons_state >> PSEUDO_BTN_SAVE_STATE) & 1) {
		goto save_state;
	}

	if (!((prev_buttons_state >> PSEUDO_BTN_LOAD_STATE) & 1)
		&& (buttons_state >> PSEUDO_BTN_LOAD_STATE) & 1) {
		goto load_state;
	}
#endif

	SDL_Event ev;
	while (SDL_PollEvent(&ev)) switch (ev.type) {
	case SDL_QUIT: running = 0; break;
	case SDL_KEYDOWN: {
#if SDL_MAJOR_VERSION >= 2
		if (ev.key.repeat) break; //no key repeat
#endif
		if (ev.key.keysym.sym == SDLK_ESCAPE) { //do pause
		toggle_pause:
			if (paused) {
				// unpausing: resume audio and hide menu
				Mix_Resume(-1); Mix_ResumeMusic();
				paused = 0;
				pause_menu_active = 0;
			}
			else {
				// pausing: pause audio and show menu
				Mix_Pause(-1); Mix_PauseMusic();
				paused = 1;
				pause_menu_active = 1;
				pause_menu_index = 0;
				pause_scaling_choice = screen_scaling_mode;
			}
			break;
		}
		else if (ev.key.keysym.sym == SDLK_DELETE) { //exit
		press_exit:
			running = 0;
			break;
		}
		else if (ev.key.keysym.sym == SDLK_F11 && !(kbstate[SDLK_LSHIFT] || kbstate[SDLK_ESCAPE])) {
			//if (SDL_WM_ToggleFullScreen(screenReal)) { //this doesn't work on windows..
				//OSDset("toggle fullscreen");
			//}
			//screenReal = SDL_GetVideoSurface();
			break;
		}
		else if (0 && ev.key.keysym.sym == SDLK_5) {
			Celeste_P8__DEBUG();
			break;
		}
		else if (ev.key.keysym.sym == SDLK_s && kbstate[SDLK_LSHIFT]) { //save state
		save_state:
			game_state = game_state ? game_state : SDL_malloc(Celeste_P8_get_state_size());
			if (game_state) {
				OSDset("save state");
				Celeste_P8_save_state(game_state);
				game_state_music = current_music;
			}
			break;
		}
		else if (ev.key.keysym.sym == SDLK_d && kbstate[SDLK_LSHIFT]) { //load state
		load_state:
			if (game_state) {
				OSDset("load state");
				if (paused) paused = 0, Mix_Resume(-1), Mix_ResumeMusic();
				Celeste_P8_load_state(game_state);
				if (current_music != game_state_music) {
					Mix_HaltMusic();
					current_music = game_state_music;
					if (game_state_music) Mix_PlayMusic(game_state_music, -1);
				}
			}
			break;
		}
		else if ( //toggle screenshake (e / L+R)
#ifdef _3DS
		(ev.key.keysym.sym == SDLK_d && kbstate[SDLK_s]) || (ev.key.keysym.sym == SDLK_s && kbstate[SDLK_d])
#else
			ev.key.keysym.sym == SDLK_e
#endif
			) {
			enable_screenshake = !enable_screenshake;
			OSDset("screenshake: %s", enable_screenshake ? "on" : "off");
		} break;
	}
	}

	if (!TAS) {

		//HCF: Con esto si que va!!!
		//buttons_state = 255;

		//HCF Si que entra aqui!!

		/*
		SDL_Rect rc;
		rc.x = 0;
		rc.y = 0;
		SDL_FillRect(screenReal, NULL, SDL_MapRGB(screenReal->format, 255, 0, 0));
		SDL_Flip(screenReal);
		while (1);
		*/

		if (kbstate[SDLK_LEFT])  buttons_state |= (1 << 0);
		if (kbstate[SDLK_RIGHT]) buttons_state |= (1 << 1);
		if (kbstate[SDLK_UP])    buttons_state |= (1 << 2);
		if (kbstate[SDLK_DOWN])  buttons_state |= (1 << 3);
		if (kbstate[SDLK_z] || kbstate[SDLK_c] || kbstate[SDLK_n])
		{
			buttons_state |= (1 << 4);
			//HCF Aqui no entra, obviamente
		}
		if (kbstate[SDLK_x] || kbstate[SDLK_v] || kbstate[SDLK_m]) buttons_state |= (1 << 5);
	}
	else if (TAS && !paused) {
		static int t = 0;
		t++;
		if (t == 1) buttons_state = 1 << 4;
		else if (t > 80) {
			int btn;
			fscanf(TAS, "%d,", &btn);
			buttons_state = btn;
		}
		else buttons_state = 0;
	}

	// === pause menu navigation (DPAD + A/Start) ===
	if (paused && pause_menu_active) {
		// Edge helpers: true only on the frame the button goes down.
#define EDGE_BIT(bs, prev, idx) (((((prev) >> (idx)) & 1) == 0) && (((bs) >> (idx)) & 1))
		const _Bool edge_left = EDGE_BIT(buttons_state, prev_buttons_state, 0);
		const _Bool edge_right = EDGE_BIT(buttons_state, prev_buttons_state, 1);
		const _Bool edge_up = EDGE_BIT(buttons_state, prev_buttons_state, 2);
		const _Bool edge_down = EDGE_BIT(buttons_state, prev_buttons_state, 3);
		const _Bool edge_A = EDGE_BIT(buttons_state, prev_buttons_state, 4); // confirm / apply
		// TODO: set START_BIT to whatever bit your input code uses for Start/pause toggle.
		// Commonly this is 6, but change if yours differs.
		const int START_BIT = 6;
		const _Bool edge_START = EDGE_BIT(buttons_state, prev_buttons_state, START_BIT);
#undef EDGE_BIT

		// We now have only 2 rows: 0=Scaling, 1=Apply
		if (edge_up)   pause_menu_index = (pause_menu_index + 1) % 2;  // wrap 0..1
		if (edge_down) pause_menu_index = (pause_menu_index + 1) % 2;

		// On "Scaling" row (row 0), change option with Left/Right
		if (pause_menu_index == 0) {
			if (edge_left)  pause_scaling_choice = (pause_scaling_choice + 2) % 3;
			if (edge_right) pause_scaling_choice = (pause_scaling_choice + 1) % 3;
		}

		// A = Apply when on row 1
		if (edge_A && pause_menu_index == 1) {
			screen_scaling_mode = pause_scaling_choice;
			RecomputeScreenDstRect();
			OSDset("scaling: %s", scaling_names[screen_scaling_mode]);
		}

		// Start = unpause (always)
		if (edge_START) {
			Mix_Resume(-1);
			Mix_ResumeMusic();
			paused = 0;
			pause_menu_active = 0;
			buttons_state = 0;
			prev_buttons_state = 0;
		}
	}


	if (paused) {
		// draw the game frame so any old OSD pixels get painted over
		Celeste_P8_draw();  // <-- add this

		// Always draw the small banner
		DrawPausedBanner();

		if (pause_menu_active) {
			const int w = 120, h = 40;
			const int x = (PICO8_W - w) / 2;
			// const int y = 16;  // old
			const int y = 26;     // new, lower

			// panel frame
			p8_rectfill(x - 2, y - 2, x + w + 2, y + h + 2, 6);
			p8_rectfill(x - 1, y - 1, x + w + 1, y + h + 1, 0);

			// centered title MENU
			const char* title = "menu";
			int tw = 4 * (int)strlen(title);
			p8_print(title, x + (w - tw) / 2, y + 2, 7);

			const int row0 = y + 12;
			p8_print("scaling:", x + 8, row0, (pause_menu_index == 0) ? 10 : 7);

			const char* val = scaling_names[pause_scaling_choice];
			int vw = 4 * (int)strlen(val);
			int right_half_left = x + w / 2;
			int right_half_width = w / 2 - 8;
			int vx = right_half_left + (right_half_width - vw) / 2;
			p8_print(val, vx, row0, (pause_menu_index == 0) ? 10 : 7);

			const char* t = "apply";
			tw = 4 * (int)strlen(t);
			p8_print(t, x + (w - tw) / 2, row0 + 8, (pause_menu_index == 1) ? 10 : 7);
			OSDset("scaling = %dx%d", screen_dstRect.w, screen_dstRect.h);
		}
	}
	else {
		Celeste_P8_update();
		Celeste_P8_draw();
	}
	OSDdraw();

	//HCF
	SDL_Rect rc;

	rc.x = 64;
	rc.y = 0;

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, screen);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, &screen_dstRect);
	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(texture);

	//SDL_FillRect(screenReal, NULL, SDL_MapRGB(screenReal->format, 0, 0, 0));
	//SDL_BlitSurface(screen, NULL, screenReal, &rc);
	//SDL_Flip(screenReal);

	//SDL_Flip(screen);

#ifdef EMSCRIPTEN //emscripten_set_main_loop already sets the fps
	SDL_Delay(1);
#elif defined(_3DS)
	gspWaitForVBlank(), gspWaitForVBlank();
#else
	static int t = 0;
	static unsigned frame_start = 0;
	unsigned frame_end = SDL_GetTicks();
	unsigned frame_time = frame_end - frame_start;
	unsigned target_millis;
	// frame timing for 30fps is 33.333... ms, but we only have integer granularity
	// so alternate between 33 and 34 ms, like [33,33,34,33,33,34,...] which averages out to 33.333...
	if (t < 2) target_millis = 33;
	else       target_millis = 34;

	if (++t == 3) t = 0;

	if (frame_time < target_millis) {
		SDL_Delay(target_millis - frame_time);
	}
	frame_start = SDL_GetTicks();
#endif
}

static int gettileflag(int, int);
static void p8_line(int, int, int, int, unsigned char);

//lots of code from https://github.com/SDL-mirror/SDL/blob/bc59d0d4a2c814900a506d097a381077b9310509/src/video/SDL_surface.c#L625
//coordinates should be scaled already
static inline void Xblit(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect, int color, int flipx, int flipy) {
	assert(src && dst && !src->locked && !dst->locked);
	assert(dst->format->BitsPerPixel == 32 && src->format->BitsPerPixel == 8);
	SDL_Rect fulldst;
	/* If the destination rectangle is NULL, use the entire dest surface */
	if (!dstrect)
		dstrect = (fulldst = (SDL_Rect){ 0,0,dst->w,dst->h }, &fulldst);
	//dstrect = (fulldst = (SDL_Rect){ 64,0,dst->w,dst->h }, &fulldst);
	//HCF

	int srcx, srcy, w, h;

	/* clip the source rectangle to the source surface */
	if (srcrect) {
		int maxw, maxh;

		srcx = srcrect->x;
		w = srcrect->w;
		if (srcx < 0) {
			w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if (maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if (srcy < 0) {
			h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if (maxh < h)
			h = maxh;

	}
	else {
		srcx = srcy = 0;
		w = src->w;
		h = src->h;
	}

	/* clip the destination rectangle against the clip rectangle */
	{
		SDL_Rect* clip = &dst->clip_rect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if (dx > 0) {
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if (dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if (dy > 0) {
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if (dy > 0)
			h -= dy;
	}

	if (w && h) {
		unsigned char* srcpix = src->pixels;
		int srcpitch = src->pitch;
		Uint32* dstpix = dst->pixels;
#define _blitter(dp, xflip) do                                                                  \
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {                                       \
      unsigned char p = srcpix[!xflip ? srcx+x+(srcy+y)*srcpitch : srcx+(w-x-1)+(srcy+y)*srcpitch]; \
      if (p) dstpix[dstrect->x+x + (dstrect->y+y)*dst->w] = getcolor(dp);                           \
    } while(0)
		if (color && flipx) _blitter(color, 1);
		else if (!color && flipx) _blitter(p, 1);
		else if (color && !flipx) _blitter(color, 0);
		else if (!color && !flipx) _blitter(p, 0);
#undef _blitter
	}
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col) {
	int w = (x1 - x0 + 1) * scale;
	int h = (y1 - y0 + 1) * scale;
	if (w > 0 && h > 0) {
		SDL_Rect rc = { x0 * scale,y0 * scale, w,h };
		SDL_FillRect(screen, &rc, getcolor(col));
	}
}

static void p8_print(const char* str, int x, int y, int col) {
	for (char c = *str; c; c = *(++str)) {
		c &= 0x7F;
		SDL_Rect srcrc = { 8 * (c % 16), 8 * (c / 16) };
		srcrc.x *= scale;
		srcrc.y *= scale;
		srcrc.w = srcrc.h = 8 * scale;

		SDL_Rect dstrc = { x * scale, y * scale, scale, scale };
		Xblit(font, &srcrc, screen, &dstrc, col, 0, 0);
		x += 4;
	}
}

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...) {
	static int camera_x = 0, camera_y = 0;
	if (!enable_screenshake) {
		camera_x = camera_y = 0;
	}

	va_list args;
	int ret = 0;
	va_start(args, call);

#define   INT_ARG() va_arg(args, int)
#define  BOOL_ARG() (Celeste_P8_bool_t)va_arg(args, int)
#define RET_INT(_i)   do {ret = (_i); goto end;} while (0)
#define RET_BOOL(_b) RET_INT(!!(_b))

	switch (call) {
	case CELESTE_P8_MUSIC: { //music(idx,fade,mask)
		int index = INT_ARG();
		int fade = INT_ARG();
		int mask = INT_ARG();

		(void)mask; //we do not care about this since sdl mixer keeps sounds and music separate

		if (index == -1) { //stop playing
			Mix_FadeOutMusic(fade);
			current_music = NULL;
		}
		else if (mus[index / 10]) {
			Mix_Music* musi = mus[index / 10];
			current_music = musi;
			Mix_FadeInMusic(musi, -1, fade);
		}
	} break;
	case CELESTE_P8_SPR: { //spr(sprite,x,y,cols,rows,flipx,flipy)
		int sprite = INT_ARG();
		int x = INT_ARG();
		int y = INT_ARG();
		int cols = INT_ARG();
		int rows = INT_ARG();
		int flipx = BOOL_ARG();
		int flipy = BOOL_ARG();

		(void)cols;
		(void)rows;

		assert(rows == 1 && cols == 1);

		if (sprite >= 0) {
			SDL_Rect srcrc = {
				8 * (sprite % 16),
				8 * (sprite / 16)
			};
			srcrc.x *= scale;
			srcrc.y *= scale;
			srcrc.w = srcrc.h = scale * 8;
			SDL_Rect dstrc = {
				//HCF
				//64+(x - camera_x)* scale, (y - camera_y)* scale,
				(x - camera_x) * scale, (y - camera_y) * scale,
				scale, scale
			};
			Xblit(gfx, &srcrc, screen, &dstrc, 0, flipx, flipy);
		}
	} break;
	case CELESTE_P8_BTN: { //btn(b)
		int b = INT_ARG();
		assert(b >= 0 && b <= 5);
		RET_BOOL(buttons_state & (1 << b));
	} break;
	case CELESTE_P8_SFX: { //sfx(id)
		int id = INT_ARG();

		if (id < (sizeof snd) / (sizeof * snd) && snd[id])
			Mix_PlayChannel(-1, snd[id], 0);
	} break;
	case CELESTE_P8_PAL: { //pal(a,b)
		int a = INT_ARG();
		int b = INT_ARG();
		if (a >= 0 && a < 16 && b >= 0 && b < 16) {
			//swap palette colors
			palette[a] = base_palette[b];
		}
	} break;
	case CELESTE_P8_PAL_RESET: { //pal()
		ResetPalette();
	} break;
	case CELESTE_P8_CIRCFILL: { //circfill(x,y,r,col)
		int cx = INT_ARG() - camera_x;
		int cy = INT_ARG() - camera_y;
		int r = INT_ARG();
		int col = INT_ARG();

		int realcolor = getcolor(col);

		if (r <= 1) {
			SDL_FillRect(screen, &(SDL_Rect){scale* (cx - 1), scale* cy, scale * 3, scale}, realcolor);
			SDL_FillRect(screen, &(SDL_Rect){scale* cx, scale* (cy - 1), scale, scale * 3}, realcolor);
		}
		else if (r <= 2) {
			SDL_FillRect(screen, &(SDL_Rect){scale* (cx - 2), scale* (cy - 1), scale * 5, scale * 3}, realcolor);
			SDL_FillRect(screen, &(SDL_Rect){scale* (cx - 1), scale* (cy - 2), scale * 3, scale * 5}, realcolor);
		}
		else if (r <= 3) {
			SDL_FillRect(screen, &(SDL_Rect){scale* (cx - 3), scale* (cy - 1), scale * 7, scale * 3}, realcolor);
			SDL_FillRect(screen, &(SDL_Rect){scale* (cx - 1), scale* (cy - 3), scale * 3, scale * 7}, realcolor);
			SDL_FillRect(screen, &(SDL_Rect){scale* (cx - 2), scale* (cy - 2), scale * 5, scale * 5}, realcolor);
		}
		else { //i dont think the game uses this
			int f = 1 - r; //used to track the progress of the drawn circle (since its semi-recursive)
			int ddFx = 1; //step x
			int ddFy = -2 * r; //step y
			int x = 0;
			int y = r;

			//this algorithm doesn't account for the diameters
			//so we have to set them manually
			p8_line(cx, cy - y, cx, cy + r, col);
			p8_line(cx + r, cy, cx - r, cy, col);

			while (x < y) {
				if (f >= 0) {
					y--;
					ddFy += 2;
					f += ddFy;
				}
				x++;
				ddFx += 2;
				f += ddFx;

				//build our current arc
				p8_line(cx + x, cy + y, cx - x, cy + y, col);
				p8_line(cx + x, cy - y, cx - x, cy - y, col);
				p8_line(cx + y, cy + x, cx - y, cy + x, col);
				p8_line(cx + y, cy - x, cx - y, cy - x, col);
			}
		}
	} break;
	case CELESTE_P8_PRINT: { //print(str,x,y,col)
		const char* str = va_arg(args, const char*);
		int x = INT_ARG() - camera_x;
		int y = INT_ARG() - camera_y;
		int col = INT_ARG() % 16;

#ifdef _3DS
		if (!strcmp(str, "x+c")) {
			//this is confusing, as 3DS uses a+b button, so use this hack to make it more appropiate
			str = "a+b";
		}
#endif

		p8_print(str, x, y, col);
	} break;
	case CELESTE_P8_RECTFILL: { //rectfill(x0,y0,x1,y1,col)
		int x0 = INT_ARG() - camera_x;
		int y0 = INT_ARG() - camera_y;
		int x1 = INT_ARG() - camera_x;
		int y1 = INT_ARG() - camera_y;
		int col = INT_ARG();

		p8_rectfill(x0, y0, x1, y1, col);
	} break;
	case CELESTE_P8_LINE: { //line(x0,y0,x1,y1,col)
		int x0 = INT_ARG() - camera_x;
		int y0 = INT_ARG() - camera_y;
		int x1 = INT_ARG() - camera_x;
		int y1 = INT_ARG() - camera_y;
		int col = INT_ARG();

		p8_line(x0, y0, x1, y1, col);
	} break;
	case CELESTE_P8_MGET: { //mget(tx,ty)
		int tx = INT_ARG();
		int ty = INT_ARG();

		RET_INT(tilemap_data[tx + ty * 128]);
	} break;
	case CELESTE_P8_CAMERA: { //camera(x,y)
		if (enable_screenshake) {
			camera_x = INT_ARG();
			camera_y = INT_ARG();
		}
	} break;
	case CELESTE_P8_FGET: { //fget(tile,flag)
		int tile = INT_ARG();
		int flag = INT_ARG();

		RET_INT(gettileflag(tile, flag));
	} break;
	case CELESTE_P8_MAP: { //map(mx,my,tx,ty,mw,mh,mask)
		int mx = INT_ARG(), my = INT_ARG();
		int tx = INT_ARG(), ty = INT_ARG();
		int mw = INT_ARG(), mh = INT_ARG();
		int mask = INT_ARG();

		for (int x = 0; x < mw; x++) {
			for (int y = 0; y < mh; y++) {
				int tile = tilemap_data[x + mx + (y + my) * 128];
				//hack
				if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask - 1 : mask)) {
					SDL_Rect srcrc = {
						8 * (tile % 16),
						8 * (tile / 16)
					};
					srcrc.x *= scale;
					srcrc.y *= scale;
					srcrc.w = srcrc.h = scale * 8;
					SDL_Rect dstrc = {
						//HCF
						//64+(tx + x * 8 - camera_x)* scale, (ty + y * 8 - camera_y)* scale,
						(tx + x * 8 - camera_x) * scale, (ty + y * 8 - camera_y) * scale,
						scale * 8, scale * 8
					};

					if (0) {
						srcrc.x = srcrc.y = 0;
						srcrc.w = srcrc.h = 8;
						dstrc.x = x * 8, dstrc.y = y * 8;
						dstrc.w = dstrc.h = 8;
					}

					Xblit(gfx, &srcrc, screen, &dstrc, 0, 0, 0);
				}
			}
		}
	} break;
	}

end:
	va_end(args);
	return ret;
}

static int gettileflag(int tile, int flag) {
	return tile < sizeof(tile_flags) / sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

//coordinates should NOT be scaled before calling this
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color) {
#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
	CLAMP(x0, 0, screen->w);
	CLAMP(y0, 0, screen->h);
	CLAMP(x1, 0, screen->w);
	CLAMP(y1, 0, screen->h);

	Uint32 realcolor = getcolor(color);

#undef CLAMP
#define PLOT(x,y) do {                                                        \
     SDL_FillRect(screen, &(SDL_Rect){x*scale,y*scale,scale,scale}, realcolor); \
	} while (0)
	int sx, sy, dx, dy, err, e2;
	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	if (!dx && !dy) return;

	if (x0 < x1) sx = 1; else sx = -1;
	if (y0 < y1) sy = 1; else sy = -1;
	err = dx - dy;
	if (!dy && !dx) return;
	else if (!dx) { //vertical line
		for (int y = y0; y != y1; y += sy) PLOT(x0, y);
	}
	else if (!dy) { //horizontal line
		for (int x = x0; x != x1; x += sx) PLOT(x, y0);
	} while (x0 != x1 || y0 != y1) {
		PLOT(x0, y0);
		e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
#undef PLOT
}

#if SDL_MAJOR_VERSION >= 2
//SDL2: read input from connected gamepad

struct mapping {
	SDL_GameControllerButton sdl_btn;
	Uint16 pico8_btn;
};
static const char* pico8_btn_names[] = {
	"left", "right", "up", "down", "jump", "dash", "save", "load", "exit", "pause"
};

// initialized with default mapping
static struct mapping controller_mappings[30] = {
	{SDL_CONTROLLER_BUTTON_DPAD_LEFT,  0}, //left
	{SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1}, //right
	{SDL_CONTROLLER_BUTTON_DPAD_UP,    2}, //up
	{SDL_CONTROLLER_BUTTON_DPAD_DOWN,  3}, //down
	{SDL_CONTROLLER_BUTTON_A,          4}, //jump
	{SDL_CONTROLLER_BUTTON_B,          5}, //dash

	{SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  PSEUDO_BTN_SAVE_STATE}, //save
	{SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PSEUDO_BTN_LOAD_STATE}, //load
	{SDL_CONTROLLER_BUTTON_GUIDE,         PSEUDO_BTN_EXIT}, //exit
	{SDL_CONTROLLER_BUTTON_START,         PSEUDO_BTN_PAUSE}, //pause
	{0xff, 0xff}
};
static const Uint16 stick_deadzone = 32767 / 2; //about half

static void ReadGamepadInput(Uint16* out_buttons) {
	*out_buttons = 0;

	SDL_PumpEvents();
	SDL_JoystickUpdate(); //manual refresh of the gamepad(s)

	static _Bool read_config = 0;
	if (!read_config) {
		read_config = 1;
		const char* cfg_file_path = "D:\\ccleste-input-cfg.txt";

		FILE* cfg = fopen(cfg_file_path, "r");
		if (cfg) {
			int i;
			for (i = 0; i < sizeof controller_mappings / sizeof *controller_mappings - 1;) {
				char line[150], p8btn[31], sdlbtn[31];
				fgets(line, sizeof line - 1, cfg);
				if (feof(cfg) || ferror(cfg)) break;
				line[sizeof line - 1] = 0;
				if (*line == '#') {
					//comment
				} else if (sscanf(line, "%30s %30s", p8btn, sdlbtn) == 2) {
					p8btn[sizeof p8btn - 1] = sdlbtn[sizeof sdlbtn - 1] = 0;
					for (int btn = 0; btn < sizeof pico8_btn_names / sizeof *pico8_btn_names; btn++) {
						if (!SDL_strcasecmp(pico8_btn_names[btn], p8btn)) {
							//printf("input cfg: %s -> %s\n", p8btn, sdlbtn);
							controller_mappings[i].sdl_btn = SDL_GameControllerGetButtonFromString(sdlbtn);
							controller_mappings[i].pico8_btn = btn;
							i++;
						}
					}
				}
			}
			controller_mappings[i].pico8_btn = 0xFF;
			fclose(cfg);
		} else {
			cfg = fopen(cfg_file_path, "w");
			if (cfg) {
				//printf("creating ccleste-input-cfg.txt with default button mappings\n");
				fprintf(cfg, "# in-game \tcontroller\n");
				for (struct mapping* mapping = controller_mappings; mapping->pico8_btn != 0xFF; mapping++) {
					fprintf(cfg, "%s \t%s\n", pico8_btn_names[mapping->pico8_btn], SDL_GameControllerGetStringForButton(mapping->sdl_btn));
				}
				fclose(cfg);
			}
		}
	}

	static SDL_GameController* controller = NULL;
	if (!controller) {
		static int tries_left = 30;
		if (!tries_left) return;
		tries_left--;

		//use first available controller
		int count = SDL_NumJoysticks();
		//printf("sdl reports %i controllers\n", count);
		for (int i = 0; i < count; i++) {
			if (SDL_IsGameController(i)) {
				controller = SDL_GameControllerOpen(i);
				if (!controller) {
					//fprintf(stderr, "error opening controller: %s\n", SDL_GetError());
					return;
				}
				//printf("picked controller: '%s'\n", SDL_GameControllerName(controller));
				break;
			}
		}
	}

	//pico 8 buttons and pseudo buttons
	for (int i = 0; i < sizeof controller_mappings / sizeof *controller_mappings; i++) {
		struct mapping mapping = controller_mappings[i];
		if (mapping.pico8_btn == 0xFF) break;
		_Bool pressed = SDL_GameControllerGetButton(controller, mapping.sdl_btn);
		Uint16 mask = ~(1 << mapping.pico8_btn);
		*out_buttons = (*out_buttons & mask) | (pressed << mapping.pico8_btn);
	}
}
#endif


// vim: ts=2 sw=2 noexpandtab