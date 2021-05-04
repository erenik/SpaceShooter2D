/// Emil Hedemalm
/// 2015-01-21
/// Weapon..

#include "../SpaceShooter2D.h"

#include "File/File.h"
#include "File/LogFile.h"
#include "String/StringUtil.h"

#include "TextureManager.h"
#include "Graphics/GraphicsProperty.h"
#include "Model/ModelManager.h"

#include "PlayerShip.h"

#include "Entity/EntityManager.h"
#include "Entity/Entity.h"
#include "PlayingLevel.h"

Vector4f defaultAlliedProjectileColor = Vector4f(1.f, 0.5f, .1f, 1.f);

String toString(Weapon::Type type) { 
	return Weapon::GetTypeName(type); 
};

WeaponSet::WeaponSet()
{

}
WeaponSet::~WeaponSet()
{
	ClearAndDelete();
}
WeaponSet::WeaponSet(const WeaponSet & otherWeaponSet)
{
	// Create duplicates of all weapons!
	for (int i = 0; i < otherWeaponSet.Size(); ++i)
	{
		Weapon * weap = new Weapon(*otherWeaponSet[i]);
		AddItem(weap);
	}
}

Weapon::Weapon()
{
	type = Type::None;
	enabled = true;
	circleSpam = false;
	linearScaling = 1;
	currCooldownMs = 0;
	linearDamping = 1.f;
	stability = 1.f;
	numberOfProjectiles = 1;
	relativeStrength = 1.f;
	explosionRadius = 0;
	penetration = 0;
	burst = false;
	reloading = false;
	aim = false;
	currentAim = Vector3f(1, 0, 0);
	shotsLeft = 0;
	estimatePosition = false;
	projectilePath = STRAIGHT;
	burstStart = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);
	cooldown = Time(TimeType::MILLISECONDS_NO_CALENDER, 1000);
	damage = 5;
	angle = 0;
	homingFactor = 0;
	projectileSpeed = 5.f;
	arcDelay = 1000;
	distribution = CONE;
	burstRounds = 3;
	burstRoundsShot = 0;
	burstRoundDelay = Time(TimeType::MILLISECONDS_NO_CALENDER, 50);
	lifeTimeMs = 5000;
	ammunition = 0;
	acceleration = 0;
}

String Weapon::TypeName() {
	return GetTypeName(type);
}

List<Weapon> Weapon::GetAllOfType(Type type) {
	List<Weapon> ofType;
	for (int i = 0; i < types.Size(); ++i) {
		if (types[i].type == type)
			ofType.Add(types[i]);
	}
	return ofType;
}


String Weapon::GetTypeName(Type type) {
	switch (type) {
	case Type::MachineGun:
		return "Machine Gun";
	case Type::SmallRockets:
		return "Small Rockets";
	case Type::BigRockets:
		return "Big Rockets";
	case Type::Lightning:
		return "Lightning";
	case Type::HeatWave:
		return "Heat wave";
	case Type::IonCannon:
		return "Ion cannon";
	case Type::LaserBeam:
		return "Laser beam";
	case Type::LaserBurst:
		return "Burst laser";
	default:
		assert(false);
		return "Implement Me";
	}
}

Weapon::Type Weapon::ParseType(String fromString) {
	fromString.SetComparisonMode(String::NOT_CASE_SENSITIVE);
	if (fromString == GetTypeName(Type::MachineGun))
		return Type::MachineGun;
	else if (fromString == GetTypeName(Type::SmallRockets))
		return Type::SmallRockets;
	else if (fromString == GetTypeName(Type::BigRockets))
		return Type::BigRockets;
	else if (fromString == GetTypeName(Type::Lightning))
		return Type::Lightning;
	else if (fromString == GetTypeName(Type::HeatWave))
		return Type::HeatWave;
	else if (fromString == GetTypeName(Type::IonCannon))
		return Type::IonCannon;
	else if (fromString == GetTypeName(Type::LaserBeam))
		return Type::LaserBeam;
	else if (fromString == GetTypeName(Type::LaserBurst))
		return Type::LaserBurst;
//	else if (fromString == GetTypeName(Type::))
	LogMain("Could find no type for string " + fromString, WARNING);
	return Type::None;
}

