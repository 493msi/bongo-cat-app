#include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <windowsx.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>
#include <deque>
#include <time.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <atomic>

constexpr int WINDOW_WIDTH = 500;
constexpr int WINDOW_HEIGHT = 500;

typedef struct Textures {
	SDL_Texture *none;
	SDL_Texture *idle;
	SDL_Texture *mouse;
	SDL_Texture *mouseClick;
	SDL_Texture *mouseDown;
	SDL_Texture *mouseDownClick;
	SDL_Texture *mouseUp;
	SDL_Texture *mouseUpClick;
	SDL_Texture *mouseLeft;
	SDL_Texture *mouseLeftClick;
	SDL_Texture *mouseRight;
	SDL_Texture *mouseRightClick;
	SDL_Texture *keyboardOverlay;
	SDL_Texture *keyboardOverlay2;
	SDL_Texture *keyboardOverlay3;
} Textures;

constexpr unsigned int kbVariations = 3;

typedef struct TexLoader {
	SDL_Texture **texture;
	std::string location;
} TexLoader;

enum class LoadingResult {
	SUCCESS,
	FAILURE
};

enum class Orientation {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	NONE,
	DEFAULT
};

void showErrorMessage(const std::string &message);
LoadingResult loadT(SDL_Renderer *renderer, TexLoader *texLoader);
DWORD WINAPI hookInput( LPVOID lpParam );

std::map<DWORD, bool> keyboardMapPrev;
std::map<DWORD, bool> keyboardMap;
std::mutex kbMapMutex;
std::atomic<Orientation> mouseOrientation(Orientation::NONE);
std::atomic_long mouseX(LONG_MIN);
std::atomic_long mouseY(LONG_MIN);
std::atomic_long mouseXDelta(0);
std::atomic_long mouseYDelta(0);

LRESULT CALLBACK KBProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
	DWORD vkCode = p->vkCode;

	if (wParam == WM_KEYDOWN)
	{
		if (mouseOrientation == Orientation::NONE)
		{
			mouseOrientation = Orientation::DEFAULT;
		}

		kbMapMutex.lock();
		keyboardMap[vkCode] = true;
		kbMapMutex.unlock();
	}

	if (wParam == WM_KEYUP)
	{
		kbMapMutex.lock();
		keyboardMap[vkCode] = false;
		kbMapMutex.unlock();
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT) lParam;

	if (wParam == WM_MOUSEMOVE)
	{
		long x = p->pt.x;
		long y = p->pt.y;

		if (mouseX != LONG_MIN)
		{
			mouseXDelta = x - mouseX;
		}

		if (mouseY != LONG_MIN)
		{
			mouseYDelta = y - mouseY;
		}

		mouseX = x;
		mouseY = y;

		constexpr long meaningfulMovement = 2;

		if (fabs((double) mouseXDelta) > fabs((double) mouseYDelta))
		{
			if (mouseXDelta > meaningfulMovement)
			{
				mouseOrientation = Orientation::RIGHT;
			}
			if (mouseXDelta < -meaningfulMovement)
			{
				mouseOrientation = Orientation::LEFT;
			}
		}
		else
		{
			if (mouseYDelta > meaningfulMovement)
			{
				mouseOrientation = Orientation::DOWN;
			}
			if (mouseYDelta < -meaningfulMovement)
			{
				mouseOrientation = Orientation::UP;
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LoadingResult loadT(SDL_Renderer *renderer, TexLoader *texLoader)
{
	if (texLoader == nullptr)
	{
		*(texLoader->texture) = nullptr;
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Texture loader was null.");
		return LoadingResult::FAILURE;
	}

	if (texLoader->texture == nullptr)
	{
		*(texLoader->texture) = nullptr;
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Texture pointer is null.");
		return LoadingResult::FAILURE;
	}

	SDL_Surface *image = IMG_Load(texLoader->location.c_str());

	if (!image)
	{
		*(texLoader->texture) = nullptr;
		std::string message = "Required images could not be loaded: " + texLoader->location;
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, message.c_str());
		return LoadingResult::FAILURE;
	}

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	if (tex == nullptr)
	{
		*(texLoader->texture) = nullptr;
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "A texture error has occurred.");
		return LoadingResult::FAILURE;
	}

	*(texLoader->texture) = tex;

	return LoadingResult::SUCCESS;
}

void showErrorMessage(const std::string &message)
{
	const SDL_MessageBoxButtonData buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, ":(" }
	};

	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR,
		NULL,
		"BongoCat has crashed.",
		message.c_str(),
		SDL_arraysize(buttons),
		buttons,
		NULL
	};

	int buttonid;

	if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0)
	{
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error displaying error message.");
	}

	SDL_LogError(SDL_LOG_CATEGORY_ERROR, message.c_str());
}

