class HB_Payload
{
    ref array<string> Items = new array<string>;
    float Health; float Blood; float Energy; float Water;

    static HB_Payload FromPlayer(PlayerBase p) {
        HB_Payload pl = new HB_Payload();
        pl.Health = p.GetHealth("","Health");
        pl.Blood  = p.GetHealth("","Blood");
        array<EntityAI> items = new array<EntityAI>();
        p.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        foreach (EntityAI it : items) {
            if (!it) continue;
            string line = it.GetType() + "|" + it.GetHealth("","Health");
            Magazine mag; if (Class.CastTo(mag, it)) line = line + "|" + mag.GetAmmoCount().ToString();
            pl.Items.Insert(line);
        }
        return pl;
    }
}

class HB_FileBridge
{
    static string Prof() { return "$profile:\\HiveBridge"; }

    static void SaveTransfer(PlayerBase p)
    {
        PlayerIdentity id = p.GetIdentity(); if (!id) return;
        string outdir = Prof() + "\\outgoing";
        if (!FileExist(outdir)) MakeDirectory(outdir);
        string path = outdir + "\\" + id.GetPlainId() + ".json";
        HB_Payload payload = HB_Payload.FromPlayer(p);
        JsonFileLoader<HB_Payload>.JsonSaveFile(path, payload);
        Print("[HiveBridge] wrote " + path);
    }

    static void TryApplyTransfer(PlayerIdentity id, PlayerBase p)
    {
        if (!id) return;
        string indir = Prof() + "\\incoming";
        string path  = indir + "\\" + id.GetPlainId() + ".json";
        if (!FileExist(path)) return;

        HB_Payload pl = new HB_Payload();
        if (!JsonFileLoader<HB_Payload>.JsonLoadFile(path, pl)) return;

        // 1) vider inventaire joueur puis recréer items
        p.RemoveAllItems();
        foreach (string line : pl.Items) {
            TStringArray toks = new TStringArray; line.Split("|", toks);
            if (toks.Count() >= 1) p.GetInventory().CreateInInventory(toks[0]);
        }
        // 2) stats simples
        p.SetHealth("", "Health", pl.Health);
        // 3) spawn safe + ghost
        vector pos = HB_Spawn.Select(); p.SetPosition(pos);
        p.SetAllowDamage(false);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(HB_FileBridge.EndGhost, 12000, false, p);

        // 4) consommer le fichier
        DeleteFile(path);
        Print("[HiveBridge] applied and consumed " + path);
    }

    static void EndGhost(PlayerBase p) { if (p) p.SetAllowDamage(true); }
}

class HB_Spawn
{
    static ref array<vector> Points;
    static void Ensure()
    {
        if (Points) return;
        Points = new array<vector>;
        // fallback: 2 points safe par défaut (à adapter)
        Points.Insert(Vector(13900,0,13200));
        Points.Insert(Vector(12500,0,11600));
        // tu peux aussi charger depuis $profile:\HiveBridge.json si présent
    }
    static vector Select() { Ensure(); return Points.Get(Math.RandomInt(0, Points.Count())); }
}
