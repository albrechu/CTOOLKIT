#include <TOOLKIT/FILE.H>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <TOOLKIT/CLI.H>
#include <TOOLKIT/COMPRESSION.H>
#include <TOOLKIT/ALLOCATOR.H>
#include <TOOLKIT/STRING.H>

RESULT ErrnoAsResult(errno_t err)
{
	switch (err)
	{
	case 0:      return OK;
	case EACCES: return ACCESS_DENIED;
	case ENOMEM: return OUT_OF_MEMORY;
	case EBUSY:  return BUSY;
	default:     return UNDEFINED_ERROR;
	}
}

RESULT BeginFile(ALLOCATOR allocator, FSTR path, FILEMODE mode, U64 initial_capacity, PFILEDATA file)
{
	memzero(file);
	file->Allocator = allocator;
	if (mode != FILEMODE_NONE)
	{
		if (fstrinvalid(path))
			return UNDEFINED_ERROR;

		RESULT result = FileOpen(path, mode, file);
		if (result != OK)
			return result;
	}

	if (initial_capacity)
	{
		if (not (file->Buffer.str = Alloc(&allocator, initial_capacity, sizeof(U8))))
			return OUT_OF_MEMORY;
		file->Buffer.size = 0;
		file->BufferCapacity = initial_capacity;
	}
	return OK;
}
RESULT FileOpen(FSTR path, FILEMODE mode, PFILEDATA file)
{
	CAssert(not fstrinvalid(path) and "Path string view needs to be valid.");
	if (file->File)
	{
		FileClose(file);
	}
	memcpy(file->Path, path.str, path.size);
	file->PathView = fstr(file->Path, path.size);
	file->Path[path.size] = '\0';
	file->Mode = mode;

	CSTR file_mode = null;
	switch (mode)
	{
	case FILEMODE_READ:      file_mode = "rb"; break;
	case FILEMODE_READTEXT:  file_mode = "r";  break;
	case FILEMODE_WRITE:     file_mode = "w";  break;
	case FILEMODE_WRITETEXT: file_mode = "wb"; break;
	default: break;
	}
	CAssert(file_mode != null and "File mode needs to be known.");

	FILE *stream;
	errno_t err = fopen_s(&stream, file->Path, file_mode);
	RESULT rc = ErrnoAsResult(err);
	if (rc == OK)
	{
		file->File = (PFILE)stream;
		if (mode == FILEMODE_READ or mode == FILEMODE_READTEXT)
		{
			fseek(stream, 0, SEEK_END);
			long size = ftell(stream);
			rewind(stream);
			if (size < 0)
			{
				FileClose(file);
				return UNDEFINED_ERROR;
			}
			file->FileSize = (U64)size;
		}
	}
	return rc;
}
RESULT FileReopen(PFILEDATA file, FILEMODE mode)
{
	if (mode == file->Mode) // The same
		return OK;
	CAssert(file->File != null and "File must be open to reopen.");

	CSTR file_mode = null;
	switch (mode)
	{
	case FILEMODE_READ:      file_mode = "rb"; break;
	case FILEMODE_READTEXT:  file_mode = "r";  break;
	case FILEMODE_WRITE:     file_mode = "w";  break;
	case FILEMODE_WRITETEXT: file_mode = "wb"; break;
	default: break;
	}
	
	FILE *stream;
	errno_t err = freopen_s(&stream, file->Path, file_mode, (FILE *)file->File);
	RESULT rc = ErrnoAsResult(err);
	if (rc == OK)
	{
		file->File = (PFILE)stream;
		if (mode == FILEMODE_READ or mode == FILEMODE_READTEXT)
		{
			fseek(stream, 0, SEEK_END);
			long size = ftell(stream);
			rewind(stream);
			if (size < 0)
			{
				_ = FileClose(file);
				return UNDEFINED_ERROR;
			}
			file->FileSize = (U64)size;
		}
	}
	file->Mode = mode;
	return OK;
}
RESULT FileWriteAll(PFILEDATA file, BOOL as_text, FSTR content)
{
	CAssert(file->File != null and "File must be open to write to.");
	RESULT rc;
	if (as_text)
	{
		if (file->Mode != FILEMODE_WRITETEXT and (rc = FileReopen(file, FILEMODE_WRITETEXT)))
			return rc;
	}
	else
	{
		if (file->Mode != FILEMODE_WRITE and (rc = FileReopen(file, FILEMODE_WRITE)))
			return rc;
	}
	rewind((FILE *)file->File);
	file->Offset = 0;
	U64 written = fwrite(content.str, sizeof(CHAR), content.size, (FILE *)file->File);
	if (written != content.size)
	{
		errno_t err = ferror((FILE *)file->File);
		return ErrnoAsResult(err);
	}
	// Sync internal state
	file->Offset   = content.size;
	file->FileSize = content.size; // Update known file size
	return OK;
}
RESULT FileRead(PFILEDATA file, U64 count, PFSTR content)
{
	CAssert(file->File != null and "File must be open to read from.");
	RESULT rc;
	if ((rc = FileEnsureCapacity(file, count)))
		return rc;

	U64 read_count = fread(file->Buffer.str, sizeof(CHAR), count, (FILE *)file->File);
	if (read_count < count) // Less bytes read than wanted;
	{
		if (!feof((FILE *)file->File)) 
		{
			fseek((FILE *)file->File, file->Offset, SEEK_SET);
			return UNDEFINED_ERROR;
		}
		return END_OF_FILE;
	}
	file->Buffer.size = read_count;
	file->Offset += read_count;
	*content = file->Buffer;
	return OK;
}
RESULT FileReadAll(PFILEDATA file, BOOL as_text, PFSTR content)
{
	CAssert(file->File != null and "File must be open to read from.");
	RESULT rc;
	if (as_text)
	{
		if (file->Mode != FILEMODE_READTEXT and (rc = FileReopen(file, FILEMODE_READTEXT)))
			return rc;
	}
	else
	{
		if (file->Mode != FILEMODE_READ and (rc = FileReopen(file, FILEMODE_READ)))
			return rc;
	}
	if ((rc = FileEnsureCapacity(file, file->FileSize)))
		return rc;

	rewind((FILE *)file->File);

	U64 read_count = fread(file->Buffer.str, sizeof(CHAR), file->FileSize, (FILE *)file->File);
	if (read_count < file->FileSize and !feof((FILE *)file->File)) // Account for mode = "r" 
	{
		if (ferror((FILE *)file->File))
			return ErrnoAsResult(ferror((FILE *)file->File));
	}

	file->Buffer.size = read_count;
	file->Offset      = read_count;
	*content          = file->Buffer;
	return OK;
}
RESULT FileEnsureCapacity(PFILEDATA file, U64 capacity)
{
	if (capacity > file->BufferCapacity)
	{
		PCHAR str = (PCHAR)Realloc(&file->Allocator, file->Buffer.str, capacity, 1);
		if (not str)
			return OUT_OF_MEMORY;
		
		file->Buffer.str = str;
		file->BufferCapacity = capacity;
	}
	return OK;
}
static RESULT FileReReadBuffered(PFILEDATA file, PCHAR buffer, U64 buffer_size, CHAR terminator_replacement, U64 found_end, U64 relative_offset)
{
	relative_offset += (U64)found_end;
	RESULT rc = FileEnsureCapacity(file, relative_offset);
	if (rc != OK) // Unable to ensure capacity. E.g. out of memory. So rewind back.
	{
		if (fseek((FILE *)file->File, (long)file->Offset, SEEK_SET))
		{
			errno_t err = ferror((FILE *)file->File);
			rc = ErrnoAsResult(err);
		}
		return rc;
	}
	file->Buffer.size = relative_offset;
	// Copy
	if (relative_offset <= buffer_size) // Don't need to reread 
	{
		buffer[found_end - 1] = terminator_replacement; // Apply replacement
		memcpy(file->Buffer.str, buffer, relative_offset);

		file->Offset += relative_offset;
		if (fseek((FILE *)file->File, (long)file->Offset, SEEK_SET))
		{
			errno_t err = ferror((FILE *)file->File);
			rc = ErrnoAsResult(err);
		}
	}
	else
	{
		if (fseek((FILE *)file->File, (long)file->Offset, SEEK_SET))
		{
			errno_t err = ferror((FILE *)file->File);
			rc = ErrnoAsResult(err);
		}
		else
		{
			U64 read2 = fread(file->Buffer.str, sizeof(CHAR), relative_offset, (FILE *)file->File);
			if (read2 == relative_offset) // OK
			{
				if (relative_offset > 0) // Apply replacement
				{
					file->Buffer.str[relative_offset - 1] = terminator_replacement;
				}
				file->Offset += relative_offset;
			}
			else // Unable to read. So rewind back.
			{
				errno_t err = ferror((FILE *)file->File);
				rc = ErrnoAsResult(err);
				if (fseek((FILE *)file->File, (long)file->Offset, SEEK_SET))
				{
					errno_t err = ferror((FILE *)file->File);
					rc = ErrnoAsResult(err);
				}
			}
		}
	}
	return rc;
}
RESULT FileWrite(PFILEDATA file, FSTR content, BOOL as_text)
{
	CAssert(file->File != null and "File must be open to write to.");
	RESULT rc;
	if (as_text)
	{
		if (file->Mode != FILEMODE_READTEXT and (rc = FileReopen(file, FILEMODE_WRITETEXT)))
			return rc;
	}
	else
	{
		if (file->Mode != FILEMODE_READ and (rc = FileReopen(file, FILEMODE_WRITE)))
			return rc;
	}
	U64 written = fwrite(content.str, sizeof(*content.str), content.size, (FILE *)file->File);
	if (written != content.size)
	{
		return UNDEFINED_ERROR;
	}
	return OK;
}
RESULT FileReadLine(PFILEDATA file, CHAR terminator, CHAR terminator_replacement, BOOL as_text, PFSTR content)
{
	CAssert(file->File != null and "File must be open to read from.");
	RESULT rc;
	if (as_text)
	{
		if (file->Mode != FILEMODE_READTEXT and (rc = FileReopen(file, FILEMODE_READTEXT)))
			return rc;
	}
	else
	{
		if (file->Mode != FILEMODE_READ and (rc = FileReopen(file, FILEMODE_READ)))
			return rc;
	}

	CHAR buffer[256];
	U64 relative_offset = 0;
	while (true)
	{
		U64 read = fread(buffer, sizeof(CHAR), sizeof(buffer), (FILE *)file->File);
		if (read != 0) // Fits in buffer
		{
			PCHAR eol = (PCHAR)memchr(buffer, terminator, sizeof(buffer)); // Find line terminator
			if (eol) // End of line reached, copy.
			{
				rc = FileReReadBuffered(file, buffer, sizeof(buffer), terminator_replacement, ((U64)(eol - buffer)) + 1, relative_offset);
				*content = file->Buffer;
				return rc;
			}
			else // Line not ended, continue reading.
			{
				relative_offset += read;
			}
		}
		else // Larger than buffer or EOF reached.
		{ 
			return END_OF_FILE;
		}
	}
	return OK;
}
VOID FileRewind(PFILEDATA file)
{
	rewind((FILE *)file->File), file->Offset = 0;
}
RESULT FileClose(PFILEDATA file)
{
	file->PathView = FSTR_INVALID;
	if (fclose((FILE *)file->File))
	{
		errno_t err = ferror((FILE *)file->File);
		return ErrnoAsResult(err);
	}
	file->File = null;
	file->FileSize = 0;
	file->Offset = 0;
	file->Mode = FILEMODE_NONE;
	return OK;
}
VOID EndFile(PFILEDATA file)
{
	if (file->File)
		FileClose(file);
	if (file->Buffer.str)
		Free(&file->Allocator, file->Buffer.str);
	memzero(file);
}

