#include "Asteroid.h"
#include "Asteroids.h"
#include "Animation.h"
#include "AnimationManager.h"
#include "GameUtil.h"
#include "GameWindow.h"
#include "GameWorld.h"
#include "GameDisplay.h"
#include "Spaceship.h"
#include "BoundingShape.h"
#include "BoundingSphere.h"
#include "GUILabel.h"
#include "Explosion.h"

// PUBLIC INSTANCE CONSTRUCTORS ///////////////////////////////////////////////

/** Constructor. Takes arguments from command line, just in case. */
Asteroids::Asteroids(int argc, char *argv[])
	: GameSession(argc, argv)
{
	mLevel = 0;
	mAsteroidCount = 0;
}

/** Destructor. */
Asteroids::~Asteroids(void)
{
}

// PUBLIC INSTANCE METHODS ////////////////////////////////////////////////////

/** Start an asteroids game. */
void Asteroids::Start()
{
	// Create a shared pointer for the Asteroids game object - DO NOT REMOVE
	shared_ptr<Asteroids> thisPtr = shared_ptr<Asteroids>(this);

	// Add this class as a listener of the game world
	mGameWorld->AddListener(thisPtr.get());

	// Add this as a listener to the world and the keyboard
	mGameWindow->AddKeyboardListener(thisPtr);

	// Add a score keeper to the game world
	mGameWorld->AddListener(&mScoreKeeper);

	// Add this class as a listener of the score keeper
	mScoreKeeper.AddListener(thisPtr);

	// Create an ambient light to show sprite textures
	GLfloat ambient_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat diffuse_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
	glEnable(GL_LIGHT0);

	Animation *explosion_anim = AnimationManager::GetInstance().CreateAnimationFromFile("explosion", 64, 1024, 64, 64, "explosion_fs.png");
	Animation *asteroid1_anim = AnimationManager::GetInstance().CreateAnimationFromFile("asteroid1", 128, 8192, 128, 128, "asteroid1_fs.png");
	Animation *spaceship_anim = AnimationManager::GetInstance().CreateAnimationFromFile("spaceship", 128, 128, 128, 128, "spaceship_fs.png");

	//Create the GUI
	CreateGUI();

	// Hide GUI elements for start screen
	mLivesLabel->SetVisible(false);
	mScoreLabel->SetVisible(false);
	mHighScoreLabel->SetVisible(false);

	// Add a player (watcher) to the game world
	mGameWorld->AddListener(&mPlayer);

	// Add this class as a listener of the player
	mPlayer.AddListener(thisPtr);

	// Start the game
	GameSession::Start();
}

/** Stop the current game. */
void Asteroids::Stop()
{
	// Stop the game
	GameSession::Stop();
}

// PUBLIC INSTANCE METHODS IMPLEMENTING IKeyboardListener /////////////////////
int start = 0;	// Required so that the spaceship doesn't duplicate
int demo = 0;	// Required so that the demo doesn't duplicate
void Asteroids::OnKeyPressed(uchar key, int x, int y)
{
	switch (key)
	{
	case ' ':
		if (start == 1)
		{
			mSpaceship->Shoot();
		}
		if (start == 0 && demo == 0)
		{
			mDemoModeLabel->SetVisible(false);

			mGameWorld->AddObject(CreateSpaceship());
			SetTimer(200, DEMO_MODE);
			demo = 1;
		}
		break;
	case '\r': // '\r' represents the Enter key
		if (start == 0)
		{
			mGameTitleLabel->SetVisible(false);
			mStartScreenLabel->SetVisible(false);
			mDemoModeLabel->SetVisible(false);
			mScoreLabel->SetVisible(true);
			mHighScoreLabel->SetVisible(true);
			mLivesLabel->SetVisible(true);

			mSpaceship->Reset();
			mGameWorld->AddObject(mSpaceship);
			CreateAsteroids(10);

			start = 1;
		}
		break;
	default:
		break;
	}
}

void Asteroids::OnKeyReleased(uchar key, int x, int y) {}

