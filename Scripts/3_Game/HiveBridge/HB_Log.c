class HB_Version { static const string VERSION = "0.1.1"; }

class HB_Log
{
	static const string PREFIX = "[HiveBridge] ";

	static void Info(string msg)  { Print(PREFIX + msg); }
	static void Warn(string msg)  { Print(PREFIX + "WARN "  + msg); }
	static void Error(string msg) { Print(PREFIX + "ERROR " + msg); }
}