// Checks type and level in GameVars
bool Weapon::PlayerOwns(Weapon& weapon) {
	GameVar * var = GameVars.Get(weapon.name);
	if (var == nullptr)
		return false;
	return var->iValue > 0;
}

void Weapon::SetOwnedQuantity(Weapon& weapon, int ownedQuantity) {
	GameVars.SetInt(weapon.name, ownedQuantity);
}

Weapon Weapon::StartingWeapon() {
	return types[0];
}

bool Weapon::Get(String byName, Weapon * weapon)
{
	for (int i = 0; i < types.Size(); ++i)
	{
		Weapon & weap = types[i];
		if (weap.name == byName)
		{
			*weapon = weap;
			return true;
		}
	}
	return false;
}

/** For player-based, returns pointer, but should be used as reference only (*-dereference straight away). 
	Returns 0 if it doesn't exist. */
const Weapon * const Weapon::Get(Type type, int level)
{
	for (int i = 0; i < types.Size(); ++i)
	{
		Weapon & weap = types[i];
		if (weap.type == type && weap.level == level)
		{
			return & weap;
		}
	}
	LogMain("Unable to grab weapon "+ GetTypeName(type) +" of level "+String(level)+" among "+ String(types.Size())+" types.", ERROR);
//	assert(false);
	return 0;
}


