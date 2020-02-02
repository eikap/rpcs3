#include "stdafx.h"
#include "StaticHLE.h"

LOG_CHANNEL(static_hle);

// for future use
DECLARE(ppu_module_manager::static_hle)("static_hle", []() {});

const std::vector<std::array<std::string, 6>> shle_ppu_patterns_list{
    {"2BA5000778630020788400207C6B1B78419D00702C2500004D82002028A5000F", "FF", "36D0", "05C4", "sys_libc", "memcpy"},
    {"2BA5000778630020788400207C6B1B78419D00702C2500004D82002028A5000F", "5C", "87A0", "05C4", "sys_libc", "memcpy"},
    {"2B8500077CA32A14788406207C6A1B78409D009C3903000198830000788B45E4", "B4", "1453", "00D4", "sys_libc", "memset"},
    {"280500087C661B7840800020280500004D8200207CA903A69886000038C60001", "F8", "F182", "0118", "sys_libc", "memset"},
    {"2B8500077CA32A14788406207C6A1B78409D009C3903000198830000788B45E4", "70", "DFDA", "00D4", "sys_libc", "memset"},
    {"7F832000FB61FFD8FBE1FFF8FB81FFE0FBA1FFE8FBC1FFF07C7B1B787C9F2378", "FF", "25B5", "12D4", "sys_libc", "memmove"},
    {"2B850007409D00B07C6923785520077E2F800000409E00ACE8030000E9440000", "FF", "71F1", "0158", "sys_libc", "memcmp"},
    {"280500007CE32050788B0760418200E028850100786A07607C2A580040840210", "FF", "87F2", "0470", "sys_libc", "memcmp"},
    {"2B850007409D00B07C6923785520077E2F800000409E00ACE8030000E9440000", "68", "EF18", "0158", "sys_libc", "memcmp"},
};

