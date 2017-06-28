#include "scene_app.h"
#include <system/platform.h>
#include <graphics/sprite_renderer.h>
#include <graphics/font.h>
#include <system/debug_log.h>
#include <graphics/renderer_3d.h>
#include <graphics/mesh.h>
#include <maths/math_utils.h>
#include <input/sony_controller_input_manager.h>
#include <graphics/sprite.h>
#include "load_texture.h"

SceneApp::SceneApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	renderer_3d_(NULL),
	primitive_builder_(NULL),
	input_manager_(NULL),
	audio_manager_(NULL),
	font_(NULL),
	world_(NULL),
	player_body_(NULL),
	sfx_id_(-1),
	sfx_voice_id_(-1),
	button_icon_(NULL)
{
}

void SceneApp::Init()
{
	srand(NULL);
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);
	InitFont();

	// initialise input manager
	input_manager_ = gef::InputManager::Create(platform_);

	// initialise audio manager
	audio_manager_ = gef::AudioManager::Create();

	

	// set the initial state of the game state machine
	game_state_ = FRONTEND;
	FrontendInit();
}

void SceneApp::CleanUp()
{
	delete audio_manager_;
	audio_manager_ = NULL;

	delete input_manager_;
	input_manager_ = NULL;

	CleanUpFont();

	delete sprite_renderer_;
	sprite_renderer_ = NULL;
}

bool SceneApp::Update(float frame_time)
{
	fps_ = 1.0f / frame_time;

	gef::Matrix44 player_transform;

	gef::Matrix44 player_scale_matrix;
	gef::Matrix44 player_rotX_matrix;
	gef::Matrix44 player_rotY_matrix;
	gef::Matrix44 player_rotZ_matrix;

	

	input_manager_->Update();

	switch (game_state_)
	{
		case FRONTEND:
		{
			FrontendUpdate(frame_time);
			break;
		}
		case JUMPER:
		{
			JumperUpdate(frame_time);
			break;
		}
		case GAMEOVER:
		{
			GameOverUpdate(frame_time);
			break;
		}

	}
	// scale 
	//player_scale_matrix.Scale(gef::Vector4(2.0f, 1.0f, 1.0f));
	//player_transform = player_scale_matrix;
	// rotation

	// translation 
	//player_transform.SetTranslation(gef::Vector4(0.0f, 2.0f, 0.0f));

	
	//player_.set_transform(player_transform);

	return true;
}

void SceneApp::Render()
{
	switch (game_state_)
	{
		case FRONTEND:
		{
			FrontendRender();
			break;
		}
		
		case JUMPER:
		{
			JumperRender();
			break;
		}

		case GAMEOVER:
		{
			GameOverRender();
			break;
		}
	}
}


void SceneApp::InitPlayer()
{
	// setup the mesh for the player
	player_.set_mesh(primitive_builder_->GetDefaultCubeMesh());

	// create a physics body for the player
	b2BodyDef player_body_def;
	player_body_def.type = b2_dynamicBody;
	player_body_def.position = b2Vec2(0.0f, 3.0f);

	player_body_ = world_->CreateBody(&player_body_def);

	// create the shape for the player
	b2PolygonShape player_shape;
	player_shape.SetAsBox(0.5f, 0.5f);

	// create the fixture
	b2FixtureDef player_fixture_def;
	player_fixture_def.shape = &player_shape;
	player_fixture_def.density = 1.0f;

	// create the fixture on the rigid body
	player_body_->CreateFixture(&player_fixture_def);

	// update visuals from simulation data
	player_.UpdateFromSimulation(player_body_);

	// create a connection between the rigid body and GameObject
	player_body_->SetUserData(&player_);
}

void SceneApp::CreateObstacle(float startX, float startY, float halfWidth, float halfHeight)
{

	GameObject* newObstacle = new GameObject();
	// setup the mesh for the gameObject
	newObstacle->set_mesh(primitive_builder_->CreateBoxMesh(gef::Vector4(halfWidth, halfHeight, halfWidth)));

	// create a physics body for the object
	b2BodyDef gameObject_body_def;
	gameObject_body_def.type = b2_staticBody;
	gameObject_body_def.position = b2Vec2(startX, startY);

	b2Body* gameObject_body_ = world_->CreateBody(&gameObject_body_def);

	// create the shape for the object
	b2PolygonShape gameObject_shape;
	gameObject_shape.SetAsBox(halfWidth, halfHeight);

	// create the fixture
	b2FixtureDef gameObject_fixture_def;
	gameObject_fixture_def.shape = &gameObject_shape;
	gameObject_fixture_def.density = 1.0f;

	// create the fixture on the rigid body
	gameObject_body_->CreateFixture(&gameObject_fixture_def);

	// update visuals from simulation data
	newObstacle->UpdateFromSimulation(gameObject_body_);

	// create a connection between the rigid body and GameObject
	gameObject_body_->SetUserData(newObstacle);

	obstacles_.push_back(newObstacle);
	obstacle_bodys_.push_back(gameObject_body_);
}