bool Weapon::LoadTypes(String fromFile)
{
	List<String> lines = File::GetLines(fromFile);
	if (lines.Size() == 0)
		return false;

	String separator;
	/// Column-names. Parse from the first line.
	List<String> columns;
	String firstLine = lines[0];
	// Keep empty strings or all will break.
	char delimiter = FindCSVDelimiter(firstLine);
	columns = TokenizeCSV(firstLine, delimiter);
	LogMain("Loading weapons from file: "+fromFile, INFO);

	List<String> values;

	// For each line after the first one, parse data.
	for (int j = 1; j < lines.Size(); ++j)
	{
		String & line = lines[j];
		// Keep empty strings or all will break.
		List<String> values = TokenizeCSV(line, delimiter);
		// If not, now loop through the words, parsing them according to the column name.
		// First create the new spell to load the data into!
		Weapon weapon;
		for (int k = 0; k < values.Size(); ++k)
		{
			String column;
			bool error = false;
			/// In-case extra data is added beyond the columns defined above..?
			if (columns.Size() > k)
				column = columns[k];
			String value = values[k];
			column.SetComparisonMode(String::NOT_CASE_SENSITIVE);
			if (column == "Weapon Name")
				weapon.name = value;
			else if (column == "Cost")
				weapon.cost = value.ParseInt();
			else if (column == "Id")
				weapon.id = value;
			else if (column == "Type")
				weapon.type = ParseType(value);
			else if (column == "ShootSFX")
				weapon.shootSFX = value;
			else if (column == "HitSFX")
				weapon.hitSFX = value;
			else if (column == "Acceleration")
				weapon.acceleration = value.ParseFloat();
			else if (column == "Level")
				weapon.level = value.ParseInt();
			else if (column == "Explosion Radius")
				weapon.explosionRadius = value.ParseFloat();
			else if (column == "Linear Scaling")
				weapon.linearScaling = value.ParseFloat();
			else if (column == "Cooldown")
				weapon.cooldown = Time::Milliseconds((int) value.ParseFloat());
			else if (column == "Penetration")
				weapon.penetration = value.ParseFloat();
			else if (column == "lifeTimeMs")
				weapon.lifeTimeMs = value.ParseInt();
			else if (column == "Burst")
				weapon.burst = value.ParseBool();
			else if (column == "Burst details")
			{
				List<String> tokens = value.Tokenize(",");
				if (tokens.Size() >= 2)
				{
					weapon.burstRoundDelay = Time::Milliseconds(tokens[0].ParseInt());
					weapon.burstRounds = tokens[1].ParseInt();
				}
			}
			else if (column == "Number of Projectiles")
				weapon.numberOfProjectiles = value.ParseInt();
			else if (column == "Stability")
			{
				weapon.stability = value.ParseFloat();
//				LogMain("Weapon "+weapon.name+" stability: "+String(weapon.stability), INFO);
			}
			else if (column == "Linear Damping") {
				weapon.linearDamping = value.ParseFloat();
				assert(weapon.linearDamping > 0);
			}
			else if (column == "HomingFactor")
				weapon.homingFactor = value.ParseFloat();
			else if (column == "Angle")
			{
				if (value.Contains("Aim"))
					weapon.aim = true;
				else if (value.Contains("Predict"))
					weapon.estimatePosition = true;
				else if (value.Contains("Circle"))
					weapon.circleSpam = true;
				else 
					weapon.angle = value.ParseInt();
			}
			else if (column == "Projectile path")
			{
				if (value == "Homing")
					weapon.projectilePath = HOMING;
				else if (value == "Spinning outward")
					weapon.projectilePath = SPINNING_OUTWARD;
			}
			else if (column == "Projectile speed")
				weapon.projectileSpeed = value.ParseFloat();
			else if (column == "Damage")
				weapon.damage = value.ParseFloat();
			else if (column == "Abilities")
			{
				// la...
			}
			else if (column == "Ability Trigger")
			{
				// lall..
			}
			else if (column == "Projectile Shape")
				weapon.projectileShape = value;
			else if (column == "Projectile Scale")
				weapon.projectileScale = value.ParseFloat();
			else if (column == "Max Range")
				weapon.maxRange = value.ParseFloat();
			else if (column == "Arc Delay")
				weapon.arcDelay = (int) value.ParseFloat();
			else if (column == "Max Bounces")
				weapon.maxBounces = (int) value.ParseFloat();
			else 
			{
		//		std::cout<<"\nUnknown column D:";
			}
			if (error)
			{
				std::cout<<"\n .. when parsing line \'"<<line<<"\'";
			}
		}
		// Check for pre-existing ship of same name, remove it if so.
		for (int i = 0; i < types.Size(); ++i)
		{
			Weapon & type = types[i];
			if (type.name == weapon.name)
			{
				types.RemoveIndex(i);
				--i;
			}
		}
		if (weapon.name.Length() == 0) {
			continue;
		}
		if (weapon.projectileShape.Length() == 0) {
			LogMain("Weapon " + weapon.name + " missing projectile shape. Skipping.", ERROR);
		}
		LogMain("Weapon loaded: "+weapon.name, DEBUG);
		types.Add(weapon);

		switch (weapon.type) {
			case Weapon::Type::None:
				break;
			default: {
				LogMain("Adding loaded weapon to Gear list as well.", INFO);
				Gear gear;
				gear.name = weapon.name;
				gear.type = Gear::Type::Weapon;
				gear.weapon = weapon;
				gear.price = weapon.cost;
				Gear::weapons.Add(gear);
				break;
			}
		}


	}
	return true;
}

Random shootRand;

/// Moves the aim of this weapon turrent.
void Weapon::Aim(PlayingLevel& playingLevel, Ship* ship)
{
	// If no aim, just align it with the ship?
	if (!aim)
		return;

	Entity* target = NULL;
	// Aim.
	if (ship->allied)
	{
		assert(false && "Implement. Press ignore to continue anyway.");
	}
	else 
	{
		if (playingLevel.playerShip)
			target = playingLevel.playerShip->entity;
	}
	if (target == NULL)
		return;
	Entity* shipEntity = ship->entity;
	// Estimate position upon impact?
	Vector3f targetPos = target->worldPosition;
	Vector3f toTarget = targetPos - weaponWorldPosition;
	if (estimatePosition)
	{
		float dist = toTarget.Length();
		// Check velocity of target.
		Vector3f vel = target->Velocity();
		// Estimated position upon impact... wat.
		float seconds = dist / projectileSpeed;
		Vector3f estimatedPosition = targetPos + vel * seconds;
		toTarget = estimatedPosition - weaponWorldPosition;
	}
	// Aim at the player.
	currentAim = toTarget.NormalizedCopy();
}

/// Based on ship.
Vector3f Weapon::WorldPosition(Entity* basedOnShipEntity)
{
	Vector3f worldPos = basedOnShipEntity->transformationMatrix * location;
	worldPos.z = 0;
	Vector3f ref = basedOnShipEntity->worldPosition;
	return worldPos;
}

