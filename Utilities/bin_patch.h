#pragma once

#include "BEType.h"
#include <vector>
#include <string>
#include <unordered_map>

enum class patch_type
{
	load,
	byte,
	le16,
	le32,
	le64,
	lef32,
	lef64,
	be16,
	be32,
	be64,
	bef32,
	bef64,
	smart,
};

enum class sp_patch_type
{
	normal,
	branch,
};

class patch_engine
{
	struct sp_patch_pattern
	{
		u32 value;
		sp_patch_type type;
	};

	struct patch
	{
		patch_type type;
		u32 offset;
		u64 value;

		std::vector<sp_patch_pattern> sp_pattern;
		std::vector<sp_patch_pattern> sp_result;

		template <typename T>
		T& value_as()
		{
			return *reinterpret_cast<T*>(reinterpret_cast<char*>(&value));
		}
	};

	// Database
	std::unordered_map<std::string, std::vector<patch>> m_map;

public:
	// Load from file
	void append(const std::string& path);

	// Apply patch (returns the number of entries applied)
	std::size_t apply(const std::string& name, u8* dst) const;
	// For smart patches
	std::size_t apply_smart(u32* addr, u32 size) const;
};
