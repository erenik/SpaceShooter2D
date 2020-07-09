/// Emil Hedemalm
/// 2015-03-03
/// Spawn group, yo.

#include "../SpaceShooter2D.h"
#include "SpawnGroup.h"
#include "Level.h"
#include "Entity/EntityManager.h"
#include "TextureManager.h"
#include "Model/ModelManager.h"
#include "File/LogFile.h"
#include "PlayingLevel.h"

String GetName(Formation forFormationType)
{
	switch(forFormationType)
	{
	case Formation::BAD_FORMATION: return "BAD_FORMATION";
	case Formation::LINE_X: return "LINE_X";
	case Formation::DOUBLE_LINE_X: return "DOUBLE_LINE_X";
	case Formation::LINE_Y: return "LINE_Y";
	case Formation::LINE_XY: return "LINE_XY";
	case Formation::X: return "X";
	case Formation::SQUARE: return "SQUARE";
	case Formation::CIRCLE: return "CIRCLE";
	case Formation::HALF_CIRCLE_LEFT: return "HALF_CIRCLE_LEFT";
	case Formation::HALF_CIRCLE_RIGHT: return "HALF_CIRCLE_RIGHT";
	case Formation::V_X: return "V_X";
	case Formation::V_Y: return "V_Y";
	case Formation::SWARM_BOX_XY: return "SWARM_BOX_XY";
	default:
		assert(false);
	}
}

Formation GetFormationByName(String name)
{
	for (int i = 0; i < (int) Formation::FORMATIONS; ++i)
	{
		String n = GetName(Formation(i));
		if (n == name)
			return Formation(i);
	}
	return Formation::BAD_FORMATION;
}


SpawnGroup::SpawnGroup()
{
	spawnTime = Time(TimeType::MILLISECONDS_NO_CALENDER);
	pausesGameTime = false;
	spawnIntervalMsBetweenEachShipInFormation = 0;
	Reset();
	lastSpawn = AETime(TimeType::MILLISECONDS_NO_CALENDER, 0);
}

SpawnGroup::~SpawnGroup()
{
//	ships.ClearAndDelete();
	for (int i = 0; i < ships.Size(); ++i)
	{
		ShipPtr s = ships[i];
		s->spawnGroup = 0;
	}
}

void SpawnGroup::Reset()
{
	pausesGameTime = false;
	finishedSpawning = false;
	preparedForSpawning = false;
	relativeSpeed = 5.f;
	shoot = true;
	shipsSpawned = false;
	defeated = false;
	survived = false;
	shipsDefeatedOrDespawned = 0;
	shipsDespawned = shipsDefeated = 0;
	spawnTime = Time(TimeType::MILLISECONDS_NO_CALENDER);
}

void SpawnGroup::RemoveThis(Ship* sp) {
	for (int i = 0; i < ships.Size(); ++i) {
		if (ships[i].get() == sp)
			ships.RemoveIndex(i);
	}
}


Random spawnGroupRand;
/** Spawns ze entities. 
	True if spawning sub-part of an aggregate formation-type. 
	Returns true if it has finished spawning. 
	Call again until it returns true each iteration (required for some formations).
*/
bool SpawnGroup::Spawn(PlayingLevel& playingLevel)
{
	/// Prepare spawning.
	if (!preparedForSpawning)
		PrepareForSpawning();

	// Spawn all?
	if (spawnIntervalMsBetweenEachShipInFormation == 0)
	{
		for (int i = 0; i < ships.Size(); ++i)
		{
			ShipPtr ship = ships[i];
			ship->Spawn(ship->position, 0, playingLevel);
			activeLevel->ships.AddItem(ship);
			++shipsSpawned;
			ships.RemoveItem(ship);
		}
		finishedSpawning = true;
		return true;
	}
	/// Spawn one at a time?
	else
	{
		if (lastSpawn.Seconds() == 0 || (playingLevel.levelTime - lastSpawn).Milliseconds() > spawnIntervalMsBetweenEachShipInFormation)
		{
			ShipPtr ship = ships[0];
			ship->Spawn(ship->position, 0, playingLevel);
			activeLevel->ships.AddItem(ship);
			ships.RemoveIndex(0, ListOption::RETAIN_ORDER);
			lastSpawn = playingLevel.levelTime;
		}
		// Spawn one ata  time.
		if (ships.Size() == 0)
		{
			finishedSpawning = true;
			return true;
		}
	}
	/// o.o
	return false;
}

void SpawnGroup::OnFinishedSpawning(PlayingLevel& playingLevel) {
	if (pausesGameTime && DefeatedOrDespawned()) {
		playingLevel.gameTimePaused = false;
	}
}

/// To avoid spawning later.
void SpawnGroup::SetFinishedSpawning()
{
	finishedSpawning = true;
}