// / / / / / / / / / / / / / / / / / / / 
// INI FILE                            /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID IniGroup(PFILEDATA filedata, FSTR group)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	fprintf((FILE *)filedata->File, "[%.*s]\n", (int)group.size, group.str);
}
VOID IniInteger(PFILEDATA filedata, FSTR key, I32 value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	fprintf((FILE *)filedata->File, "%.*s = %d\n", (int)key.size, key.str, (int)value);
}
VOID IniFloat(PFILEDATA filedata, FSTR key, F32 value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	fprintf((FILE *)filedata->File, "%.*s = %g\n", (int)key.size, key.str, value); // auto-precision
}
VOID IniBool(PFILEDATA filedata, FSTR key, bool value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	fprintf((FILE *)filedata->File, "%.*s = %.*s\n", (int)key.size, key.str, value ? 4 : 5, value ? "true" : "false");
}
VOID IniString(PFILEDATA filedata, FSTR key, FSTR value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	fprintf((FILE *)filedata->File, "%.*s = %.*s\n", (int)key.size, key.str, (int)value.size, value.str);
}
//// READING ///////////////////////////
static PCHAR Trim(PCHAR str)
{
	while (isspace((U8)*str)) str++;
	if (*str == '\0') return str;
	PCHAR end = str + strlen(str) - 1;
	while (end > str and isspace((U8)*end)) --end;
	end[1] = '\0';
	return str;
}
RESULT IniReadNextEntry(PFILEDATA filedata, PINIENTRY entry)
{
	CAssert(filedata->Mode == FILEMODE_READTEXT and "Not in read text mode.");
	FSTR line;
	RESULT rc;
	while ((rc = FileReadLine(filedata, '\n', '\0', true, &line)) == OK)
	{
		PCHAR current = Trim(line.str);
		if (*current == '\0' or *current == ';' or *current == '#') continue; // Empty/Comment

		if (*current == '[') // [GroupName]
		{
			PCHAR end = strchr(current, ']');
			if (end) 
			{
				*end = '\0'; // Remove trailing bracket
				strcpy_s(entry->Group.Name, sizeof(entry->Group.Name), current + 1);
				entry->Group.Length = (int)strlen(entry->Group.Name);
				continue; // Continue searching for a key within this or next section
			}
		}

		PCHAR equal = strchr(current, '=');
		if (equal) 
		{
			*equal = '\0'; // Split at '='

			PCHAR key = Trim(current), value = Trim(equal + 1);
			strcpy_s(entry->Key.Name, sizeof(entry->Key.Name), key);
			entry->Key.Length = (int)strlen(entry->Key.Name);

			if (_stricmp(value, "true") == 0) 
			{
				entry->Value.Boolean = true;
				entry->ValueType = VALUETYPE_BOOL;
			}
			else if (_stricmp(value, "false") == 0) 
			{
				entry->Value.Boolean = false;
				entry->ValueType = VALUETYPE_BOOL;
			}
			else if (strpbrk(value, ".")) 
			{
				entry->Value.Float = (F32)atof(value);
				entry->ValueType   = VALUETYPE_FLOAT;
			}
			else if (isdigit(value[0]) or (value[0] == '-' and isdigit(value[1]))) 
			{
				entry->Value.Int = atoi(value);
				entry->ValueType = VALUETYPE_INT;
			}
			else 
			{
				strcpy_s(entry->Value.String.Name, sizeof(entry->Value.String.Name), value);
				entry->ValueType = VALUETYPE_STRING;
			}
			return OK;
		}
	}
	return rc;
}

