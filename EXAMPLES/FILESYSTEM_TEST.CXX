#include <TOOLKIT/FILESYSTEM.H>
#include <TOOLKIT/CLI.H>

int main()
{
	// Get a directory from the user and list some of its contents
	OPATH dir;
	FSTR result = OpenDirDialog(dir);
	if (fstrinvalid(result))
	{
		infoln("Directory selection cancelled or failed.");
		return 0;
	}

	PFILESYSTEM fs = LoadFilesystem(DefaultAllocator(), result);
	// List all entries
	U64 count;
	PDET entries = FilesystemEntries(fs, &count);
	for (U64 i = 0; i < count; i++)
	{
		succln(entries[i].is_dir ? "[DIR]  %.*s" : "[FILE] %.*s", (int)entries[i].name.size, entries[i].name.str);
	}
	// Go into the first directory we find
	for (U64 i = 0; i < count; i++)
	{
		if (entries[i].is_dir)
		{
			FilesystemGoto(fs, entries[i].name);
			break;
		}
	}
	// List files of the first directory we find
	PDET files = FilesystemFiles(fs, &count);
	for (U64 i = 0; i < count; i++)
	{
		succln("\t[FILE] %.*s", (int)files[i].name.size, files[i].name.str);
	}
	// Go back up to the parent directory
	_ = FilesystemUp(fs);
	// Release
	FreeFilesystem(fs);
	// Assertion 
	Assert(false, "This is a test assertion failure to demonstrate the dialog. Click 'Yes' to break into the debugger.");
	// Notify the user that the test is finished
	NotifyDialog("Filesystem Test", "Finished testing filesystem functions. Check the console output for results.");
}