/// Shoots using previously calculated aim.
void Weapon::Shoot(PlayingLevel& playingLevel, Ship* ship)
{
	if (!enabled)
		return;
	float firingSpeedDivisor = ship->activeSkill == ATTACK_FRENZY? 0.8f : 1.f;  // +20% firing speed
	if (currCooldownMs - ship->weaponCooldownBonus > 0)
		return;

	if (burst)
	{
		/// Mid-burst?
		if (shotsLeft)
		{
			--shotsLeft;
			// Set next cooldown to be the burst delay.
			currCooldownMs = (int) burstRoundDelay.Milliseconds();
		}
		/// End of burst.
		else {
			reloading = true;
			currCooldownMs = (int) cooldown.Milliseconds();
		}
	}
	else
		currCooldownMs = (int) cooldown.Milliseconds();

	// Shoot!
	if (type == Type::Lightning)
	{
		/// Create a lightning storm...!
		ProcessLightning(playingLevel, ship, true);
//		lastShot = flyTime;
		return;
	}

	for (int i = 0; i < numberOfProjectiles; ++i)
	{
		Entity* shipEntity = ship->entity;
		Color color;
		if (ship->allied)
			color = defaultAlliedProjectileColor;
		else
			color = Vector4f(0.8f,0.7f,0.1f,1.f);
		Texture * tex = TexMan.GetTextureByColor(color);
		// Grab model.
		Model * model = ModelMan.GetModel("obj/Proj/"+projectileShape);
		if (!model)
			model = ModelMan.GetModel("sphere.obj");

		float flakMultiplier = 1.f, flakDividendMultiplier = 1.f;
		if (type == Type::IonCannon)
		{
			flakMultiplier = shootRand.Randf(1.f) + 1.f;
			flakDividendMultiplier = 1 / flakMultiplier;
		}
		Entity* projectileEntity = EntityMan.CreateEntity(name + " Projectile", model, tex);
		ProjectileProperty * projProp = new ProjectileProperty(*this, projectileEntity, ship->enemy);
		projProp->weapon.damage *= flakMultiplier;
		projectileEntity->properties.Add(projProp);
		// Set scale and position.
		projectileEntity->localPosition = weaponWorldPosition;
		projectileEntity->SetScale(Vector3f(1,1,1) * projectileScale * flakMultiplier);
		projProp->color = color;
		projectileEntity->RecalculateMatrix();
		// pew
		Vector3f dir(-1.f,0,0);
		// For defaults of forward, invert for player
		if (ship->allied)
			dir = currentAim;
		if (aim)
		{
			dir = currentAim;
		}
		// Angle, +180
		else if (angle)
		{
			/// Grab current forward-vector.
			Vector3f forwardDir = shipEntity->transformationMatrix.Product(Vector4f(0,0,-1,1)).NormalizedCopy();
			/// Get angles from current dir.
			float angleDegrees = GetAngled(forwardDir.x, forwardDir.y);

			float worldAngle = DEGREES_TO_RADIANS((float)angleDegrees + 180 + angle);
			dir[0] = cos(worldAngle);
			dir[1] = sin(worldAngle);
		}
		/// Change initial direction based on stability of the weapon?
		float currStab = stability;
		if (ship->activeSkill == ATTACK_FRENZY)
			currStab *= 0.95f;
		// Get orthogonal direction.
		Vector3f dirRight = dir.CrossProduct(Vector3f(0,0,1));
		if (currStab < 1.f && type != Type::LaserBeam)
		{
			float amplitude = 1 - currStab;
			float randomEffect = shootRand.Randf(amplitude * 2.f) - amplitude;
			dir = dir + dirRight * randomEffect;
			dir.Normalize();
		}
		projProp->direction = dir; // For autoaim initial direction.

		Vector3f vel = dir * (projectileSpeed + ship->projectileSpeedBonus) * flakDividendMultiplier;
		vel.z = 0;
		PhysicsProperty * pp = projectileEntity->physics = new PhysicsProperty();
		pp->type = PhysicsType::DYNAMIC;
		pp->velocity = vel;
		pp->collisionCallback = true;
		pp->maxCallbacks = -1; // unlimited callbacks or penetrating projs won't work
		pp->faceVelocityDirection = false;
		pp->velocitySmoothing = 0.f;
		// Set collision category and filter.
		pp->collisionCategory = ship->allied? CC_PLAYER_PROJ : CC_ENEMY_PROJ;
		pp->collisionFilter = ship->allied? CC_ENEMY : CC_PLAYER;
		assert(linearDamping != 0);
		pp->linearDamping = linearDamping;
		pp->relativeAcceleration.z = acceleration;

		
		Matrix4f & rot = projectileEntity->rotationMatrix;
		Vector2f up(0, 1);
		Angle ang(up);
		Vector2f normVel = pp->velocity.NormalizedCopy();
		Angle look(normVel);
		Angle toLook = look - ang;
		projectileEntity->rotation.x = PI / 2;
		projectileEntity->rotation.y = toLook.Radians();
		projectileEntity->hasRotated = true;
		projectileEntity->RecalcRotationMatrix(true);

		GraphicsProperty * gp = projectileEntity->graphics = new GraphicsProperty(projectileEntity);
		gp->flags |= RenderFlag::ALPHA_ENTITY;

		// Add to map.
		MapMan.AddEntity(projectileEntity);
		playingLevel.projectileEntities.Add(projectileEntity);

		bool createThrustEmitter = false;
		if (homingFactor > 0) {
			createThrustEmitter = true;
		}

		// Add some tasty thrust particles!
 		switch (type) {
		case Type::MachineGun:
			projProp->CreateProjectileTraceEmitter(weaponWorldPosition);
			break;
		case Type::SmallRockets:
		case Type::BigRockets:
			createThrustEmitter = true;
			break;
		default:
			break;
		}

		if (createThrustEmitter)
			projProp->CreateThrustEmitter(weaponWorldPosition);

		if (ship->allied) {
			++PlayingLevelRef().projectilesFired;
		}

	//	lastShot = flyTime;
	}
	// Play sfx
	QueueAudio(new AMPlaySFX("sfx/"+shootSFX+".wav", 1.f + numberOfProjectiles * 0.01f));
}

