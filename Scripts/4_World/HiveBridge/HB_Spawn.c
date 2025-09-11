// -----------------------------------------------------------------------------
// HB_Spawn (XML-only)
//  - RESET  -> points <fresh>  de $mission\db\cfgplayerspawnpoints.xml
//  - SAFE   -> points <hop>    de $mission\db\cfgplayerspawnpoints.xml
// Ignore <travel>. Logs via HB_Log (3_Game). Module: 4_World.
// -----------------------------------------------------------------------------

enum HB_CfgSpawnSection { FRESH = 1, HOP = 2, TRAVEL = 4 }


class HB_Spawn
{

	protected static ref array<vector> s_Fresh; // utilisés par SelectNormal() (RESET)
	protected static ref array<vector> s_Hop;   // utilisés par SelectSafe()   (SAFE)
	protected static bool s_Loaded = false;

	protected static string CfgPath() { return "$mission:\\db\\cfgplayerspawnpoints.xml"; }

	static const float HB_SNAP_OFFSET = 0.35; // un peu plus haut que 0.25

	// Y terrain, avec garde-fou
	protected static float GroundY(float x, float z)
	{
		float y = GetGame().SurfaceY(x, z);
		// si la valeur est aberrante, on fallback à 0
		if (y < -1000 || y > 10000) y = 0;
		return y;
	}

	// Snap terrain simple (sans RaycastRV)
	protected static vector SnapToWorld(vector p, float offset)
	{
		float y = GroundY(p[0], p[2]);
		return Vector(p[0], y + offset, p[2]);
	}

	// Recalage après streaming
	protected static void _HB_RecheckSnap(PlayerBase p)
	{
		if (!p) return;
		vector pos = p.GetPosition();
		float gy = GroundY(pos[0], pos[2]);

		// Si on est sous la surface (ou trop au-dessus), on recale
		if (pos[1] < gy - 0.05 || pos[1] > gy + 2.0)
		{
			p.SetPosition(SnapToWorld(pos, HB_SNAP_OFFSET));
		}
	}

	// Place le joueur sur le sol + rechecks différés
	static void EnsureOnGround(PlayerBase p)
	{
		if (!p) return;

		vector pos = p.GetPosition();
		p.SetPosition(SnapToWorld(pos, HB_SNAP_OFFSET));

		// 2 rechecks pour laisser le temps au streaming
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(_HB_RecheckSnap, 250, false, p);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(_HB_RecheckSnap, 1500, false, p);
	}

	


	protected static int FindNextQuote(string s, int start)
	{
		int len = s.Length();
		if (start < 0 || start >= len) return -1;
		string tail = s.Substring(start, len - start);
		int rel = tail.IndexOf("\"");   // <-- IndexOf à 1 seul paramètre
		if (rel < 0) return -1;
		return start + rel;
	}

	protected static void LoadFromCfgOnce()
	{
		if (s_Loaded) return;
		s_Loaded = true;

		s_Fresh = new array<vector>();
		s_Hop   = new array<vector>();

		string path = CfgPath();
		if (!FileExist(path)) { HB_Log.Warn("[HB_Spawn] cfgplayerspawnpoints.xml introuvable: " + path); return; }

		FileHandle fh = OpenFile(path, FileMode.READ);
		if (!fh) { HB_Log.Warn("[HB_Spawn] Impossible d'ouvrir: " + path); return; }

		int cur = 0; // section courante
		string line;
		while (FGets(fh, line) > 0)
		{
			string low = line; low.ToLower();

			// Sections
			if (low.Contains("<fresh>")) { cur = HB_CfgSpawnSection.FRESH; continue; }
			if (low.Contains("</fresh>")) { cur = 0; continue; }
			if (low.Contains("<hop>")) { cur = HB_CfgSpawnSection.HOP; continue; }
			if (low.Contains("</hop>")) { cur = 0; continue; }
			if (low.Contains("<travel>")) { cur = HB_CfgSpawnSection.TRAVEL; continue; }
			if (low.Contains("</travel>")) { cur = 0; continue; }

			// On ne retient QUE FRESH et HOP
			if (!(cur == HB_CfgSpawnSection.FRESH || cur == HB_CfgSpawnSection.HOP)) continue;

			// Format 1: <pos x="1234.56" z="7890.12" />
			int xi = low.IndexOf("x=\"");
			int zi = low.IndexOf("z=\"");
			if (xi >= 0 && zi >= 0)
			{
				xi += 3;                       // après x="
				int xe = FindNextQuote(low, xi);
				zi += 3;                       // après z="
				int ze = FindNextQuote(low, zi);

				if (xe > xi && ze > zi)
				{
					string sx = low.Substring(xi, xe - xi);
					string sz = low.Substring(zi, ze - zi);
					float fx = sx.ToFloat();
					float fz = sz.ToFloat();
					vector v = SnapToWorld(Vector(fx, 0, fz),HB_SNAP_OFFSET);
					if (cur == HB_CfgSpawnSection.FRESH) s_Fresh.Insert(v); else s_Hop.Insert(v);
					continue;
				}
			}

			// (Optionnel) Format 2: <spawn pos="X Y Z" />
			int pi = low.IndexOf("pos=\"");
			if (pi >= 0)
			{
				pi += 5;                       // après pos="
				int pe = FindNextQuote(low, pi);
				if (pe > pi)
				{
					string spos = low.Substring(pi, pe - pi); // "X Y Z"
					TStringArray toks = new TStringArray();
					spos.Split(" ", toks);
					if (toks.Count() >= 3)
					{
						float fx2 = toks[0].ToFloat();
						float fz2 = toks[2].ToFloat();
						vector v2 = SnapToWorld(Vector(fx2, 0, fz2),HB_SNAP_OFFSET);
						if (cur == HB_CfgSpawnSection.FRESH) s_Fresh.Insert(v2); else s_Hop.Insert(v2);
					}
				}
			}
		}
		CloseFile(fh);

		HB_Log.Info("[HB_Spawn] FRESH: " + s_Fresh.Count().ToString() + " points");
		HB_Log.Info("[HB_Spawn] HOP  : "   + s_Hop.Count().ToString()   + " points");
	}

	protected static vector Pick(array<vector> arr, string tagIfEmpty)
	{
		if (!arr || arr.Count() == 0) {
			HB_Log.Warn("[HB_Spawn] Liste vide: " + tagIfEmpty);
			return Vector(7500, 0, 7500); // ← vector, pas string
		}
		int idx = Math.RandomInt(0, arr.Count());
		return arr.Get(idx);
	}

	// RESET -> FRESH (vanilla)
	static vector SelectNormal()
	{
		LoadFromCfgOnce();
		return Pick(s_Fresh, "FRESH");
	}

	// SAFE -> HOP
	static vector SelectSafe()
	{
		LoadFromCfgOnce();
		// si pas de HOP défini, on retombe sur FRESH
		if (!s_Hop || s_Hop.Count() == 0) return SelectNormal();
		return Pick(s_Hop, "HOP");
	}
}
