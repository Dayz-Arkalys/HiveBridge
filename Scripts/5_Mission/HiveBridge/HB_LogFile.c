class HB_LogFile
{
	static string ProfDir() { return "$profile:\\HiveBridge"; }

	protected static void EnsureDir()
	{
		string dir = ProfDir();
		if (!FileExist(dir)) MakeDirectory(dir);
	}

	protected static string Timestamp()
	{
		int y, m, d, hh, mm, ss;
		GetYearMonthDay(y, m, d);
		GetHourMinuteSecond(hh, mm, ss);
		return y.ToString() + "-" + m.ToString() + "-" + d.ToString() + " " + hh.ToString() + ":" + mm.ToString() + ":" + ss.ToString();
	}

	protected static void Write(string level, string msg)
	{
		// RPT
		HB_Log.Info(msg);

		// Fichier (open → write → close)
		EnsureDir();
		string path = ProfDir() + "\\hivebridge.log";
		FileHandle fh = OpenFile(path, FileMode.APPEND);
		if (fh != 0)
		{
			FPrintln(fh, Timestamp() + " " + level + " " + msg);
			CloseFile(fh);
		}
	}

	static void Info(string msg)  { Write("INFO ",  msg); }
	static void Warn(string msg)  { Write("WARN ",  msg); }
	static void Error(string msg) { Write("ERROR",  msg); }
}