// / / / / / / / / / / / / / / / / / / / 
// JSON FILE                           /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID JsonBegin(PFILEDATA filedata)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	filedata->Json.Depth++;
	fprintf((FILE *)filedata->File, "{\n");
}
VOID JsonEnd(PFILEDATA filedata)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	filedata->Json.Depth--;
	fprintf((FILE *)filedata->File, "}\n");
}
VOID JsonObjectBegin(PFILEDATA filedata, FSTR key)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": {\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str);
	else
		fprintf((FILE *)filedata->File, "%*s{\n", 3 * filedata->Json.Depth, "");
	filedata->Json.Depth++;
}
VOID JsonObjectEnd(PFILEDATA filedata)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	filedata->Json.Depth--;
	fprintf((FILE *)filedata->File, "%*s},\n", 3 * filedata->Json.Depth, "");
}
VOID JsonArrayBegin(PFILEDATA filedata, FSTR key)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": [\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str);
	else
		fprintf((FILE *)filedata->File, "%*s[\n", 3 * filedata->Json.Depth, "");
	filedata->Json.Depth++;
}
VOID JsonArrayEnd(PFILEDATA filedata)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	filedata->Json.Depth--;
	fprintf((FILE *)filedata->File, "%*s],\n", 3 * filedata->Json.Depth, "");
}
VOID JsonInteger(PFILEDATA filedata, FSTR key, I32 value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": %d,\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str, (int)value);
	else
		fprintf((FILE *)filedata->File, "%*s%d,\n", 3 * filedata->Json.Depth, "", (int)value);
}
VOID JsonFloat(PFILEDATA filedata, FSTR key, F32 value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": %g,\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str, value);
	else
		fprintf((FILE *)filedata->File, "%*s%g,\n", 3 * filedata->Json.Depth, "", value);
}
VOID JsonBool(PFILEDATA filedata, FSTR key, BOOL value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": %.*s,\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str, value ? 4 : 5, value ? "true" : "false");
	else
		fprintf((FILE *)filedata->File, "%*s%.*s,\n", 3 * filedata->Json.Depth, "", value ? 4 : 5, value ? "true" : "false");
}
VOID JsonString(PFILEDATA filedata, FSTR key, FSTR value)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": \"%.*s\",\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str, (int)value.size, value.str);
	else
		fprintf((FILE *)filedata->File, "%*s\"%.*s\",\n", 3 * filedata->Json.Depth, "", (int)value.size, value.str);
}
VOID JsonNull(PFILEDATA filedata, FSTR key)
{
	CAssert(filedata->Mode == FILEMODE_WRITETEXT and "Not in write text mode.");
	if (key.size > 0)
		fprintf((FILE *)filedata->File, "%*s\"%.*s\": null,\n", 3 * filedata->Json.Depth, "", (int)key.size, key.str);
	else
		fprintf((FILE *)filedata->File, "%*snull,\n", 3 * filedata->Json.Depth, "");
}