void SceneApp::InitGround()
{
	// ground dimensions
	gef::Vector4 ground_half_dimensions(50.0f, 0.1f, 50.0f);

	// setup the mesh for the ground
	ground_mesh_ = primitive_builder_->CreateBoxMesh(ground_half_dimensions);
	ground_.set_mesh(ground_mesh_);

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;
	body_def.position = b2Vec2(0.0f, -3.5f);

	ground_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(ground_half_dimensions.x(), ground_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;

	// create the fixture on the rigid body
	ground_body_->CreateFixture(&fixture_def);

	// update visuals from simulation data
	ground_.UpdateFromSimulation(ground_body_);
}

void SceneApp::InitFont()
{
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
}

void SceneApp::CleanUpFont()
{
	delete font_;
	font_ = NULL;
}

void SceneApp::DrawHUD()
{
	if(font_)
	{
		// display frame rate
		font_->RenderText(sprite_renderer_, gef::Vector4(850.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "FPS: %.1f", fps_);
	}
}

void SceneApp::SetupLights()
{
	// grab the data for the default shader used for rendering 3D geometry
	gef::Default3DShaderData& default_shader_data = renderer_3d_->default_shader_data();

	// set the ambient light
	default_shader_data.set_ambient_light_colour(gef::Colour(0.25f, 0.25f, 0.25f, 1.0f));

	// add a point light that is almost white, but with a blue tinge
	// the position of the light is set far away so it acts light a directional light
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(-500.0f, 400.0f, 700.0f));
	default_shader_data.AddPointLight(default_point_light);
}

void SceneApp::UpdateSimulation(float frame_time)
{
	// update physics world
	float32 timeStep = 1.0f / 60.0f;

	int32 velocityIterations = 6;
	int32 positionIterations = 2;

	world_->Step(timeStep, velocityIterations, positionIterations);

	// update object visuals from simulation data
	player_.UpdateFromSimulation(player_body_);
	for (int i = 0; i < obstacles_.size(); i++) {
		obstacles_.at(i)->UpdateFromSimulation(obstacle_bodys_.at(i));
	}
	//gameObject_.UpdateFromSimulation(gameObject_body_);

	// don't have to update the ground visuals as it is static

	// collision detection
	// get the head of the contact list
	b2Contact* contact = world_->GetContactList();
	// get contact count
	int contact_count = world_->GetContactCount();

	for (int contact_num = 0; contact_num<contact_count; ++contact_num)
	{
		if (contact->IsTouching())
		{
			// get the colliding bodies
			b2Body* bodyA = contact->GetFixtureA()->GetBody();
			b2Body* bodyB = contact->GetFixtureB()->GetBody();

			// DO COLLISION RESPONSE HERE
			Player* player = NULL;

			GameObject* gameObjectA = NULL;
			GameObject* gameObjectB = NULL;

			gameObjectA = (GameObject*)bodyA->GetUserData();
			gameObjectB = (GameObject*)bodyB->GetUserData();

			if (gameObjectA)
			{
				if (gameObjectA->type() == PLAYER)
				{
					player = (Player*)bodyA->GetUserData();
				}
			}

			if (gameObjectB)
			{
				if (gameObjectB->type() == PLAYER)
				{
					player = (Player*)bodyB->GetUserData();
				}
			}

		}

		// Get next contact point
		contact = contact->GetNext();
	}
}

///////////////////////////////////////////////// FRONTEND STATE FUNCTIONS ///////////////////////////////////////////////////////////////

void SceneApp::FrontendInit()
{
	button_icon_ = CreateTextureFromPNG("playstation-cross-dark-icon.png", platform_);
	Scroller_Bkgrd_ = CreateTextureFromPNG("clouds@2x.png", platform_);

	// initialise the difficulty and score variables
	difficulty = 1;
	score = 0;
}

void SceneApp::FrontendRelease()
{
	delete button_icon_;
	button_icon_ = NULL;
}

void SceneApp::FrontendUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);

	if ((controller->buttons_pressed() & gef_SONY_CTRL_CROSS)||(controller->buttons_pressed() & gef_SONY_CTRL_START))
	{
		// free up all resources use by the frontend 
		FrontendRelease();
		
		// transition to JUMPER
		game_state_ = JUMPER;
		JumperInit();
	}

	if (controller->buttons_pressed() & gef_SONY_CTRL_LEFT && difficulty > 1)
	{
		difficulty--;
	}
	if (controller->buttons_pressed()& gef_SONY_CTRL_RIGHT && difficulty < 10)
	{
		difficulty++;
	}
	
}

