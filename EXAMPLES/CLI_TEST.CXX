#include <TOOLKIT/CLI.H>

int main()
{
	I32  a = 0;
	F32  b = 0;
	BOOL c = false;
	CSTR d = "Default";
	PATH e = "V:/Default/Path/To/Your/Folder/Or/File";

	CHAR arg0[] = "cli_test.exe";
    CHAR arg1[] = "-a";
    CHAR arg2[] = "-5325";
    CHAR arg3[] = "-b=3.1415";
    CHAR arg4[] = "-c";
    CHAR arg5[] = "-d=My String"; // Anmerkung zu den Anführungszeichen siehe unten
    CHAR arg6[] = "-e";
    CHAR arg7[] = "Relative/Path/To/A/File";
	PCHAR argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
	int argc = arrsize(argv);

	ARGOPTION options[] =
	{
		{ "a", 1, ARGTYPE_I32, &a },
		{ "b", 1, ARGTYPE_F32, &b },
		{ "c", 0, ARGTYPE_BOOL, &c },
		{ "d", 1, ARGTYPE_CSTR, &d },
		{ "e", 1, ARGTYPE_PATH, e }, 
	};
	BOOL success = LoadArgs(argc, argv, arrsize(options), options);
	if (success)
	{
		succln("Parsed arguments successfully:");
		succln("a = %d", a);
		succln("b = %f", b);
		succln("c = %s", c ? "true" : "false");
		succln("d = %s", d);
		succln("e = %s", e);
	}
	else
	{
		errln("Failed to parse arguments.");
	}

	for (U64 i = 0; i < (U64)1e9; i++)
	{
		if (i % (U64)1e7 == 0)
			Progress(LOGGING_SPECIAL, (I32)(i / 1e7), "Processing...");
	}
}