/*
	SF_RoadVehicleSpawnerComponent.c  -  Arma Reforger (Enforce Script)

	SETUP
	1. Put this file in your addon's Scripts/Game/ folder.
	2. Add the component "SF_RoadVehicleSpawnerComponent" to a prefab that
	   exists when the mission starts (e.g. your game mode prefab, or a new
	   empty entity prefab that you place in the world).
	3. On the component, fill in "Vehicle Prefabs" (the pool of vehicle types) and set "Vehicle Count"
	   (total number of vehicles). Each vehicle gets its own location and a random prefab from the pool.
	4. In the world editor place two entities named exactly:
	     US_CRASH
	     US_BASE

	Runs once, on the server only, a few seconds after mission start.
*/

[ComponentEditorProps(category: "Custom", description: "Spawns vehicles once at mission start, beside roads across the whole map.")]
class SF_RoadVehicleSpawnerComponentClass : ScriptComponentClass
{
}

class SF_RoadVehicleSpawnerComponent : ScriptComponent
{
	// ---------------------------------------------------------------- VEHICLE LIST
	[Attribute(desc: "Pool of vehicle prefabs. Each spawned vehicle is a random pick from this list.", uiwidget: UIWidgets.ResourceNamePicker, params: "et")]
	protected ref array<ResourceName> m_aVehiclePrefabs;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Total number of vehicles to spawn")]
	protected int m_iVehicleCount;

	// ---------------------------------------------------------------- RULES
	[Attribute(defvalue: "US_CRASH", desc: "World entity name of the crash site")]
	protected string m_sCrashName;

	[Attribute(defvalue: "US_BASE", desc: "World entity name of the base")]
	protected string m_sBaseName;

	[Attribute(defvalue: "1000", desc: "No vehicles within this distance of US_CRASH (m)")]
	protected float m_fCrashExclusion;

	[Attribute(defvalue: "525", desc: "No vehicles within this distance of US_BASE (m)")]
	protected float m_fBaseExclusion;

	[Attribute(defvalue: "0.10", desc: "Minimum starting fuel (0-1)")]
	protected float m_fFuelMin;

	[Attribute(defvalue: "0.80", desc: "Maximum starting fuel (0-1)")]
	protected float m_fFuelMax;

	[Attribute(defvalue: "2", desc: "Radius that must be free of objects, trees, AI, players and vehicles. Small = can sit right next to things, just not inside them (m)")]
	protected float m_fClearRadius;

	[Attribute(defvalue: "100", desc: "Minimum distance between spawned vehicles (m)")]
	protected float m_fMinSpacing;

	[Attribute(defvalue: "4", desc: "How far to the side of the road centreline to park (m)")]
	protected float m_fRoadSideOffset;

	[Attribute(defvalue: "1.5", desc: "Max height difference between road and ground at the parking spot (rejects bridges / embankments / water crossings) (m)")]
	protected float m_fMaxHeightDiff;

	[Attribute(defvalue: "2000", desc: "Random attempts per vehicle before giving up")]
	protected int m_iMaxAttempts;

	[Attribute(defvalue: "5000", desc: "Delay after mission start before spawning (ms)")]
	protected int m_iStartDelayMs;

	// ---------------------------------------------------------------- STATE
	protected ref array<BaseRoad> m_aRoads = {};
	protected ref array<vector> m_aSpawnedPos = {};
	protected IEntity m_Owner;
	protected IEntity m_Crash;
	protected IEntity m_Base;
	protected int m_iBlockers;
	protected bool m_bHasRun;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		m_Owner = owner;

		if (!GetGame().InPlayMode())
			return;