void cellAtomicAdd32_spu(spu_thread* spu)
{
	be_t<u32> toadd = spu->gpr[5]._u32[3];

	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_add(toadd);

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicAdd64_spu(spu_thread* spu)
{
	be_t<u64> toadd = spu->gpr[5]._u64[1];

	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_add(toadd);

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicAnd32_spu(spu_thread* spu)
{
	be_t<u32> mask = spu->gpr[5]._u32[3];

	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_and(mask);

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicAnd64_spu(spu_thread* spu)
{
	be_t<u64> mask = spu->gpr[5]._u64[1];

	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_and(mask);

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicCompareAndSwap32_spu(spu_thread* spu)
{
	be_t<u32> expected = spu->gpr[5]._u32[3];
	be_t<u32> desired  = spu->gpr[6]._u32[3];

	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	datomic.compare_exchange(expected, desired);

	spu->gpr[3]._u64[1] = expected;
}
void cellAtomicCompareAndSwap64_spu(spu_thread* spu)
{
	be_t<u64> expected = spu->gpr[5]._u64[1];
	be_t<u64> desired  = spu->gpr[6]._u64[1];

	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	datomic.compare_exchange(expected, desired);

	spu->gpr[3]._u64[1] = expected;
}

void cellAtomicDecr32_spu(spu_thread* spu)
{
	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_sub(1);

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicDecr64_spu(spu_thread* spu)
{
	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_sub(1);

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicIncr32_spu(spu_thread* spu)
{
	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_add(1);

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicIncr64_spu(spu_thread* spu)
{
	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_add(1);

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicNop32_spu(spu_thread* spu)
{
	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.load();

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicNop64_spu(spu_thread* spu)
{
	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.load();

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicOr32_spu(spu_thread* spu)
{
	be_t<u32> mask = spu->gpr[5]._u32[3];

	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_or(mask);

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicOr64_spu(spu_thread* spu)
{
	be_t<u64> mask = spu->gpr[5]._u64[1];

	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_or(mask);

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicStore32_spu(spu_thread* spu)
{
	be_t<u32> value = spu->gpr[5]._u32[3];

	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_op([&](be_t<u32>& v) { v = value; });

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicStore64_spu(spu_thread* spu)
{
	be_t<u64> value = spu->gpr[5]._u64[1];

	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_op([&](be_t<u64>& v) { v = value; });

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicSub32_spu(spu_thread* spu)
{
	be_t<u32> tosub = spu->gpr[5]._u32[3];

	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_sub(tosub);

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicSub64_spu(spu_thread* spu)
{
	be_t<u64> tosub = spu->gpr[5]._u64[1];

	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_sub(tosub);

	spu->gpr[3]._u64[1] = previous;
}

void cellAtomicTestAndDecr32_spu(spu_thread* spu)
{
	atomic_t<be_t<u32>>& datomic = *reinterpret_cast<atomic_t<be_t<u32>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u32> previous           = datomic.fetch_op([&](be_t<u32>& v) { if (v) { v--; } });

	spu->gpr[3]._u64[1] = previous;
}
void cellAtomicTestAndDecr64_spu(spu_thread* spu)
{
	atomic_t<be_t<u64>>& datomic = *reinterpret_cast<atomic_t<be_t<u64>>*>(vm::base(spu->gpr[4]._u64[1]));
	be_t<u64> previous           = datomic.fetch_op([&](be_t<u64>& v) { if (v) { v--; } });

	spu->gpr[3]._u64[1] = previous;
}

// Raw patterns are used for spu
const std::vector<std::tuple<std::string, std::string, statichle_spu_func>> shle_spu_patterns_list{
    {"141FC1913FE102104080000C7C0008881400C80F141FC80E7C00078D0F3F870A0C0006878161C6087F00058033......4080400C3280000B0F6085063FE0018A4080680E40805A0D1820820418018186040002083FE1020921A0080A21A008"
     "8821A0090921A0098C21A00A0B21A00A8E01A00D8200600000340003163EC003153B818B0718014394B2458A15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF5030400038335000000",
        "cellAtomicAdd32_spu", &cellAtomicAdd32_spu},
    {"141FC1913FE102104080000C7C0008881401C80F141FC80E7C00078D0F3F470A0C0006878161C6087F00058033......4080400C33......0F60C5063280000B0400018A4080680F40805A0D18208204180181873FE102090400020821A0080A2"
     "1A0088821A0090921A0098C21A00A0B21A00A8F01A00D8200600000340003973EE003953B81CB8618414316B2858B0E68014314B245CA15240003920060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF40304"
     "00030335000000",
        "cellAtomicAdd64_spu", &cellAtomicAdd64_spu},
    {"141FC1913FE102104080000C7C0008881400C80F141FC80E7C00078D0F3F870A0C0006878161C6087F00058033......4080400C3280000B0F6085063FE0018A4080680E40805A0D1820820418018186040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340003163EC003153B818B0718214394B2458A15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF5030400038335000000",
        "cellAtomicAnd32_spu", &cellAtomicAnd32_spu},
    {"141FC1913FE102104080000C7C0008881401C80F141FC80E7C00078D0F3F470A0C0006878161C6087F00058033......4080400C3280000B0F60C5063FE0018A4080680E40805A0D1820820418018186040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340003163EE003153B818B0718214394B2458A15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF5030400038335000000",
        "cellAtomicAnd64_spu", &cellAtomicAnd64_spu},
    {"141FC1923FE102114080000C7C00090D1400C890141FC88F7C00080E0F3F878A0C0007088162060D7F00058033......4080400C3280000B0F6085073FE0018A4080680E40805A0D182082041801C183040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340001943EC001843B80CA07780143932000069334000197B2C5C304240001960060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D95217FF4150400038335"
     "000000",
        "cellAtomicCompareAndSwap32_spu", &cellAtomicCompareAndSwap32_spu},
    {"141FC1923FE102114080000C7C00090D1401C890141FC88F7C00080E0F3F478A0C0007088162060D7F00058033......4080400C3280000B0F60C5073FE0018A4080680E40805A0D182082041801C183040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340001963EE001843B80CB077801439536000A944C02CA132000069334000199B3064304240001980060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D9721"
     "7FF3170400038335000000",
        "cellAtomicCompareAndSwap64_spu", &cellAtomicCompareAndSwap64_spu},
    {"141FC1903FE1020F4080000B7C0008071400C78E141FC78D7C00070C0F3F86890C000606814185877F00050033......4080400B3280000A0F6084853FE001894080680D40805A0C1820820418014185040002073FE1020821A0080921A008872"
     "1A0090821A0098B21A00A0A21A00A8D01A00D8200600000340002953EC002943B814A861CFFC313B2254994240002910060000021A0080921A0088721A0090821A0098B21A00A0A21A00A8C01A00D83217FF5030400030335000000",
        "cellAtomicDecr32_spu", &cellAtomicDecr32_spu},
    {"141FC1903FE1020F4080000B7C0008071401C78E141FC78D7C00070C0F3F46890C000606814185877F00050033......4080400C33......0F60C4853280000B0400018932FFFF8A4080680F40805A0D1820820418014186040002073FE102082"
     "1A0080921A0088721A0090821A0098C21A00A0B21A00A8F01A00D8200600000340003163EE003143B818B0518428295B2654A8E68028293B2258994240003110060000021A0080921A0088721A0090821A0098C21A00A0B21A00A8D01A00D8321"
     "7FF4030400028335000000",
        "cellAtomicDecr64_spu", &cellAtomicDecr64_spu},
    {"141FC1903FE1020F4080000B7C0008071400C78E141FC78D7C00070C0F3F86890C000606814185877F00050033......4080400B3280000A0F6084853FE001894080680D40805A0C1820820418014185040002073FE1020821A0080921A008872"
     "1A0090821A0098B21A00A0A21A00A8D01A00D8200600000340002953EC002943B814A861C004313B2254994240002910060000021A0080921A0088721A0090821A0098B21A00A0A21A00A8C01A00D83217FF5030400030335000000",
        "cellAtomicIncr32_spu", &cellAtomicIncr32_spu},
    {"141FC1913FE102104080000B7C00088C1401C80F141FC80E7C00078D0F3F47080C0006868141858C7F00050033......4080400C328080870F60C40533......0400018A3280000B4080680F15404387182082041801418640805A0D3FE102090"
     "400020821A0080A21A0088821A0090921A0098C21A00A0B21A00A8F01A00D8200600000340003173EE003153B818B851841C296B2858B0E6801C294B245CA15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01"
     "A00D83217FF4030400028335000000",
        "cellAtomicIncr64_spu", &cellAtomicIncr64_spu},
    {"141FC1953580001E408000103FE102087C000A913FE001861400C4147C000A130C00099281E488117F00078033......40804009328000070400018C21A0080C1823820D408068040400068B3FE1068A21A0088B21A0090A21A0098921A00A072"
     "1A00A8401A00D8200600000141F0402180181053880C1033B81418335000000",
        "cellAtomicNop32_spu", &cellAtomicNop32_spu},
    {"141FC1953580001E408000103FE102087C000A913FE001861401C4147C000A130C00099281E488117F00078033......40804009328000070400018C21A0080C1823820D408068040400068B3FE1068A21A0088B21A0090A21A0098921A00A072"
     "1A00A8401A00D8200600000141E0402180181053880C1033B81418335000000",
        "cellAtomicNop64_spu", &cellAtomicNop64_spu},
    {"141FC1913FE102104080000C7C0008881400C80F141FC80E7C00078D0F3F870A0C0006878161C6087F00058033......4080400C3280000B0F6085063FE0018A4080680E40805A0D1820820418018186040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340003163EC003153B818B0708214394B2458A15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF5030400038335000000",
        "cellAtomicOr32_spu", &cellAtomicOr32_spu},
    {"141FC1913FE102104080000C7C0008881401C80F141FC80E7C00078D0F3F470A0C0006878161C6087F00058033......4080400C3280000B0F60C5063FE0018A4080680E40805A0D1820820418018186040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340003163EE003153B818B0708214394B2458A15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF5030400038335000000",
        "cellAtomicOr64_spu", &cellAtomicOr64_spu},
    {"141FC1913FE102104080000B7C00088C1400C80F141FC80E7C00078D0F3F87090C0006878141C58C7F00050033......4080400B3280000A0F6084863FE001894080680D40805A0C1820820418018186040002073FE1020821A0080921A008872"
     "1A0090821A0098B21A00A0A21A00A8D01A00D8200600000340003163EC003153B818B13B2458295240003120060000021A0080921A0088721A0090821A0098B21A00A0A21A00A8C01A00D83217FF5830400098335000000",
        "cellAtomicStore32_spu", &cellAtomicStore32_spu},
    {"141FC1913FE102104080000B7C00088C1401C80F141FC80E7C00078D0F3F47090C0006878141C58C7F00050033......4080400B3280000A0F60C4863FE001894080680D40805A0C1820820418018186040002073FE1020821A0080921A008872"
     "1A0090821A0098B21A00A0A21A00A8D01A00D8200600000340003163EE003153B818B13B2458295240003120060000021A0080921A0088721A0090821A0098B21A00A0A21A00A8C01A00D83217FF5830400098335000000",
        "cellAtomicStore64_spu", &cellAtomicStore64_spu},
    {"141FC1913FE102104080000C7C0008881400C80F141FC80E7C00078D0F3F870A0C0006878161C6087F00058033......4080400C3280000B0F6085063FE0018A4080680E40805A0D1820820418018186040002083FE1020921A0080A21A008882"
     "1A0090921A0098C21A00A0B21A00A8E01A00D8200600000340003163EC003153B818B070801C294B2458A15240003120060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF5030400038335000000",
        "cellAtomicSub32_spu", &cellAtomicSub32_spu},
    {"141FC1913FE102104080000C7C0008881401C80F141FC80E7C00078D0F3F470A0C0006878161C6087F00058033......4080400C33......0F60C5063280000B0400018A4080680F40805A0D18208204180181873FE102090400020821A0080A2"
     "1A0088821A0090921A0098C21A00A0B21A00A8F01A00D8200600000340003973EE003953B81CB8608418296B2858B0E68218294B245CA15240003920060000021A0080A21A0088821A0090921A0098C21A00A0B21A00A8D01A00D83217FF40304"
     "00030335000000",
        "cellAtomicSub64_spu", &cellAtomicSub64_spu},
    {"141FC1903FE1020F4080000B7C0008071400C78E141FC78D7C00070C0F3F86890C000606814185877F00050033......4080400B3280000A0F6084853FE001894080680D40805A0C1820820418014185040002073FE1020821A0080921A008872"
     "1A0090821A0098B21A00A0A21A00A8D01A00D8200600000340002913EC002863B8148831CFFC1842500000334000294B2650206240002930060000021A0080921A0088721A0090821A0098B21A00A0A21A00A8C01A00D92217FF41235000000",
        "cellAtomicTestAndDecr32_spu", &cellAtomicTestAndDecr32_spu},
    {"141FC1903FE1020F4080000B7C0008071401C78E141FC78D7C00070C0F3F46880C000606814185877F00050033......4080400D33......0F60C4053FE0018A4080000C328000104080681132FFFF8B40805A0E18208204180141863FE102090"
     "400020821A0080A21A0088821A0090921A0098D21A00A0C21A00A9101A00D8200600000340003153EE003073B818A85780402941842C29236000A13B084890F4C02C983210007036802C28434000318B2E60207240003170060000021A0080A21"
     "A0088821A0090921A0098D21A00A0C21A00A8E01A00D96217FF1960400028335000000",
        "cellAtomicTestAndDecr64_spu", &cellAtomicTestAndDecr64_spu},
};

statichle_handler::statichle_handler(int)
{
	load_ppu_patterns();
	load_spu_patterns();
}

statichle_handler::~statichle_handler()
{
}

bool statichle_handler::convert_flair(flair_pattern& flair, const std::string& start_pattern, const std::string& crc16_length, const std::string& crc16, const std::string& total_length)
{
	if (start_pattern.size() != 64)
	{
		static_hle.error("Start pattern length != 64");
		return false;
	}
	if (crc16_length.size() != 2)
	{
		static_hle.error("Crc16_length != 2");
		return false;
	}
	if (crc16.size() != 4)
	{
		static_hle.error("Crc16 length != 4");
		return false;
	}
	if (total_length.size() != 4)
	{
		static_hle.error("Total length != 4");
		return false;
	}

	auto char_to_u8 = [&](u8 char1, u8 char2) -> u16 {
		u8 hv, lv;
		if (char1 == '.' && char2 == '.')
			return 0xFFFF;

		if (char1 == '.' || char2 == '.')
		{
			static_hle.error("Broken byte pattern");
			return -1;
		}

		hv = char1 > '9' ? char1 - 'A' + 10 : char1 - '0';
		lv = char2 > '9' ? char2 - 'A' + 10 : char2 - '0';

		return (hv << 4) | lv;
	};

	for (u32 j = 0; j < 32; j++)
		flair.start_pattern[j] = char_to_u8(start_pattern[j * 2], start_pattern[(j * 2) + 1]);

	flair.crc16_length = char_to_u8(crc16_length[0], crc16_length[1]);
	flair.crc16        = (char_to_u8(crc16[0], crc16[1]) << 8) | char_to_u8(crc16[2], crc16[3]);
	flair.total_length = (char_to_u8(total_length[0], total_length[1]) << 8) | char_to_u8(total_length[2], total_length[3]);

	return true;
}

bool statichle_handler::load_ppu_patterns()
{
	hle_ppu_patterns.clear();

	for (const auto& pattern : shle_ppu_patterns_list)
	{
		shle_ppu_pattern dapat;

		if (!convert_flair(dapat.flair, pattern[0], pattern[1], pattern[2], pattern[3]))
		{
			static_hle.error("Error in pattern for %s::%s", pattern[4], pattern[5]);
			continue;
		}

		dapat.module = pattern[4];
		dapat.name   = pattern[5];

		dapat.fnid = ppu_generate_id(dapat.name.c_str());

		static_hle.notice("Added a PPU pattern for %s(id:0x%X)", dapat.name, dapat.fnid);
		hle_ppu_patterns.push_back(std::move(dapat));
	}

	return true;
}

bool statichle_handler::load_spu_patterns()
{
	hle_spu_patterns.clear();

	for (const auto& [pattern, func_name, func] : shle_spu_patterns_list)
	{
		shle_spu_pattern dapat;

		if ((pattern.size() % 2) != 0)
			continue;

		auto char_to_u8 = [&](u8 char1, u8 char2) -> u16 {
			u8 hv, lv;
			if (char1 == '.' && char2 == '.')
				return 0xFFFF;

			if (char1 == '.' || char2 == '.')
			{
				static_hle.error("Broken byte pattern");
				return -1;
			}

			hv = char1 > '9' ? char1 - 'A' + 10 : char1 - '0';
			lv = char2 > '9' ? char2 - 'A' + 10 : char2 - '0';

			return (hv << 4) | lv;
		};

		for (u32 j = 0; j < pattern.size() / 2; j++)
			dapat.pattern.push_back(char_to_u8(pattern[j * 2], pattern[(j * 2) + 1]));

		dapat.func_name = func_name;
		dapat.func      = func;

		static_hle.notice("Added a SPU pattern for %s", dapat.func_name);
		hle_spu_patterns.push_back(std::move(dapat));
	}

	return true;
}

const std::pair<const std::string, const statichle_spu_func> statichle_handler::get_spu_func(u16 index)
{
	return std::make_pair(hle_spu_patterns[index].func_name, hle_spu_patterns[index].func);
}

uint16_t statichle_handler::gen_CRC16(const uint8_t* data_p, size_t length)
{
	unsigned char i;
	unsigned int data;

	if (length == 0)
		return 0;
	unsigned int crc = 0xFFFF;
	do
	{
		data = *data_p++;
		for (i = 0; i < 8; i++)
		{
			if ((crc ^ data) & 1)
				crc = (crc >> 1) ^ 0x8408;
			else
				crc >>= 1;
			data >>= 1;
		}
	} while (--length != 0);

	crc  = ~crc;
	data = crc;
	crc  = (crc << 8) | ((data >> 8) & 0xff);
	return static_cast<u16>(crc);
}

bool statichle_handler::check_against_flair(const flair_pattern& flair, const u8* data, u32 size)
{
	if (size < flair.total_length)
		return false;

	// check start pattern
	for (std::size_t i = 0; i < 32; i++)
	{
		if (flair.start_pattern[i] == 0xFFFF)
			continue;
		if (flair.start_pattern[i] != data[i])
			return false;
	}

	// start pattern ok, checking middle part
	if (flair.crc16_length != 0)
		if (gen_CRC16(&data[32], flair.crc16_length) != flair.crc16)
			return false;

	return true;
}

bool statichle_handler::check_against_rawpattern(const std::vector<u16>& pattern, const u8* data, u32 size)
{
	if (size < pattern.size())
		return false;

	// check pattern
	for (std::size_t i = 0; i < pattern.size(); i++)
	{
		if (pattern[i] == 0xFFFF)
			continue;
		if (pattern[i] != data[i])
			return false;
	}

	return true;
}

bool statichle_handler::check_against_ppu_patterns(const vm::cptr<u8> data, u32 size, u32 addr)
{
	for (auto& pat : hle_ppu_patterns)
	{
		if (!check_against_flair(pat.flair, data.get_ptr(), size))
			continue;

		// we got a match!
		static_hle.success("Found PPU function %s at 0x%x", pat.name, addr);

		// patch the code
		const auto smodule = ppu_module_manager::get_module(pat.module);

		if (smodule == nullptr)
		{
			static_hle.error("Couldn't find module: %s", pat.module);
			return false;
		}

		const auto sfunc = &smodule->functions.at(pat.fnid);
		const u32 target = ppu_function_manager::addr + 8 * sfunc->index;

		// write stub
		vm::write32(addr, ppu_instructions::LIS(0, (target & 0xFFFF0000) >> 16));
		vm::write32(addr + 4, ppu_instructions::ORI(0, 0, target & 0xFFFF));
		vm::write32(addr + 8, ppu_instructions::MTCTR(0));
		vm::write32(addr + 12, ppu_instructions::BCTR());

		return true;
	}

	return false;
}

bool statichle_handler::check_against_spu_patterns(u8* data, u32 size)
{
	u16 i = 0;
	for (u16 i = 0; i < hle_spu_patterns.size(); i++)
	{
		const auto& pat = hle_spu_patterns[i];
		if (!check_against_rawpattern(pat.pattern, data, size))
			continue;

		// we got a match!
		static_hle.success("Found SPU function %s", pat.func_name);

		// WRITE STATICHLE instruction and BI LR
		// 0x04 << 21 | func_number
		// 0x35 00 00 00 BI LR

		be_t<u32>* ptr = reinterpret_cast<be_t<u32>*>(data);

		ptr[0] = (0x04 << 21) | i;
		ptr[1] = 0x35000000;

		return true;
	}

	return false;
}
