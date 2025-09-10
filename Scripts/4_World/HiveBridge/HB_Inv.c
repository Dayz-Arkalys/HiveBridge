// ===================== Inventory serialization (HB_Inv) ======================

class HB_Location
{
	string kind;      // "HANDS" | "ATTACH" | "CARGO" | "OTHER"
	int    slot_id = -1;
	string slot_name;

	int  idx  = -1;   // cargo index
	int  row  = -1;   // cargo row
	int  col  = -1;   // cargo col
	bool flip = false;
}

class HB_Item
{
	string type;
	float  health = 100.0;

	int qty  = -1;    // ItemBase quantity (pile, food, fluid, etc.)
	int ammo = -1;    // Magazine ammo count

	ref HB_Location loc;
	ref array<ref HB_Item> attachments;
	ref array<ref HB_Item> cargo;

	void HB_Item()
	{
		loc = new HB_Location();
		attachments = new array<ref HB_Item>();
		cargo = new array<ref HB_Item>();
	}
}

class HB_Payload
{
	float Health;
	float Blood;

	// --- États (nouveau)
	bool  Bleeding = false;   // le joueur saignait au moment de l’export ?
	int   AgentsMask = 0;     // bitmask des infections (si dispo)

	bool  Reset = false;
	string Reason;

	ref array<ref HB_Item> Roots;

	void HB_Payload()
	{
		Roots = new array<ref HB_Item>();
	}
}



// ---------- Core: build / spawn tree ----------------------------------------

class HB_Inv
{

	static void DeleteTree(EntityAI e)
    {
        if (!e) return;

        // cargo d'abord
        CargoBase c = e.GetInventory().GetCargo();
        if (c)
        {
            for (int i = c.GetItemCount() - 1; i >= 0; i--)
            {
                EntityAI child = c.GetItem(i);
                DeleteTree(child);
            }
        }

        // attachments ensuite
        int ac = e.GetInventory().AttachmentCount();
        for (int ai = ac - 1; ai >= 0; ai--)
        {
            EntityAI att = e.GetInventory().GetAttachmentFromIndex(ai);
            DeleteTree(att);
        }

        // enfin l'item lui-même
        GetGame().ObjectDelete(e);
    }

    // Nettoie l'inventaire du joueur sans RemoveAllItems()
    static void RemoveAllSafe(PlayerBase p)
    {
        if (!p) return;

        // mains
        EntityAI h = p.GetHumanInventory().GetEntityInHands();
        if (h) GetGame().ObjectDelete(h);

        // attachments du joueur (vêtements, sac, armes portées, etc.)
        int ac = p.GetInventory().AttachmentCount();
        for (int i = ac - 1; i >= 0; i--)
        {
            EntityAI att = p.GetInventory().GetAttachmentFromIndex(i);
            DeleteTree(att);
        }
    }
	// Build one node from an EntityAI (recursive)
	static HB_Item FromEntity(EntityAI e)
	{
		HB_Item node = new HB_Item();
		node.type   = e.GetType();
		node.health = e.GetHealth("", "Health");

		// Generic quantities
		ItemBase ib;
		if (Class.CastTo(ib, e) && ib.HasQuantity())
			node.qty = ib.GetQuantity();

		// Magazines
		Magazine mag;
		if (Class.CastTo(mag, e))
			node.ammo = mag.GetAmmoCount();

		// Location
		InventoryLocation il = new InventoryLocation();
		if (e.GetInventory() && e.GetInventory().GetCurrentInventoryLocation(il))
		{
			int t = il.GetType();
			if (t == InventoryLocationType.ATTACHMENT)
			{
				node.loc.kind      = "ATTACH";
				node.loc.slot_id   = il.GetSlot();
				node.loc.slot_name = InventorySlots.GetSlotName(node.loc.slot_id);
			}
			else if (t == InventoryLocationType.CARGO)
			{
				node.loc.kind = "CARGO";
				node.loc.idx  = il.GetIdx();
				node.loc.row  = il.GetRow();
				node.loc.col  = il.GetCol();
				node.loc.flip = il.GetFlip();
			}
			else if (t == InventoryLocationType.HANDS)
			{
				node.loc.kind = "HANDS";
			}
			else
			{
				node.loc.kind = "OTHER";
			}
		}

		// Children: ATTACHMENTS
		int ac = e.GetInventory().AttachmentCount();
		for (int ai = 0; ai < ac; ai++)
		{
			EntityAI att = e.GetInventory().GetAttachmentFromIndex(ai);
			if (att) node.attachments.Insert(FromEntity(att));
		}

		// Children: CARGO
		CargoBase c = e.GetInventory().GetCargo();
		if (c)
		{
			int n = c.GetItemCount();
			for (int ci = 0; ci < n; ci++)
			{
				EntityAI child = c.GetItem(ci);
				if (child) node.cargo.Insert(FromEntity(child));
			}
		}

		return node;
	}

