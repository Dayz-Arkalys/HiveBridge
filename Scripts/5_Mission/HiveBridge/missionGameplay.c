modded class MissionGameplay
{
    protected ref HB_CooldownMenu m_HBMenu;

    static const int HB_RPC_COOLDOWN_OPEN  = 777400;
    static const int HB_RPC_COOLDOWN_CLOSE = 777401;

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (rpc_type == HB_RPC_COOLDOWN_OPEN)
        {
            Param1<int> p;
            if (!ctx.Read(p)) return;

            if (!m_HBMenu) {
                m_HBMenu = HB_CooldownMenu.Cast(GetGame().GetUIManager().EnterScriptedMenu(MENU_CUSTOM1, null));
            }
            if (m_HBMenu) m_HBMenu.OpenFor(p.param1);
        }
        else if (rpc_type == HB_RPC_COOLDOWN_CLOSE)
        {
            if (m_HBMenu) {
                GetGame().GetUIManager().CloseMenu(MENU_CUSTOM1);
                m_HBMenu = null;
            }
        }
    }
}
