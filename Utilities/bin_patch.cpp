#include "bin_patch.h"
#include <yaml-cpp/yaml.h>
#include "File.h"
#include "Config.h"
#include "../rpcs3/Crypto/utils.h"
#include "../rpcs3/Emu/Cell/PPUOpcodes.h"

template <>
void fmt_class_string<patch_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](patch_type value)
	{
		switch (value)
		{
		case patch_type::load: return "load";
		case patch_type::byte: return "byte";
		case patch_type::le16: return "le16";
		case patch_type::le32: return "le32";
		case patch_type::le64: return "le64";
		case patch_type::bef32: return "bef32";
		case patch_type::bef64: return "bef64";
		case patch_type::be16: return "be16";
		case patch_type::be32: return "be32";
		case patch_type::be64: return "be64";
		case patch_type::lef32: return "lef32";
		case patch_type::lef64: return "lef64";
		case patch_type::smart: return "smart";
		}

		return unknown;
	});
}

void patch_engine::append(const std::string& patch)
{
	if (fs::file f{patch})
	{
		YAML::Node root;

		try
		{
			root = YAML::Load(f.to_string());
		}
		catch (const std::exception& e)
		{
			LOG_FATAL(GENERAL, "Failed to load patch file %s\n%s thrown: %s", patch, typeid(e).name(), e.what());
			return;
		}

		for (auto pair : root)
		{
			auto& name = pair.first.Scalar();
			auto& data = m_map[name];

			for (auto patch : pair.second)
			{
				u64 type64 = 0;
				cfg::try_to_enum_value(&type64, &fmt_class_string<patch_type>::format, patch[0].Scalar());

				struct patch info{};
				info.type   = static_cast<patch_type>(type64);
				info.offset = patch[1].as<u32>(0);

				switch (info.type)
				{
				case patch_type::load:
				{
					// Special syntax: copy named sequence (must be loaded before)
					const auto found = m_map.find(patch[1].Scalar());

					if (found != m_map.end())
					{
						// Address modifier (optional)
						const u32 mod = patch[2].as<u32>(0);

						for (const auto& rd : found->second)
						{
							info = rd;
							info.offset += mod;
							data.emplace_back(info);
						}
						continue;
					}

					// TODO: error
					break;
				}
				case patch_type::bef32:
				case patch_type::lef32:
				{
					info.value_as<f32>() = patch[2].as<f32>();
					break;
				}
				case patch_type::bef64:
				case patch_type::lef64:
				{
					info.value_as<f64>() = patch[2].as<f64>();
					break;
				}
				case patch_type::smart:
				{
					//	parse patterns here
					auto parse_smart = [](std::string& pattern, std::vector<sp_patch_pattern>& vres) -> bool
					{
						if (pattern.length() % 8 != 0)
						{
							LOG_ERROR(GENERAL, "Faulty smart pattern");
							return false;
						}

						for (std::size_t i = 0; i < pattern.length() / 8; i++)
						{
							if ((pattern[i * 8] >= 'A' && pattern[i * 8] <= 'F') || (pattern[i * 8] >= '0' && pattern[i * 8] <= '9'))
							{
								//	normal u32
								char hexstuff[9];
								sp_patch_pattern result;

								pattern.copy(hexstuff, 8, (i * 8));
								hexstuff[8] = 0;

								if (is_hex(hexstuff, 8) == false)
								{
									LOG_ERROR(GENERAL, "Expected 8 hexadecimal chars in smart patch");
									return false;
								}

								result.value = (u32)hex_to_u64(hexstuff);
								result.type = sp_patch_type::normal;
								vres.push_back(result);
								continue;
							}

							if (pattern[i * 8] == 'R' && pattern[(i * 8) + 1] == 'B')
							{
								//	B relative instruction
								//	next 3 bytes encodes the relative value index
								char hexstuff[7];
								sp_patch_pattern result;

								pattern.copy(hexstuff, 6, (i * 8) + 2);
								hexstuff[6] = 0;

								if (is_hex(hexstuff, 6) == false)
								{
									LOG_ERROR(GENERAL, "Expected 6 hexadecimal chars after RB indicator");
									return false;
								}

								result.value = (u32)hex_to_u64(hexstuff);
								result.type = sp_patch_type::branch;
								vres.push_back(result);
								continue;
							}

							return false;
						}
						return true;
					};

					auto pattern = patch[1].Scalar();
					auto result_pattern = patch[2].Scalar();

					if (pattern.length() != result_pattern.length())
					{
						LOG_ERROR(GENERAL, "Both smart patterns should be the same length");
						break;
					}

					if (parse_smart(pattern, info.sp_pattern) == false)	break;
					if (parse_smart(result_pattern, info.sp_result) == false) break;

					break;
				}
				default:
				{
					info.value = patch[2].as<u64>();
					break;
				}
				}

				data.emplace_back(info);
			}
		}
	}
}