//// READING ///////////////////////////
static CHAR JsonSkipWhitespace(FSTR *line, U64 *offset)
{
	while (*offset < line->size)
	{
		CHAR c = line->str[*offset];
		if (c > ' ' and c != ',')
			return c;
		if (c != ' ' and c != '\t' and c != ',' and c != ':')
			return c;
		(*offset)++;
	}
	return '\0';
}
static U64 JsonParseString(FSTR *line, U64 *offset, PCHAR out_buf, U64 max_size)
{
	U64 len = 0;
	(*offset)++; // Skip opening double quote '"'

	while (*offset < line->size and len < max_size - 1)
	{
		CHAR c = line->str[(*offset)++];
		if (c == '"') // Closing quote
			break;

		if (c == '\\') // Fast escape bypass
		{
			if (*offset < line->size)
				c = line->str[(*offset)++];
		}
		out_buf[len++] = c;
	}
	out_buf[len] = '\0';
	return len;
}
VOID JsonReadReset(PFILEDATA filedata)
{
	filedata->Json.Depth = 0;
	filedata->Json.HasExplicitKey = false;
	filedata->Json.LineOffset = 0;
	filedata->Json.CurrentLine = LIT(FSTR, 0);
}
RESULT JsonReadNextEntry(PFILEDATA filedata, PJSONENTRY entry)
{
	CAssert((filedata->Mode == FILEMODE_READTEXT || filedata->Mode == FILEMODE_READ) and "Not in valid read mode.");
	CAssert(filedata and entry and "Parameters need to be valid.");

	FSTR *line  = &filedata->Json.CurrentLine;
	U64 *offset = &filedata->Json.LineOffset;
	while (true)
	{
		// If we've consumed the current line, read next one
		if (*offset >= line->size)
		{
			RESULT res = FileReadLine(filedata, '\n', '\0', true, line);
			if (res != OK) // E.g. error or  eof
			{
				JsonReadReset(filedata); // Reset state for potential future reads
				return res;
			}
			*offset = 0;
		}

		CHAR token = JsonSkipWhitespace(line, offset);
		if (token == '\0')
		{
			*offset = line->size; // Force read next line
			continue;
		}

		if (token == '{' or token == '[') // Array or Object start
		{
			entry->ValueType = (token == '{') ? VALUETYPE_OBJECT : VALUETYPE_ARRAY;
			entry->Depth = filedata->Json.Depth;
			filedata->Json.Depth++;

			if (!filedata->Json.HasExplicitKey)
			{
				entry->Key.Length = 0;
				entry->Key.Name[0] = '\0';
				entry->Key.Hash = 0;
			}

			(*offset)++; // Consume structural token
			filedata->Json.HasExplicitKey = false;
			return OK;
		}

		if (token == '}' or token == ']')
		{
			if (filedata->Json.Depth > 0) filedata->Json.Depth--;
			(*offset)++;
			continue;
		}

		// Check if this is a key-value pair (look ahead for ':')
		bool isKeyValuePair = false;
		if (token == '"')
		{
			U64 search_ptr = *offset + 1;
			while (search_ptr < line->size and line->str[search_ptr] != '"')
			{
				if (line->str[search_ptr] == '\\') search_ptr++;
				search_ptr++;
			}
			search_ptr++;

			while (search_ptr < line->size and (line->str[search_ptr] == ' ' or line->str[search_ptr] == '\t'))
			{
				search_ptr++;
			}
			if (search_ptr < line->size and line->str[search_ptr] == ':')
				isKeyValuePair = true;
		}

		// Parse key if this is a key-value pair
		if (isKeyValuePair)
		{
			// Read Key
			entry->Key.Length = JsonParseString(line, offset, entry->Key.Name, sizeof(entry->Key.Name));
			entry->Key.Hash = HashFNV1a(fdata(entry->Key.Name, entry->Key.Length));

			// ':' after key
			token = JsonSkipWhitespace(line, offset);
			if (token != ':')  
				return INVALID_FORMAT;
			(*offset)++; // step over ':'

			// If next is '\0', read next line
			token = JsonSkipWhitespace(line, offset);
			if (token == '\0')
			{
				*offset = line->size; // Force read next line
				filedata->Json.HasExplicitKey = true; // Remember we parsed a key
				continue; // Go back to top, will read new line and re-process
			}

			// Check for array or object 
			if (token == '{')
			{
				entry->ValueType = VALUETYPE_OBJECT;
				entry->Depth = filedata->Json.Depth;
				filedata->Json.Depth++;
				(*offset)++;
				return OK;
			}
			else if (token == '[')
			{
				entry->ValueType = VALUETYPE_ARRAY;
				entry->Depth = filedata->Json.Depth;
				filedata->Json.Depth++;
				(*offset)++;
				return OK;
			}
		}
		else if (filedata->Json.HasExplicitKey) // Key already parsed on previous line. Now get its value.
		{
			filedata->Json.HasExplicitKey = false;
			// [[fallthrough]]
		}
		else
		{
			entry->Key.Length = 0;
			entry->Key.Name[0] = '\0';
			entry->Key.Hash = 0;
		}

		entry->Depth = filedata->Json.Depth;
		if (token == '"')
		{
			entry->ValueType = VALUETYPE_STRING;
			entry->Value.String.Length = JsonParseString(line, offset, entry->Value.String.Name, sizeof(entry->Value.String.Name));
			entry->Value.String.Hash = HashFNV1a(fdata(entry->Value.String.Name, entry->Value.String.Length));
			return OK;
		}
		else if (token == '{' or token == '[')
		{
			continue;
		}

		// Parse literals/numbers
		CHAR value_segment[256] = { 0 };
		U64 v_len = 0;
		while (*offset < line->size and v_len < sizeof(value_segment) - 1)
		{
			CHAR c = line->str[*offset];
			if (c == ' ' or c == '\t' or c == ',' or c == '}' or c == ']' or c == ':')
				break;
			value_segment[v_len++] = c;
			(*offset)++;
		}
		value_segment[v_len] = '\0';

		if (_stricmp(value_segment, "true") == 0)
		{
			entry->Value.Boolean = true;
			entry->ValueType = VALUETYPE_BOOL;
		}
		else if (_stricmp(value_segment, "false") == 0)
		{
			entry->Value.Boolean = false;
			entry->ValueType = VALUETYPE_BOOL;
		}
		else if (_stricmp(value_segment, "null") == 0)
		{
			entry->ValueType = VALUETYPE_NULL;
		}
		else if (strchr(value_segment, '.'))
		{
			entry->Value.Float = (F32)atof(value_segment);
			entry->ValueType = VALUETYPE_FLOAT;
		}
		else
		{
			entry->Value.Int = atoi(value_segment);
			entry->ValueType = VALUETYPE_INT;
		}
		return OK;
	}
}