void Asteroids::OnSpecialKeyPressed(int key, int x, int y)
{

	switch (key)
	{
	// If up arrow key is pressed start applying forward thrust
	case GLUT_KEY_UP: mSpaceship->Thrust(10); break;
	// If left arrow key is pressed start rotating anti-clockwise
	case GLUT_KEY_LEFT: mSpaceship->Rotate(90); break;
	// If right arrow key is pressed start rotating clockwise
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(-90); break;
	// Default case - do nothing
	default: break;
	}
}

void Asteroids::OnSpecialKeyReleased(int key, int x, int y)
{
	// The spaceship can only be controlled when the player starts the game
	// This is so that the spaceship doesn't move when the player is in demo mode
	if (start == 1) {
		switch (key)
		{
			// If up arrow key is released stop applying forward thrust
		case GLUT_KEY_UP: mSpaceship->Thrust(0); break;
			// If left arrow key is released stop rotating
		case GLUT_KEY_LEFT: mSpaceship->Rotate(0); break;
			// If right arrow key is released stop rotating
		case GLUT_KEY_RIGHT: mSpaceship->Rotate(0); break;
			// Default case - do nothing
		default: break;
		}
	}
}


// PUBLIC INSTANCE METHODS IMPLEMENTING IGameWorldListener ////////////////////

void Asteroids::OnObjectRemoved(GameWorld* world, shared_ptr<GameObject> object)
{
	if (object->GetType() == GameObjectType("Asteroid"))
	{
		shared_ptr<GameObject> explosion = CreateExplosion();
		explosion->SetPosition(object->GetPosition());
		explosion->SetRotation(object->GetRotation());
		mGameWorld->AddObject(explosion);
		mAsteroidCount--;
		if (mAsteroidCount <= 0) 
		{ 
			SetTimer(500, START_NEXT_LEVEL); 
		}
	}
}

// PUBLIC INSTANCE METHODS IMPLEMENTING ITimerListener ////////////////////////

void Asteroids::OnTimer(int value)
{
	if (value == CREATE_NEW_PLAYER)
	{
		mSpaceship->Reset();
		mGameWorld->AddObject(mSpaceship);
	}

	if (value == START_NEXT_LEVEL)
	{
		mLevel++;
		int num_asteroids = 10 + 2 * mLevel;
		CreateAsteroids(num_asteroids);
	}

	if (value == SHOW_GAME_OVER)
	{
		mGameOverLabel->SetVisible(true);
	}

	if (start == 0) { // When the player launches the game
		if (value == DEMO_MODE) {
			int i = (rand() % 4); // generate a random number between 0 and 3
			switch (i) {
			case 0: // move up
				mSpaceship->Thrust(10);
				break;
			case 1: // move down
				mSpaceship->Thrust(-10);
				break;
			case 2: // rotate left
				mSpaceship->Rotate(90);
				break;
			case 3: // rotate right
				mSpaceship->Rotate(-90);
				break;
			}
			mSpaceship->Shoot(); // always shoot when in demo mode
			SetTimer(250, DEMO_MODE);
		}
	}
}

// PROTECTED INSTANCE METHODS /////////////////////////////////////////////////
shared_ptr<GameObject> Asteroids::CreateSpaceship()
{
	// Create a raw pointer to a spaceship that can be converted to
	// shared_ptrs of different types because GameWorld implements IRefCount
	mSpaceship = make_shared<Spaceship>();
	mSpaceship->SetBoundingShape(make_shared<BoundingSphere>(mSpaceship->GetThisPtr(), 4.0f));
	shared_ptr<Shape> bullet_shape = make_shared<Shape>("bullet.shape");
	mSpaceship->SetBulletShape(bullet_shape);
	Animation *anim_ptr = AnimationManager::GetInstance().GetAnimationByName("spaceship");
	shared_ptr<Sprite> spaceship_sprite =
		make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	mSpaceship->SetSprite(spaceship_sprite);
	mSpaceship->SetScale(0.1f);
	// Reset spaceship back to centre of the world
	mSpaceship->Reset();
	// Return the spaceship so it can be added to the world
	return mSpaceship;
}