void SceneApp::FrontendRender()
{
	sprite_renderer_->Begin();
	
	// Render wall texture
	gef::Sprite wall;
	wall.set_texture(Scroller_Bkgrd_);
	wall.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	wall.set_height(640.0f);
	wall.set_width(1136.0f);
	sprite_renderer_->DrawSprite(wall);
	

	// render "PRESS" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f - 56.0f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"PRESS");

	// Render button icon
	gef::Sprite button;
	button.set_texture(button_icon_);
	button.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	button.set_height(32.0f);
	button.set_width(32.0f);
	sprite_renderer_->DrawSprite(button);

	// render "TO START" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f + 32.0f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"TO START");

	// render "TO START" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f-100, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"\n\nSelect a Difficulty (1 being easiest): %i", (int)difficulty);


	DrawHUD();
	sprite_renderer_->End();
}

///////////////////////////////////////////////// END FRONTEND STATE FUNCTIONS //////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// JUMPER STATE FUNCTIONS ///////////////////////////////////////////////////////////////////////////////////////////

void SceneApp::JumperInit()
{
	// create the renderer for draw 3D geometry
	renderer_3d_ = gef::Renderer3D::Create(platform_);

	// initialise primitive builder to make create some 3D geometry easier
	primitive_builder_ = new PrimitiveBuilder(platform_);

	//Scroller_Bkgrd_ = CreateTextureFromPNG("Side_Scroller_Bkgrd.png", platform_);
	Scroller_Bkgrd_ = CreateTextureFromPNG("clouds@2x.png", platform_);
	//ground_texture_ = CreateTextureFromPNG("cartoon-stone-texture_1110-576.png", platform_);
	
	SetupLights();

	// initialise the physics world
	b2Vec2 gravity(0.0f, -9.81f);
	world_ = new b2World(gravity);

	InitPlayer();
	//CreateObstacle(10.0f, 0.5f, 0.5f, 1.5f);
	InitGround();

	timer_ = new Timer();
	timer_->set_timer(1);// +1000 / difficulty);

	// load audio assets
	if (audio_manager_)
	{
		// load a sound effect
		sfx_id_ = audio_manager_->LoadSample("box_collected.wav", platform_);

		// load in music
		audio_manager_->LoadMusic("music.wav", platform_);

		// play music
		audio_manager_->PlayMusic();
	}
}

void SceneApp::JumperRelease()
{
	// unload audio resources
	if (audio_manager_)
	{
		audio_manager_->StopMusic();
		audio_manager_->UnloadAllSamples();
		sfx_id_ = -1;
		sfx_voice_id_ = -1;
	}

	// destroying the physics world also destroys all the objects within it
	delete world_;
	world_ = NULL;

	delete ground_mesh_;
	ground_mesh_ = NULL;

	delete primitive_builder_;
	primitive_builder_ = NULL;

	delete renderer_3d_;
	renderer_3d_ = NULL;

	delete Scroller_Bkgrd_;
	Scroller_Bkgrd_ = NULL;

	delete timer_;
	timer_ = NULL;

	for (int i = 0; i < obstacles_.size(); i++) 
	{
		delete obstacles_.at(i);
	}

	obstacles_.clear();
	obstacle_bodys_.clear();

}