/// Gathers all ships internally for spawning.
void SpawnGroup::PrepareForSpawning(SpawnGroup * parent)
{
	List<Vector3f> positions;
	Vector3f offsetPerSpawn;
	ships.Clear();
	if (!parent)
		LogMain("Spawning spawn group at time: "+String(spawnTime.ToString("m:S.n")), INFO);

	std::cout<<"\nSpawning formation: "<<GetName(formation);

	/// Initial adjustments, or sub-group spawning.
	switch(formation)
	{
		case Formation::X:
			if (number < 5)
				number = 5;
			break;
		case Formation::SQUARE:
			/// Number will be the amount of ships per side?
			SpawnGroup copy = *this;
			if (number < 3)
				number = 3;
			int num = number;
			Vector2f spacing = size / (float)(num - 1);
			Vector2f lineSize = spacing * (float)(num - 2);
			// Bottom
			copy.formation = Formation::LINE_X;
			copy.number = num - 1;
			copy.position = position + Vector2f(-size.x * 0.5f, size.y * 0.5f);
			copy.size = Vector2f(lineSize.x, 0);
			copy.ships.Clear();
			copy.PrepareForSpawning(this);
			copy.position = position + Vector2f(-size.x * 0.5f + spacing.x, - size.y * 0.5f);
			copy.ships.Clear();
			copy.PrepareForSpawning(this);
			copy.position = position + Vector2f(size.x * 0.5f, spacing.y * 0.5f);
			copy.formation = Formation::LINE_Y;
			copy.size = Vector2f(0, -lineSize.y);
			copy.ships.Clear();
			copy.PrepareForSpawning(this);
			copy.position = position + Vector2f(-size.x * 0.5f, - spacing.y * 0.5f);
			copy.size = Vector2f(0, lineSize.y);
			copy.ships.Clear();
			copy.PrepareForSpawning(this);
			goto end;
	}

	/// Fetch amount.
	int offsetNum = max(1, number - 1);
	// Always nullify Z.
	size.z = 0;
	switch(formation)
	{
		case Formation::LINE_XY:
			break;
		case Formation::V_X:
		case Formation::V_Y:
			break;
		case Formation::SWARM_BOX_XY:
			break;
		default:
			;//std::cout<<"\nImplement";
	}
	offsetPerSpawn = size / (float) offsetNum;

	/// Adjust offsetPerSpawn accordingly
	switch(formation)
	{
		case Formation::V_X:
			offsetPerSpawn.x = size.x / MaximumFloat(((float)floor((number) / 2.0 + 0.5) - 1), 1);
			break;
		case Formation::V_Y:
			offsetPerSpawn.y = size.y / MaximumFloat(((float)floor((number) / 2.0 + 0.5) - 1), 1);
			break;
		case Formation::X:
		{
			int steps = (int)floor((number - 1) / 4.0);
			offsetPerSpawn = size * 0.5f / (float)steps;
			break;
		}
		case Formation::SWARM_BOX_XY:
			offsetPerSpawn.y = 0; // Spawn erratically in Y only. Have some X offset each spawn?
			break;
		case Formation::LINE_X:
		case Formation::DOUBLE_LINE_X:
			offsetPerSpawn.y = 0;
			break;
		case Formation::LINE_Y:
			offsetPerSpawn.x = 0;
			break;
	}

	/// Check formation to specify vectors for all
	for (int i = 0; i < number; ++i)
	{

		Vector3f position;
		int offsetIndex = i;
		// Just add offset?
		switch(formation)
		{
			// for them half-way-flippers.
			case Formation::V_X:
			case Formation::V_Y:
				offsetIndex = (i + 1)/ 2;
				break;
			case Formation::X:
				offsetIndex = (i + 3) / 4;
				break;
			case Formation::LINE_X:
//				position.y += size.y * 0.5f;
				break;
			case Formation::LINE_XY:
			case Formation::LINE_Y:
			case Formation::SWARM_BOX_XY:
			case Formation::DOUBLE_LINE_X:
				position.y -= size.y * 0.5f; // Center it.
				break;
			default:
				;// std::cout<<"\nImplement";
		}	
		position += offsetPerSpawn * (float)offsetIndex;
		bool angles = false;
		float degrees = 0;
		float startDegree = 0;
		Vector2f radialMultiplier(1,1);
		// Flip/randomize accordingly, 
		switch(formation)
		{
			case Formation::V_X:
				if ((i + 1) % 2 == 0)
					position.y *= -1;
				break;
			case Formation::V_Y:
				if ((i + 1) % 2 == 0)
					position.x *= -1;
				break;
			case Formation::X:
				/// If .. stuff
				if ((i + 1) % 4 < 2)
					position.x *= -1;
				if ((i + 1) % 2 == 0)
					position.y *= -1;
				break;
			case Formation::SWARM_BOX_XY:
				position.y += spawnGroupRand.Randf(size.y);
				break;
			case Formation::DOUBLE_LINE_X:
				break;
			default:
				break;//std::cout<<"\nImplement";
			case Formation::CIRCLE:
				angles = true;
				degrees = 360.0f / number;
				startDegree = 180;
				break;
			case Formation::HALF_CIRCLE_LEFT:
				angles = true;
				degrees = 180.0f / max((number - 1), 1);
				startDegree = 90;
				radialMultiplier.x = 2;
				break;
			case Formation::HALF_CIRCLE_RIGHT:
				angles = true;
				degrees = 180.0f / max((number - 1), 1);
				startDegree = -90;
				radialMultiplier.x = 2;
				break;
		}
		/// Radial spawning.
		if (angles)
		{
			position = Vector3f();
			Angle ang = Angle::FromDegrees(degrees * i + startDegree);
			Vector2f angXY = ang.ToVector2f();
			position += angXY * size * 0.5f * radialMultiplier;
		}

		/// Center it - wat?
//		position.y -= size.y * 0.5f;

		/// Add group position offset (if any)
		position += this->position;

		assert(position.x == position.x);
		assert(position.y == position.y);
		LogMain(String("Spawning ship @ x")+ String(position.x)+" y"+position.y, INFO);

		/// Add current position offset to it.
//		position += Vector3f(spawnPositionRight, activeLevel->height * 0.5f, 0); 


		/// Create entity
		AddShipAtPosition(position);
		if (formation == Formation::DOUBLE_LINE_X)
		{
			AddShipAtPosition(position + Vector3f(0, size.y, 0));
		}
	}

end:
	preparedForSpawning = true;
	/// Add all ships to parent if subAggregate
	if (parent)
		parent->ships.Add(ships);
	assert(ships.Size());
	/// Re-link all ships to point spawngroup to the current one.
	for (int i = 0; i < ships.Size(); ++i)
	{
		ships[i]->spawnGroup = this;
	}
}

