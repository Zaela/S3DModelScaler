
#include "wld.h"

namespace WLD
{
	void DecodeName(void* in_name, uint32 len)
	{
		static byte hashval[] = {0x95,0x3A,0xC5,0x2A,0x95,0x7A,0x95,0x6A};
		byte* name = (byte*)in_name;
		for (uint32 i = 0; i < len; ++i)
		{
			name[i] ^= hashval[i & 7];
		}
	}

	const char* GetName(const char* string_block, const int strings_len, int ref)
	{
		if (ref < 0 && -ref < strings_len)
			return &string_block[-ref];
		return nullptr;
	}

	int Read(lua_State* L)
	{
		//data table at 1
		lua_getfield(L, 1, "ptr");
		byte* ptr = (byte*)lua_touserdata(L, -1);
		lua_pop(L, 1);

		Header* header = (Header*)ptr;
		if (header->magicWord != Header::MAGIC)
			return luaL_argerror(L, 1, "file is not a valid WLD file");
		int version = header->version & 0xFFFFFFFE;
		if (version != Header::VERSION1 && version != Header::VERSION2)
			return luaL_argerror(L, 1, "file is not a valid WLD version");

		lua_createtable(L, 0, 2); //to return

		uint32 pos = Header::SIZE;
		const char* string_block = (const char*)&ptr[pos];
		const int strings_len = header->nameHashLen;

		byte* str = (byte*)lua_newuserdata(L, strings_len);
		memcpy(str, string_block, strings_len);
		DecodeName(str, strings_len);

		lua_setfield(L, -2, "string_block");
		lua_pushinteger(L, strings_len);
		lua_setfield(L, 2, "strings_len");

		string_block = (const char*)str;
		pos += strings_len;

		int tbl_pos = 0;
		for (int i = 0; i < header->maxFragment; ++i)
		{
			FragHeader* fh = (FragHeader*)&ptr[pos];

			if (fh->type == 0x14)
			{
				lua_pushinteger(L, ++tbl_pos);
				lua_pushstring(L, GetName(string_block, strings_len, fh->nameref));
				lua_settable(L, -3);
			}

			pos += FragHeader::SIZE + fh->len;
		}

		return 1;
	}

