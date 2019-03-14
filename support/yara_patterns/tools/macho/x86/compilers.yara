/*
 * YARA rules for x86 Mach-O compiler detection.
 * Copyright (c) 2017 Avast Software, licensed under the MIT license
 */

import "macho"

rule embarcadero_delphi_01_x86
{
	meta:
		tool = "C"
		name = "Embarcadero Delphi"
		source = "Made by RetDec Team"
		language = "Delphi"
		pattern = "8BD483C4F183E4F06800EF00BE558BEC???????3???4????????F?????????????????F???8???????0?????0?????????0?"
	strings:
		$1 = { 8B D4 83 C4 F1 83 E4 F0 68 00 EF 00 BE 55 8B EC ?? ?? ?? ?3 ?? ?4 ?? ?? ?? ?? F? ?? ?? ?? ?? ?? ?? ?? ?? F? ?? 8? ?? ?? ?? 0? ?? ?? 0? ?? ?? ?? ?? 0? }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_X86)
}

rule embarcadero_delphi_02_x86
{
	meta:
		tool = "C"
		name = "Embarcadero Delphi"
		source = "Made by RetDec Team"
		language = "Delphi"
		pattern = "8BD483C4F183E4F06800EF00BE558BEC83C4F85383C4F4E8?8??F?FF83C40C81C3A???F?FF8B83????0?00C600018D83?8A?0?0083C4F4E8?4??F?FF83C40C83C4F48??3??3?0?008D?3????0?00E8????FFFF???0?????????40?????????F"
	strings:
		$1 = { 8B D4 83 C4 F1 83 E4 F0 68 00 EF 00 BE 55 8B EC 83 C4 F8 53 83 C4 F4 E8 ?8 ?? F? FF 83 C4 0C 81 C3 A? ?? F? FF 8B 83 ?? ?? 0? 00 C6 00 01 8D 83 ?8 A? 0? 00 83 C4 F4 E8 ?4 ?? F? FF 83 C4 0C 83 C4 F4 8? ?3 ?? 3? 0? 00 8D ?3 ?? ?? 0? 00 E8 ?? ?? FF FF ?? ?0 ?? ?? ?? ?? ?4 0? ?? ?? ?? ?? }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_X86)
}

rule xcode_osx_sdk_01_x86
{
	meta:
		tool = "C"
		name = "XCode"
		extra = "with OS X SDK v10.4"
		source = "Made by RetDec Team"
		pattern = "6A0089E583E4F083EC108B5D04895C24008D4D08894C240483C301C1E30201CB895C2408E8????????F4"
	strings:
		$1 = { 6A 00 89 E5 83 E4 F0 83 EC 10 8B 5D 04 89 5C 24 00 8D 4D 08 89 4C 24 04 83 C3 01 C1 E3 02 01 CB 89 5C 24 08 E8 ?? ?? ?? ?? F4 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_X86)
}

rule xcode_osx_sdk_02_x86
{
	meta:
		tool = "C"
		name = "XCode"
		extra = "with OS X SDK v10.5"
		source = "Made by RetDec Team"
		pattern = "6A0089E583E4F083EC108B5D04895C24008D4D08894C240483C301C1E30201CB895C24088B0383C30485C075F7895C240CE8????????89442400E8????????F4"
	strings:
		$1 = { 6A 00 89 E5 83 E4 F0 83 EC 10 8B 5D 04 89 5C 24 00 8D 4D 08 89 4C 24 04 83 C3 01 C1 E3 02 01 CB 89 5C 24 08 8B 03 83 C3 04 85 C0 75 F7 89 5C 24 0C E8 ?? ?? ?? ?? 89 44 24 00 E8 ?? ?? ?? ?? F4 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_X86)
}

rule xcode_osx_sdk_03_x86
{
	meta:
		tool = "C"
		name = "XCode"
		extra = "with OS X SDK v10.6 or higher"
		source = "Made by RetDec Team"
		pattern = "6A0089E583E4F083EC108B5D04891C248D4D08894C240483C301C1E30201CB895C24088B0383C30485C075F7895C240CE8????????890424E8????????F4"
	strings:
		$1 = { 6A 00 89 E5 83 E4 F0 83 EC 10 8B 5D 04 89 1C 24 8D 4D 08 89 4C 24 04 83 C3 01 C1 E3 02 01 CB 89 5C 24 08 8B 03 83 C3 04 85 C0 75 F7 89 5C 24 0C E8 ?? ?? ?? ?? 89 04 24 E8 ?? ?? ?? ?? F4 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_X86)
}

rule gc_x86
{
	meta:
		tool = "C"
		name = "gc"
		language = "Go"
		pattern = "83EC088B4424088D5C240C890424895C2404E809000000CD03??????????????E9?BD?FFFF??????????????????????B801000000CD80C705F1000000F1000000C3????????????????????????????B869010000CD80730AC705F1000000F1000000C3????????????????????????B805000000CD8073"
	strings:
		$1 = { 83 EC 08 8B 44 24 08 8D 5C 24 0C 89 04 24 89 5C 24 04 E8 09 00 00 00 CD 03 ?? ?? ?? ?? ?? ?? ?? E9 ?B D? FF FF ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? B8 01 00 00 00 CD 80 C7 05 F1 00 00 00 F1 00 00 00 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? B8 69 01 00 00 CD 80 73 0A C7 05 F1 00 00 00 F1 00 00 00 C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? B8 05 00 00 00 CD 80 73 }
	condition:
		$1 at macho.entry_point or $1 at macho.entry_point_for_arch(macho.CPU_TYPE_X86)
}
