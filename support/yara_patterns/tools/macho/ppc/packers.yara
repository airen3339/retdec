/*
 * YARA rules for PowerPC Mach-O packer detection.
 * Copyright (c) 2017 Avast Software, licensed under the MIT license
 */

import "macho"

rule upx_391_lzma_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.91 [LZMA]"
		source = "Made by RetDec Team"
		pattern = "48000B592807000E40820A447C0802A67CC93378810600007CA72B7838A4FFFE388300029001000888030000540BE8FE5402077E3860FA007C6358303863F17C7C260B787C211A1454210034380000007CC33378900900009403FFFC7C0118404180FFF8"
	strings:
		$1 = { 48 00 0B 59 28 07 00 0E 40 82 0A 44 7C 08 02 A6 7C C9 33 78 81 06 00 00 7C A7 2B 78 38 A4 FF FE 38 83 00 02 90 01 00 08 88 03 00 00 54 0B E8 FE 54 02 07 7E 38 60 FA 00 7C 63 58 30 38 63 F1 7C 7C 26 0B 78 7C 21 1A 14 54 21 00 34 38 00 00 00 7C C3 33 78 90 09 00 00 94 03 FF FC 7C 01 18 40 41 80 FF F8 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC)
}

rule upx_391_nrv2b_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.91 [NRV2B]"
		source = "Made by RetDec Team"
		pattern = "4800021D7C0029EC7DA802A628070002408200E490A600007C841A143C0080003D2080003863FFFF38A5FFFF3940FFFF480000B47C0900407D2948144CA20020392000017D291C2C386300047C0900407D2949144E8000208D0300019D0500014BFFFFD5"
	strings:
		$1 = { 48 00 02 1D 7C 00 29 EC 7D A8 02 A6 28 07 00 02 40 82 00 E4 90 A6 00 00 7C 84 1A 14 3C 00 80 00 3D 20 80 00 38 63 FF FF 38 A5 FF FF 39 40 FF FF 48 00 00 B4 7C 09 00 40 7D 29 48 14 4C A2 00 20 39 20 00 01 7D 29 1C 2C 38 63 00 04 7C 09 00 40 7D 29 49 14 4E 80 00 20 8D 03 00 01 9D 05 00 01 4B FF FF D5 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC)
}

rule upx_391_nrv2d_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.91 [NRV2D]"
		source = "Made by RetDec Team"
		pattern = "480002597C0029EC7DA802A6280700054082012090A600007C841A143C0080003D2080003863FFFF38A5FFFF3940FFFF480000F0392000017D291C2C386300047C0900407D294814612900014E8000208D0300019D0500017C0900407D294A1441A2FFD5"
	strings:
		$1 = { 48 00 02 59 7C 00 29 EC 7D A8 02 A6 28 07 00 05 40 82 01 20 90 A6 00 00 7C 84 1A 14 3C 00 80 00 3D 20 80 00 38 63 FF FF 38 A5 FF FF 39 40 FF FF 48 00 00 F0 39 20 00 01 7D 29 1C 2C 38 63 00 04 7C 09 00 40 7D 29 48 14 61 29 00 01 4E 80 00 20 8D 03 00 01 9D 05 00 01 7C 09 00 40 7D 29 4A 14 41 A2 FF D5 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC)
}

rule upx_391_nrv2e_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.91 [NRV2E]"
		source = "Made by RetDec Team"
		pattern = "480002757C0029EC7DA802A6280700084082013C90A600007C841A143C0080003D2080003863FFFF38A5FFFF3940FFFF4800010C392000017D291C2C386300047C0900407D294814612900014E8000208D0300019D0500017C0900407D294A1441A2FFD5"
	strings:
		$1 = { 48 00 02 75 7C 00 29 EC 7D A8 02 A6 28 07 00 08 40 82 01 3C 90 A6 00 00 7C 84 1A 14 3C 00 80 00 3D 20 80 00 38 63 FF FF 38 A5 FF FF 39 40 FF FF 48 00 01 0C 39 20 00 01 7D 29 1C 2C 38 63 00 04 7C 09 00 40 7D 29 48 14 61 29 00 01 4E 80 00 20 8D 03 00 01 9D 05 00 01 7C 09 00 40 7D 29 4A 14 41 A2 FF D5 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC)
}

