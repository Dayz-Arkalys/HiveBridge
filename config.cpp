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
            class gameScriptModule   { files[] = {"HiveBridge/scripts/3_Game"};   };
            class worldScriptModule  { files[] = {"HiveBridge/scripts/4_World"};  };
            class missionScriptModule{ files[] = {"HiveBridge/scripts/5_Mission"};};
        };
    };
};
