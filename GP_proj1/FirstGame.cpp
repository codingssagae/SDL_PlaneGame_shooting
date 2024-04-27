#include "GameFunc.h"
#include <windows.h>
#include <vector>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include "SDL_image.h" // SDL_image 헤더 포함
#include "SDL_ttf.h"
#include <sstream> // std::ostringstream을 사용하기 위해 필요
#include <iomanip> // std::setw와 std::setfill을 사용하기 위해 필요
#include "SDL_mixer.h" // SDL_mixer 헤더 포함
#include <chrono> // 시간 측정을 위해 추가
using namespace std;


enum GameState {
	INTRO,
	GAME,
	ENDING
};

GameState currentGameState = INTRO;  // 초기 게임 상태

int g_input;
std::string g_output;
int missileCount = 5;  // 초기 미사일 수
bool showReloadButton = false;  // 재충전 버튼 표시 여부
int reloadButtonWidth = 100; // 초기 너비 추정값
int reloadButtonHeight = 50; // 초기 높이 추정값
SDL_Color whiteColor = { 255, 255, 255, 255 };  // RGBA for white
int reloadButtonX = 800 - 110-140; // 화면 오른쪽 끝에서 버튼의 너비만큼 빼고 10px 여백
int reloadButtonY = 600 - 60; // 화면 하단에서 버튼의 높이만큼 빼고 10px 여백
bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;
bool spacePressed = false;

int g_X;
int g_Y;
// 흘러간 시간 기록
double g_elapsed_time_ms; 
double g_plane_angle = 0;
// 소리 파일 불러오기
Mix_Chunk* g_reloadSound; // 전역 변수 선언 
Mix_Music* introMusic;
Mix_Music* endingMusic;

int m_state;
int m_x, m_y;
int g_score = 0; // 게임 점수

SDL_Texture* textureMissile = nullptr;
SDL_Texture* texturePlane = nullptr;
SDL_Texture* textureBackground = nullptr;
SDL_Texture* textureIntro = nullptr;
SDL_Texture* textureEnding = nullptr;

// 창의 크기에 따른 최대값 정의
const int maxX = 800 - 50; // 비행기 이미지의 너비를 고려
const int maxY = 600 - 50; // 비행기 이미지의 높이를 고려

TTF_Font* font = nullptr;

struct Missile {
	int x, y; // 미사일의 위치
	bool isActive; // 미사일이 활성 상태인지 여부
	double direction; // 미사일의 발사 방향

	// 기본 생성자
	Missile() : x(0), y(0), isActive(false), direction(0) {}

	// 필요한 값들로 초기화하는 생성자
	Missile(int initX, int initY, bool active, double dir) : x(initX), y(initY), isActive(active), direction(dir) {}
};

Mix_Chunk* g_missileSound;
Mix_Music* g_backgroundMusic;

std::vector<Missile> missiles;

// 마지막 미사일 발사 시간을 기록하는 전역 변수 추가
std::chrono::steady_clock::time_point lastMissileTime = std::chrono::steady_clock::now();


/////////////////////////////////////////////////////////////
// InitGame() 
// 프로그램이 시작될 때 최초에 한 번 호출되는 함수.
// 이 함수에서 게임에 필요한 자원(이미지, 사운드 등)을 로딩하고, 상태 변수들을 초기화 해야한다.
void InitGame() {
	g_input = 0;
	g_flag_running = true;

	g_X = 375;
	g_Y = 275;
	g_elapsed_time_ms = 0; 

	// SDL_mixer 초기화
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cout << "SDL_mixer 초기화 실패: " << Mix_GetError() << std::endl;
	}  

	// 미사일 소리와 배경 음악을 로드합니다.
	g_missileSound = Mix_LoadWAV("../../Resources/misasound.wav"); 
	g_backgroundMusic = Mix_LoadMUS("../../Resources/back.mp3"); 
	g_reloadSound = Mix_LoadWAV("../../Resources/reload.wav"); // 재장전 소리 파일 경로
	introMusic = Mix_LoadMUS("../../Resources/intro.mp3");
	endingMusic = Mix_LoadMUS("../../Resources/ending.mp3");

	Mix_VolumeChunk(g_missileSound, MIX_MAX_VOLUME / 2); // 볼륨 조정 예시
	Mix_VolumeMusic(MIX_MAX_VOLUME); // 배경 음악 볼륨을 최대로 설정

	SDL_Surface* tempSurface = IMG_Load("../../Resources/beehang3.png"); // 비행기 이미지 로드
	texturePlane = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface); // 더 이상 필요없는 서피스 해제

	tempSurface = IMG_Load("../../Resources/intro.jpg");
	textureIntro = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface);

	tempSurface = IMG_Load("../../Resources/ending.jpg");
	textureEnding = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface);

	tempSurface = IMG_Load("../../Resources/misa2.png"); // 미사일 이미지 로드
	textureMissile = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface); // 서피스 해제

	tempSurface = IMG_Load("../../Resources/space.jpg");
	textureBackground = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface);
	
	m_state = 0;

	TTF_Init();
	font = TTF_OpenFont("../../Resources/YEONGJUSeonbi.ttf", 24); // 폰트 파일 경로와 크기 지정

	if (currentGameState == INTRO) {
		Mix_PlayMusic(introMusic, -1);
	}

	// std::cout 출력에 버퍼를 사용하여, 출력 속도가 느려지는 현상을 피한다.
	setvbuf(stdout, NULL, _IOLBF, 4096);

	// 표준출력 화면을 깨끗히 지운다.
	system("cls");
}