void SceneApp::JumperUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);

	//increase the score the longer the player lives
	score++;

	// matrix to handle player transforms
	gef::Matrix44 player_transform;
	// matrix to handle gameObject transforms
	gef::Matrix44 gameObject_transform;

	// trigger a sound effect
	if (audio_manager_)
	{
		if (controller->buttons_pressed() & gef_SONY_CTRL_CROSS)
		{
			sfx_voice_id_ = audio_manager_->PlaySample(sfx_id_, true);

			gef::VolumeInfo volume_info;
			volume_info.volume = 0.5f;
			volume_info.pan = -1.0f;

			audio_manager_->SetSampleVoiceVolumeInfo(sfx_voice_id_, volume_info);

			audio_manager_->SetSamplePitch(sfx_voice_id_, 1.5f);
		}

		if (controller->buttons_pressed() & gef_SONY_CTRL_TRIANGLE)
		{
			if (sfx_voice_id_ != -1)
			{
				audio_manager_->StopPlayingSampleVoice(sfx_voice_id_);
				sfx_voice_id_ = -1;
			}
		}
	}
	// End sound effect

	if (timer_->update(frame_time)) 
	{
	 // rand 1/0  + 10*(rand()%2)
		CreateObstacle(10.0f, -1.0f + 6.0f * (rand() % 2), 0.5f, 3.5f);
	}


	// update the simulation
	UpdateSimulation(frame_time);


	// Player force inputs //
	if (controller->buttons_down() & gef_SONY_CTRL_RIGHT)
	{
		// effectively teleports the player 0.1 units to the right, while the right Dpad button is down
		player_body_->SetTransform(b2Vec2((player_body_->GetPosition().x) + 0.1f, player_body_->GetPosition().y), 0);
		gef::DebugOut("RIGHT\n");
	}
	if (controller->buttons_down() & gef_SONY_CTRL_LEFT)
	{
		// effectively teleports the player 0.1 units to the left, while the left Dpad button is down
		player_body_->SetTransform(b2Vec2((player_body_->GetPosition().x) - 0.1f, player_body_->GetPosition().y), 0);
		gef::DebugOut("LEFT\n");
	}
	if (controller->buttons_pressed() & gef_SONY_CTRL_UP)
	{
		// Applies a force upwards to the player object, allowing the player to jump
		player_body_->ApplyLinearImpulse(b2Vec2(0, 10), player_body_->GetWorldCenter(), true);
		gef::DebugOut("UP\n");
	}
	if (controller->buttons_pressed() & gef_SONY_CTRL_DOWN)
	{
		// Applies a force downwards to the player object, allowing the player to duck
		player_body_->ApplyLinearImpulse(b2Vec2(0, -10), player_body_->GetWorldCenter(), true);
		gef::DebugOut("UP\n");
	}
	// End Player Force inputs //

	//increase the difficulty slowly over time
	difficulty += 0.1f*frame_time;
	// effectively teleports the gameobject difficulty/10 units to the left
	for (int i = 0; i < obstacle_bodys_.size(); i++)
	{
		obstacle_bodys_.at(i)->SetTransform(b2Vec2((obstacle_bodys_.at(i)->GetPosition().x) - ((float)difficulty / 10), obstacle_bodys_.at(i)->GetPosition().y), 0);
	}

	// collision detection
	// get the head of the contact list
	b2Contact* contact = world_->GetContactList();
	// get contact count
	int contact_count = world_->GetContactCount();

	for (int contact_num = 0; contact_num<contact_count; ++contact_num)
	{
		if (contact->IsTouching())
		{
			// get the colliding bodies
			b2Body* bodyA = contact->GetFixtureA()->GetBody();
			b2Body* bodyB = contact->GetFixtureB()->GetBody();

			// DO COLLISION RESPONSE HERE
			//end the game
			JumperRelease();
			game_state_ = GAMEOVER;
			GameOverInit();


		}

		// Get next contact point
		//contact = contact->GetNext();
	}
	// get to start menu
	if (controller->buttons_pressed() & gef_SONY_CTRL_START)
	{
		JumperRelease();
		game_state_ = FRONTEND;
		FrontendInit();
	}

	if (controller->buttons_pressed() & gef_SONY_CTRL_SQUARE)
	{
		JumperRelease();
		game_state_ = GAMEOVER;
		GameOverInit();
	}

}

