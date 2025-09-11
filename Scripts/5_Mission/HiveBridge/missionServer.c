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

	// Fresh spawn uniquement (création du perso)
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		if (player && identity) {
			HB_LogFile.Info("OnClientNewEvent: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
		}
		return player;
	}

	// Passe à chaque connexion (nouveau ET reconnect)
	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		if (!identity || !player) return;

		HB_LogFile.Info("OnClientReadyEvent: " + identity.GetName() + " (" + identity.GetPlainId() + ")");

		if (!player.m_HB_Applied) {
			player.m_HB_Applied = true;

			// NE PAS passer 2 args à CallLater : on emballe dans un Param2
			autoptr Param2<PlayerIdentity, PlayerBase> ctx = new Param2<PlayerIdentity, PlayerBase>(identity, player);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(HB_FileBridge.TryApplyTransferDelayed, 50, false, ctx);
		} else {
			HB_LogFile.Info("Transfer already applied for this session.");
		}
	}

	// Toujours exporter à la déconnexion :
	// - si vivant/conscient -> export normal
	// - si mort/inconscient -> export RESET (décidé dans SaveTransfer)
	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
		super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
		if (!player || !identity) return;

		HB_LogFile.Info( "OnClientDisconnectedEvent: export packet for " + identity.GetPlainId() + " (alive=" + player.IsAlive().ToString() + ", unconscious=" + player.IsUnconscious().ToString() +", restrained=" + player.IsRestrained().ToString() + ")" );
		HB_FileBridge.SaveTransfer(identity, player);

	}
}
