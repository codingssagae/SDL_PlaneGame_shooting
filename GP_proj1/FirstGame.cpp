#include "GameFunc.h"
#include <windows.h>
#include <vector>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include "SDL_image.h" // SDL_image ��� ����
#include "SDL_ttf.h"
#include <sstream> // std::ostringstream�� ����ϱ� ���� �ʿ�
#include <iomanip> // std::setw�� std::setfill�� ����ϱ� ���� �ʿ�
#include "SDL_mixer.h" // SDL_mixer ��� ����
#include <chrono> // �ð� ������ ���� �߰�
using namespace std;


enum GameState {
	INTRO,
	GAME,
	ENDING
};

GameState currentGameState = INTRO;  // �ʱ� ���� ����

int g_input;
std::string g_output;
int missileCount = 5;  // �ʱ� �̻��� ��
bool showReloadButton = false;  // ������ ��ư ǥ�� ����
int reloadButtonWidth = 100; // �ʱ� �ʺ� ������
int reloadButtonHeight = 50; // �ʱ� ���� ������
SDL_Color whiteColor = { 255, 255, 255, 255 };  // RGBA for white
int reloadButtonX = 800 - 110-140; // ȭ�� ������ ������ ��ư�� �ʺ�ŭ ���� 10px ����
int reloadButtonY = 600 - 60; // ȭ�� �ϴܿ��� ��ư�� ���̸�ŭ ���� 10px ����
bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;
bool spacePressed = false;

int g_X;
int g_Y;
// �귯�� �ð� ���
double g_elapsed_time_ms; 
double g_plane_angle = 0;
// �Ҹ� ���� �ҷ�����
Mix_Chunk* g_reloadSound; // ���� ���� ���� 
Mix_Music* introMusic;
Mix_Music* endingMusic;

int m_state;
int m_x, m_y;
int g_score = 0; // ���� ����

SDL_Texture* textureMissile = nullptr;
SDL_Texture* texturePlane = nullptr;
SDL_Texture* textureBackground = nullptr;
SDL_Texture* textureIntro = nullptr;
SDL_Texture* textureEnding = nullptr;

// â�� ũ�⿡ ���� �ִ밪 ����
const int maxX = 800 - 50; // ����� �̹����� �ʺ� ���
const int maxY = 600 - 50; // ����� �̹����� ���̸� ���

TTF_Font* font = nullptr;

struct Missile {
	int x, y; // �̻����� ��ġ
	bool isActive; // �̻����� Ȱ�� �������� ����
	double direction; // �̻����� �߻� ����

	// �⺻ ������
	Missile() : x(0), y(0), isActive(false), direction(0) {}

	// �ʿ��� ����� �ʱ�ȭ�ϴ� ������
	Missile(int initX, int initY, bool active, double dir) : x(initX), y(initY), isActive(active), direction(dir) {}
};

Mix_Chunk* g_missileSound;
Mix_Music* g_backgroundMusic;

std::vector<Missile> missiles;

// ������ �̻��� �߻� �ð��� ����ϴ� ���� ���� �߰�
std::chrono::steady_clock::time_point lastMissileTime = std::chrono::steady_clock::now();


/////////////////////////////////////////////////////////////
// InitGame() 
// ���α׷��� ���۵� �� ���ʿ� �� �� ȣ��Ǵ� �Լ�.
// �� �Լ����� ���ӿ� �ʿ��� �ڿ�(�̹���, ���� ��)�� �ε��ϰ�, ���� �������� �ʱ�ȭ �ؾ��Ѵ�.
void InitGame() {
	g_input = 0;
	g_flag_running = true;

	g_X = 375;
	g_Y = 275;
	g_elapsed_time_ms = 0; 

	// SDL_mixer �ʱ�ȭ
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cout << "SDL_mixer �ʱ�ȭ ����: " << Mix_GetError() << std::endl;
	}  

	// �̻��� �Ҹ��� ��� ������ �ε��մϴ�.
	g_missileSound = Mix_LoadWAV("../../Resources/misasound.wav"); 
	g_backgroundMusic = Mix_LoadMUS("../../Resources/back.mp3"); 
	g_reloadSound = Mix_LoadWAV("../../Resources/reload.wav"); // ������ �Ҹ� ���� ���
	introMusic = Mix_LoadMUS("../../Resources/intro.mp3");
	endingMusic = Mix_LoadMUS("../../Resources/ending.mp3");

	Mix_VolumeChunk(g_missileSound, MIX_MAX_VOLUME / 2); // ���� ���� ����
	Mix_VolumeMusic(MIX_MAX_VOLUME); // ��� ���� ������ �ִ�� ����

	SDL_Surface* tempSurface = IMG_Load("../../Resources/beehang3.png"); // ����� �̹��� �ε�
	texturePlane = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface); // �� �̻� �ʿ���� ���ǽ� ����

	tempSurface = IMG_Load("../../Resources/intro.jpg");
	textureIntro = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface);

	tempSurface = IMG_Load("../../Resources/ending.jpg");
	textureEnding = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface);

	tempSurface = IMG_Load("../../Resources/misa2.png"); // �̻��� �̹��� �ε�
	textureMissile = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface); // ���ǽ� ����

	tempSurface = IMG_Load("../../Resources/space.jpg");
	textureBackground = SDL_CreateTextureFromSurface(g_renderer, tempSurface);
	SDL_FreeSurface(tempSurface);
	
	m_state = 0;

	TTF_Init();
	font = TTF_OpenFont("../../Resources/YEONGJUSeonbi.ttf", 24); // ��Ʈ ���� ��ο� ũ�� ����

	if (currentGameState == INTRO) {
		Mix_PlayMusic(introMusic, -1);
	}

	// std::cout ��¿� ���۸� ����Ͽ�, ��� �ӵ��� �������� ������ ���Ѵ�.
	setvbuf(stdout, NULL, _IOLBF, 4096);

	// ǥ����� ȭ���� ������ �����.
	system("cls");
}

