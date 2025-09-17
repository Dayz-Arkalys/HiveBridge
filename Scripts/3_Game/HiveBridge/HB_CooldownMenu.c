class HB_CooldownMenu : UIScriptedMenu
{
    protected TextWidget m_Timer;
    protected TextWidget m_Info;
    protected ref Timer  m_Tick;
    protected int        m_Remaining;

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("HiveBridge/gui/cooldown.layout");
        m_Timer = TextWidget.Cast(layoutRoot.FindAnyWidget("Timer"));
        m_Info  = TextWidget.Cast(layoutRoot.FindAnyWidget("Info"));
        GetGame().GetInputManager().ActivateContext("UINone"); // optionnel
        return layoutRoot;
    }

    void OpenFor(int secs)
    {
        m_Remaining = Math.Max(0, secs);
        UpdateLabel();

        if (!m_Tick) m_Tick = new Timer(CALL_CATEGORY_GUI);
        m_Tick.Run(1.0, this, "Tick", null, true);
    }

    void Tick()
    {
        m_Remaining = Math.Max(0, m_Remaining - 1);
        UpdateLabel();
        if (m_Remaining == 0) Close();
    }

    void UpdateLabel()
    {
        int mm = m_Remaining / 60;
        int ss = m_Remaining % 60;
        if (m_Timer) m_Timer.SetText(mm.ToStringLen(2) + ":" + ss.ToStringLen(2));
    }

    override bool CanClose() { return false; }

    override void OnHide()
    {
        if (m_Tick) m_Tick.Stop();
        GetGame().GetInputManager().DeactivateContext("UINone");
        super.OnHide();
    }
}