void Weapon::QueueReload()
{
	if (!burst)
		return;
	reloading = true;
	currCooldownMs = int (cooldown.Milliseconds());
}

// Path to icon
String Weapon::Icon() {
	return "img/icons/WeaponType/" + toString(type);
}

/// Called to update the various states of the weapon, such as reload time, making lightning arcs jump, etc.
void Weapon::Process(PlayingLevel& playingLevel, Ship* ship, int timeInMs)
{
	if (!enabled)
		return;
	weaponWorldPosition = WorldPosition(ship->entity);


	if (shotsLeft <= 0 && !reloading)
	{
		QueueReload();
	}
	
	// Reduce cooldown every frame.
	currCooldownMs -= timeInMs;
	if (reloading && ship->activeSkill == ATTACK_FRENZY) // Extra reload speed during frenzy - 5x reload speed
		currCooldownMs -= timeInMs * 4;

	if (currCooldownMs < 0) {
		currCooldownMs = 0;
		if (reloading)
		{
			reloading = false;
			shotsLeft = this->burstRounds;
		}
	}


	switch(type)
	{
	case Type::Lightning:
			if (arcs.Size())
				ProcessLightning(playingLevel, ship);
			break;
	case Type::HeatWave:
//			ProcessHeatwave();
			break;
	}
}

LightningArc::LightningArc()
{
	graphicalEntity = 0;
	arcFinished = false;
	child = 0;
	damage = -1;
	maxBounces = -1;
}