void RenderScore() {
	SDL_Color color = { 255, 255, 255 }; // ������ ���� (���⼭�� ���)

	// ������ �ټ� �ڸ� ���ڷ� �����ϱ�
	/*std::ostringstream scoreStream;
	scoreStream << u8"����: " << std::setfill('0') << std::setw(5) << g_score;
	std::string scoreText = scoreStream.str();

	SDL_Surface* surface = TTF_RenderUTF8_Solid(font, scoreText.c_str(), color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);

	int textWidth = surface->w;
	int textHeight = surface->h;
	SDL_FreeSurface(surface);

	SDL_Rect renderQuad = { 10, 10, textWidth, textHeight }; // ������ ȭ�鿡 ǥ���� ��ġ�� ũ��
	SDL_RenderCopy(g_renderer, texture, NULL, &renderQuad);
	SDL_DestroyTexture(texture); // ��� �� �ؽ�ó ����*/
}

/////////////////////////////////////////////////////////////
// UpdateMissiles() - �̻��� ���¸� ������Ʈ�ϴ� �Լ�
void UpdateMissiles() {
	for (auto& missile : missiles) {
		if (missile.isActive) {
			// �̻����� ���⿡ ���� ��ġ ����
			switch ((int)missile.direction) {
			case 0: // ����
				missile.y -= 25;
				break;
			case 90: // ������
				missile.x += 25;
				break;
			case 180: // �Ʒ���
				missile.y += 25;
				break;
			case 270: // ����
				missile.x -= 25;
				break;
			}

			// ȭ�� ������ ���� �̻��� ��Ȱ��ȭ
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
// ������ ������ ������Ʈ�ϴ� �Լ�.
// ���� ������ ���� �����ؾ��ϴ� ��.
// ���ӿ��� �Ͼ�� ��ȭ�� ��� �� ������ �����Ѵ�.
// main �Լ��� while loop�� ���ؼ� ������ �ݺ� ȣ��ȴٴ� ���� ����.
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

		// �̻��� ���� ������Ʈ
		if (m_state == 1) {
			m_y -= 1; // �̻����� �׻� ���� �̵��Ѵ�..

			if (m_y < 0) {
				m_state = 0; // �̻����� ȭ�� ����� �Ѿ�� �ٽ� ���� ���� �ƴ� ���·� ����
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
// �׸��� �׸��� �Լ�.
// main �Լ��� while loop�� ���ؼ� ������ �ݺ� ȣ��ȴٴ� ���� ����.
void Render() {
	SDL_RenderClear(g_renderer);  // ȭ�� Ŭ��� �� ���� ����

	SDL_Texture* currentTexture = nullptr;
	switch (currentGameState) {
	case INTRO:
		currentTexture = textureIntro;
		break;
	case GAME:
		currentTexture = textureBackground;
		// ���� ������ �� �߰� ������ ����
		break;
	case ENDING:
		currentTexture = textureEnding;
		break;
	}

	// ��� �ؽ�ó ������
	SDL_Rect backgroundRect = { 0, 0, 800, 600 };
	SDL_RenderCopy(g_renderer, currentTexture, NULL, &backgroundRect);

	if (currentGameState == GAME) {
		COORD cur;
		cur.X = 0;
		cur.Y = 0;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cur);

		// ȭ�� Ŭ����
		SDL_RenderClear(g_renderer);

		// ��� ������
		SDL_Rect backgroundRect = { 0, 0, 800, 600 }; // ȭ�� ��ü ũ��� ����
		SDL_RenderCopy(g_renderer, textureBackground, NULL, &backgroundRect);


		// ����� ������
		SDL_Rect planeRect = { g_X, g_Y, 50, 50 };
		SDL_RenderCopyEx(g_renderer, texturePlane, NULL, &planeRect, g_plane_angle, NULL, SDL_FLIP_NONE);


		// �̻��� ������
		for (const auto& missile : missiles) {
			if (missile.isActive) {
				SDL_Rect missileRect = { missile.x, missile.y, 10, 20 }; // �̻����� ��ġ�� ũ��
				// �̻����� ���⿡ ���� �׸� ȸ��
				double angle = 0.0;
				switch ((int)missile.direction) {
				case 0: // ����
					angle = 0;
					break;
				case 90: // ������
					angle = 90;
					break;
				case 180: // �Ʒ���
					angle = 180;
					break;
				case 270: // ����
					angle = 270;
					break;
				}
				// �̻����� ���⿡ �°� ȸ���Ͽ� �׸���
				SDL_RenderCopyEx(g_renderer, textureMissile, NULL, &missileRect, angle, NULL, SDL_FLIP_NONE);
			}
		}

		// �̻��� �� ǥ��
		std::ostringstream missileCountStream;
		missileCountStream << "" << missileCount;
		SDL_Surface* missileCountSurface = TTF_RenderText_Solid(font, missileCountStream.str().c_str(), whiteColor);
		SDL_Texture* missileCountTexture = SDL_CreateTextureFromSurface(g_renderer, missileCountSurface);
		int missileCountWidth = missileCountSurface->w, missileCountHeight = missileCountSurface->h;
		SDL_FreeSurface(missileCountSurface);
		SDL_Rect missileCountRect = { 800 - 10 - missileCountWidth, 600 - 10 - missileCountHeight, missileCountWidth, missileCountHeight };
		SDL_RenderCopy(g_renderer, missileCountTexture, NULL, &missileCountRect);
		SDL_DestroyTexture(missileCountTexture);

		// ������ ��ư ������
		if (showReloadButton) {
			// ��ư�� ����� ������� ����
			SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 255);  // RGBA for white
			SDL_Rect reloadButtonRect = { reloadButtonX, reloadButtonY, reloadButtonWidth, reloadButtonHeight };
			SDL_RenderFillRect(g_renderer, &reloadButtonRect);

			// "������" �ؽ�Ʈ ������
			SDL_Surface* reloadButtonSurface = TTF_RenderUTF8_Solid(font, u8"������", { 0, 0, 0 });  // Black text
			SDL_Texture* reloadButtonTexture = SDL_CreateTextureFromSurface(g_renderer, reloadButtonSurface);
			int textWidth = reloadButtonSurface->w;
			int textHeight = reloadButtonSurface->h;
			SDL_FreeSurface(reloadButtonSurface);

			// �ؽ�Ʈ�� ��ư �߾ӿ� ��ġ��Ŵ
			SDL_Rect textRect = { reloadButtonX + (reloadButtonWidth - textWidth) / 2, reloadButtonY + (reloadButtonHeight - textHeight) / 2, textWidth, textHeight };
			SDL_RenderCopy(g_renderer, reloadButtonTexture, NULL, &textRect);
			SDL_DestroyTexture(reloadButtonTexture);
		}


		// std::cout���� ����� ���� ��, ���� ȭ�鿡 ǥ�õ� �ʰ� ���ۿ� ���� �ִ� ���� �ִٸ�, ��� ȭ�鿡 ��µǵ��� �Ѵ�.
		std::cout.flush();
	}
	// ȭ�鿡 �׸� ������ ������Ʈ
	SDL_RenderPresent(g_renderer);
}



/////////////////////////////////////////////////////////////
// HandleEvents() 
// �̺�Ʈ�� ó���ϴ� �Լ�.
// main �Լ��� while loop�� ���ؼ� ������ �ݺ� ȣ��ȴٴ� ���� ����.
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
					Mix_PlayChannel(-1, g_missileSound, 0);  // �̻��� �߻� �Ҹ� ���

					// �̻��� ��ü ���� �� ���Ϳ� �߰�
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
			if (event.button.button == SDL_BUTTON_RIGHT) { // ���콺 ������ ��ư ó��
				// ���� ���� ���� ����
				if (currentGameState == INTRO) {
					currentGameState = GAME;
					Mix_HaltMusic();  // ��Ʈ�� ���� ����
					// ���� ���� �ʱ�ȭ
					g_X = 375;  // ����� �ʱ� ��ġ (ȭ�� �߾�)
					g_Y = 275;
					missileCount = 5;  // �̻��� ���� �ʱ�ȭ
					g_plane_angle = 0;  // ������� ������ '��'�� ����
					// ��� �̻����� ��Ȱ��ȭ
					for (auto& missile : missiles) {
						missile.isActive = false;
					}
				}
				else if (currentGameState == GAME) {
					currentGameState = ENDING;
					Mix_PlayMusic(endingMusic, -1); // ���� ���� ���
				}
				else if (currentGameState == ENDING) {
					currentGameState = INTRO;
					Mix_PlayMusic(introMusic, -1); // ��Ʈ�� ���� ���
				}
			}

			if (showReloadButton &&
				event.button.x >= reloadButtonX &&
				event.button.x <= reloadButtonX + reloadButtonWidth &&
				event.button.y >= reloadButtonY &&
				event.button.y <= reloadButtonY + reloadButtonHeight) {
				missileCount = 5;
				showReloadButton = false;
				Mix_PlayChannel(-1, g_reloadSound, 0); // ������ �Ҹ� ���
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