void SceneApp::JumperRender()
{
	sprite_renderer_->Begin();

	

	// Render wall texture
	gef::Sprite wall;
	wall.set_texture(Scroller_Bkgrd_);
	wall.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	wall.set_height(640.0f);
	wall.set_width(1136.0f);
	sprite_renderer_->DrawSprite(wall);
	

	// render "Score" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.75f, platform_.height()*0.15f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"SCORE: %i", score);

	// render "Difficulty" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.25f, platform_.height()*0.15f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"DIFFICULTY: %i", (int)difficulty);
	sprite_renderer_->End();

	// Setup camera

	// projection
	float fov = gef::DegToRad(45.0f);
	float aspect_ratio = (float)platform_.width() / (float)platform_.height();
	gef::Matrix44 projection_matrix;
	projection_matrix = platform_.PerspectiveProjectionFov(fov, aspect_ratio, 0.1f, 100.0f);
	renderer_3d_->set_projection_matrix(projection_matrix);

	// jumper camera view
	
	gef::Vector4 camera_eye(0.0f, 2.0f, 10.0f); // Alternate Cam algle at 2, 6, 10
	gef::Vector4 camera_lookat(0.0f, 2.0f, 0.0f);
	gef::Vector4 camera_up(0.0f, 1.0f, 0.0f);
	gef::Matrix44 view_matrix;
	view_matrix.LookAt(camera_eye, camera_lookat, camera_up);
	renderer_3d_->set_view_matrix(view_matrix);
	// end camera setup

	// draw 3d geometry, but dont clear the frame buffer
	renderer_3d_->Begin(false);

	// draw ground
	renderer_3d_->set_override_material(&primitive_builder_->ground_material());
	renderer_3d_->DrawMesh(ground_);
	renderer_3d_->set_override_material(NULL);

	// draw gameObject
	renderer_3d_->set_override_material(&primitive_builder_->red_material());
	for (int i = 0; i < obstacles_.size(); i++) 
	{
		renderer_3d_->DrawMesh(*obstacles_.at(i));
	}
	
	renderer_3d_->set_override_material(NULL);
	// draw player
	renderer_3d_->set_override_material(&primitive_builder_->green_material());
	renderer_3d_->DrawMesh(player_);
	renderer_3d_->set_override_material(NULL);

	renderer_3d_->End();
	// end 3d drawing

	// start drawing sprites, but don't clear the frame buffer
	sprite_renderer_->Begin(false);
	DrawHUD();
	sprite_renderer_->End();
}

///////////////////////////////////////////////// END JUMPER STATE FUNCTIONS /////////////////////////////////////////////////////////////////

///////////////////////////////////////////////// GAMEOVER STATE FUNCTIONS ///////////////////////////////////////////////////////////////

void SceneApp::GameOverInit()
{
	gameOverScreen = CreateTextureFromPNG("CloudGameOver.png", platform_);
}

void SceneApp::GameOverRelease()
{
	delete gameOverScreen;
	gameOverScreen = NULL;
}

void SceneApp::GameOverUpdate(float frame_time)
{
	const gef::SonyController* controller = input_manager_->controller_input()->GetController(0);

	if(controller->buttons_pressed() & gef_SONY_CTRL_START)
	{
		// free up resources used by GAMEOVER
		GameOverRelease();
		game_state_ = FRONTEND;
		FrontendInit();
	}

}

void SceneApp::GameOverRender()
{
	
	sprite_renderer_->Begin();

	// Render wall texture
	gef::Sprite gameOverBkgrd;
	gameOverBkgrd.set_texture(gameOverScreen);
	gameOverBkgrd.set_position(gef::Vector4(platform_.width()*0.5f, platform_.height()*0.5f, -0.99f));
	gameOverBkgrd.set_height(640.0f);
	gameOverBkgrd.set_width(1136.0f);
	sprite_renderer_->DrawSprite(gameOverBkgrd);

	// render "Final Score" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.5f, platform_.height()*0.3f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"FINAL SCORE: %i w/ difficulty modifier", score*(int)difficulty);

	// render "Score" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.75f, platform_.height()*0.15f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"SCORE: %i", score);

	// render "Difficulty" text
	font_->RenderText(
		sprite_renderer_,
		gef::Vector4(platform_.width()*0.25f, platform_.height()*0.15f, -0.99f),
		1.0f,
		0xff595959,
		gef::TJ_CENTRE,
		"DIFFICULTY %i", (int)difficulty);
	sprite_renderer_->End();
	
	
	
}

///////////////////////////////////////////////// END GAMEOVER STATE FUNCTIONS ///////////////////////////////////////////////////////////////
