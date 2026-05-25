#include <TOOLKIT/CLI.H>
#include <TOOLKIT/FILE.H>
#include <TOOLKIT/ALLOCATOR.H>
#include <TOOLKIT/FILESYSTEM.H>

int main()
{
	PathSetWorkingDirToExecutable();

	PATH cache_path;
	FSTR cache_file = PathAbs(fstrfromstr("../CMakeCache.txt"), cache_path);

	FILEDATA f;
	// Read the entire content of the CMakeCache.txt file
	if (BeginFile(DefaultAllocator(), cache_file, FILEMODE_READ, 0, &f) == OK)
	{
		FSTR content;
		RESULT rc = FileReadAll(&f, true, &content);
		if (rc == OK)
		{
			noteln("File content:\n%.*s", (I32)content.size, content.str);
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
	if (FileOpen(fstrfromstr("test.ini"), FILEMODE_WRITETEXT, &f) == OK)
	{
		IniGroup(&f, fstrfromstr("Settings"));
		IniFloat(&f, fstrfromstr("Volume"), 0.75f);
		IniInteger(&f, fstrfromstr("ResolutionWidth"), 1920);
		IniBool(&f, fstrfromstr("Fullscreen"), true);

		IniGroup(&f, fstrfromstr("User"));
		IniString(&f, fstrfromstr("Username"), fstrfromstr("JohnDoe"));
		IniInteger(&f, fstrfromstr("Age"), 42);

		FileClose(&f);
	}
	// Read back the INI file to verify it was written correctly
	INIENTRY entry;
	FileOpen(fstrfromstr("test.ini"), FILEMODE_READTEXT, &f);
	while (IniReadNextEntry(&f, &entry) == OK)
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
	FileClose(&f);

	// Read back the JSON file to verify it was written correctly
	if (FileOpen(fstrfromstr("test2.json"), FILEMODE_READTEXT, &f) == OK)
	{
		JSONENTRY json_entry;
		while (JsonReadNextEntry(&f, &json_entry) == OK)
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
		FileClose(&f);
	}

	// Write a JSON file 
	if (FileOpen(fstrfromstr("test2.json"), FILEMODE_WRITETEXT, &f) == OK)
	{
		JsonBegin(&f);
		{
			JsonObjectBegin(&f, fstrfromstr("player"));
			{
				JsonString(&f, fstrfromstr("name"), fstrfromstr("Alice"));
				JsonInteger(&f, fstrfromstr("score"), 12345);
				JsonFloat(&f, fstrfromstr("health"), 0.85f);
				JsonArrayBegin(&f, fstrfromstr("position"));
				for (I32 i = 0; i < 3; i++)
				{
					JsonFloat(&f, FSTR_INVALID, (F32)(i * 1.5f));
				}
				JsonArrayEnd(&f);

				JsonArrayBegin(&f, fstrfromstr("inventory"));
				{
					JsonString(&f, FSTR_INVALID, fstrfromstr("Sword"));
					JsonString(&f, FSTR_INVALID, fstrfromstr("Hammer"));
				}
				JsonArrayEnd(&f);

				JsonObjectBegin(&f, fstrfromstr("skills"));
				JsonObjectEnd(&f);
				JsonFloat(&f, fstrfromstr("movement speed"), 4.512331);
			}
			JsonObjectEnd(&f);
		}
		JsonEnd(&f);
	}

	

	EndFile(&f);

	return 0;
}