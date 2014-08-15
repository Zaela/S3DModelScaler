
#include <lua.hpp>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <regex>
#include "types.h"

namespace WLD
{
	void LoadFunctions(lua_State* L);

	struct Header
	{
		int magicWord;
		int version;
		int maxFragment;
		int unknownA[2];
		int nameHashLen;
		int unknownB;
		static const uint32 SIZE = sizeof(int) * 7;
		static const int MAGIC = 0x54503D02;
		static const int VERSION1 = 0x00015500;
		static const int VERSION2 = 0x1000C800;
	};

	struct FragHeader
	{
		uint32 len;
		uint32 type;
		static const uint32 SIZE = sizeof(uint32) * 2;
		int nameref;
	};

	struct Frag36
	{
		FragHeader header;
		uint32 flag;
		int texture_list_ref;
		int anim_vert_ref;
		uint32 unknownA[2];
		float x, y, z;
		float rotation[3];
		float max_dist;
		float min_x, min_y, min_z;
		float max_x, max_y, max_z;
		uint16 vert_count;
		uint16 uv_count;
		uint16 normal_count;
		uint16 color_count;
		uint16 poly_count;
		uint16 vert_piece_count;
		uint16 poly_texture_count;
		uint16 vert_texture_count;
		uint16 size9;
		uint16 scale;
		static const uint32 SIZE = sizeof(uint16) * 10 + sizeof(uint32) * 19;
	};

	struct Vertex
	{
		int16 x, y, z;
		static const uint32 SIZE = sizeof(int16) * 3;
	};

	struct BoneValues
	{
		int16 unused[4];
		int16 x, y, z;
		int16 denom;
		static const uint32 SIZE = sizeof(int16) * 8;
	};

	/*struct BoneValues
	{
		int16 denom;
		int16 x, y, z;
		int16 unused[4];
		static const uint32 SIZE = sizeof(int16) * 8;
	};*/

	struct Frag12
	{
		FragHeader header;
		uint32 flag;
		uint32 count;
		static const uint32 SIZE = sizeof(uint32) * 3;
	};
}