void RenderScore() {
	SDL_Color color = { 255, 255, 255 }; // 점수의 색상 (여기서는 흰색)

	// 점수를 다섯 자리 숫자로 포맷하기
	/*std::ostringstream scoreStream;
	scoreStream << u8"점수: " << std::setfill('0') << std::setw(5) << g_score;
	std::string scoreText = scoreStream.str();

	SDL_Surface* surface = TTF_RenderUTF8_Solid(font, scoreText.c_str(), color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);

	int textWidth = surface->w;
	int textHeight = surface->h;
	SDL_FreeSurface(surface);

	SDL_Rect renderQuad = { 10, 10, textWidth, textHeight }; // 점수를 화면에 표시할 위치와 크기
	SDL_RenderCopy(g_renderer, texture, NULL, &renderQuad);
	SDL_DestroyTexture(texture); // 사용 후 텍스처 해제*/
}

/////////////////////////////////////////////////////////////
// UpdateMissiles() - 미사일 상태를 업데이트하는 함수
void UpdateMissiles() {
	for (auto& missile : missiles) {
		if (missile.isActive) {
			// 미사일의 방향에 따른 위치 조정
			switch ((int)missile.direction) {
			case 0: // 위쪽
				missile.y -= 25;
				break;
			case 90: // 오른쪽
				missile.x += 25;
				break;
			case 180: // 아래쪽
				missile.y += 25;
				break;
			case 270: // 왼쪽
				missile.x -= 25;
				break;
			}

			// 화면 밖으로 나간 미사일 비활성화
			if (missile.x < 0 || missile.x > maxX || missile.y < 0 || missile.y > maxY) {
				missile.isActive = false;
			}
		}
	}

	if (g_input == 6) {
		if (std::count_if(missiles.begin(), missiles.end(), [](const Missile& m) { return m.isActive; }) < 5) {
			Missile newMissile(g_X, g_Y, true, g_plane_angle);
			// Adjust missile starting position based on the plane's angle
			switch ((int)g_plane_angle) {
			case 0: // Top
				newMissile.x += 25; // Center of the plane width
				break;
			case 90: // Right
				newMissile.x += 50; // Plane width
				break;
			case 180: // Down
				newMissile.x += 25; // Center of the plane width
				newMissile.y += 50; // Plane height
				break;
			case 270: // Left
				newMissile.y += 25; // Center of the plane height
				break;
			}
			missiles.push_back(newMissile);
			Mix_PlayChannel(-1, g_missileSound, 0);
			g_score += 10;
		}
		g_input = 0;
	}

	

}


/////////////////////////////////////////////////////////////
// Update() 
// 게임의 내용을 업데이트하는 함수.
// 실제 게임의 룰을 구현해야하는 곳.
// 게임에서 일어나는 변화는 모두 이 곳에서 구현한다.
// main 함수의 while loop에 의해서 무한히 반복 호출된다는 것을 주의.
void Update() {
	if (currentGameState == GAME) {
		if (leftPressed) {
			g_X -= 10;
		}
		if (rightPressed) {
			g_X += 10;
		}
		if (upPressed) {
			g_Y -= 10;
		}
		if (downPressed) {
			g_Y += 10;
		}

		UpdateMissiles();

		if (g_X > maxX) {
			g_X = 0;
		}
		if (g_X < 0) {
			g_X = maxX;
		}
		if (g_Y < 0) {
			g_Y = maxY;
		}
		if (g_Y > maxY) {
			g_Y = 0;
		}

		// 미사일 상태 업데이트
		if (m_state == 1) {
			m_y -= 1; // 미사일은 항상 위로 이동한다..

			if (m_y < 0) {
				m_state = 0; // 미사일이 화면 상단을 넘어가면 다시 비행 중이 아닌 상태로 변경
			}
		}
		else {
			m_x = g_X;
			m_y = g_Y;
		}

		g_elapsed_time_ms += 33;
	}
}




