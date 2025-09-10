modded class MissionServer
{
	void MissionServer()
	{
		HB_LogFile.Info("Chargement du mod HiveBridge v" + HB_Version.VERSION);
	}

	void ~MissionServer()
	{
		HB_LogFile.Info("Arrêt MissionServer");
	}

	// Se déclenche quand le perso est créé (fresh spawn seulement)
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		if (player && identity) {
			HB_LogFile.Info("OnClientNewEvent: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
			if (!player.m_HB_Applied) {
				HB_LogFile.Info("Applying transfer on ReadyEvent for " + identity.GetPlainId());
				HB_FileBridge.TryApplyTransfer(identity, player); // inventaire + TP + ghost
				player.m_HB_Applied = true;
			} else {
				HB_LogFile.Info("Transfer already applied for this session.");
			}		
		}
		return player;
	}

	// Se déclenche à chaque connexion quand le client est prêt (nouveau ET reconnect)
	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);

		if (!identity || !player) return;

		HB_LogFile.Info("OnClientReadyEvent: " + identity.GetName() + " (" + identity.GetPlainId() + ")");

		if (!player.m_HB_Applied) {
			HB_LogFile.Info("Applying transfer on ReadyEvent for " + identity.GetPlainId());
			HB_FileBridge.TryApplyTransfer(identity, player); // inventaire + TP + ghost
			player.m_HB_Applied = true;
		} else {
			HB_LogFile.Info("Transfer already applied for this session.");
		}
	}

	// Sauvegarde le paquet à la déconnexion
	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
	    super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
	
	    if (!player || !identity) return;
	
	    if (!player.IsAlive())
	    {
	        HB_LogFile.Info("OnClientDisconnectedEvent: no export (dead)");
	        return;
	    }
	    if (player.IsUnconscious())
	    {
	        HB_LogFile.Info("OnClientDisconnectedEvent: no export (unconscious)");
	        return;
	    }
	
	    HB_LogFile.Info("OnClientDisconnectedEvent: export allowed for " + identity.GetPlainId());
	    HB_FileBridge.SaveTransfer(player);
	}

}