void SpawnGroup::AddShipAtPosition(ConstVec3fr position)
{
	ShipPtr newShip = Ship::New(shipType);
	if (!newShip)
	{
		LogMain("SpawnGroup::SpawnShip: Unable to create ship of type: "+shipType, CAUSE_ASSERTION_ERROR);
		return;
	}
	ShipPtr ship = newShip;
	ship->RandomizeWeaponCooldowns();
	/// Apply spawn group properties.
	ship->shoot &= shoot;
	ship->speed *= relativeSpeed;
	ship->position = position;
	/// ....
	if(mp.movements.Size())
	{
		ship->movements = mp.movements;
	}
	if(mp.rotations.Size())
	{
		ship->rotations = mp.rotations;
	}
	ships.AddItem(ship);
}


// ?!
/*EntitySharedPtr SpawnGroup::SpawnShip(ConstVec3fr atPosition)
{
}*/

/// Query, compares active ships vs. spawned amount
bool SpawnGroup::DefeatedOrDespawned()
{
	return shipsDefeatedOrDespawned >= shipsSpawned;
}

void SpawnGroup::SetDefeated()
{
	SetFinishedSpawning();
	shipsDefeatedOrDespawned = shipsDefeated = shipsSpawned;
	for (int i = 0; i < ships.Size(); ++i)
	{
		ShipPtr s = ships[i];
		s->spawnGroup = 0;
	}
	// Destroy ships!
	ships.Clear();
}

void SpawnGroup::OnShipDestroyed(PlayingLevel& playingLevel, ShipPtr ship)
{
	++shipsDefeated;
	++shipsDefeatedOrDespawned;
	if (shipsDefeated >= number)
	{
		playingLevel.defeatedAllEnemies = true;
		defeated = true;
		survived = true;
		if (pausesGameTime)
			playingLevel.gameTimePaused = false;
	}
}

void SpawnGroup::OnShipDespawned(PlayingLevel& playingLevel, ShipPtr ship)
{
	++shipsDespawned;
	++shipsDefeatedOrDespawned;
	if (shipsDefeatedOrDespawned >= number)
	{
		playingLevel.defeatedAllEnemies = false;
		survived = true;
		if (pausesGameTime)
			playingLevel.gameTimePaused = false;
	}
}

String SpawnGroup::GetLevelCreationString(Time t)
{
	String str;
	str = "\nSpawnGroup "+String(t.Minute())+":"+String(t.Second())+"."+String(t.Millisecond());
	if (name.Length())
		str += "\nName "+name;
	str += "\nShipType "+shipType;
	str += "\nFormation "+GetName(formation);
	str += "\nNumber "+String(number);
	str += "\nSize xy "+String(size.x)+" "+String(size.y);
	str += "\nPosition xy "+String(position.x)+" "+String(position.y);
	if (spawnIntervalMsBetweenEachShipInFormation > 0)
		str += "\nTimeBetweenShipSpawnsMs "+String(this->spawnIntervalMsBetweenEachShipInFormation);

	return str;
}

void SpawnGroup::ParseFormation(String fromString)
{
	formation = GetFormationByName(fromString);
}