/////////////////////////////////////////////////////////////
// Render() 
// 그림을 그리는 함수.
// main 함수의 while loop에 의해서 무한히 반복 호출된다는 것을 주의.
void Render() {
	SDL_RenderClear(g_renderer);  // 화면 클리어를 한 번만 수행

	SDL_Texture* currentTexture = nullptr;
	switch (currentGameState) {
	case INTRO:
		currentTexture = textureIntro;
		break;
	case GAME:
		currentTexture = textureBackground;
		// 게임 상태일 때 추가 렌더링 로직
		break;
	case ENDING:
		currentTexture = textureEnding;
		break;
	}

	// 배경 텍스처 렌더링
	SDL_Rect backgroundRect = { 0, 0, 800, 600 };
	SDL_RenderCopy(g_renderer, currentTexture, NULL, &backgroundRect);

	if (currentGameState == GAME) {
		COORD cur;
		cur.X = 0;
		cur.Y = 0;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cur);

		// 화면 클리어
		SDL_RenderClear(g_renderer);

		// 배경 렌더링
		SDL_Rect backgroundRect = { 0, 0, 800, 600 }; // 화면 전체 크기로 설정
		SDL_RenderCopy(g_renderer, textureBackground, NULL, &backgroundRect);


		// 비행기 렌더링
		SDL_Rect planeRect = { g_X, g_Y, 50, 50 };
		SDL_RenderCopyEx(g_renderer, texturePlane, NULL, &planeRect, g_plane_angle, NULL, SDL_FLIP_NONE);


		// 미사일 렌더링
		for (const auto& missile : missiles) {
			if (missile.isActive) {
				SDL_Rect missileRect = { missile.x, missile.y, 10, 20 }; // 미사일의 위치와 크기
				// 미사일의 방향에 따라 그림 회전
				double angle = 0.0;
				switch ((int)missile.direction) {
				case 0: // 위쪽
					angle = 0;
					break;
				case 90: // 오른쪽
					angle = 90;
					break;
				case 180: // 아래쪽
					angle = 180;
					break;
				case 270: // 왼쪽
					angle = 270;
					break;
				}
				// 미사일을 방향에 맞게 회전하여 그리기
				SDL_RenderCopyEx(g_renderer, textureMissile, NULL, &missileRect, angle, NULL, SDL_FLIP_NONE);
			}
		}

		// 미사일 수 표시
		std::ostringstream missileCountStream;
		missileCountStream << "" << missileCount;
		SDL_Surface* missileCountSurface = TTF_RenderText_Solid(font, missileCountStream.str().c_str(), whiteColor);
		SDL_Texture* missileCountTexture = SDL_CreateTextureFromSurface(g_renderer, missileCountSurface);
		int missileCountWidth = missileCountSurface->w, missileCountHeight = missileCountSurface->h;
		SDL_FreeSurface(missileCountSurface);
		SDL_Rect missileCountRect = { 800 - 10 - missileCountWidth, 600 - 10 - missileCountHeight, missileCountWidth, missileCountHeight };
		SDL_RenderCopy(g_renderer, missileCountTexture, NULL, &missileCountRect);
		SDL_DestroyTexture(missileCountTexture);

		// 재충전 버튼 렌더링
		if (showReloadButton) {
			// 버튼의 배경을 흰색으로 설정
			SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);  // RGBA for white
			SDL_Rect reloadButtonRect = { reloadButtonX, reloadButtonY, reloadButtonWidth, reloadButtonHeight };
			SDL_RenderFillRect(g_renderer, &reloadButtonRect);

			// "재충전" 텍스트 렌더링
			SDL_Surface* reloadButtonSurface = TTF_RenderUTF8_Solid(font, u8"재충전", { 0, 0, 0 });  // Black text
			SDL_Texture* reloadButtonTexture = SDL_CreateTextureFromSurface(g_renderer, reloadButtonSurface);
			int textWidth = reloadButtonSurface->w;
			int textHeight = reloadButtonSurface->h;
			SDL_FreeSurface(reloadButtonSurface);

			// 텍스트를 버튼 중앙에 위치시킴
			SDL_Rect textRect = { reloadButtonX + (reloadButtonWidth - textWidth) / 2, reloadButtonY + (reloadButtonHeight - textHeight) / 2, textWidth, textHeight };
			SDL_RenderCopy(g_renderer, reloadButtonTexture, NULL, &textRect);
			SDL_DestroyTexture(reloadButtonTexture);
		}


		// std::cout으로 출력한 내용 중, 아직 화면에 표시되 않고 버퍼에 남아 있는 것이 있다면, 모두 화면에 출력되도록 한다.
		std::cout.flush();
	}
	// 화면에 그린 내용을 업데이트
	SDL_RenderPresent(g_renderer);
}