void Asteroids::CreateAsteroids(const uint num_asteroids)
{
	mAsteroidCount = num_asteroids;
	for (uint i = 0; i < num_asteroids; i++)
	{
		Animation *anim_ptr = AnimationManager::GetInstance().GetAnimationByName("asteroid1");
		shared_ptr<Sprite> asteroid_sprite
			= make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
		asteroid_sprite->SetLoopAnimation(true);
		shared_ptr<GameObject> asteroid = make_shared<Asteroid>();
		asteroid->SetBoundingShape(make_shared<BoundingSphere>(asteroid->GetThisPtr(), 10.0f));
		asteroid->SetSprite(asteroid_sprite);
		asteroid->SetScale(0.2f);
		mGameWorld->AddObject(asteroid);
	}
}

void Asteroids::CreateGUI()
{
	// Create a new GUILabel and wrap it up in a shared_ptr
	mGameTitleLabel = shared_ptr<GUILabel>(new GUILabel("Asteroids"));
	// Set the horizontal alignment of the label to GUI_HALIGN_CENTER
	mGameTitleLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	// Set the vertical alignment of the label to GUI_VALIGN_MIDDLE
	mGameTitleLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	// Set the visibility of the label to true (visible)
	mGameTitleLabel->SetVisible(true);
	// Add the GUILabel to the GUIContainer  
	shared_ptr<GUIComponent> game_title_component
		= static_pointer_cast<GUIComponent>(mGameTitleLabel);
	mGameDisplay->GetContainer()->AddComponent(game_title_component, GLVector2f(0.5f, 0.75f));

	// Add a (transparent) border around the edge of the game display
	mGameDisplay->GetContainer()->SetBorder(GLVector2i(10, 10));
	// Create a new GUILabel and wrap it up in a shared_ptr
	mScoreLabel = make_shared<GUILabel>("Score: 0");
	// Set the vertical alignment of the label to GUI_VALIGN_TOP
	mScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_TOP);
	// Add the GUILabel to the GUIComponent  
	shared_ptr<GUIComponent> score_component
		= static_pointer_cast<GUIComponent>(mScoreLabel);
	mGameDisplay->GetContainer()->AddComponent(score_component, GLVector2f(0.0f, 1.0f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mHighScoreLabel = make_shared<GUILabel>("High Score: 0");
	// Set the vertical alignment of the label to GUI_VALIGN_TOP
	mHighScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_TOP);
	// Add the GUILabel to the GUIComponent  
	shared_ptr<GUIComponent> high_score_component
		= static_pointer_cast<GUIComponent>(mHighScoreLabel);
	mGameDisplay->GetContainer()->AddComponent(high_score_component, GLVector2f(0.6f, 1.0f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mLivesLabel = make_shared<GUILabel>("Lives: 3");
	// Set the vertical alignment of the label to GUI_VALIGN_BOTTOM
	mLivesLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_BOTTOM);
	// Add the GUILabel to the GUIComponent  
	shared_ptr<GUIComponent> lives_component = static_pointer_cast<GUIComponent>(mLivesLabel);
	mGameDisplay->GetContainer()->AddComponent(lives_component, GLVector2f(0.0f, 0.0f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mGameOverLabel = shared_ptr<GUILabel>(new GUILabel("GAME OVER"));
	// Set the horizontal alignment of the label to GUI_HALIGN_CENTER
	mGameOverLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	// Set the vertical alignment of the label to GUI_VALIGN_MIDDLE
	mGameOverLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	// Set the visibility of the label to false (hidden)
	mGameOverLabel->SetVisible(false);
	// Add the GUILabel to the GUIContainer  
	shared_ptr<GUIComponent> game_over_component
		= static_pointer_cast<GUIComponent>(mGameOverLabel);
	mGameDisplay->GetContainer()->AddComponent(game_over_component, GLVector2f(0.5f, 0.5f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mStartScreenLabel = shared_ptr<GUILabel>(new GUILabel("Press 'Enter' to start"));
	// Set the horizontal alignment of the label to GUI_HALIGN_CENTER
	mStartScreenLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	// Set the vertical alignment of the label to GUI_VALIGN_MIDDLE
	mStartScreenLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	// Set the visibility of the label to true (visible)
	mStartScreenLabel->SetVisible(true);
	// Add the GUILabel to the GUIContainer  
	shared_ptr<GUIComponent> start_screen_component
		= static_pointer_cast<GUIComponent>(mStartScreenLabel);
	mGameDisplay->GetContainer()->AddComponent(start_screen_component, GLVector2f(0.5f, 0.5f));

	// Create a new GUILabel and wrap it up in a shared_ptr
	mDemoModeLabel = shared_ptr<GUILabel>(new GUILabel("Press 'Space' for demo mode"));
	// Set the horizontal alignment of the label to GUI_HALIGN_CENTER
	mDemoModeLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	// Set the vertical alignment of the label to GUI_VALIGN_MIDDLE
	mDemoModeLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	// Set the visibility of the label to true (visible)
	mDemoModeLabel->SetVisible(true);
	// Add the GUILabel to the GUIContainer  
	shared_ptr<GUIComponent> demo_mode_component
		= static_pointer_cast<GUIComponent>(mDemoModeLabel);
	mGameDisplay->GetContainer()->AddComponent(demo_mode_component, GLVector2f(0.5f, 0.4f));
}

int highScore;
void Asteroids::OnScoreChanged(int score)
{
	// Format the score message using an string-based stream
	std::ostringstream msg_stream;
	msg_stream << "Score: " << score;

	// Get the score message as a string
	std::string score_msg = msg_stream.str();
	mScoreLabel->SetText(score_msg);

	// Checks if there is an existing high score
	std::ifstream file("highscore.txt");
	if (file.is_open()) {
		file >> highScore;
		file.close();
	}

	// Check if current score is greater than high score
	if (score > highScore)
	{
		// Update high score in the file
		std::ofstream outfile("highscore.txt");
		outfile << score;
		outfile.close();

		// Update current high score in-game
		std::ostringstream msg_stream;
		msg_stream << "High Score: " << score;
		std::string high_score_msg = msg_stream.str();
		mHighScoreLabel->SetText(high_score_msg);

		// Update high score
		highScore = score;
	}
	else
	{
		// Update current high score in-game with the high score from file
		std::ostringstream msg_stream;
		msg_stream << "High Score: " << highScore;
		std::string high_score_msg = msg_stream.str();
		mHighScoreLabel->SetText(high_score_msg);
	}
}

void Asteroids::OnPlayerKilled(int lives_left)
{
	shared_ptr<GameObject> explosion = CreateExplosion();
	explosion->SetPosition(mSpaceship->GetPosition());
	explosion->SetRotation(mSpaceship->GetRotation());
	mGameWorld->AddObject(explosion);

	// Format the lives left message using an string-based stream
	std::ostringstream msg_stream;
	msg_stream << "Lives: " << lives_left;
	// Get the lives left message as a string
	std::string lives_msg = msg_stream.str();
	mLivesLabel->SetText(lives_msg);

	if (lives_left > 0) 
	{ 
		SetTimer(1000, CREATE_NEW_PLAYER); 
	}
	else
	{
		SetTimer(500, SHOW_GAME_OVER);
	}
}

shared_ptr<GameObject> Asteroids::CreateExplosion()
{
	Animation *anim_ptr = AnimationManager::GetInstance().GetAnimationByName("explosion");
	shared_ptr<Sprite> explosion_sprite =
		make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	explosion_sprite->SetLoopAnimation(false);
	shared_ptr<GameObject> explosion = make_shared<Explosion>();
	explosion->SetSprite(explosion_sprite);
	explosion->Reset();
	return explosion;
}




