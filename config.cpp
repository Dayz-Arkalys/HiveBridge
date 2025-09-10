class CfgPatches
{
    class HiveBridge { units[] = {}; weapons[] = {}; requiredVersion = 0.1; requiredAddons[] = {"DZ_Data"}; };
};
class CfgMods
{
    class HiveBridge
    {
        dir = "HiveBridge";
        name = "HiveBridge";
        author = "Toi";
        type = "mod";
        dependencies[] = {"Game","World","Mission"};
        class defs
        {
            class gameScriptModule   { files[] = {"HiveBridge/Scripts/3_Game"};   };
            class worldScriptModule  { files[] = {"HiveBridge/Scripts/4_World"};  };
            class missionScriptModule{ files[] = {"HiveBridge/Scripts/5_Mission"};};
        };
    };
};
