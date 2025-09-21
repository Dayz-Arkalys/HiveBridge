// -----------------------------------------------------------------------------
// HB_Quickbar : capture et applique les raccourcis (quickbar) 0..9
// -----------------------------------------------------------------------------
class HB_Quickbar
{
	// ----- CAPTURE ------------------------------------------------------------
	static void Capture(PlayerBase p, HB_Payload pl)
	{
		if (!p || !pl || !pl.Quickbar) return;
		pl.Quickbar.Clear();

		for (int i = 0; i < 10; i++)
		{
			EntityAI e = p.GetQuickBarEntity(i);       // DayZ expose cette API sur PlayerBase
			if (!e) continue;

			HB_QBSlot s = new HB_QBSlot();
			s.Index = i;
			s.Type  = e.GetType();
			s.Hands = (p.GetItemInHands() == e);

			// Optionnel : nom de slot d’attache s’il existe
			InventoryLocation il = new InventoryLocation();
			if (e.GetInventory() && e.GetInventory().GetCurrentInventoryLocation(il))
			{
				int slotId = il.GetSlot();
				if (slotId != -1)
				{
					string sn;
					InventorySlots.GetSlotName(slotId, sn);
					s.SlotName = sn;
				}
			}

			pl.Quickbar.Insert(s);
		}

		HB_Log.Info("[HB_QB] captured " + pl.Quickbar.Count().ToString() + " shortcuts");
	}

	// ----- APPLY --------------------------------------------------------------
	// On applique après l’import de l’inventaire → petit délai
	static void Apply(PlayerBase p, HB_Payload pl)
	{
		if (!p || !pl || !pl.Quickbar || pl.Quickbar.Count() == 0) return;
		autoptr Param2<PlayerBase, ref array<ref HB_QBSlot>> ctx = new Param2<PlayerBase, ref array<ref HB_QBSlot>>(p, pl.Quickbar);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(_ApplyNow, 250, false, ctx);
	}

	protected static void _ApplyNow(Param2<PlayerBase, ref array<ref HB_QBSlot>> ctx)
	{
		if (!ctx) return;
		PlayerBase p = ctx.param1;
		ref array<ref HB_QBSlot> arr = ctx.param2;
		if (!p || !arr) return;

		foreach (HB_QBSlot s : arr)
		{
			EntityAI e = _FindCandidate(p, s.Type, s.Hands, s.SlotName);
			if (e)
			{
				// forceSwap=true pour écraser un slot occupé
				p.SetQuickBarEntityShortcut(e, s.Index, true);
			}
		}
		HB_Log.Info("[HB_QB] applied " + arr.Count().ToString() + " shortcuts");
	}

	// Trouve un item correspondant dans l’inventaire du joueur
	protected static EntityAI _FindCandidate(PlayerBase p, string type, bool preferHands, string slotName)
	{
		// 1) mains en priorité si demandé
		EntityAI h = p.GetItemInHands();
		if (preferHands && h && h.GetType() == type) return h;

		// 2) attache par nom de slot (Shoulder, Melee, Headgear, Vest, etc.)
		if (slotName && slotName != "")
		{
			EntityAI a = p.FindAttachmentBySlotName(slotName);
			if (a && a.GetType() == type) return a;
		}

		// 3) fouille cargo direct du joueur
		CargoBase c = p.GetInventory().GetCargo();
		if (c)
		{
			for (int y = 0; y < c.GetHeight(); y++)
			for (int x = 0; x < c.GetWidth();  x++)
			{
				EntityAI it = c.GetItem(x, y);
				if (it && it.GetType() == type) return it;
			}
		}

		// 4) en dernier ressort, si l’item est en mains
		if (h && h.GetType() == type) return h;

		return null;
	}
}
