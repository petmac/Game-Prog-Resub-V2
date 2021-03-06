#ifndef _SCENE_APP_H
#define _SCENE_APP_H

#include <system/application.h>
#include <maths/vector2.h>
#include "primitive_builder.h"
#include <graphics/mesh_instance.h>
#include <audio/audio_manager.h>
#include <input/input_manager.h>
#include <box2d/Box2D.h>
#include "game_object.h"
#include <vector>
#include <stdlib.h>
#include "Timer.h"

// enum for game states
enum GAMESTATE 
{
	FRONTEND,			// main menu
	JUMPER,				// 1st game state, Jumper state
	GAMEOVER,			// 2nd game state, game over screen
};

// FRAMEWORK FORWARD DECLARATIONS
namespace gef
{
	class Platform;
	class SpriteRenderer;
	class Font;
	class InputManager;
	class Renderer3D;
}

class SceneApp : public gef::Application
{
public:
	SceneApp(gef::Platform& platform);
	void Init();
	void CleanUp();
	bool Update(float frame_time);
	void Render();
private:
	void InitPlayer();
	void CreateObstacle(float, float, float, float);
	void InitGround();
	void InitFont();
	void CleanUpFont();
	void DrawHUD();
	void SetupLights();
	void UpdateSimulation(float frame_time);
    
	gef::SpriteRenderer* sprite_renderer_;
	gef::Font* font_;
	gef::InputManager* input_manager_;
	gef::AudioManager* audio_manager_;

	//
	// GAME STATE VARIABLES
	//
	GAMESTATE game_state_;

	//
	// FRONTEND DECLARATIONS
	//
	gef::Texture* button_icon_;

	//
	// GAMEOVER DECLARATIONS
	//
	gef::Texture* gameOverScreen;

	//
	// JUMPER DECLARATIONS
	//

	gef::Texture* Scroller_Bkgrd_;
	gef::Texture* ground_texture_;

	gef::Renderer3D* renderer_3d_;
	PrimitiveBuilder* primitive_builder_;

	// create the physics world
	b2World* world_;

	// player variables
	Player player_;
	b2Body* player_body_;
	

	// Obstacle Variables

	std::vector<GameObject*> obstacles_;
	std::vector<b2Body*> obstacle_bodys_;

	Timer* timer_;

	// ground variables
	gef::Mesh* ground_mesh_;
	GameObject ground_;
	b2Body* ground_body_;

	// audio variables
	int sfx_id_;
	int sfx_voice_id_;

	// difficulty variable
	float difficulty;
	unsigned int score;

	float fps_;

	void FrontendInit();
	void FrontendRelease();
	void FrontendUpdate(float frame_time);
	void FrontendRender();

	void JumperInit();
	void JumperRelease();
	void JumperUpdate(float frame_time);
	void JumperRender();
	
	void GameOverInit();
	void GameOverRelease();
	void GameOverUpdate(float frame_time);
	void GameOverRender();
};

#endif // _SCENE_APP_H