/////////////////////////////////////////////////////////////
// HandleEvents() 
// 이벤트를 처리하는 함수.
// main 함수의 while loop에 의해서 무한히 반복 호출된다는 것을 주의.
void HandleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {

		case SDL_QUIT:
			g_flag_running = false;
			break;

		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_LEFT && !leftPressed) {
				leftPressed = true;
				g_plane_angle = 270;
			}
			else if (event.key.keysym.sym == SDLK_RIGHT && !rightPressed) {
				rightPressed = true;
				g_plane_angle = 90;
			}
			else if (event.key.keysym.sym == SDLK_UP && !upPressed) {
				upPressed = true;
				g_plane_angle = 0;
			}
			else if (event.key.keysym.sym == SDLK_DOWN && !downPressed) {
				downPressed = true;
				g_plane_angle = 180;
			}
			else if (event.key.keysym.sym == SDLK_SPACE) {
				if (event.key.keysym.sym == SDLK_SPACE && !spacePressed && missileCount > 0) {
					spacePressed = true;
					missileCount--;
					Mix_PlayChannel(-1, g_missileSound, 0);  // 미사일 발사 소리 재생

					// 미사일 객체 생성 및 벡터에 추가
					Missile newMissile(g_X+20, g_Y+15, true, g_plane_angle);
					missiles.push_back(newMissile);

					if (missileCount == 0) {
						showReloadButton = true;
					}
				}
			}
			break;

		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_LEFT) leftPressed = false;
			if (event.key.keysym.sym == SDLK_RIGHT) rightPressed = false;
			if (event.key.keysym.sym == SDLK_UP) upPressed = false;
			if (event.key.keysym.sym == SDLK_DOWN) downPressed = false;
			if (event.key.keysym.sym == SDLK_SPACE) spacePressed = false;
			break;

		case SDL_MOUSEBUTTONDOWN: 
			if (event.button.button == SDL_BUTTON_RIGHT) { // 마우스 오른쪽 버튼 처리
				// 게임 상태 변경 로직
				if (currentGameState == INTRO) {
					currentGameState = GAME;
					Mix_HaltMusic();  // 인트로 음악 중지
					// 게임 상태 초기화
					g_X = 375;  // 비행기 초기 위치 (화면 중앙)
					g_Y = 275;
					missileCount = 5;  // 미사일 개수 초기화
					g_plane_angle = 0;  // 비행기의 방향을 '위'로 설정
					// 모든 미사일을 비활성화
					for (auto& missile : missiles) {
						missile.isActive = false;
					}
				}
				else if (currentGameState == GAME) {
					currentGameState = ENDING;
					Mix_PlayMusic(endingMusic, -1); // 엔딩 음악 재생
				}
				else if (currentGameState == ENDING) {
					currentGameState = INTRO;
					Mix_PlayMusic(introMusic, -1); // 인트로 음악 재생
				}
			}

			if (showReloadButton &&
				event.button.x >= reloadButtonX &&
				event.button.x <= reloadButtonX + reloadButtonWidth &&
				event.button.y >= reloadButtonY &&
				event.button.y <= reloadButtonY + reloadButtonHeight) {
				missileCount = 5;
				showReloadButton = false;
				Mix_PlayChannel(-1, g_reloadSound, 0); // 재장전 소리 재생
			}
			break;


		default:
			break;
		}
	}
}

void ClearGame()
{
	SDL_DestroyTexture(textureMissile);
	SDL_DestroyTexture(texturePlane);
	SDL_DestroyTexture(textureBackground);
	SDL_DestroyTexture(textureIntro);
	SDL_DestroyTexture(textureEnding);
	TTF_CloseFont(font);
	TTF_Quit();
	Mix_FreeChunk(g_missileSound);
	Mix_FreeMusic(g_backgroundMusic);
	Mix_FreeMusic(introMusic);
	Mix_FreeMusic(endingMusic);
	Mix_CloseAudio();
}
