class HB_Reset
{
	// vide le cargo d'un container (vêtements, sac, gilet)
	protected static void ClearCargo(EntityAI cont)
	{
		CargoBase c = cont.GetInventory().GetCargo();
		if (!c) return;

		// on supprime du fond vers le début pour éviter les décalages
		for (int i = c.GetItemCount() - 1; i >= 0; i--)
		{
			EntityAI item = c.GetItem(i);
			if (item) item.Delete();
		}
	}

	// remove armes/objets non-vêtements, vider cargos des vêtements/gilet/sac
	protected static void StripToClothes(PlayerBase p)
	{
		// mains
		EntityAI hands = p.GetHumanInventory().GetEntityInHands();
		if (hands) { hands.Delete(); }

		// attachments
		int ac = p.GetInventory().AttachmentCount();
		for (int i = ac - 1; i >= 0; i--)
		{
			EntityAI att = p.GetInventory().GetAttachmentFromIndex(i);
			if (!att) continue;

			Clothing cloth;
			if (Class.CastTo(cloth, att))
			{
				// garder le vêtement mais vider son cargo
				ClearCargo(att);
				continue;
			}

			// pas un vêtement (ex: arme, ceinture utilitaire, etc.) → supprimer
			att.Delete();
		}
	}

	// petit kit de départ (optionnel, minimaliste)
	protected static void GiveStarterKit(PlayerBase p)
	{
		ItemBase rags = ItemBase.Cast(p.GetInventory().CreateInInventory("Rag"));
		if (rags) rags.SetQuantity(4);

		p.GetInventory().CreateInInventory("Chemlight_White");

		string fruits[3] = {"Apple","Pear","Plum"};
		p.GetInventory().CreateInInventory(fruits[Math.RandomInt(0,3)]);
	}

	static void ToFreshState(PlayerBase p)
	{
		if (!p) return;

		// Santé de base (on ne réanime pas si mort ailleurs ; ici on reset l'équipement)
		// Laisse Health/Blood tels quels pour ne pas "soigner gratis".
		StripToClothes(p);
		GiveStarterKit(p);
	}
}