	int Scale(lua_State* L)
	{
		//wld data table, 3-letter model name, scale factor, output table with string block and len
		lua_getfield(L, 1, "ptr");
		byte* ptr = (byte*)lua_touserdata(L, -1);

		const char* name = luaL_checkstring(L, 2);
		float scale_factor = luaL_checknumber(L, 3);

		luaL_checktype(L, 4, LUA_TTABLE);
		lua_getfield(L, 4, "string_block");
		const char* string_block = (const char*)lua_touserdata(L, -1);
		lua_getfield(L, 4, "strings_len");
		const int strings_len = lua_tointeger(L, -1);
		lua_pop(L, 3);

		Header* header = (Header*)ptr;
		uint32 pos = Header::SIZE + strings_len;

		std::string str = "\\b[A-Z][0-9][0-9]";
		str += name;
		std::regex reg(str);

		str = name;
		str += "_TRACKDEF";
		std::regex rootreg(str);

		float z_diff = 0;

		for (int i = 0; i < header->maxFragment; ++i)
		{
			FragHeader* fh = (FragHeader*)&ptr[pos];
			pos += FragHeader::SIZE;

			if (fh->type == 0x12)
			{
				Frag12* f = (Frag12*)fh;
				const char* n = GetName(string_block, strings_len, fh->nameref);
				if (n == nullptr || ((n[0] != name[0] || n[1] != name[1] || n[2] != name[2]) && !std::regex_search(n, reg)))
					goto SKIP_FRAG;

				uint32 p = pos + Frag12::SIZE;

				if (std::regex_search(n, rootreg))
				{
					for (uint32 j = 0; j < f->count; ++j)
					{
						BoneValues* bv = (BoneValues*)&ptr[p];
						p += BoneValues::SIZE;

						if (bv->denom == 0)
							continue;
						bv->x *= scale_factor;
						bv->y *= scale_factor;
						bv->z *= scale_factor;
						float z = bv->z / (float)bv->denom;
						z += z_diff;
						bv->z = (int16)(z * bv->denom);
					}
				}
				else
				{
					for (uint32 j = 0; j < f->count; ++j)
					{
						BoneValues* bv = (BoneValues*)&ptr[p];
						p += BoneValues::SIZE;

						if (bv->denom == 0 || (bv->x == 0 && bv->y == 0 && bv->z == 0))
							continue;
						bv->x *= scale_factor;
						bv->y *= scale_factor;
						bv->z *= scale_factor;
					}
				}
			}
			else if (fh->type == 0x36)
			{
				Frag36* f = (Frag36*)fh;
				const char* n = GetName(string_block, strings_len, fh->nameref);
				if (n == nullptr || n[0] != name[0] || n[1] != name[1] || n[2] != name[2])
					goto SKIP_FRAG;

				uint32 start = pos + Frag36::SIZE;
				uint32 p = start;

				f->max_dist *= scale_factor;

				float scaler = 1.0f / (1 << f->scale);
				std::vector<float> values;
				float lowest_z = 99999;
				
				//retrieve real vertex positions and scale them by the desired amount
				for (uint32 j = 0; j < f->vert_count; ++j)
				{
					Vertex* v = (Vertex*)&ptr[p];
					p += Vertex::SIZE;

					float sv;
					sv = ((float)v->x * scaler) * scale_factor;
					values.push_back(sv);
					sv = ((float)v->y * scaler) * scale_factor;
					values.push_back(sv);
					float z = (float)v->z * scaler;
					sv = z * scale_factor;
					values.push_back(sv);
					if (z < lowest_z)
					{
						lowest_z = z;
						z_diff = z - sv;
					}
				}

				int16 new_scale = -1;
				//find a scaling value that will work for all the new values within a reasonable tolerance
				for (int16 value = 1; value < 32; ++value)
				{
					float new_scaler = 1.0f / (1 << value);
					for (float sv : values)
					{
						float test = floor(sv / new_scaler);
						test = abs(sv - (test * new_scaler));
						if (test > 0.0001f)
							goto SKIP_MESH;
					}
					//if we're still here, success
					new_scale = value;
					break;
				SKIP_MESH: ;
				}

				if (new_scale == -1)
					return luaL_error(L, "could not maintain mesh vertex tolerances for the given scale factor");

				//write the new storage values
				f->scale = new_scale;
				scaler = 1.0f / (1 << new_scale);

				int16* d = (int16*)&ptr[start];
				float* data = values.data();

				const uint32 vn = f->vert_count * 3;
				for (uint32 j = 0; j < vn; ++j)
				{
					*d++ = (int16)(*data++ / scaler);
				}
			}

		SKIP_FRAG:
			pos += fh->len;
		}

		return 0;
	}

	int AdjustZ(lua_State* L)
	{
		//wld data table, 3-letter model name, scale factor, output table with string block and len
		lua_getfield(L, 1, "ptr");
		byte* ptr = (byte*)lua_touserdata(L, -1);

		const char* name = luaL_checkstring(L, 2);
		float z_adjust = luaL_checknumber(L, 3);

		luaL_checktype(L, 4, LUA_TTABLE);
		lua_getfield(L, 4, "string_block");
		const char* string_block = (const char*)lua_touserdata(L, -1);
		lua_getfield(L, 4, "strings_len");
		const int strings_len = lua_tointeger(L, -1);
		lua_pop(L, 3);

		Header* header = (Header*)ptr;
		uint32 pos = Header::SIZE + strings_len;

		std::string str = name;
		str += "_TRACKDEF";
		std::regex reg(str);

		for (int i = 0; i < header->maxFragment; ++i)
		{
			FragHeader* fh = (FragHeader*)&ptr[pos];
			pos += FragHeader::SIZE;

			if (fh->type == 0x12)
			{
				Frag12* f = (Frag12*)fh;
				const char* n = GetName(string_block, strings_len, fh->nameref);
				if (n == nullptr || !std::regex_search(n, reg))
					goto SKIP_FRAG;

				uint32 p = pos + Frag12::SIZE;

				for (uint32 j = 0; j < f->count; ++j)
				{
					BoneValues* bv = (BoneValues*)&ptr[p];
					p += BoneValues::SIZE;

					if (bv->denom == 0)
						continue;
	
					float z = bv->z / (float)bv->denom;
					z += z_adjust;
					bv->z = (int16)(z * bv->denom);
				}
			}

		SKIP_FRAG:
			pos += fh->len;
		}

		return 0;
	}

	static const luaL_Reg funcs[] = {
		{"Read", Read},
		{"Scale", Scale},
		{"AdjustZ", AdjustZ},
		{nullptr, nullptr}
	};

	void LoadFunctions(lua_State* L)
	{
		luaL_register(L, "wld", funcs);
	}
}
