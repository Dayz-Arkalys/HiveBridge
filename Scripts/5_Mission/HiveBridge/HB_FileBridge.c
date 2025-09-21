class HB_FileBridge
{
	protected static string OutPathFor(PlayerIdentity pid)
    {
        string root   = HB_LogFile.ProfDir();
        string outdir = root + "\\outgoing";
        if (!FileExist(outdir)) MakeDirectory(outdir);
        return outdir + "\\" + pid.GetPlainId() + ".json";
    }

    protected static void WriteReset(PlayerIdentity pid, string reason)
    {
        string path = OutPathFor(pid);

        HB_Payload pl = new HB_Payload();
        pl.Reset  = true;
        pl.Reason = reason;

        JsonFileLoader<HB_Payload>.JsonSaveFile(path, pl);
        HB_LogFile.Info("Export RESET écrit (" + reason + "): " + path);
    }

    static string PeekCharacterType(PlayerIdentity pid)
    {
        if (!pid) return "";
        string path = InPathFor(pid);  // tu as déjà InPathFor(...) dans ton fichier
        if (!FileExist(path)) return "";

        HB_Payload tmp = new HB_Payload();
        JsonFileLoader<HB_Payload>.JsonLoadFile(path, tmp);

        if (tmp && tmp.CharType && tmp.CharType != "")
            return tmp.CharType;

        return "";
    }

    // ← NEW: on prend l'identity en param, et on n'utilise plus p.GetIdentity() comme unique source
    static void SaveTransfer(PlayerIdentity pid, PlayerBase p)
    {
        // sécurité : si l'identity de l'event est null, tenter via le player
        if (!pid && p) pid = p.GetIdentity();
        if (!pid) {
            HB_LogFile.Warn("SaveTransfer: SKIP (no identity)");
            return;
        }

        // log de contexte
        HB_LogFile.Info("SaveTransfer: id=" + pid.GetPlainId() + ", hasPlayer=" + (p != null).ToString());
    
        // cas RESET (mort / inconscient / ou plus de player pour une raison quelconque)
        bool doReset = false;
        string reason = "";
        if (!p) { doReset = true; reason = "no_player"; }
        else if (!p.IsAlive()) { doReset = true; reason = "dead"; }
        else if (p.IsUnconscious()) { doReset = true; reason = "unconscious"; }
        else if (p.IsRestrained()) { doReset = true; reason = "restrained"; }


        if (doReset) {
            WriteReset(pid, reason);
            return;
        }

        // export normal
        string path = OutPathFor(pid);
        HB_Payload payload = HB_PayloadEx.FromPlayer(p);
        JsonFileLoader<HB_Payload>.JsonSaveFile(path, payload);

        HB_LogFile.Info("Export NORMAL écrit: " + path);
    }

	static void TryApplyTransfer(PlayerIdentity id, PlayerBase p)
    {
        if (!id) return;

        string root  = HB_LogFile.ProfDir();
        string indir = root + "\\incoming";
        string path  = indir + "\\" + id.GetPlainId() + ".json";

        if (!FileExist(path)) {
            HB_LogFile.Info("TryApplyTransfer: aucun paquet pour " + id.GetPlainId());
            return;
        }

        HB_LogFile.Info("TryApplyTransfer: paquet trouvé -> " + path);

        // Charger (peut être un "reset" ou un payload normal)
        HB_Payload pl = new HB_Payload();
        JsonFileLoader<HB_Payload>.JsonLoadFile(path, pl);

        bool isReset = pl.Reset;
        // filet de sécurité : si roots vides ET santé/sang nuls, traite comme reset
        if (!isReset && pl.Roots && pl.Roots.Count() == 0 && pl.Health <= 0 && pl.Blood <= 0)
            isReset = true;

        if (isReset)
		{
			HB_LogFile.Info("Import RESET (" + pl.Reason + ") → réinitialisation du stuff");
			HB_Reset.ToFreshState(p);

			// ------ SPAWN "NORMAL" (fresh du XML)
			vector posN = HB_Spawn.SelectNormal();
            p.SetPosition(posN);
            HB_Spawn.EnsureOnGround(p); // <— AJOUT
            HB_LogFile.Info("Spawn NORMAL (reset): " + posN.ToString());

		}
		else
		{
			// ------ Import inventaire + états
			HB_PayloadEx.ApplyTo(p, pl);

			// ------ SPAWN "SAFE" (hop du XML)
			vector posS = HB_Spawn.SelectSafe();
            p.SetPosition(posS);
            HB_Spawn.EnsureOnGround(p); // <— AJOUT
            HB_LogFile.Info("Spawn SAFE (import): " + posS.ToString());

		}

        p.SetAllowDamage(false);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EndGhost, 12000, false, p);

        // Consommer le paquet
        if (!DeleteFile(path))
            HB_LogFile.Warn("Suppression du paquet échouée: " + path);
        else
            HB_LogFile.Info("Paquet consommé et supprimé: " + path);
    }

    static void TryApplyTransferDelayed(Param2<PlayerIdentity, PlayerBase> ctx)
    {
        if (!ctx) return;
        PlayerIdentity id = ctx.param1;
        PlayerBase p = ctx.param2;
        TryApplyTransfer(id, p);
    }


	static void EndGhost(PlayerBase p)
	{
		if (p) { p.SetAllowDamage(true); HB_LogFile.Info("Ghost terminé"); }
	}
}
