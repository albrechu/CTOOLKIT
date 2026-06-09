#include <TOOLKIT/CLI.H>
#include <TOOLKIT/FILE.H>
#include <TOOLKIT/ALLOCATOR.H>
#include <TOOLKIT/FILESYSTEM.H>

int main()
{
	PathSetWorkingDirToExecutable();

	PATH cache_path;
	SV cache_file = PathAbs(svlit("../CMakeCache.txt"), cache_path);

	PFILEDATA f;
	// Read the entire content of the CMakeCache.txt file
	if (IS_OK(LoadFile(null, cache_file, FILEMODE_READ, 0, &f)))
	{
		SV content;
		RESULT rc = FileReadAll(f, true, &content);
		if (rc == OK)
		{
			noteln("File content:\n%.*s", (I32)content.Size, content.String);
		}
		else
		{
			errln("Failed to read file. Result code: %d", rc);
		}
	}
	else
	{
		errln("Failed to open file.");
	}
	// Write an INI file with various types of values
	if (IS_OK(FileOpen(f, svlit("test.ini"), FILEMODE_WRITETEXT)))
	{
		IniGroup(f, svlit("Settings"));
		IniFloat(f, svlit("Volume"), 0.75f);
		IniInteger(f, svlit("ResolutionWidth"), 1920);
		IniBool(f, svlit("Fullscreen"), true);

		IniGroup(f, svlit("User"));
		IniString(f, svlit("Username"), svlit("JohnDoe"));
		IniInteger(f, svlit("Age"), 42);

		FileClose(f);
	}
	// Read back the INI file to verify it was written correctly
	INIENTRY entry;
	if (IS_OK(FileOpen(f, svlit("test.ini"), FILEMODE_READTEXT)))
	{
	   while (IniReadNextEntry(f, &entry) == OK)
	   {
		   switch (entry.ValueType)
		   {
		   case VALUETYPE_BOOL:
			   noteln("[%.*s] %.*s = %s", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.Boolean ? "true" : "false");
			   break;
		   case VALUETYPE_FLOAT:
			   noteln("[%.*s] %.*s = %f", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.Float);
			   break;
		   case VALUETYPE_INT:
			   noteln("[%.*s] %.*s = %d", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.Int);
			   break;
		   case VALUETYPE_STRING:
			   noteln("[%.*s] %.*s = %.*s", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name, entry.Value.String.Length, entry.Value.String.Name);
			   break;
		   default:
			   errln("Unknown value type for key [%.*s] %.*s", entry.Group.Length, entry.Group.Name, entry.Key.Length, entry.Key.Name);
			   break;
		   }
	   }
	   FileClose(f);
	}

	// Read back the JSON file to verify it was written correctly
	if (FileOpen(f, svlit("test2.json"), FILEMODE_READTEXT) == OK)
	{
		JSONENTRY json_entry;
		while (JsonReadNextEntry(f, &json_entry) == OK)
		{
			switch (json_entry.ValueType)
			{
			case VALUETYPE_BOOL:
				noteln("%.*s = %s", json_entry.Key.Length, json_entry.Key.Name, json_entry.Value.Boolean ? "true" : "false");
				break;
			case VALUETYPE_FLOAT:
				noteln("%.*s = %f", json_entry.Key.Length, json_entry.Key.Name, json_entry.Value.Float);
				break;
			case VALUETYPE_INT:
				noteln("%.*s = %d", json_entry.Key.Length, json_entry.Key.Name, json_entry.Value.Int);
				break;
			case VALUETYPE_STRING:
				noteln("%.*s = %.*s", json_entry.Key.Length, json_entry.Key.Name, json_entry.Value.String.Length, json_entry.Value.String.Name);
				break;
			case VALUETYPE_ARRAY:
				noteln("%.*s = [Array]", json_entry.Key.Length, json_entry.Key.Name);
				break;
			case VALUETYPE_OBJECT:
				if (json_entry.Key.Length)
					noteln("%.*s = {Object}", json_entry.Key.Length, json_entry.Key.Name);
				break;
			case VALUETYPE_NULL:
				noteln("%.*s = null", json_entry.Key.Length, json_entry.Key.Name);
				break;
			default:
				errln("Unknown value type for key %.*s", json_entry.Key.Length, json_entry.Key.Name);
				break;
			}
		}
		FileClose(f);
	}

	// Write a JSON file 
	if (FileOpen(f, svlit("test2.json"), FILEMODE_WRITETEXT) == OK)
	{
		JsonBegin(f);
		{
			JsonObjectBegin(f, svlit("player"));
			{
				JsonString(f, svlit("name"), svlit("Alice"));
				JsonInteger(f, svlit("score"), 12345);
				JsonFloat(f, svlit("health"), 0.85f);
				JsonArrayBegin(f, svlit("position"));
				for (I32 i = 0; i < 3; i++)
				{
					JsonFloat(f, SV_INVALID, (F32)(i * 1.5f));
				}
				JsonArrayEnd(f);

				JsonArrayBegin(f, svlit("inventory"));
				{
					JsonString(f, SV_INVALID, svlit("Sword"));
					JsonString(f, SV_INVALID, svlit("Hammer"));
				}
				JsonArrayEnd(f);

				JsonObjectBegin(f, svlit("skills"));
				JsonObjectEnd(f);
				JsonFloat(f, svlit("movement speed"), 4.512331);
			}
			JsonObjectEnd(f);
		}
		JsonEnd(f);
	}
	FreeFile(f);

	return 0;
}