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
			EntityAI e = p.GetQuickBarEntity(i);
			if (!e) continue;

			HB_QBSlot s = new HB_QBSlot(); // <-- classe définie UNE SEULE FOIS dans hb_inv.c
			s.Index = i;
			s.Type  = e.GetType();
			s.Hands = (p.GetItemInHands() == e);

			InventoryLocation il = new InventoryLocation();
			if (e.GetInventory() && e.GetInventory().GetCurrentInventoryLocation(il))
			{
				int slotId = il.GetSlot();
				if (slotId != -1) s.SlotName = InventorySlots.GetSlotName(slotId);
			}

			pl.Quickbar.Insert(s);
		}

		HB_Log.Info("[HB_QB] captured " + pl.Quickbar.Count().ToString() + " shortcuts");
	}

	// ----- APPLY --------------------------------------------------------------
	static void Apply(PlayerBase p, HB_Payload pl)
	{
		if (!p || !pl || !pl.Quickbar || pl.Quickbar.Count() == 0) return;
		autoptr Param2<PlayerBase, ref array<ref HB_QBSlot>> ctx =
			new Param2<PlayerBase, ref array<ref HB_QBSlot>>(p, pl.Quickbar);
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
			if (e) p.SetQuickBarEntityShortcut(e, s.Index, true); // forceSwap
		}
		HB_Log.Info("[HB_QB] applied " + arr.Count().ToString() + " shortcuts");
	}

	// ----------------- helpers de recherche -----------------------------------
	protected static EntityAI _FindCandidate(PlayerBase p, string type, bool preferHands, string slotName)
	{
		EntityAI h = p.GetItemInHands();
		if (preferHands && h && h.GetType() == type) return h;

		if (slotName && slotName != "")
		{
			EntityAI a = p.FindAttachmentBySlotName(slotName);
			if (a && a.GetType() == type) return a;
		}

		CargoBase c = p.GetInventory().GetCargo();
		if (c)
		{
			int n = c.GetItemCount();
			for (int i = 0; i < n; i++)
			{
				EntityAI it = c.GetItem(i);
				if (it && it.GetType() == type) return it;
			}
		}

		if (h && h.GetType() == type) return h;
		return null;
	}
}
