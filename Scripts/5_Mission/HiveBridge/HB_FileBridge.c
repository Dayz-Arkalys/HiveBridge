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
		HB_LogFile.Info("Export RESET écrit: " + reason + " -> " + path);
	}

	static void SaveTransfer(PlayerBase p)
	{
		PlayerIdentity pid = p.GetIdentity();
		if (!pid) return;

		// ——— cas reset
		if (!p.IsAlive())       { WriteReset(pid, "dead");         return; }
		if (p.IsUnconscious())  { WriteReset(pid, "unconscious");  return; }

		// ——— export normal
		HB_LogFile.Info("SaveTransfer -> " + pid.GetPlainId());

		string path = OutPathFor(pid);
		HB_Payload payload = HB_PayloadEx.FromPlayer(p);
		JsonFileLoader<HB_Payload>.JsonSaveFile(path, payload);

		HB_LogFile.Info("payload écrit: " + path);
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
            HB_Reset.ToFreshState(p);  // (voir §4)
        }
        else
        {
            // Import normal (inventaire détaillé)
            HB_PayloadEx.ApplyTo(p, pl);
            HB_LogFile.Info("Inventaire appliqué (import normal).");
        }

        // Spawn safe + ghost
        vector pos = HB_Spawn.Select();
        p.SetPosition(pos);
        p.SetAllowDamage(false);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EndGhost, 12000, false, p);

        // Consommer le paquet
        if (!DeleteFile(path))
            HB_LogFile.Warn("Suppression du paquet échouée: " + path);
        else
            HB_LogFile.Info("Paquet consommé et supprimé: " + path);
    }


	static void EndGhost(PlayerBase p)
	{
		if (p) { p.SetAllowDamage(true); HB_LogFile.Info("Ghost terminé"); }
	}
}