// / / / / / / / / / / / / / / / / / / / 
// CSV FILE                            /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID CsvBegin(PFILEDATA filedata)
{
	filedata->Csv.HasWrittenFirstValue = false;
}
VOID CsvEnd(PFILEDATA filedata)
{
	filedata->Csv.HasWrittenFirstValue = false;
	filedata->Csv.CurrentLine = LIT(FSTR, 0);
	filedata->Csv.LineOffset = 0;
}
VOID CsvRow(PFILEDATA filedata)
{
	if (filedata->Csv.HasWrittenFirstValue)
		fprintf((FILE *)filedata->File, "\n");
	filedata->Csv.HasWrittenFirstValue = false;
}
VOID CsvInteger(PFILEDATA filedata, I32 value)
{
	fprintf((FILE *)filedata->File, filedata->Csv.HasWrittenFirstValue ? ",%d" : "%d", (int)value);
	filedata->Csv.HasWrittenFirstValue = true;
}
VOID CsvFloat(PFILEDATA filedata, F32 value)
{
	fprintf((FILE *)filedata->File, filedata->Csv.HasWrittenFirstValue ? ",%g" : "%g", value); // auto-precision
	filedata->Csv.HasWrittenFirstValue = true;
}
VOID CsvBool(PFILEDATA filedata, BOOL value)
{
	fprintf((FILE *)filedata->File, filedata->Csv.HasWrittenFirstValue ? ",%.*s" : "%.*s", value ? 4 : 5, value ? "true" : "false");
	filedata->Csv.HasWrittenFirstValue = true;
}
VOID CsvString(PFILEDATA filedata, FSTR value)
{
	fprintf((FILE *)filedata->File, filedata->Csv.HasWrittenFirstValue ? ",%.*s" : "%.*s", (int)value.size, value.str);
	filedata->Csv.HasWrittenFirstValue = true;
}
//// READING ///////////////////////////
RESULT CsvReadNextValue(PFILEDATA filedata, PCSVENTRY entry)
{
	CAssert((filedata->Mode == FILEMODE_READTEXT or filedata->Mode == FILEMODE_READ) and "Not in valid read mode.");
	if (!filedata or !entry) return INVALID_PARAMETER;

	FSTR *line = &filedata->Csv.CurrentLine;
	U64  *offset = &filedata->Csv.LineOffset;
	if (*offset >= line->size) // Need to read next line
	{
		RESULT res = FileReadLine(filedata, '\n', '\0', true, line);
		if (res == END_OF_FILE) return END_OF_FILE;
		if (res != OK) return res;
		*offset = 0;
	}

	if (*offset >= line->size) // End of line
	{
		entry->ValueType = VALUETYPE_ENDOFLINE;
		return OK;
	}

	CHAR c = line->str[*offset];
	U64 start = *offset;

	if (c == '"') // Quoted value
	{
		(*offset)++; // Skip opening quote
		U64 len = 0;

		while (*offset < line->size and len < sizeof(entry->Value.String.Name) - 1)
		{
			c = line->str[*offset];

			if (c == '"') // Check for escaped quote ("")
			{
				if (*offset + 1 < line->size and line->str[*offset + 1] == '"')
				{
					entry->Value.String.Name[len++] = '"';
					*offset += 2;
				}
				else
				{
					// End of quoted string
					(*offset)++;
					break;
				}
			}
			else // Value character
			{
				entry->Value.String.Name[len++] = c;
				(*offset)++;
			}
		}

		entry->Value.String.Name[len] = '\0';
		entry->Value.String.Length    = len;
		entry->Value.String.Hash      = HashFNV1a(fdata(entry->Value.String.Name, len));
		entry->ValueType              = VALUETYPE_STRING;

		while (*offset < line->size and line->str[*offset] != ',') // Skip to next comma or EOL
			(*offset)++;

		if (*offset < line->size and line->str[*offset] == ',') // Skip comma
			(*offset)++;
		return OK;
	} // else: Handle unquoted values till comma or EOL

	U64 len = 0;
	CHAR value_buf[256];
	while (*offset < line->size and line->str[*offset] != ',') 
	{
		if (len < sizeof(value_buf) - 1)
			value_buf[len++] = line->str[*offset];
		(*offset)++;
	}

	if (*offset < line->size and line->str[*offset] == ',')
		(*offset)++; // Skip comma

	value_buf[len] = '\0'; // Null-terminate for easier processing
	while (len > 0 and (value_buf[len - 1] == ' ' or value_buf[len - 1] == '\t')) // Trim trailing whitespaces
		len--;
	value_buf[len] = '\0';

	if (len == 0) // Empty value
	{
		entry->ValueType = VALUETYPE_NULL;
		return OK;
	} // else: Try to parse as number
	
	U64 i = 0;
	while (i < len and (value_buf[i] == ' ' or value_buf[i] == '\t')) // Skip leading whitespaces
		i++;

	if (i < len and value_buf[i] == '-') // Check for negative sign
		i++;

	// Check digits
	bool is_number = i >= len ? false : true;
	bool has_decimal = false;
	for (; i < len and is_number; i++)
	{
		if (value_buf[i] >= '0' and value_buf[i] <= '9')
		{
			continue;
		}
		else if (value_buf[i] == '.' and !has_decimal)
		{
			has_decimal = true;
			continue;
		}
		else if (value_buf[i] == ' ' or value_buf[i] == '\t') // Trailing whitespace
		{
			break;
		}
		else
		{
			is_number = false;
			break;
		}
	}

	if (is_number and has_decimal)
	{
		entry->Value.Float = (F32)atof(value_buf);
		entry->ValueType   = VALUETYPE_FLOAT;
		return OK;
	}
	else if (is_number)
	{
		entry->Value.Int = atoi(value_buf);
		entry->ValueType = VALUETYPE_INT;
		return OK;
	}
	// else: Not a number, check for boolean

	if (_stricmp(value_buf, "true") == 0)
	{
		entry->Value.Boolean = true;
		entry->ValueType = VALUETYPE_BOOL;
		return OK;
	}
	else if (_stricmp(value_buf, "false") == 0)
	{
		entry->Value.Boolean = false;
		entry->ValueType = VALUETYPE_BOOL;
		return OK;
	}

	// Default to string
	if (len < sizeof(entry->Value.String.Name))
	{
		memcpy(entry->Value.String.Name, value_buf, len + 1);
		entry->Value.String.Length = len;
		entry->Value.String.Hash   = HashFNV1a(fdata(value_buf, len));
		entry->ValueType           = VALUETYPE_STRING;
	}
	else
	{
		return OUT_OF_MEMORY;
	}
	return OK;
}
// / / / / / / / / / / / / / / / / / / / 
// MESSAGEPACK                         /
// / / / / / / / / / / / / / / / / / / /
//// WRITING ///////////////////////////
VOID MsgPackNull(PFILEDATA filedata)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");
	const U8 v = 0xc0; // nil
	fwrite(&v, 1, 1, (FILE *)filedata->File);
}
VOID MsgPackBool(PFILEDATA filedata, BOOL value)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");
	const U8 v = value ? 0xc3 : 0xc2; // true : false
	fwrite(&v, 1, 1, (FILE *)filedata->File);
}
VOID MsgPackInteger(PFILEDATA filedata, I32 value)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");
	if (value >= 0)
	{
		if (value <= 127) // positive fixint
		{
			const U8 v = (U8)value;
			fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		}
		else if (value <= 255) // uint8
		{
			const U8 vs[2] = { 0xcc, (U8)value };
			fwrite(&vs[0], sizeof(vs[0]), arrsize(vs), (FILE *)filedata->File);
		}
		else if (value <= 65535) // uint16
		{
			const U8 v = 0xcd; 
			const U16 v2 = (U16)value;
			fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
			fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
		}
		else // uint32
		{
			const U8 v = 0xce;
			const U32 v2 = (U32)value;
			fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
			fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
		}
	}
	else
	{
		if (value >= -32) // negative fixint
		{
			const U8 v = 0xe0 | ((U8)value + 32); // Encode as negative fixint
			fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		}
		else if (value >= -128) // int8
		{
			const U8 vs[] = {0xd0, (U8)value};
			fwrite(&vs[0], sizeof(vs[0]), arrsize(vs), (FILE *)filedata->File);
		}
		else if (value >= -32768) // int16
		{
			const U8 v = 0xd1; 
			const U16 v2 = (U16)value;
			fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
			fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
		}
		else // int32
		{
			const U8 v = 0xd1;
			const U32 v2 = (U32)value;
			fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
			fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
		}
	}
}
VOID MsgPackFloat(PFILEDATA filedata, F32 value)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");
	const U8 v = 0xca; // float32
	fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);

	U32 bits;
	memcpy(&bits, &value, sizeof(F32));
	fwrite(&bits, sizeof(bits), 1, (FILE *)filedata->File);
}
VOID MsgPackString(PFILEDATA filedata, FSTR value)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");
	if (value.size <= 31) // fixstr
	{
		const U8 v = 0xa0 | (U8)value.size; 
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
	}
	else if (value.size <= 255) // str8
	{
		const U8 vs[] = { 0xd9, (U8)value.size }; 
		fwrite(&vs[0], sizeof(vs[0]), arrsize(vs), (FILE *)filedata->File);
	}
	else if (value.size <= 65535) // str16
	{
		const U8 v = 0xda;
		const U16 v2 = (U16)value.size;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
	}
	else // str32
	{
		const U8 v = 0xdb;
		const U32 v2 = (U32)value.size;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
	}
	fwrite(value.str, 1, value.size, (FILE *)filedata->File);
}
VOID MsgPackArrayBegin(PFILEDATA filedata, U32 count)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");

	if (count <= 15) // fixarray
	{
		const U8 v = 0x90 | (U8)count; 
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
	}
	else if (count <= 65535) // array16
	{
		const U8 v = 0xdc;
		const U16 v2 = (U16)count;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
	}
	else // array32
	{
		const U8 v = 0xdd;
		const U32 v2 = (U32)count;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
	}
}
VOID MsgPackMapBegin(PFILEDATA filedata, U32 count)
{
	CAssert(filedata->Mode == FILEMODE_WRITE and "Not in write mode.");
	if (count <= 15) // fixmap
	{
		const U8 v = 0x80 | (U8)count;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
	}
	else if (count <= 65535) // map16
	{
		const U8 v = 0xde;
		const U16 v2 = (U16)count;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
	}
	else // map32
	{
		const U8 v = 0xdf;
		const U32 v2 = (U32)count;
		fwrite(&v, sizeof(v), 1, (FILE *)filedata->File);
		fwrite(&v2, sizeof(v2), 1, (FILE *)filedata->File);
	}
}
//// READING ///////////////////////////
static U8 MsgPackReadU8(PFILEDATA filedata)
{
	U8 value;
	if (filedata->Offset < filedata->FileSize)
	{
		value = filedata->Buffer.str[filedata->Offset++];
		return value;
	}
	return 0;
}
static U16 MsgPackReadU16(PFILEDATA filedata)
{
	U16 value = 0;
	if (filedata->Offset + 1 < filedata->FileSize)
	{
		value = ((U16)filedata->Buffer.str[filedata->Offset] << 8) |
			((U16)filedata->Buffer.str[filedata->Offset + 1]);
		filedata->Offset += 2;
	}
	return value;
}
static U32 MsgPackReadU32(PFILEDATA filedata)
{
	U32 value = 0;
	if (filedata->Offset + 3 < filedata->FileSize)
	{
		value = ((U32)filedata->Buffer.str[filedata->Offset] << 24) |
			((U32)filedata->Buffer.str[filedata->Offset + 1] << 16) |
			((U32)filedata->Buffer.str[filedata->Offset + 2] << 8) |
			((U32)filedata->Buffer.str[filedata->Offset + 3]);
		filedata->Offset += 4;
	}
	return value;
}
VOID MsgPackReadReset(PFILEDATA filedata)
{
	filedata->Offset = 0;
}
RESULT MsgPackReadNextEntry(PFILEDATA filedata, PMSGPACKENTRY entry)
{
	CAssert(filedata->Mode == FILEMODE_READ and "Not in read mode.");
	if (!filedata or !entry) return INVALID_PARAMETER;
	if (filedata->Offset >= filedata->FileSize) return END_OF_FILE;

	// Read and interpret the type byte
	U8 type = MsgPackReadU8(filedata);
	if (type <= 0x7f) // Positive fixint (0x00 - 0x7f)
	{
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = type;
		return OK;
	}
	else if (type >= 0x80 and type <= 0x8f) // Fixmap (0x80 - 0x8f)
	{
		entry->ValueType = VALUETYPE_OBJECT;
		entry->Value.Count = type & 0x0f;
		return OK;
	}
	else if (type >= 0x90 and type <= 0x9f) // Fixarray (0x90 - 0x9f)
	{
		entry->ValueType = VALUETYPE_ARRAY;
		entry->Value.Count = type & 0x0f;
		return OK;
	}
	else if (type >= 0xa0 and type <= 0xbf) // Fixstr (0xa0 - 0xbf)
	{
		U32 len = type & 0x1f;
		if (len < sizeof(entry->Value.String.Name))
		{
			if (filedata->Offset + len <= filedata->FileSize)
			{
				memcpy(entry->Value.String.Name, &filedata->Buffer.str[filedata->Offset], len);
				entry->Value.String.Name[len] = '\0';
				entry->Value.String.Length = len;
				entry->Value.String.Hash = HashFNV1a(fdata(entry->Value.String.Name, len));
				filedata->Offset += len;
				entry->ValueType = VALUETYPE_STRING;
				return OK;
			}
		}
		return INVALID_FORMAT;
	}
	else if (type >= 0xe0) // Negative fixint (0xe0 - 0xff)
	{
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = (I8)type; // Sign extend
		return OK;
	}

	switch (type) // Extended types
	{
	case 0xc0: // nil
		entry->ValueType = VALUETYPE_NULL;
		return OK;
	case 0xc2: // false
		entry->ValueType = VALUETYPE_BOOL;
		entry->Value.Boolean = false;
		return OK;
	case 0xc3: // true
		entry->ValueType = VALUETYPE_BOOL;
		entry->Value.Boolean = true;
		return OK;
	case 0xcc: // uint8
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = MsgPackReadU8(filedata);
		return OK;
	case 0xcd: // uint16
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = MsgPackReadU16(filedata);
		return OK;
	case 0xce: // uint32
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = (I32)MsgPackReadU32(filedata);
		return OK;
	case 0xd0: // int8
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = (I8)MsgPackReadU8(filedata);
		return OK;
	case 0xd1: // int16
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = (I16)MsgPackReadU16(filedata);
		return OK;
	case 0xd2: // int32
		entry->ValueType = VALUETYPE_INT;
		entry->Value.Int = (I32)MsgPackReadU32(filedata);
		return OK;
	case 0xca: // float32
	{
		U32 bits = MsgPackReadU32(filedata);
		memcpy(&entry->Value.Float, &bits, sizeof(F32));
		entry->ValueType = VALUETYPE_FLOAT;
		return OK;
	}
	case 0xd9: // str8
	{
		U32 len = MsgPackReadU8(filedata);
		if (len < sizeof(entry->Value.String.Name) and filedata->Offset + len <= filedata->FileSize)
		{
			memcpy(entry->Value.String.Name, &filedata->Buffer.str[filedata->Offset], len);
			entry->Value.String.Name[len] = '\0';
			entry->Value.String.Length = len;
			entry->Value.String.Hash = HashFNV1a(fdata(entry->Value.String.Name, len));
			filedata->Offset += len;
			entry->ValueType = VALUETYPE_STRING;
			return OK;
		}
		return INVALID_FORMAT;
	}
	case 0xda: // str16
	{
		U32 len = MsgPackReadU16(filedata);
		if (len < sizeof(entry->Value.String.Name) and filedata->Offset + len <= filedata->FileSize)
		{
			memcpy(entry->Value.String.Name, &filedata->Buffer.str[filedata->Offset], len);
			entry->Value.String.Name[len] = '\0';
			entry->Value.String.Length = len;
			entry->Value.String.Hash = HashFNV1a(fdata(entry->Value.String.Name, len));
			filedata->Offset += len;
			entry->ValueType = VALUETYPE_STRING;
			return OK;
		}
		return INVALID_FORMAT;
	}
	case 0xdc: // array16
		entry->ValueType = VALUETYPE_ARRAY;
		entry->Value.Count = MsgPackReadU16(filedata);
		return OK;
	case 0xdd: // array32
		entry->ValueType = VALUETYPE_ARRAY;
		entry->Value.Count = MsgPackReadU32(filedata);
		return OK;
	case 0xde: // map16
		entry->ValueType = VALUETYPE_OBJECT;
		entry->Value.Count = MsgPackReadU16(filedata);
		return OK;
	case 0xdf: // map32
		entry->ValueType = VALUETYPE_OBJECT;
		entry->Value.Count = MsgPackReadU32(filedata);
		return OK;
	default: return INVALID_FORMAT;
	}
}