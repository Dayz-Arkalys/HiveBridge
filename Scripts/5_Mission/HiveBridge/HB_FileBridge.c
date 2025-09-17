class HB_FileBridge
{
    // --- RPC (mêmes valeurs côté client)
    static const int HB_RPC_COOLDOWN_OPEN  = 777400;
    static const int HB_RPC_COOLDOWN_CLOSE = 777401;

    // Cooldown en secondes pour un transfert "server_switch"
    static const int HB_COOLDOWN_SECS = 90;

    protected static string OutPathFor(PlayerIdentity pid)
    {
        string root = HB_LogFile.ProfDir();
        string outdir = root + "\\outgoing";
        if (!FileExist(outdir)) MakeDirectory(outdir);
        return outdir + "\\" + pid.GetPlainId() + ".json";
    }

    protected static string InPathFor(PlayerIdentity pid)
    {
        string root = HB_LogFile.ProfDir();
        string indir = root + "\\incoming";
        return indir + "\\" + pid.GetPlainId() + ".json";
    }

    protected static void WriteReset(PlayerIdentity pid, string reason)
    {
        string path = OutPathFor(pid);

        HB_Payload pl = new HB_Payload();
        pl.Reset  = true;
        pl.Reason = reason;

        JsonFileLoader<HB_Payload>.JsonSaveFile(path, pl);
        HB_LogFile.Info("Export RESET (" + reason + "): " + path);
    }

    // -------- Export à la déconnexion
    static void SaveTransfer(PlayerIdentity pid, PlayerBase p)
    {
        if (!pid && p) pid = p.GetIdentity();
        if (!pid) {
            HB_LogFile.Warn("SaveTransfer: SKIP (no identity)");
            return;
        }

        // Cas RESET
        bool doReset = false;
        string reason = "";

        if (!p) {
            doReset = true;  reason = "no_player";
        }
        else if (!p.IsAlive()) {
            doReset = true;  reason = "dead";
        }
        else if (p.IsUnconscious()) {
            doReset = true;  reason = "unconscious";
        }
        else if (p.IsRestrained()) {
            doReset = true;  reason = "restrained";
        }

        if (doReset) {
            WriteReset(pid, reason);
            return;
        }

        // Export normal → marquer le passage de serveur
        string path = OutPathFor(pid);
        HB_Payload payload = HB_PayloadEx.FromPlayer(p);
        payload.Reason = "server_switch"; // clé pour déclencher le cooldown
        JsonFileLoader<HB_Payload>.JsonSaveFile(path, payload);
        HB_LogFile.Info("Export NORMAL: " + path);
    }

    // -------- Import à la connexion
    static void TryApplyTransfer(PlayerIdentity id, PlayerBase p)
    {
        if (!id) return;

        string path = InPathFor(id);
        if (!FileExist(path)) {
            HB_LogFile.Info("TryApplyTransfer: aucun paquet pour " + id.GetPlainId());
            return;
        }

        HB_Payload pl = new HB_Payload();
        JsonFileLoader<HB_Payload>.JsonLoadFile(path, pl);

        bool isReset = pl.Reset;
        if (!isReset && pl.Roots && pl.Roots.Count() == 0 && pl.Health <= 0 && pl.Blood <= 0) {
            isReset = true; // filet de sécurité
        }

        int cooldown = 0;
        if (!isReset && pl.Reason == "server_switch") {
            cooldown = HB_COOLDOWN_SECS;
        }

        if (p) p.SetAllowDamage(false);

        if (cooldown > 0) {
            // Ouvrir l’UI cooldown côté client
            ScriptRPC rpcOpen = new ScriptRPC();
            rpcOpen.Write(cooldown);
            rpcOpen.Send(null, HB_RPC_COOLDOWN_OPEN, true, id);

            // Contexte pour appliquer après le délai
            ref HB_ApplyCtx ctx = new HB_ApplyCtx();
            ctx.Identity   = id;
            ctx.Player     = p;
            ctx.Payload    = pl;
            ctx.PacketPath = path;

            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(__ApplyAfterCooldown, cooldown * 1000, false, ctx);
            HB_LogFile.Info("Cooldown " + cooldown.ToString() + "s (server_switch).");
            return;
        }

        __ApplyNow(id, p, pl, path);
    }

    private static void __ApplyAfterCooldown(HB_ApplyCtx ctx)
    {
        if (!ctx) return;

        // Fermer l’UI côté client
        ScriptRPC rpcClose = new ScriptRPC();
        rpcClose.Send(null, HB_RPC_COOLDOWN_CLOSE, true, ctx.Identity);

        __ApplyNow(ctx.Identity, ctx.Player, ctx.Payload, ctx.PacketPath);
    }

    private static void __ApplyNow(PlayerIdentity id, PlayerBase p, HB_Payload pl, string path)
    {
        if (!id || !p) {
            HB_LogFile.Warn("__ApplyNow: missing identity or player");
            return;
        }

        bool isReset = pl.Reset;
        if (!isReset && pl.Roots && pl.Roots.Count() == 0 && pl.Health <= 0 && pl.Blood <= 0) {
            isReset = true;
        }

        if (isReset) {
            HB_LogFile.Info("Import RESET (" + pl.Reason + ")");
            HB_Reset.ToFreshState(p);
            vector posN = HB_Spawn.SelectNormal();
            p.SetPosition(posN);
            HB_Spawn.EnsureOnGround(p);
        } else {
            HB_PayloadEx.ApplyTo(p, pl);
            vector posS = HB_Spawn.SelectSafe();
            p.SetPosition(posS);
            HB_Spawn.EnsureOnGround(p);
        }

        if (p) {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EndGhost, 12000, false, p);
        }

        if (!DeleteFile(path)) {
            HB_LogFile.Warn("Suppression du paquet échouée: " + path);
        } else {
            HB_LogFile.Info("Paquet consommé: " + path);
        }
    }

    static void TryApplyTransferDelayed(Param2<PlayerIdentity, PlayerBase> ctx)
    {
        if (!ctx) return;
        TryApplyTransfer(ctx.param1, ctx.param2);
    }

    static void EndGhost(PlayerBase p)
    {
        if (p) p.SetAllowDamage(true);
    }
}

class HB_ApplyCtx
{
    PlayerIdentity Identity;
    PlayerBase     Player;
    HB_Payload     Payload;
    string         PacketPath;
}