void Weapon::ProcessLightning(PlayingLevel& playingLevel, Ship* owner, bool initial /* = true*/)
{
	if (initial)
	{
		// Create a dummy arc?
		LightningArc * arc = new LightningArc();
		arc->position = owner->entity->worldPosition;
		arc->maxRange = maxRange;
		arc->damage = (int) damage;
		arc->arcTime = playingLevel.flyTime;
		arc->maxBounces = maxBounces;
		arcs.AddItem(arc);
		nextTarget = 0;
		shipsStruckThisArc.Clear();
	}
	// Proceed all arcs which have already begun.
	bool arced = false;
	bool arcingAllDone = true;
	for (int i = 0; i < arcs.Size(); ++i)
	{
		// Create arc to target entity.
		LightningArc * arc = arcs[i];
		if (arc->arcFinished)
			continue;
		arcingAllDone = false;

		// Find next entity.
		if (nextTarget == 0)
		{
			List<float> distances;
			List<Ship*> possibleTargets = PlayingLevelRef().GetLevel().GetShipsAtPoint(arc->position, maxRange, distances);
			if (!initial)
				std::cout<<"\nPossible targets: "<<possibleTargets.Size();
			possibleTargets.RemoveUnsorted(shipsStruckThisArc);
			if (!initial)
				std::cout<<" - shipsAlreadyStruck("<<shipsStruckThisArc.Size()<<") = "<<possibleTargets.Size();
			if (possibleTargets.Size() == 0)
			{
				/// Unable to Arc, set the child to a bad number?
				arc->arcFinished = true;
				arcingAllDone = true;
				continue;
			}
			// Grab first one which hasn't already been struck?
			Ship* target = possibleTargets[0];
			// Recalculate distance since list was unsorted earlier...
			float distance = (target->entity->worldPosition - arc->position).Length();
			/// Grab closest one.
			for (int j = 1; j < possibleTargets.Size(); ++j)
			{
				Ship* t2 = possibleTargets[j];
				float d2 = (t2->entity->worldPosition - arc->position).Length();
				if (d2 < distance)
				{
					target = t2;
					distance = d2;
				}
			}
			if (distance > arc->maxRange)
			{
				arc->arcFinished = true;
				arcingAllDone = true;
				continue;
			}
			nextTarget = target;
		}
		/// Wait some time before each bounce too.
		if ((playingLevel.flyTime - arc->arcTime).Milliseconds() < arcDelay)
			continue;

		Ship* target = nextTarget;
		float distance = (target->entity->worldPosition - arc->position).Length();
		LightningArc * newArc = new LightningArc();
		newArc->position = target->entity->worldPosition;
		newArc->maxRange = arc->maxRange - distance;
		newArc->damage = (int) (arc->damage * 0.8);
		newArc->maxBounces = arc->maxBounces - 1;
		if (newArc->maxBounces <= 0)
			newArc->arcFinished = true;
		arc->child = newArc;
		arc->arcFinished = true;
		newArc->arcTime = playingLevel.flyTime;
		arcs.AddItem(newArc);
		// Pew-pew it!
		shipsStruckThisArc.AddItem(target);
		assert(shipsStruckThisArc.Duplicates() == 0);
		std::cout<<"\nThunderstruck! "<<target->entity->worldPosition;
		target->Damage(playingLevel, (float)arc->damage, false, DamageSource::Projectile);
		/// Span up a nice graphical entity to represent the bolt
		Entity* entity = EntityMan.CreateEntity("BoldPart", ModelMan.GetModel("cube.obj"), TexMan.GetTexture("0x00FFFF"));
		entity->localPosition = (arc->position + newArc->position) * 0.5f;
		/// Rotate it accordingly.
		Vector3f direction = newArc->position - arc->position;
		direction.Normalize();
		entity->rotation.z = Angle(direction.x, direction.y).Radians();
		/// Scale it.
		entity->scale = Vector3f(distance, 0.1f, 0);
		entity->RecalculateMatrix();
		MapMan.AddEntity(entity, true, false);
		newArc->graphicalEntity = entity;
	}
	//	std::cout<<"\nLightning Clean-up";
	for (int i = 0; i < arcs.Size(); ++i)
	{
		LightningArc * arc = arcs[i];
		if ((playingLevel.flyTime - arc->arcTime).Milliseconds() > arcDelay * 2)
		{
			if (arc->graphicalEntity)
				MapMan.DeleteEntity(arc->graphicalEntity);
			arcs.RemoveItem(arc);
			--i;
			delete arc;
		}
		nextTarget = 0;
	}
	if (initial && arcingAllDone)
	{
		currCooldownMs = 0;
	}
}
