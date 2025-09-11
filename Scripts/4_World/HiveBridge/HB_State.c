// -----------------------------------------------------------------------------
// HB_State : capture et (optionnellement) applique les états du joueur
// -----------------------------------------------------------------------------
class HB_State
{
    // Réglages d'application à l'import
    static const bool APPLY_AGENTS   = true;   // réinfecter selon le masque
    static const bool APPLY_BLEEDING = true;   // recréer les coupures capturées
    static const int  AGENT_DOSE     = 1;      // dose par agent appliqué

    // --- Bitmask interne (portable) ------------------------------------------
    static const int HBAG_CHOLERA      = 1;        // 1 << 0
    static const int HBAG_SALMONELLA   = 1 << 1;
    static const int HBAG_INFLUENZA    = 1 << 2;
    static const int HBAG_FOOD_POISON  = 1 << 3;
    static const int HBAG_WOUND        = 1 << 4;

    // ---- Capture (export) ----------------------------------------------------
    static void Capture(PlayerBase p, HB_Payload pl)
    {
        if (!p || !pl) return;

        // --- VITAUX (Nourriture/Eau)
        auto se = p.GetStatEnergy();
        auto sw = p.GetStatWater();
        if (se) pl.Energy = se.Get();
        if (sw) pl.Water  = sw.Get();

        // --- saignement : bool + nombre de sources
        int bleedCnt = 0;
        auto bm = p.GetBleedingManagerServer();
        if (bm)
        {
            // NOTE : si ta build a 'GetBleedingSourceCount()' (singulier),
            // renomme la ligne ci-dessous.
            bleedCnt = bm.GetBleedingSourcesCount();
        }
        if (bleedCnt == 0 && p.IsBleeding())
            bleedCnt = 1; // filet de sécu

        pl.Bleeding      = (bleedCnt > 0);
        pl.BleedingCount = bleedCnt;

        // --- agents / maladies : on stocke le bitmask moteur tel quel
        int a = p.GetAgents();
        pl.AgentsMask = a;

        HB_Log.Info("[HB_State] Capture: E=" + pl.Energy.ToString() + ", W=" + pl.Water.ToString() + ", bleeding=" + pl.Bleeding.ToString() + " (count=" + bleedCnt.ToString() + ")" + ", agentsMask=" + pl.AgentsMask.ToString());
    }

    // ---- Application (import) ------------------------------------------------
    static void Apply(PlayerBase p, HB_Payload pl)
    {
        if (!p || !pl) return;

        // VITAUX : uniquement si présents dans le JSON (>= 0)
        if ((pl.Energy >= 0) || (pl.Water >= 0)) {
            autoptr Param3<PlayerBase, float, float> vit = new Param3<PlayerBase, float, float>(p, pl.Energy, pl.Water);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(_ApplyVitalsNow, 200, false, vit);
        }

        // Agents
        if (APPLY_AGENTS) {
            autoptr Param2<PlayerBase,int> ctxA = new Param2<PlayerBase,int>(p, pl.AgentsMask);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(_ApplyAgentsNow, 300, false, ctxA);
        }

        // Bleeding (tu gardes ta version actuelle si tu préfères le while/i++)
        if (APPLY_BLEEDING) {
            // soit ta version while …, soit mon _StartBleedingRestore
            // exemple avec ton while :
            autoptr Param2<PlayerBase,int> ctxB = new Param2<PlayerBase,int>(p, pl.BleedingCount);
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(_ApplyBleedingNow, 400, false, ctxB);
        }
    }

    // --- Applique Energy/Water en sécurité -----------------------------------
    protected static void _ApplyVitalsNow(Param3<PlayerBase,float,float> ctx)
    {
        if (!ctx) return;
        PlayerBase p = ctx.param1;
        float e = ctx.param2;
        float w = ctx.param3;
        if (!p) return;

        auto se = p.GetStatEnergy();
        auto sw = p.GetStatWater();

        if (se && e >= 0) se.Set(e);
        if (sw && w >= 0) sw.Set(w);

        string eStr = "skip";
	    if (e >= 0) eStr = e.ToString();
	    string wStr = "skip";
	    if (w >= 0) wStr = w.ToString();
	
	    HB_Log.Info("[HB_State] Vitals applied E=" + eStr + " W=" + wStr);
    }

    // -- implémentations différées --------------------------------------------
    protected static void _ApplyAgentsNow(Param2<PlayerBase,int> ctx)
    {
        if (!ctx) return;
        PlayerBase p = ctx.param1;
        int m = ctx.param2;
        if (!p) return;
        p.RemoveAllAgents(); // option stricte

        // Applique en se basant sur le bitmask moteur
        if ((m & eAgents.CHOLERA)     != 0) p.InsertAgent(eAgents.CHOLERA,     AGENT_DOSE);
        if ((m & eAgents.SALMONELLA)  != 0) p.InsertAgent(eAgents.SALMONELLA,  AGENT_DOSE);
        if ((m & eAgents.INFLUENZA)   != 0) p.InsertAgent(eAgents.INFLUENZA,   AGENT_DOSE);
        if ((m & eAgents.FOOD_POISON) != 0) p.InsertAgent(eAgents.FOOD_POISON, AGENT_DOSE);
        // NOTE build : si WOUND_AGENT n'existe pas, remplace par WOUND_INFECTION
        if ((m & eAgents.WOUND_AGENT) != 0) p.InsertAgent(eAgents.WOUND_AGENT, AGENT_DOSE);

        HB_Log.Info("[HB_State] Reapplied agents (mask=" + m.ToString() + ")");
    }

    protected static void _ApplyBleedingNow(Param2<PlayerBase,int> ctx)
    {
        if (!ctx) return;
        PlayerBase p = ctx.param1;
        int want = ctx.param2;
        if (!p) return;

        auto bm = p.GetBleedingManagerServer();
        if (!bm) { HB_Log.Info("[HB_State] Bleeding wanted but no manager"); return; }

		bm.RemoveAnyBleedingSource();

        if (want <= 0) {
			HB_Log.Info("[HB_State] No bleeding to apply");
			return;
		}
		
		// Par sûreté, limite raisonnable (ex: 8 max)
        if (want < 0) want = 0;
        if (want > 8) want = 8;

        // Active N sources (0..N-1). Sur la plupart des builds, les indices 0..3 couvrent
        // les membres principaux ; au-delà, le manager ignore les index invalides.
        int i = 0;
		while (bm.GetBleedingSourcesCount()<want)
			{
			bm.AttemptAddBleedingSource(i);
			i++;		
			}

        HB_Log.Info("[HB_State] Recreated bleeding sources: " + want.ToString());
    }
}