std::size_t patch_engine::apply_smart(u32* addr, u32 size) const
{
	const auto found = m_map.find("smart");
	std::size_t applied = 0;

	if (found == m_map.cend())
	{
		return 0;
	}

	for (const auto& p : found->second)
	{
		auto ptr = addr;
		if (p.type != patch_type::smart)
		{
			LOG_ERROR(GENERAL, "Non-smart patch inside smart section");
			continue;
		}

		u32 matching = 0;
		std::unordered_map<u32, std::pair<u32,u32>> RelativeBranches;

		//	check whole segment for pattern(naive 
		for (u32 i = size; i < (size / 4); i++)
		{
			switch (p.sp_pattern[matching].type)
			{
			case sp_patch_type::normal:
			{
				if (addr[i] == p.sp_pattern[matching].value) matching++;
				else
				{
					i -= matching;
					matching = 0;
				}
				break;
			}
			case sp_patch_type::branch:
			{
				//checks if the instruction is a branch
				if ((addr[i] & 0xfc000000) == ppu_instructions::B({}, {}))
				{
					//saves the jump for later(maybe)
					RelativeBranches[p.sp_pattern[matching].value] = std::make_pair(addr[i], matching);
					matching++;
				}
				else
				{
					i -= matching;
					matching = 0;
				}
				break;
			}
			default:
			{
				LOG_ERROR(GENERAL, "Unknown sp_patch_type");
				matching = 0;
				break;
			}
			}

			if (matching == p.sp_pattern.size())
			{
				//We got a match
				for (u32 pat_index = 0; pat_index < p.sp_pattern.size(); pat_index++)
				{
					switch (p.sp_result[pat_index].type)
					{
					case sp_patch_type::normal:
					{
						addr[(i - p.sp_pattern.size()) + 1] = p.sp_result[pat_index].value;
						break;
					}
					case sp_patch_type::branch:
					{
						auto rbranch = RelativeBranches.find(p.sp_result[pat_index].value);
						if (rbranch == RelativeBranches.end())
						{
							LOG_ERROR(GENERAL, "Relative branch 0x%x wasn't found in detection pattern", p.sp_result[pat_index].value);
							break;
						}


						//Adjust relative jump offset
						//TODO: add a check for overflow
						addr[(i - p.sp_pattern.size()) + 1] = RelativeBranches[p.sp_result[pat_index].value].first + ((pat_index- RelativeBranches[p.sp_result[pat_index].value].second)*4);
						break;
					}
					default:
					{
						LOG_ERROR(GENERAL, "Unknown sp_patch_type");
						break;
					}
					}
				}

				matching = 0;
				applied++;
			}
		}
	}

	return applied;
}

std::size_t patch_engine::apply(const std::string& name, u8* dst) const
{
	const auto found = m_map.find(name);

	if (found == m_map.cend())
	{
		return 0;
	}

	// Apply modifications sequentially
	for (const auto& p : found->second)
	{
		auto ptr = dst + p.offset;

		switch (p.type)
		{
		case patch_type::load:
		{
			// Invalid in this context
			break;
		}
		case patch_type::byte:
		{
			*ptr = static_cast<u8>(p.value);
			break;
		}
		case patch_type::le16:
		{
			*reinterpret_cast<le_t<u16, 1>*>(ptr) = static_cast<u16>(p.value);
			break;
		}
		case patch_type::le32:
		case patch_type::lef32:
		{
			*reinterpret_cast<le_t<u32, 1>*>(ptr) = static_cast<u32>(p.value);
			break;
		}
		case patch_type::le64:
		case patch_type::lef64:
		{
			*reinterpret_cast<le_t<u64, 1>*>(ptr) = static_cast<u64>(p.value);
			break;
		}
		case patch_type::be16:
		{
			*reinterpret_cast<be_t<u16, 1>*>(ptr) = static_cast<u16>(p.value);
			break;
		}
		case patch_type::be32:
		case patch_type::bef32:
		{
			*reinterpret_cast<be_t<u32, 1>*>(ptr) = static_cast<u32>(p.value);
			break;
		}
		case patch_type::be64:
		case patch_type::bef64:
		{
			*reinterpret_cast<be_t<u64, 1>*>(ptr) = static_cast<u64>(p.value);
			break;
		}
		}
	}

	return found->second.size();
}
