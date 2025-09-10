// -----------------------------------------------------------------------------
// HB_Spawn : charge/écrit $profile\HiveBridge\spawns_<world>.json
// Auto-crée un fichier par carte avec des points par défaut si absent.
// (Logs via HB_Log = 3_Game uniquement)
// -----------------------------------------------------------------------------

class HB_SpawnPointCfg
{
	float x;
	float y;
	float z;
}

class HB_SpawnCfg
{
	ref array<ref HB_SpawnPointCfg> points;
	bool snap_to_ground = true;

	void HB_SpawnCfg()
	{
		points = new array<ref HB_SpawnPointCfg>();
	}
}

class HB_Spawn
{
	protected static ref array<vector> s_Points;
	protected static bool s_SnapToGround = true;
	protected static bool s_Loaded = false;

	protected static string WorldKey()
	{
		string w = GetGame().GetWorldName(); // "chernarusplus" | "enoch" | ...
		w.ToLower();
		return w;
	}

	protected static string Dir()
	{
		return "$profile:\\HiveBridge";
	}

	protected static string ConfigPath()
	{
		return Dir() + "\\spawns_" + WorldKey() + ".json";
	}

	protected static vector Ground(vector p)
	{
		float y = GetGame().SurfaceY(p[0], p[2]);
		return Vector(p[0], y, p[2]);
	}

	// ----------- défauts par carte + écriture du fichier si manquant ----------

	protected static HB_SpawnCfg DefaultCfg(string world_key)
	{
		HB_SpawnCfg cfg = new HB_SpawnCfg();
		if (world_key == "enoch") // Livonia
		{
			cfg.points.Insert(NewPoint(8000, 0, 11000));
			cfg.points.Insert(NewPoint(6200, 0, 9200));
			cfg.points.Insert(NewPoint(5300, 0, 7800));
		}
		else // chernarusplus par défaut
		{
			cfg.points.Insert(NewPoint(13900, 0, 13200));
			cfg.points.Insert(NewPoint(12500, 0, 11600));
			cfg.points.Insert(NewPoint(6000,  0, 7800));
		}
		cfg.snap_to_ground = true;
		return cfg;
	}

	protected static HB_SpawnPointCfg NewPoint(float x, float y, float z)
	{
		HB_SpawnPointCfg p = new HB_SpawnPointCfg();
		p.x = x; p.y = y; p.z = z;
		return p;
	}

	protected static void EnsureConfigExists()
	{
		if (!FileExist(Dir())) MakeDirectory(Dir());

		string path = ConfigPath();
		if (!FileExist(path))
		{
			HB_SpawnCfg cfg = DefaultCfg(WorldKey());
			JsonFileLoader<HB_SpawnCfg>.JsonSaveFile(path, cfg);
			HB_Log.Info("[HB_Spawn] Fichier de spawn créé (défaut): " + path);
		}
	}

	// --------------------------- chargement mémoire ----------------------------

	protected static void FallbackDefaultsAndLog(string reason)
	{
		HB_SpawnCfg cfg = DefaultCfg(WorldKey());
		s_Points = new array<vector>();
		foreach (HB_SpawnPointCfg p : cfg.points)
			s_Points.Insert(Vector(p.x, p.y, p.z));
		s_SnapToGround = cfg.snap_to_ground;
		HB_Log.Warn("[HB_Spawn] Fallback défaut (" + reason + ").");
	}

	protected static void LoadOnce()
	{
		if (s_Loaded) return;
		s_Loaded = true;

		EnsureConfigExists();

		string path = ConfigPath();
		HB_SpawnCfg cfg = new HB_SpawnCfg();
		JsonFileLoader<HB_SpawnCfg>.JsonLoadFile(path, cfg);

		if (!cfg || !cfg.points || cfg.points.Count() == 0)
		{
			FallbackDefaultsAndLog("fichier vide/invalide");
			return;
		}

		s_Points = new array<vector>();
		foreach (HB_SpawnPointCfg p : cfg.points)
			s_Points.Insert(Vector(p.x, p.y, p.z));

		s_SnapToGround = cfg.snap_to_ground;
		HB_Log.Info("[HB_Spawn] " + s_Points.Count().ToString() + " points chargés depuis " + path);
	}

	// ------------------------------- API publique ------------------------------

	static vector Select()
	{
		LoadOnce();
		if (!s_Points || s_Points.Count() == 0)
			FallbackDefaultsAndLog("aucun point en mémoire");

		int idx = Math.RandomInt(0, s_Points.Count());
		vector pos = s_Points.Get(idx);

		if (s_SnapToGround) pos = Ground(pos);
		return pos;
	}
}