rule upx_392_lzma_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.92 [LZMA]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B4544495400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000072ADC177555058210BCC0D830000000000??????00??????0000????00000???0E0000001A03007F3B5B412F7078"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 72 AD C1 77 55 50 58 21 0B CC 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 0E 00 00 00 1A 03 00 7F 3B 5B 41 2F 70 78 }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_392_nrv2b_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.92 [NRV2B]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B45444954000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000E40AE3B855505821028C0D830000000000??????00??????0000????00000???02000000??????F9FEEDFACE00120?0?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 E4 0A E3 B8 55 50 58 21 02 8C 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 02 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_392_nrv2d_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.92 [NRV2D]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B4544495400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000093A1F6295550582102E40D830000000000??????00??????0000????00000???08000000??????F9FEEDFACE00120?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 8C 11 EF E5 55 50 58 21 02 C8 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 05 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_392_nrv2e_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.92 [NRV2E]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B454449540000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008C11EFE55550582102C80D830000000000??????00??????0000????00000???05000000??????F9FEEDFACE00120?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 93 A1 F6 29 55 50 58 21 02 E4 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 08 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_393_lzma_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.93 [LZMA]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B4544495400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000073C3C179555058210BCC0D830000000000??????00??????0000????00000???0E0000001A03007F3B5B412F7078"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 73 C3 C1 79 55 50 58 21 0B CC 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 0E 00 00 00 1A 03 00 7F 3B 5B 41 2F 70 78 }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_393_nrv2b_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.93 [NRV2B]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B45444954000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000E520E3BA55505821028C0D830000000000??????00??????0000????00000???02000000??????F9FEEDFACE00120?0?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 E5 20 E3 BA 55 50 58 21 02 8C 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 02 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_393_nrv2d_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.93 [NRV2D]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B454449540000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008D27EFE75550582102C80D830000000000??????00??????0000????00000???05000000??????F9FEEDFACE00120?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 8D 27 EF E7 55 50 58 21 02 C8 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 05 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_393_nrv2e_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.93 [NRV2E]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B4544495400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000094B7F62B5550582102E40D830000000000??????00??????0000????00000???08000000??????F9FEEDFACE00120?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 94 B7 F6 2B 55 50 58 21 02 E4 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 08 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_394_lzma_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.94 [LZMA]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B4544495454000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000745FC762555058210BD00D830000000000??????00??????0000????00000???0E0000001A03007F3B5B412F7078"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 74 5F C7 62 55 50 58 21 0B D0 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 0E 00 00 00 1A 03 00 7F 3B 5B 41 2F 70 78 }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_394_nrv2b_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.94 [NRV2B]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B45444954000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ECE7E41755505821028C0D830000000000??????00??????0000????00000???02000000??????F9FEEDFACE00120?0?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 EC E7 E4 17 55 50 58 21 02 8C 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 02 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_394_nrv2d_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.94 [NRV2D]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B4544495400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000094EEF0445550582102C80D830000000000??????00??????0000????00000???05000000??????F9FEEDFACE00120?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 94 EE F0 44 55 50 58 21 02 C8 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 05 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}

rule upx_394_nrv2e_ppc {
	meta:
		tool = "P"
		name = "UPX"
		version = "3.94 [NRV2E]"
		source = "Made by RetDec Team"
		pattern = "83E3787FC4F37838A1003838C040003929FFF84BFFFDB97C7D1B787F83E3787FC4F3784BFFF8CD7FA3EB7838214050800100087C0803A6BB81FFF04E800020000000005F5F4C494E4B454449540000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009C7EF6885550582102E40D830000000000??????00??????0000????00000???08000000??????F9FEEDFACE00120?"
		start = 1929
	strings:
		$1 = { 83 E3 78 7F C4 F3 78 38 A1 00 38 38 C0 40 00 39 29 FF F8 4B FF FD B9 7C 7D 1B 78 7F 83 E3 78 7F C4 F3 78 4B FF F8 CD 7F A3 EB 78 38 21 40 50 80 01 00 08 7C 08 03 A6 BB 81 FF F0 4E 80 00 20 00 00 00 00 5F 5F 4C 49 4E 4B 45 44 49 54 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 9C 7E F6 88 55 50 58 21 02 E4 0D 83 00 00 00 00 00 ?? ?? ?? 00 ?? ?? ?? 00 00 ?? ?? 00 00 0? ?? 08 00 00 00 ?? ?? ?? F9 FE ED FA CE 00 12 0? }
	condition:
		$1 at macho.entry_point + 1929 or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_POWERPC) + 1929
}