		GetGame().GetCallqueue().CallLater(Run, m_iStartDelayMs, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void Run()
	{
		if (!Replication.IsServer() || m_bHasRun)
			return;
		m_bHasRun = true;

		BaseWorld world = GetGame().GetWorld();

		m_Crash = world.FindEntityByName(m_sCrashName);
		m_Base = world.FindEntityByName(m_sBaseName);
		if (!m_Crash || !m_Base)
		{
			Print("[SF_RoadVehicleSpawner] ERROR: US_CRASH and/or US_BASE not found.", LogLevel.ERROR);
			return;
		}

		if (!m_aVehiclePrefabs || m_aVehiclePrefabs.IsEmpty())
		{
			Print("[SF_RoadVehicleSpawner] No vehicle prefabs set.", LogLevel.WARNING);
			return;
		}

		// grab every road on the map once
		// search box far larger than any map, so every road is returned
		vector mapMin = Vector(-100000, -1000, -100000);
		vector mapMax = Vector(100000, 5000, 100000);

		BaseRoadNetworkManager roadMgr = GetGame().GetAIWorld().GetRoadNetworkManager();
		roadMgr.GetRoadsInAABB(mapMin, mapMax, m_aRoads);

		if (m_aRoads.IsEmpty())
		{
			Print("[SF_RoadVehicleSpawner] ERROR: no roads found.", LogLevel.ERROR);
			return;
		}

		// one random prefab pick per vehicle
		array<ResourceName> valid = {};
		foreach (ResourceName p : m_aVehiclePrefabs)
		{
			if (!p.IsEmpty())
				valid.Insert(p);
		}

		if (valid.IsEmpty())
		{
			Print("[SF_RoadVehicleSpawner] No valid vehicle prefabs in the list.", LogLevel.WARNING);
			return;
		}

		array<ResourceName> picks = {};
		for (int i = 0; i < m_iVehicleCount; i++)
		{
			picks.Insert(valid.GetRandomElement());
		}

		int spawned = 0;
		foreach (ResourceName prefab : picks)
		{
			if (prefab.IsEmpty())
				continue;

			vector pos;
			float yaw;
			if (!FindSpot(pos, yaw))
			{
				Print(string.Format("[SF_RoadVehicleSpawner] WARNING: no valid spot found for %1", prefab), LogLevel.WARNING);
				continue;
			}

			IEntity veh = Spawn(prefab, pos, yaw);
			if (!veh)
				continue;

			SetRandomFuel(veh);
			RemoveJerryCans(veh);
			spawned++;
			m_aSpawnedPos.Insert(pos);
		}

		Print(string.Format("[SF_RoadVehicleSpawner] Spawned %1 of %2 vehicles.", spawned, picks.Count()));
	}

	//------------------------------------------------------------------------------------------------
	//! Picks random road segments across the whole map until a spot passes every rule
	protected bool FindSpot(out vector outPos, out float outYaw)
	{
		BaseWorld world = GetGame().GetWorld();
		vector crashPos = m_Crash.GetOrigin();
		vector basePos = m_Base.GetOrigin();

		for (int i = 0; i < m_iMaxAttempts; i++)
		{
			BaseRoad road = m_aRoads.GetRandomElement();

			array<vector> pts = {};
			road.GetPoints(pts);
			if (pts.Count() < 2)
				continue;

			// random point on a random segment of the road
			int idx = Math.RandomInt(0, pts.Count() - 1);
			vector p0 = pts[idx];
			vector p1 = pts[idx + 1];
			vector onRoad = p0 + (p1 - p0) * Math.RandomFloat01();

			float rx = p1[0] - p0[0];
			float rz = p1[2] - p0[2];
			float len = Math.Sqrt(rx * rx + rz * rz);
			if (len < 0.5)
				continue;
			rx /= len;
			rz /= len;

			// step to one side of the road
			float side = 1;
			if (Math.RandomFloat01() < 0.5)
				side = -1;

			vector pos;
			pos[0] = onRoad[0] - rz * m_fRoadSideOffset * side;
			pos[2] = onRoad[2] + rx * m_fRoadSideOffset * side;
			pos[1] = world.GetSurfaceY(pos[0], pos[2]);

			// --- hard rules ---
			if (pos[1] < world.GetOceanBaseHeight() + 0.5)
				continue;	// water (ocean)

			if (Math.AbsFloat(onRoad[1] - pos[1]) > m_fMaxHeightDiff)
				continue;	// bridge, steep embankment, or water crossing

			if (DistXZ(pos, crashPos) < m_fCrashExclusion)
				continue;

			if (DistXZ(pos, basePos) < m_fBaseExclusion)
				continue;

			bool tooClose = false;
			foreach (vector other : m_aSpawnedPos)
			{
				if (DistXZ(pos, other) < m_fMinSpacing)
				{
					tooClose = true;
					break;
				}
			}
			if (tooClose)
				continue;	// too near another spawned vehicle

			m_iBlockers = 0;
			world.QueryEntitiesBySphere(pos, m_fClearRadius, CountBlocker, FilterBlocker, EQueryEntitiesFlags.ALL);
			if (m_iBlockers > 0)
				continue;	// something is in the way

			// face along the road (either direction)
			float yaw = Math.Atan2(rx, rz) * Math.RAD2DEG;
			if (Math.RandomFloat01() < 0.5)
				yaw += 180;

			outPos = pos;
			outYaw = yaw;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool FilterBlocker(IEntity e)
	{
		// ignore this spawner's own entity and the two marker entities
		if (e == m_Owner || e == m_Crash || e == m_Base)
			return false;
		return true;
	}

	protected bool CountBlocker(IEntity e)
	{
		m_iBlockers++;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected float DistXZ(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity Spawn(ResourceName prefab, vector pos, float yawDeg)
	{
		Resource res = Resource.Load(prefab);
		if (!res || !res.IsValid())
		{
			Print(string.Format("[SF_RoadVehicleSpawner] ERROR: cannot load %1", prefab), LogLevel.ERROR);
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;

		vector angles = Vector(yawDeg, 0, 0);
		Math3D.AnglesToMatrix(angles, params.Transform);

		pos[1] = pos[1] + 0.4;	// drop in slightly above ground, physics settles it
		params.Transform[3] = pos;

		return GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
	}

	//------------------------------------------------------------------------------------------------
	//! One random percentage (default 10-80%) applied to every fuel tank on the vehicle
	protected void SetRandomFuel(IEntity veh)
	{
		FuelManagerComponent fuelMgr = FuelManagerComponent.Cast(veh.FindComponent(FuelManagerComponent));
		if (!fuelMgr)
			return;

		float pct = Math.RandomFloat(m_fFuelMin, m_fFuelMax);

		array<BaseFuelNode> nodes = {};
		fuelMgr.GetFuelNodesList(nodes);
		foreach (BaseFuelNode node : nodes)
		{
			node.SetFuel(node.GetMaxFuel() * pct);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Best effort: deletes any jerry can found in the vehicle's inventory
	protected void RemoveJerryCans(IEntity veh)
	{
		InventoryStorageManagerComponent inv = InventoryStorageManagerComponent.Cast(veh.FindComponent(InventoryStorageManagerComponent));
		if (!inv)
			return;

		array<IEntity> items = {};
		inv.GetItems(items);

		foreach (IEntity item : items)
		{
			EntityPrefabData data = item.GetPrefabData();
			if (!data)
				continue;

			string name = data.GetPrefabName();
			name.ToLower();
			if (name.Contains("jerry"))
				SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
}
