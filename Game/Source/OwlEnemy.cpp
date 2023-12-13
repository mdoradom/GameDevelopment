#include "OwlEnemy.h"
#include "App.h"
#include "Map.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Point.h"
#include "Physics.h"

#include "Window.h"
#include <cmath>
#include <iostream>

#ifdef __linux__
#include <Box2D/Common/b2Math.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#endif

void OwlEnemy::setIdleAnimation()
{
	currentAnimation = &idleAnim;
}

void OwlEnemy::setMoveAnimation()
{
	currentAnimation = &flyAnim;
	jumpAnim.Reset();
}

void OwlEnemy::setJumpAnimation()
{
	currentAnimation = &jumpAnim;
}

void OwlEnemy::Move() {
	// TODO move logic
}

void OwlEnemy::Jump() {
	// TODO jump logic
}

bool OwlEnemy::SaveState(pugi::xml_node& node) {

	//pugi::xml_node playerAttributes = node.append_child("player");
	//playerAttributes.append_attribute("x").set_value(this->position.x);
	//playerAttributes.append_attribute("y").set_value(this->position.y);
	//playerAttributes.append_attribute("angle").set_value(this->angle);

	return true;

}

bool OwlEnemy::LoadState(pugi::xml_node& node)
{
	//pbody->body->SetTransform({ PIXEL_TO_METERS(node.child("player").attribute("x").as_int()), PIXEL_TO_METERS(node.child("player").attribute("y").as_int()) }, node.child("player").attribute("angle").as_int());

	// reset enemy physics
	//pbody->body->SetAwake(false);
	//pbody->body->SetAwake(true);

	return true;
}

EntityState OwlEnemy::StateMachine() {
	// TODO state machine logic
	return EntityState::IDLE;
}

OwlEnemy::OwlEnemy() : Entity(EntityType::ENEMY)
{
	name.Create("OwlEnemy");
	state = EntityState::IDLE;
}

OwlEnemy::~OwlEnemy() {

}

bool OwlEnemy::Awake() {

	position.x = parameters.attribute("x").as_int();
	position.y = parameters.attribute("y").as_int();
	texturePath = parameters.attribute("texturepath").as_string();
	newPosition = spawnPosition = position;

	return true;
}

bool OwlEnemy::Start() {

	timer = Timer();
	timer.Start();

	movementDelay = Timer();
	timer.Start();

	//load Animations
	idleAnim = *app->map->GetAnimByName("owl-1-idle");
	idleAnim.speed = 8.0f;
	flyAnim = *app->map->GetAnimByName("owl-1-flying");
	flyAnim.speed = 8.0f;

	currentAnimation = &flyAnim;
	
	pbody = app->physics->CreateCircle(position.x, position.y, 15, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::ENEMY;

	//si quieres dar vueltos como la helice de un helicoptero Boeing AH-64 Apache pon en false la siguiente funcion
	pbody->body->SetFixedRotation(true);
	pbody->body->SetGravityScale(0);

	return true;
}

bool OwlEnemy::Update(float dt)
{

	// Update OwlEnemie state
	StateMachine();
	LOG("Owl Enemie state: %d", state);


	// PATHFINDING LOGIC
	// ------------------------------

	iPoint origin = app->map->WorldToMap(newPosition.x, newPosition.y);

	if (timer.ReadMSec() > 250) {
		iPoint destination = app->map->WorldToMap(app->scene->player->position.x, app->scene->player->position.y);
		app->map->pathfinding->CreatePath(origin, destination);
		timer.Start();
		currentPathPos = 0;
	}
	
	const DynArray<iPoint>* path = app->map->pathfinding->GetLastPath();

	if (movementDelay.ReadMSec() > 100) {
		if (currentPathPos < path->Count())
		{
			newPosition = app->map->MapToWorld(path->At(currentPathPos)->x, path->At(currentPathPos)->y);
			currentPathPos++;
			LOG("%d", currentPathPos);
			movementDelay.Start();
		}
	}

	pbody->body->SetTransform(
		{
			std::lerp(pbody->body->GetPosition().x, PIXEL_TO_METERS(newPosition.x), dt * moveSpeed / 1000),
			std::lerp(pbody->body->GetPosition().y, PIXEL_TO_METERS(newPosition.y), dt * moveSpeed / 1000)

		},

		angle * DEGTORAD
	);
	
	LOG("%d, %d", pbody->body->GetPosition().x, pbody->body->GetPosition().y);


	if (debug)
	{
		//Render Path
		if (path->Count() > 0)
		{
			for (uint i = 0; i < path->Count() - 1; ++i)
			{
				iPoint pos1 = app->map->MapToWorld(path->At(i)->x, path->At(i)->y);
				iPoint pos2 = app->map->MapToWorld(path->At(i + 1)->x, path->At(i + 1)->y);
				app->render->DrawLine(pos1.x, pos1.y, pos2.x, pos2.y, 0, 0, 255);
			}

		}

	//Debug: Render the line between Owl and Player
	app->render->DrawLine(position.x + 27, position.y + 17, app->scene->player->position.x + 20, app->scene->player->position.y + 10, 0, 0, 255);
	}

	// ------------------------------

	//Update OwlEnemie position in pixels
	position.x = METERS_TO_PIXELS(pbody->body->GetTransform().p.x) - 24;
	position.y = METERS_TO_PIXELS(pbody->body->GetTransform().p.y) - 17;

	// Update OwlEnemie sensors

	// Render OwlEnemie texture
	app->render->DrawTexture(currentAnimation->texture, position.x, position.y, &currentAnimation->GetCurrentFrame(), 1.0f, pbody->body->GetAngle()*RADTODEG, flip);

	currentAnimation->Update(dt);
	return true;
}

void OwlEnemy::moveToSpawnPoint() //Yo haria que esta funcion haga que el objetivo del Owl sea el spawnpoint y asi hace el pathfinding
{
	position = spawnPosition;

	pbody->body->SetTransform({ PIXEL_TO_METERS(position.x), PIXEL_TO_METERS(position.y) }, 0);

	// reset OwlEnemie physics
	pbody->body->SetAwake(false);
	pbody->body->SetAwake(true);
}

bool OwlEnemy::CleanUp() {

	app->tex->UnLoad(texture);

	return true;
}

void OwlEnemy::OnCollision(PhysBody* physA, PhysBody* physB) {

	switch (physB->ctype) {

	case ColliderType::ITEM:
		LOG("Collision ITEM");
		break;

	case ColliderType::PLATFORM:
		LOG("Collision PLATFORM");
		break;

	case ColliderType::DEATH:
		LOG("Collision DEATH");
		isAlive = false;
		break;

	case ColliderType::LIMITS:
		LOG("Collision LIMITS");
		break;
	case ColliderType::WIN:
		state = EntityState::WIN;
		LOG("Collision WIN");
		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;

	}

}

void OwlEnemy::EndCollision(PhysBody* physA, PhysBody* physB) {

}