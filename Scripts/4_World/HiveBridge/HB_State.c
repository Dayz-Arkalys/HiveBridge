// -----------------------------------------------------------------------------
// HB_State : capture et (optionnellement) applique les états du joueur
// -----------------------------------------------------------------------------
class HB_State
{
	// active ou non la ré-application de certaines choses à l'import
	static const bool APPLY_AGENTS   = true;   // réinfecter selon AgentsMask (voir commentaires)
	static const bool APPLY_BLEEDING = false;  // par défaut on n’impose pas un saignement au spawn

	// ---- Capture au moment de l'export --------------------------------------
	static void Capture(PlayerBase p, HB_Payload pl)
	{
		// Saignement
		pl.Bleeding = p.IsBleeding();

		// Maladies / Agents
		// Beaucoup de builds de DayZ exposent PlayerBase.GetAgents() (bitmask).
		// S'il n'existe pas sur ta version, commente la ligne suivante.
		pl.AgentsMask = p.GetAgents(); // <- si ça râle, on l’enlèvera et on fera une autre voie
	}

	// ---- Application au moment de l'import ----------------------------------
	static void Apply(PlayerBase p, HB_Payload pl)
	{
		// 1) agents / maladies
		if (APPLY_AGENTS && pl.AgentsMask > 0)
		{
			// Idée simple : reposer quelques agents connus si leur bit est présent.
			// NB: Si ta version expose d'autres valeurs dans eAgents, complète ici.
			int mask = pl.AgentsMask;

			// Si ces constantes n'existent pas dans ton build, Workbench te le dira,
			// on ajustera les noms exacts après 1ère compile.
			if ((mask & eAgents.CHOLERA) != 0)        p.InsertAgent(eAgents.CHOLERA,        1);
			if ((mask & eAgents.SALMONELLA) != 0)     p.InsertAgent(eAgents.SALMONELLA,     1);
			if ((mask & eAgents.INFLUENZA) != 0)      p.InsertAgent(eAgents.INFLUENZA,      1);
			if ((mask & eAgents.FOOD_POISON) != 0)    p.InsertAgent(eAgents.FOOD_POISON,    1);
			if ((mask & eAgents.WOUND_AGENT) != 0)    p.InsertAgent(eAgents.WOUND_AGENT,1);

			HB_Log.Info("[HB_State] Agents reapplied (mask=" + mask.ToString() + ")");
		}

		// 2) saignement
		if (APPLY_BLEEDING && pl.Bleeding)
		{
			// Par sécurité, on ne force PAS un saignement au spawn par défaut.
			// Si tu veux absolument récréer une blessure, dé-commente/implémente ici
			// selon les API de ta version (BleedingManagerServer).
			// Exemple indicatif (à adapter après 1er test de compile) :
			// auto bm = p.GetBleedingManagerServer();
			// if (bm) bm.CreateBleedingSource(eBleedingSourceZone.LEFTARM);
			HB_Log.Info("[HB_State] (bleeding captured) — not applied by default");
		}
	}
}