DWORD WINAPI hookInput( LPVOID lpParam )
{
	HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KBProc, NULL, 0);
	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);

	MSG msg;
	BOOL result;

	for (;;)
	{
	    result = GetMessage(&msg, nullptr, 0, 0);

	    if (result <= 0)
	    {
	        break;
	    }

	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(mouseHook);
	UnhookWindowsHookEx(keyboardHook);

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HANDLE inputThread;
	DWORD inputThreadID;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::string errMess("SDL2 initialization failed.\n");
		showErrorMessage(errMess + SDL_GetError());
		return EXIT_FAILURE;
	}

	inputThread = CreateThread(NULL, 0, &hookInput, NULL, 0, &inputThreadID);

	SDL_Event event;
	SDL_Window *window = SDL_CreateWindow("BongoCat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

	SDL_SetWindowTitle(window, "BongoCat");

	Textures bcTextures;

	std::vector<TexLoader> loaders;

	loaders.push_back({ &bcTextures.none, "assets/begin.png" });
	loaders.push_back({ &bcTextures.idle, "assets/idle.png" });
	loaders.push_back({ &bcTextures.mouse, "assets/mouse.png" });
	loaders.push_back({ &bcTextures.mouseClick, "assets/mouse_click.png" });
	loaders.push_back({ &bcTextures.mouseDown, "assets/mouse_down.png" });
	loaders.push_back({ &bcTextures.mouseDownClick, "assets/mouse_down_click.png" });
	loaders.push_back({ &bcTextures.mouseLeft, "assets/mouse_left.png" });
	loaders.push_back({ &bcTextures.mouseLeftClick, "assets/mouse_left_click.png" });
	loaders.push_back({ &bcTextures.mouseRight, "assets/mouse_right.png" });
	loaders.push_back({ &bcTextures.mouseRightClick, "assets/mouse_right_click.png" });
	loaders.push_back({ &bcTextures.mouseUp, "assets/mouse_up.png" });
	loaders.push_back({ &bcTextures.mouseUpClick, "assets/mouse_up_click.png" });
	loaders.push_back({ &bcTextures.keyboardOverlay, "assets/overlay_keyboard.png" });
	loaders.push_back({ &bcTextures.keyboardOverlay2, "assets/overlay_keyboard2.png" });
	loaders.push_back({ &bcTextures.keyboardOverlay3, "assets/overlay_keyboard3.png" });

	for (auto it = loaders.begin(); it != loaders.end(); it++)
	{
		auto lres = loadT(renderer, &*it);

		if (lres == LoadingResult::FAILURE)
		{
			for (auto itc = loaders.begin(); itc != it; itc++)
			{
				auto tex = *(itc->texture);

				if (tex != nullptr)
				{
					SDL_DestroyTexture(tex);
				}
			}

			showErrorMessage("A required texture could not be loaded.");
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();

			PostThreadMessage(inputThreadID, WM_QUIT, 0, 0);
			WaitForSingleObject(inputThread, INFINITE);
			CloseHandle(inputThread);

			return EXIT_FAILURE;
		}
	}

	unsigned int keyboardVariation = 0;

	unsigned int keysPressedLast = 0;

	bool shouldExit = false;

	while (!shouldExit)
	{
		SDL_SetRenderDrawColor(renderer, 0x00, 0xB1, 0x40, 0xFF);

		SDL_RenderClear(renderer);

		SDL_Rect tgt = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

		Orientation mouseDir = mouseOrientation;

		if (GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_RBUTTON))
		{
			switch (mouseDir)
			{
				case Orientation::DOWN:
					SDL_RenderCopy(renderer, bcTextures.mouseDownClick, NULL, &tgt);
					break;
				case Orientation::UP:
					SDL_RenderCopy(renderer, bcTextures.mouseUpClick, NULL, &tgt);
					break;
				case Orientation::LEFT:
					SDL_RenderCopy(renderer, bcTextures.mouseLeftClick, NULL, &tgt);
					break;
				case Orientation::RIGHT:
					SDL_RenderCopy(renderer, bcTextures.mouseRightClick, NULL, &tgt);
					break;
				case Orientation::NONE:
					SDL_RenderCopy(renderer, bcTextures.mouseClick, NULL, &tgt);
					mouseOrientation = Orientation::DEFAULT;
					break;
				default:
					SDL_RenderCopy(renderer, bcTextures.mouseClick, NULL, &tgt);
					break;
			}
		}
		else
		{
			switch (mouseDir)
			{
				case Orientation::DOWN:
					SDL_RenderCopy(renderer, bcTextures.mouseDown, NULL, &tgt);
					break;
				case Orientation::UP:
					SDL_RenderCopy(renderer, bcTextures.mouseUp, NULL, &tgt);
					break;
				case Orientation::LEFT:
					SDL_RenderCopy(renderer, bcTextures.mouseLeft, NULL, &tgt);
					break;
				case Orientation::RIGHT:
					SDL_RenderCopy(renderer, bcTextures.mouseRight, NULL, &tgt);
					break;
				case Orientation::NONE:
					SDL_RenderCopy(renderer, bcTextures.none, NULL, &tgt);
					break;
				default:
					SDL_RenderCopy(renderer, bcTextures.idle, NULL, &tgt);
					break;
			}
		}

		kbMapMutex.lock();

		unsigned int keysPressed = 0;

		for ( auto it = keyboardMap.begin(); it != keyboardMap.end(); it++ )
		{
			auto &entry = *it;

			if (entry.second)
			{
				keysPressed++;
			}
		}

		keyboardMapPrev = keyboardMap;

		if (keysPressed > 0)
		{
			if (keysPressed > keysPressedLast)
			{
				keyboardVariation = (keyboardVariation + 1) % kbVariations;
			}

			switch (keyboardVariation)
			{
				case 0:
					SDL_RenderCopy(renderer, bcTextures.keyboardOverlay, NULL, &tgt);
					break;
				case 1:
					SDL_RenderCopy(renderer, bcTextures.keyboardOverlay2, NULL, &tgt);
					break;
				case 2:
					SDL_RenderCopy(renderer, bcTextures.keyboardOverlay3, NULL, &tgt);
					break;
				default:
					break;
			}
		}

		keysPressedLast = keysPressed;

		kbMapMutex.unlock();

		SDL_RenderPresent(renderer);

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				shouldExit = true;
			}
		}
	}

	for (auto itc = loaders.begin(); itc != loaders.end(); itc++)
	{
		auto tex = *(itc->texture);

		if (tex != nullptr)
		{
			SDL_DestroyTexture(tex);
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	PostThreadMessage(inputThreadID, WM_QUIT, 0, 0);
	WaitForSingleObject(inputThread, INFINITE);
	CloseHandle(inputThread);

	return EXIT_SUCCESS;
}
