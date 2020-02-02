#pragma once

#include "../../Utilities/types.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/SPUThread.h"
#include <vector>

using statichle_spu_func = void (*)(spu_thread*);

struct flair_pattern
{
	u16 start_pattern[32];
	u8 crc16_length;
	u16 crc16;
	u16 total_length;
};

struct shle_ppu_pattern
{
	flair_pattern flair;
	std::string module;
	std::string name;
	u32 fnid;
};

struct shle_spu_pattern
{
	std::vector<u16> pattern;
	std::string func_name;
	statichle_spu_func func;
};

class statichle_handler
{
public:
	statichle_handler(int);
	~statichle_handler();

	bool load_ppu_patterns();
	bool load_spu_patterns();
	bool check_against_ppu_patterns(vm::cptr<u8> data, u32 size, u32 addr);
	bool check_against_spu_patterns(u8* data, u32 size);
	const std::pair<const std::string, const statichle_spu_func> get_spu_func(u16 index);

protected:
	bool convert_flair(flair_pattern& flair, const std::string& start_pattern, const std::string& crc16_length, const std::string& crc16, const std::string& total_length);
	bool check_against_flair(const flair_pattern& flair, const u8* data, u32 size);
	bool check_against_rawpattern(const std::vector<u16>& pattern, const u8* data, u32 size);
	uint16_t gen_CRC16(const uint8_t* data_p, size_t length);

	std::vector<shle_ppu_pattern> hle_ppu_patterns;
	std::vector<shle_spu_pattern> hle_spu_patterns;
};