	// Spawn one node into parent (recursive)
	static EntityAI SpawnInto(EntityAI parent, HB_Item src)
	{
		EntityAI spawned = null;

		if (src.loc.kind == "HANDS")
		{
			PlayerBase pb = PlayerBase.Cast(parent);
			if (pb)
			{
				spawned = parent.GetInventory().CreateInInventory(src.type);
				if (spawned) pb.PredictiveTakeEntityToHands(spawned);
			}
		}
		else if (src.loc.kind == "ATTACH")
		{
			// Simple & robuste : laisse le jeu placer au slot compatible
			spawned = parent.GetInventory().CreateAttachment(src.type);
			if (!spawned) spawned = parent.GetInventory().CreateInInventory(src.type);
		}
		else if (src.loc.kind == "CARGO")
		{
			spawned = parent.GetInventory().CreateEntityInCargoEx(src.type, src.loc.idx, src.loc.row, src.loc.col, src.loc.flip);
			if (!spawned) spawned = parent.GetInventory().CreateInInventory(src.type);
		}
		else
		{
			spawned = parent.GetInventory().CreateInInventory(src.type);
		}

		if (!spawned) return null;

		// Properties
        ItemBase ib; Magazine mag;
        if (Class.CastTo(ib, spawned))
        {
            if (src.qty >= 0) ib.SetQuantity(src.qty);
            spawned.SetHealth("", "Health", src.health);
        }
        if (Class.CastTo(mag, spawned) && src.ammo >= 0)
        {
            // DayZ: pas de SetAmmoCount(); on utilise ServerSetAmmoCount()
            mag.ServerSetAmmoCount(src.ammo);   // valeur exacte
        }


		// Recurse
		foreach (HB_Item a : src.attachments)
		{
			HB_Inv.SpawnInto(spawned, a);
		}
		foreach (HB_Item cg : src.cargo)
		{
			HB_Inv.SpawnInto(spawned, cg);
		}

		return spawned;
	}
}

// ---------- High-level payload helpers ---------------------------------------

class HB_PayloadEx
{
	static HB_Payload FromPlayer(PlayerBase p)
	{
		HB_Payload pl = new HB_Payload();
		pl.Health = p.GetHealth("", "Health");
		pl.Blood  = p.GetHealth("", "Blood");

        HB_State.Capture(p, pl);

		// Attachments worn (vest, backpack, clothes, weapon-on-shoulder, etc.)
		int ac = p.GetInventory().AttachmentCount();
		for (int i = 0; i < ac; i++)
		{
			EntityAI att = p.GetInventory().GetAttachmentFromIndex(i);
			if (att) pl.Roots.Insert(HB_Inv.FromEntity(att));
		}

		// Hands
		EntityAI hands = p.GetHumanInventory().GetEntityInHands();
		if (hands)
		{
			HB_Item ih = HB_Inv.FromEntity(hands);
			ih.loc.kind = "HANDS";
			pl.Roots.Insert(ih);
		}

		return pl;
	}

	static void ApplyTo(PlayerBase p, HB_Payload pl)
	{
		HB_Inv.RemoveAllSafe(p);
		p.SetHealth("", "Health", pl.Health);
		p.SetHealth("", "Blood",  pl.Blood);

		foreach (HB_Item root : pl.Roots)
		{
			HB_Inv.SpawnInto(p, root);
		}
        HB_State.Apply(p, pl);
	}
}
