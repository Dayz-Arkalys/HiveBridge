modded class MissionServer
{
    override void InvokeOnDisconnect(PlayerBase player)
    {
        super.InvokeOnDisconnect(player);
        HB_FileBridge.SaveTransfer(player);  // écrit JSON -> $profile:\HiveBridge\outgoing\<steamid>.json
    }

    override void OnClientNewEvent(PlayerIdentity id, PlayerBase player)
    {
        super.OnClientNewEvent(id, player);
        HB_FileBridge.TryApplyTransfer(id, player); // lit JSON incoming, applique l’inventaire, TP spawn safe, ghost
    }
}
