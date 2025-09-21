modded class PlayerBase
{
    protected ref HB_CooldownMenu m_HBMenu;

    // mêmes IDs que côté serveur
    static const int HB_RPC_COOLDOWN_OPEN  = 777400;
    static const int HB_RPC_COOLDOWN_CLOSE = 777401;

    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (rpc_type == HB_RPC_COOLDOWN_OPEN)
        {
            Param1<int> p;
            if (!ctx.Read(p)) return;

            if (!m_HBMenu)
            {
                m_HBMenu = new HB_CooldownMenu();
                GetGame().GetUIManager().ShowScriptedMenu(m_HBMenu, null);
            }
            if (m_HBMenu) m_HBMenu.OpenFor(p.param1);
        }
        else if (rpc_type == HB_RPC_COOLDOWN_CLOSE)
        {
            if (m_HBMenu)
            {
                GetGame().GetUIManager().HideScriptedMenu(m_HBMenu);
                m_HBMenu = null;
            }
        }
    }
}
