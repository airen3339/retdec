/**
 * @file src/fileformat/file_format/pe/pe_format.cpp
 * @brief Methods of PeFormat class.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <algorithm>
#include <cassert>
#include <map>
#include <regex>
#include <tuple>
#include <unordered_map>

#include <openssl/asn1.h>
#include <openssl/x509.h>

#include "retdec/utils/container.h"
#include "retdec/utils/conversion.h"
#include "retdec/utils/scope_exit.h"
#include "retdec/utils/string.h"
#include "retdec/utils/dynamic_buffer.h"
#include "retdec/fileformat/file_format/pe/pe_format.h"
#include "retdec/fileformat/file_format/pe/pe_format_parser/pe_format_parser32.h"
#include "retdec/fileformat/file_format/pe/pe_format_parser/pe_format_parser64.h"
#include "retdec/fileformat/types/dotnet_headers/metadata_tables.h"
#include "retdec/fileformat/types/dotnet_types/dotnet_type_reconstructor.h"
#include "retdec/fileformat/types/visual_basic/visual_basic_structures.h"
#include "retdec/fileformat/utils/asn1.h"
#include "retdec/fileformat/utils/conversions.h"
#include "retdec/fileformat/utils/file_io.h"
#include "retdec/crypto/crypto.h"

using namespace retdec::utils;
using namespace PeLib;

namespace retdec {
namespace fileformat {

namespace
{

const std::size_t PE_IMAGE_SYM_CLASS_EXTERNAL = 2;
const std::size_t PE_IMAGE_SYM_CLASS_STATIC = 3;
const std::size_t PE_IMAGE_SYM_CLASS_FILE = 103;

const std::size_t STANDARD_RICH_HEADER_OFFSET = 0x80;
const std::size_t MINIMAL_PDB_NB10_INFO_LENGTH = 17;
const std::size_t MINIMAL_PDB_RSDS_INFO_LENGTH = 25;

const std::vector<std::string> stubDatabase =
{
	"This program cannot be run in DOS mode",
	"This program must be run under Win32",
	"This program requires Microsoft Windows",
	"Win32 only"
};

const std::map<std::string, std::size_t> visualBasicLibrariesMap =
{
	{"msvbvm10.dll", 1},
	{"msvbvm20.dll", 2},
	{"msvbvm30.dll", 3},
	{"msvbvm40.dll", 4},
	{"msvbvm50.dll", 5},
	{"msvbvm60.dll", 6},
	{"vb40032.dll", 4}
};

const std::map<std::size_t, std::string> resourceTypeMap
{
	{PELIB_RT_CURSOR, "Cursor"},
	{PELIB_RT_BITMAP, "Bitmap"},
	{PELIB_RT_ICON, "Icon"},
	{PELIB_RT_MENU, "Menu"},
	{PELIB_RT_DIALOG, "Dialog box"},
	{PELIB_RT_STRING, "String-table entry"},
	{PELIB_RT_FONTDIR, "Font directory"},
	{PELIB_RT_FONT, "Font"},
	{PELIB_RT_ACCELERATOR, "Accelerator table"},
	{PELIB_RT_RCDATA, "Raw data"},
	{PELIB_RT_MESSAGETABLE, "Message-table entry"},
	{PELIB_RT_GROUP_CURSOR, "Cursor Group"},
	{PELIB_RT_GROUP_ICON, "Icon Group"},
	{PELIB_RT_VERSION, "Version"},
	{PELIB_RT_DLGINCLUDE, "Include"},
	{PELIB_RT_PLUGPLAY, "Plug and Play"},
	{PELIB_RT_VXD, "VXD"},
	{PELIB_RT_ANICURSOR, "Animated cursor"},
	{PELIB_RT_ANIICON, "Animated icon"},
	{PELIB_RT_HTML, "HTML"},
	{PELIB_RT_MANIFEST, "Manifest"},
	{PELIB_RT_DLGINIT, "Dialog box init"},
	{PELIB_RT_TOOLBAR, "Toolbar"}
};

const std::map<std::size_t, std::string> resourceLanguageMap
{
	{PELIB_LANG_NEUTRAL, "Neutral"},
	{PELIB_LANG_ARABIC, "Arabic"},
	{PELIB_LANG_BULGARIAN, "Bulgarian"},
	{PELIB_LANG_CATALAN, "Catalan"},
	{PELIB_LANG_CHINESE, "Chinese"},
	{PELIB_LANG_CZECH, "Czech"},
	{PELIB_LANG_DANISH, "Danish"},
	{PELIB_LANG_GERMAN, "German"},
	{PELIB_LANG_GREEK, "Greek"},
	{PELIB_LANG_ENGLISH, "English"},
	{PELIB_LANG_SPANISH, "Spanish"},
	{PELIB_LANG_FINNISH, "Finnish"},
	{PELIB_LANG_FRENCH, "French"},
	{PELIB_LANG_HEBREW, "Hebrew"},
	{PELIB_LANG_HUNGARIAN, "Hungarian"},
	{PELIB_LANG_ICELANDIC, "Icelandic"},
	{PELIB_LANG_ITALIAN, "Italian"},
	{PELIB_LANG_JAPANESE, "Japanese"},
	{PELIB_LANG_KOREAN, "Korean"},
	{PELIB_LANG_DUTCH, "Dutch"},
	{PELIB_LANG_NORWEGIAN, "Norwegian"},
	{PELIB_LANG_POLISH, "Polish"},
	{PELIB_LANG_PORTUGUESE, "Portuguese"},
	{PELIB_LANG_ROMANIAN, "Romanian"},
	{PELIB_LANG_RUSSIAN, "Russian"},
	{PELIB_LANG_CROATIAN, "Croatian"},
	{PELIB_LANG_SERBIAN_NEUTRAL, "Serbian"},
	{PELIB_LANG_BOSNIAN_NEUTRAL, "Bosnian"},
	{PELIB_LANG_SLOVAK, "Slovak"},
	{PELIB_LANG_ALBANIAN, "Albanian"},
	{PELIB_LANG_SWEDISH, "Swedish"},
	{PELIB_LANG_THAI, "Thai"},
	{PELIB_LANG_TURKISH, "Turkish"},
	{PELIB_LANG_URDU, "Urdu"},
	{PELIB_LANG_INDONESIAN, "Indonesian"},
	{PELIB_LANG_UKRAINIAN, "Ukrainian"},
	{PELIB_LANG_BELARUSIAN, "Belarusian"},
	{PELIB_LANG_SLOVENIAN, "Slovenian"},
	{PELIB_LANG_ESTONIAN, "Estonian"},
	{PELIB_LANG_LATVIAN, "Latvian"},
	{PELIB_LANG_LITHUANIAN, "Lithuanian"},
	{PELIB_LANG_PERSIAN, "Persian"},
	{PELIB_LANG_VIETNAMESE, "Vietnamese"},
	{PELIB_LANG_ARMENIAN, "Armenian"},
	{PELIB_LANG_AZERI, "Azeri"},
	{PELIB_LANG_BASQUE, "Basque"},
	{PELIB_LANG_SORBIAN, "Sorbian"},
	{PELIB_LANG_MACEDONIAN, "Macedonian"},
	{PELIB_LANG_TSWANA, "Tswana"},
	{PELIB_LANG_XHOSA, "Xhosa"},
	{PELIB_LANG_ZULU, "Zulu"},
	{PELIB_LANG_AFRIKAANS, "Afrikaans"},
	{PELIB_LANG_GEORGIAN, "Georgian"},
	{PELIB_LANG_FAEROESE, "Faeroese"},
	{PELIB_LANG_HINDI, "Hindi"},
	{PELIB_LANG_MALTESE, "Maltese"},
	{PELIB_LANG_SAMI, "Sami"},
	{PELIB_LANG_IRISH, "Irish"},
	{PELIB_LANG_MALAY, "Malay"},
	{PELIB_LANG_KAZAK, "Kazak"},
	{PELIB_LANG_KYRGYZ, "Kyrgyz"},
	{PELIB_LANG_SWAHILI, "Swahili"},
	{PELIB_LANG_UZBEK, "Uzbek"},
	{PELIB_LANG_TATAR, "Tatar"},
	{PELIB_LANG_BENGALI, "Bengali"},
	{PELIB_LANG_PUNJABI, "Punjabi"},
	{PELIB_LANG_GUJARATI, "Gujarati"},
	{PELIB_LANG_ORIYA, "Oriya"},
	{PELIB_LANG_TAMIL, "Tamil"},
	{PELIB_LANG_TELUGU, "Telugu"},
	{PELIB_LANG_KANNADA, "Kannada"},
	{PELIB_LANG_MALAYALAM, "Malayalam"},
	{PELIB_LANG_ASSAMESE, "Assamese"},
	{PELIB_LANG_MARATHI, "Marathi"},
	{PELIB_LANG_SANSKRIT, "Sanskrit"},
	{PELIB_LANG_MONGOLIAN, "Mongolian"},
	{PELIB_LANG_TIBETAN, "Tibetan"},
	{PELIB_LANG_WELSH, "Welsh"},
	{PELIB_LANG_KHMER, "Khmer"},
	{PELIB_LANG_LAO, "Lao"},
	{PELIB_LANG_GALICIAN, "Galician"},
	{PELIB_LANG_KONKANI, "Konkani"},
	{PELIB_LANG_MANIPURI, "Manipuri"},
	{PELIB_LANG_SINDHI, "Sindhi"},
	{PELIB_LANG_SYRIAC, "Syriac"},
	{PELIB_LANG_SINHALESE, "Sinhalese"},
	{PELIB_LANG_INUKTITUT, "Inuktitut"},
	{PELIB_LANG_AMHARIC, "Amharic"},
	{PELIB_LANG_TAMAZIGHT, "Tamazight"},
	{PELIB_LANG_KASHMIRI, "Kashmiri"},
	{PELIB_LANG_NEPALI, "Nepali"},
	{PELIB_LANG_FRISIAN, "Frisian"},
	{PELIB_LANG_PASHTO, "Pashto"},
	{PELIB_LANG_FILIPINO, "Filipino"},
	{PELIB_LANG_DIVEHI, "Divehi"},
	{PELIB_LANG_HAUSA, "Hausa"},
	{PELIB_LANG_YORUBA, "Yoruba"},
	{PELIB_LANG_QUECHUA, "Quechua"},
	{PELIB_LANG_SOTHO, "Sotho"},
	{PELIB_LANG_BASHKIR, "Bashkir"},
	{PELIB_LANG_LUXEMBOURGISH, "Luxembourgish"},
	{PELIB_LANG_GREENLANDIC, "Greenlandic"},
	{PELIB_LANG_IGBO, "Igbo"},
	{PELIB_LANG_TIGRIGNA, "Tigrigna"},
	{PELIB_LANG_YI, "Yi"},
	{PELIB_LANG_MAPUDUNGUN, "Mapudungun"},
	{PELIB_LANG_MOHAWK, "Mohawk"},
	{PELIB_LANG_BRETON, "Breton"},
	{PELIB_LANG_INVARIANT, "Invariant"},
	{PELIB_LANG_UIGHUR, "Uighur"},
	{PELIB_LANG_MAORI, "Maori"},
	{PELIB_LANG_OCCITAN, "Occitan"},
	{PELIB_LANG_CORSICAN, "Corsican"},
	{PELIB_LANG_ALSATIAN, "Alsatian"},
	{PELIB_LANG_YAKUT, "Yakut"},
	{PELIB_LANG_KICHE, "Kiche"},
	{PELIB_LANG_KINYARWANDA, "Kinyarwanda"},
	{PELIB_LANG_WOLOF, "Wolof"},
	{PELIB_LANG_DARI, "Dari"},
	{PELIB_LANG_MALAGASY, "Malagasy"}
};

const std::vector<std::string> usualSectionNames
{
	".00cfg", ".apiset", ".arch", ".autoload_text", ".bindat", ".bootdat", ".bss", 
	".BSS", ".buildid", ".CLR_UEF", ".code", ".cormeta", ".complua", ".CRT",
	".cygwin_dll_common", ".data", ".DATA", ".data1", ".data2", ".data3", ".debug", 
	".debug  $F", ".debug  $P", ".debug  $S", ".debug  $T", ".drectve ", ".didat", 
	".didata", ".edata", ".eh_fram", ".export", ".fasm", ".flat", ".gfids", ".giats", 
	".gljmp", ".glue_7t", ".glue_7", ".idata", ".idlsym", ".impdata", ".import",
	".itext", ".ndata", ".orpc", ".pdata", ".rdata", ".reloc", ".rodata", ".rsrc", 
	".sbss", ".script", ".shared", ".sdata", ".srdata", ".stab", ".stabstr", ".sxdata", 
	".text", ".text0", ".text1", ".text2", ".text3", ".textbss", ".tls", ".tls$",
	".udata", ".vsdata", ".xdata", ".wixburn", ".wpp_sf ", "BSS", "CODE", "DATA", 
	"DGROUP", "edata", "idata", "INIT", "minATL", "PAGE", "rdata", "sdata", "shared", 
	"Shared", "testdata", "text"
};

const std::vector<std::string> usualPackerSections
{
	".aspack", ".adata", "ASPack", ".ASPack", ".boom", ".ccg", ".charmve", "BitArts",
	"DAStub", "!EPack", ".ecode", ".edata", ".enigma1", ".enigma2", "FSG!", ".gentee",
	"kkrunchy", "lz32.dll", ".mackt", ".MaskPE", "MEW", ".mnbvcx1", ".mnbvcx2",
	".MPRESS1", ".MPRESS2", ".neolite", ".neolit", ".nsp1", ".nsp0", ".nsp2", "nsp1",
	"nsp0", "nsp2", ".packed", "pebundle", "PEBundle", "PEC2TO", "PECompact2", "PEC2",
	"pec", "pec1", "pec2", "pec3", "pec4", "pec5", "pec6", "PEC2MO", "PELOCKnt",
	".perplex", "PESHiELD", ".petite", ".pinclie", "ProCrypt", ".RLPack", ".rmnet",
	"RCryptor", ".RPCrypt", ".seau", ".sforce3", ".shrink1", ".shrink2", ".shrink3",
	".spack", ".svkp", "Themida", ".Themida", ".taz", ".tsuarch", ".tsustub", ".packed",
	"PEPACK!!", ".Upack", ".ByDwing", "UPX0", "UPX1", "UPX2", "UPX3", "UPX!", ".UPX0",
	".UPX1", ".UPX2", ".vmp0", ".vmp1", ".vmp2", "VProtect", ".winapi", "WinLicen",
	"_winzip_", ".WWPACK", ".WWP32", ".yP", ".y0da"
};

const std::map<std::string, std::size_t> usualSectionCharacteristics
{
	{".bss", (PELIB_IMAGE_SCN_CNT_UNINITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".cormeta", PELIB_IMAGE_SCN_LNK_INFO},
	{".data", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".debug", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_DISCARDABLE)},
	{".drective", PELIB_IMAGE_SCN_LNK_INFO},
	{".edata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ)},
	{".idata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".idlsym", PELIB_IMAGE_SCN_LNK_INFO},
	{".pdata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ)},
	{".rdata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ)},
	{".reloc", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_DISCARDABLE)},
	{".rsrc", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ)},
	{".sbss", (PELIB_IMAGE_SCN_CNT_UNINITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".sdata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".srdata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ)},
	{".sxdata", PELIB_IMAGE_SCN_LNK_INFO},
	{".text", (PELIB_IMAGE_SCN_CNT_CODE | PELIB_IMAGE_SCN_MEM_EXECUTE | PELIB_IMAGE_SCN_MEM_READ)},
	{".tls", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".tls$", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".vsdata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ | PELIB_IMAGE_SCN_MEM_WRITE)},
	{".xdata", (PELIB_IMAGE_SCN_CNT_INITIALIZED_DATA | PELIB_IMAGE_SCN_MEM_READ)}
};

/**
 * Try to find offset of DOS stub
 * @param plainFile Content of input file from space after MZ header to offset
 *    of PE signature
 * @return Offset of DOS stub in @a plainFile or @c string::npos if DOS stub
 *    is not found
 */
std::size_t findDosStub(const std::string &plainFile)
{
	for(const auto &item : stubDatabase)
	{
		const auto offset = plainFile.find(item);
		if(offset != std::string::npos)
		{
			return offset;
		}
	}

	return std::string::npos;
}

/**
 * Get type of symbol
 * @param link Link to PE section
 * @param value PE symbol value
 * @param storageClass PE symbol storage class
 * @return Type of symbol
 */
Symbol::Type getSymbolType(word link, dword value, byte storageClass)
{
	if(!link)
	{
		return value ? Symbol::Type::COMMON : Symbol::Type::EXTERN;
	}
	else if(link == std::numeric_limits<word>::max() || link == std::numeric_limits<word>::max() - 1)
	{
		return Symbol::Type::ABSOLUTE_SYM;
	}
	else if(storageClass == PE_IMAGE_SYM_CLASS_EXTERNAL)
	{
		return Symbol::Type::PUBLIC;
	}
	else if(storageClass == PE_IMAGE_SYM_CLASS_STATIC)
	{
		return Symbol::Type::PRIVATE;
	}

	return Symbol::Type::UNDEFINED_SYM;
}

/**
 * Get usage type of symbol
 * @param storageClass PE symbol storage class
 * @param complexType PE symbol type
 * @return Usage type of symbol
 */
Symbol::UsageType getSymbolUsageType(byte storageClass, byte complexType)
{
	if(complexType >= 0x20 && complexType < 0x30)
	{
		return Symbol::UsageType::FUNCTION;
	}
	else if(storageClass == PE_IMAGE_SYM_CLASS_FILE)
	{
		return Symbol::UsageType::FILE;
	}

	return Symbol::UsageType::UNKNOWN;
}

} // anonymous namespace

/**
 * Constructor
 * @param pathToFile Path to input file
 * @param dllListFile Path to text file containing list of OS DLLs
 * @param loadFlags Load flags
 */
PeFormat::PeFormat(const std::string & pathToFile, const std::string & dllListFile, LoadFlags loadFlags) :
		FileFormat(pathToFile, loadFlags)
{
	initStructures(dllListFile);
}

/**
 * Constructor
 * @param inputStream Representation of input file
 * @param loadFlags Load flags
 */
PeFormat::PeFormat(std::istream &inputStream, LoadFlags loadFlags) :
		FileFormat(inputStream, loadFlags)
{
	initStructures("");
}

/**
 * Constructor
 * @param data Input data.
 * @param size Input data size.
 * @param loadFlags Load flags
 */
PeFormat::PeFormat(const std::uint8_t *data, std::size_t size, LoadFlags loadFlags) :
		FileFormat(data, size, loadFlags)
{
	initStructures("");
}

/**
 * Destructor
 */
PeFormat::~PeFormat()
{
	delete file;
	delete formatParser;
}

/**
* Init information from PE loader
*/

void PeFormat::initLoaderErrorInfo(PeLib::LoaderError ldrError)
{
	if(_ldrErrInfo.loaderErrorCode == PeLib::LDR_ERROR_NONE)
	{
		_ldrErrInfo.loaderErrorCode = static_cast<std::uint32_t>(ldrError);
		_ldrErrInfo.loaderError = getLoaderErrorString(ldrError, false);
		_ldrErrInfo.loaderErrorUserFriendly = getLoaderErrorString(ldrError, true);
	}
}

void PeFormat::initLoaderErrorInfo()
{
	initLoaderErrorInfo(file->loaderError());
}

/**
 * Init internal structures
 */
void PeFormat::initStructures(const std::string & dllListFile)
{
	formatParser = nullptr;
	peHeader32 = nullptr;
	peHeader64 = nullptr;
	peClass = PEFILE_UNKNOWN;
	errorLoadingDllList = false;

	// If we got an override list of dependency DLLs, we load them into the map
	initDllList(dllListFile);

	file = openPeFile(fileStream);
	if(file)
	{
		stateIsValid = true;
		try
		{
			file->readMzHeader();
			file->readPeHeader();
			file->readCoffSymbolTable();
			file->readImportDirectory();
			file->readIatDirectory();
			file->readBoundImportDirectory();
			file->readDelayImportDirectory();
			file->readExportDirectory();
			file->readDebugDirectory();
			file->readTlsDirectory();
			file->readResourceDirectory();
			file->readSecurityDirectory();
			file->readComHeaderDirectory();

			// Fill-in the loader error info from PE file
			initLoaderErrorInfo();

			mzHeader = file->mzHeader();
			switch((peClass = getFileType(fileStream)))
			{
				case PEFILE32:
				{
					auto *f32 = dynamic_cast<PeFileT<32>*>(file);
					if(f32)
					{
						peHeader32 = &(f32->peHeader());
						formatParser = new PeFormatParser32(this, static_cast<PeFileT<32>*>(file));
					}
					stateIsValid = f32 && peHeader32;
					break;
				}
				case PEFILE64:
				{
					auto *f64 = dynamic_cast<PeFileT<64>*>(file);
					if(f64)
					{
						peHeader64 = &(f64->peHeader());
						formatParser = new PeFormatParser64(this, static_cast<PeFileT<64>*>(file));
					}
					stateIsValid = f64 && peHeader64;
					break;
				}
				default:
				{
					stateIsValid = false;
				}
			}
		} catch(...)
		{
			stateIsValid = false;
		}
	}
	else
	{
		stateIsValid = false;
	}

	if(stateIsValid)
	{
		fileFormat = Format::PE;
		loadRichHeader();
		loadSections();
		loadSymbols();
		loadImports();
		loadExports();
		loadPdbInfo();
		loadResources();
		loadCertificates();
		loadTlsInformation();
		loadDotnetHeaders();
		loadVisualBasicHeader();
		computeSectionTableHashes();
		loadStrings();
		scanForAnomalies();
	}
}

std::size_t PeFormat::initSectionTableHashOffsets()
{
	secHashInfo.emplace_back(20, 4);
	secHashInfo.emplace_back(16, 4);
	secHashInfo.emplace_back(36, 4);
	return secHashInfo.size();
}

/**
 * Calculate offset of rich header
 * @param plainFile Content of input file from space after MZ header to offset
 *    of PE signature
 *
 * Method returns default value (0x80) if detection of offset fails or rich
 * header is not present in input file.
 */
std::size_t PeFormat::getRichHeaderOffset(const std::string &plainFile)
{
	std::size_t richOffset = 0, prev = findDosStub(plainFile);

	if(prev != std::string::npos)
	{
		for(std::size_t i = 0, next = 0; (next = plainFile.find('\0', prev)) != std::string::npos; ++i)
		{
			if(i)
			{
				if(next != prev)
				{
					break;
				}
				richOffset = ++prev;
			}
			else
			{
				richOffset = prev = ++next;
			}
		}
	}

	return richOffset ? richOffset + getMzHeaderSize() : STANDARD_RICH_HEADER_OFFSET;
}

/**
 * Get nodes of resource tree except root
 * @param nodes Into this parameter nodes are stored (except root node)
 * @param levels Into this parameter is stored number of nodes in each level
 *    of tree (except root level)
 * @return @c true if nodes was successfully loaded, @c false otherwise
 */
bool PeFormat::getResourceNodes(std::vector<const PeLib::ResourceChild*> &nodes, std::vector<std::size_t> &levels)
{
	nodes.clear();
	levels.clear();
	auto root = formatParser->getResourceTreeRoot();
	if(!root || !root->getNumberOfChildren())
	{
		return false;
	}
	resourceTree = new ResourceTree();
	resourceTree->addNode(0, root->getNumberOfChildren());
	levels.push_back(root->getNumberOfChildren());

	for(std::size_t i = 0, e = root->getNumberOfChildren(); i < e; ++i)
	{
		nodes.push_back(root->getChild(i));
	}

	for(std::size_t i = 0, e = nodes.size(); i < e; ++i)
	{
		auto *actual = nodes[i];
		if(actual)
		{
			resourceTree->addNode(levels.size(), actual->getNumberOfChildren());

			for(std::size_t j = 0, f = actual->getNumberOfChildren(); j < f; ++j)
			{
				nodes.push_back(actual->getChildOfThisChild(j));
			}
		}

		// end of actual level
		if(i + 1 == e && nodes.size() > e)
		{
			levels.push_back(nodes.size() - e);
			e = nodes.size();
		}
	}

	if(!resourceTree->isValidTree())
	{
		assert(false && "Incorrect structure of resources");
	}

	return true;
}

/**
 * Load Rich header
 */
void PeFormat::loadRichHeader()
{
	if(getPeHeaderOffset() <= getMzHeaderSize())
	{
		return;
	}

	std::string plainText;
	bytesToString(bytes, plainText, getMzHeaderSize(), getPeHeaderOffset() - getMzHeaderSize());
	auto offset = getRichHeaderOffset(plainText);
	auto standardOffset = (offset == STANDARD_RICH_HEADER_OFFSET);
	if(offset >= getPeHeaderOffset())
	{
		return;
	}

	file->readRichHeader(offset, getPeHeaderOffset() - offset);
	auto &header = file->richHeader();
	std::vector<std::size_t> validStructOffsets;
	if(header.isStructureValid())
	{
		validStructOffsets.push_back(offset);
	}
	// try space immediately after DOS (MZ) header
	if(!header.isHeaderValid() && offset > getMzHeaderSize() && getPeHeaderOffset() > getMzHeaderSize())
	{
		offset = getMzHeaderSize();
		standardOffset |= (offset == STANDARD_RICH_HEADER_OFFSET);
		file->readRichHeader(offset, getPeHeaderOffset() - offset);
		if(header.isStructureValid())
		{
			validStructOffsets.push_back(offset);
		}
	}
	// try standard offset of rich header
	if(!header.isHeaderValid() && !standardOffset && STANDARD_RICH_HEADER_OFFSET < getPeHeaderOffset())
	{
		offset = STANDARD_RICH_HEADER_OFFSET;
		file->readRichHeader(offset, getPeHeaderOffset() - offset);
		if(header.isStructureValid())
		{
			validStructOffsets.push_back(offset);
		}
	}
	if(!header.isHeaderValid() && validStructOffsets.empty())
	{
		return;
	}

	std::string signature;
	richHeader = new RichHeader();
	richHeader->setOffset(offset);
	richHeader->setSuspicious(header.getNumberOfIterations() > 1);
	richHeader->setValidStructure(true);
	if(!header.isHeaderValid())
	{
		const auto nonStandardOffset = std::any_of(validStructOffsets.begin(), validStructOffsets.end(),
			[&] (const auto &off)
			{
				return off != STANDARD_RICH_HEADER_OFFSET && off != this->getMzHeaderSize();
			});
		std::size_t maxOffset = 0;

		for(const auto off : validStructOffsets)
		{
			if(off > maxOffset && (!nonStandardOffset || (off != STANDARD_RICH_HEADER_OFFSET && off != getMzHeaderSize())))
			{
				maxOffset = off;
			}
		}

		file->readRichHeader(maxOffset, getPeHeaderOffset() - maxOffset, true);
		richHeader->setOffset(maxOffset);
		richHeader->setSuspicious(header.getNumberOfIterations() > 1);
		signature = header.getDecryptedHeaderItemsSignature({0, 1, 2, 3});
	}

	for(const auto &item : header)
	{
		LinkerInfo info;
		info.setMajorVersion(item.MajorVersion);
		info.setMinorVersion(item.MinorVersion);
		info.setBuildVersion(item.Build);
		info.setNumberOfUses(item.Count);
		signature += item.Signature;
		richHeader->addRecord(info);
	}

	richHeader->setKey(header.getKey());
	richHeader->setSignature(signature);
	richHeader->setBytes(header.getDecryptedHeaderBytes());
}

/**
 * Load visual basic header
 */
void PeFormat::loadVisualBasicHeader()
{
	const auto &allBytes = getBytes();
	std::vector<std::uint8_t> bytes;
	unsigned long long version = 0;
	unsigned long long vbHeaderAddress = 0;
	unsigned long long vbHeaderOffset = 0;
	unsigned long long vbProjectInfoOffset = 0;
	unsigned long long vbComDataRegistrationOffset = 0;
	std::string projLanguageDLL;
	std::string projBackupLanguageDLL;
	std::string projExeName;
	std::string projDesc;
	std::string helpFile;
	std::string projName;
	std::size_t offset = 0;
	struct VBHeader vbh;

	if (!isVisualBasic(version))
	{
		return;
	}

	// first instruction is expected to be PUSH <vbHeaderAddress> (0x68 <b0> <b1> <b2> <b3>)
	if (!getEpBytes(bytes, 5) || bytes.size() != 5 || bytes[0] != 0x68)
	{
		return;
	}

	vbHeaderAddress = bytes[4] << 24 | bytes[3] << 16 | bytes[2] << 8 | bytes[1];
	if (!getOffsetFromAddress(vbHeaderOffset, vbHeaderAddress))
	{
		return;
	}

	if (!getBytes(bytes, vbHeaderOffset, vbh.structureSize()) || bytes.size() != vbh.structureSize())
	{
		return;
	}

	DynamicBuffer structContent(bytes, retdec::utils::Endianness::LITTLE);
	vbh.signature = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.signature);
	vbh.runtimeBuild = structContent.read<std::uint16_t>(offset); offset += sizeof(vbh.runtimeBuild);
	std::memcpy(&vbh.languageDLL, static_cast<void *>(&bytes.data()[offset]), sizeof(vbh.languageDLL)); offset += sizeof(vbh.languageDLL);
	std::memcpy(&vbh.backupLanguageDLL, static_cast<void *>(&bytes.data()[offset]), sizeof(vbh.backupLanguageDLL)); offset += sizeof(vbh.backupLanguageDLL);
	vbh.runtimeDLLVersion = structContent.read<std::uint16_t>(offset); offset += sizeof(vbh.runtimeDLLVersion);
	vbh.LCID1 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.LCID1);
	vbh.LCID2 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.LCID2);
	vbh.subMainAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.subMainAddr);
	vbh.projectInfoAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.projectInfoAddr);
	vbh.MDLIntObjsFlags = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.MDLIntObjsFlags);
	vbh.MDLIntObjsFlags2 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.MDLIntObjsFlags2);
	vbh.threadFlags = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.threadFlags);
	vbh.nThreads = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.nThreads);
	vbh.nForms = structContent.read<std::uint16_t>(offset); offset += sizeof(vbh.nForms);
	vbh.nExternals = structContent.read<std::uint16_t>(offset); offset += sizeof(vbh.nExternals);
	vbh.nThunks = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.nThunks);
	vbh.GUITableAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.GUITableAddr);
	vbh.externalTableAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.externalTableAddr);
	vbh.COMRegisterDataAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.COMRegisterDataAddr);
	vbh.projExeNameOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.projExeNameOffset);
	vbh.projDescOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.projDescOffset);
	vbh.helpFileOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.helpFileOffset);
	vbh.projNameOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbh.projNameOffset);

	if (vbh.signature != VBHEADER_SIGNATURE)
	{
		return;
	}

	if (vbh.projExeNameOffset != 0)
	{
		projExeName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
									vbHeaderOffset + vbh.projExeNameOffset, VB_MAX_STRING_LEN, true);
		visualBasicInfo.setProjectExeName(projExeName);
	}
	if (vbh.projDescOffset != 0)
	{
		projDesc = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
									vbHeaderOffset + vbh.projDescOffset, VB_MAX_STRING_LEN, true);
		visualBasicInfo.setProjectDescription(projDesc);
	}
	if (vbh.helpFileOffset != 0)
	{
		helpFile = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
									vbHeaderOffset + vbh.helpFileOffset, VB_MAX_STRING_LEN, true);
		visualBasicInfo.setProjectHelpFile(helpFile);
	}
	if (vbh.projNameOffset != 0)
	{
		projName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
									vbHeaderOffset + vbh.projNameOffset, VB_MAX_STRING_LEN, true);
		visualBasicInfo.setProjectName(projName);
	}

	for (size_t i = 0; i < sizeof(vbh.languageDLL) && vbh.languageDLL[i]; i++)
	{
		projLanguageDLL.push_back(vbh.languageDLL[i]);
	}
	for (size_t i = 0; i < sizeof(vbh.backupLanguageDLL) && vbh.backupLanguageDLL[i]; i++)
	{
		projBackupLanguageDLL.push_back(vbh.backupLanguageDLL[i]);
	}
	visualBasicInfo.setLanguageDLL(projLanguageDLL);
	visualBasicInfo.setBackupLanguageDLL(projBackupLanguageDLL);
	visualBasicInfo.setLanguageDLLPrimaryLCID(vbh.LCID1);
	visualBasicInfo.setLanguageDLLSecondaryLCID(vbh.LCID2);

	if (getOffsetFromAddress(vbProjectInfoOffset, vbh.projectInfoAddr))
	{
		parseVisualBasicProjectInfo(vbProjectInfoOffset);
	}

	if (getOffsetFromAddress(vbComDataRegistrationOffset, vbh.COMRegisterDataAddr))
	{
		parseVisualBasicComRegistrationData(vbComDataRegistrationOffset);
	}
}

/**
 * Parse visual basic COM registration data
 * @param structureOffset Offset in file where the structure starts
 * @return @c true if COM registration data was successfuly parsed, @c false otherwise
 */
bool PeFormat::parseVisualBasicComRegistrationData(std::size_t structureOffset)
{
	const auto &allBytes = getBytes();
	std::vector<std::uint8_t> bytes;
	std::size_t offset = 0;
	struct VBCOMRData vbcrd;
	std::string projName;
	std::string helpFile;
	std::string projDesc;

	if (!getBytes(bytes, structureOffset, vbcrd.structureSize()) || bytes.size() != vbcrd.structureSize())
	{
		return false;
	}

	DynamicBuffer structContent(bytes, retdec::utils::Endianness::LITTLE);
	vbcrd.regInfoOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.regInfoOffset);
	vbcrd.projNameOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.projNameOffset);
	vbcrd.helpFileOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.helpFileOffset);
	vbcrd.projDescOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.projDescOffset);
	std::memcpy(&vbcrd.projCLSID, static_cast<void *>(&bytes.data()[offset]), sizeof(vbcrd.projCLSID)); offset += sizeof(vbcrd.projCLSID);
	vbcrd.projTlbLCID = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.projTlbLCID);
	vbcrd.unknown = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.unknown);
	vbcrd.tlbVerMajor = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.tlbVerMajor);
	vbcrd.tlbVerMinor = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcrd.tlbVerMinor);

	visualBasicInfo.setTypeLibLCID(vbcrd.projTlbLCID);
	visualBasicInfo.setTypeLibMajorVersion(vbcrd.tlbVerMajor);
	visualBasicInfo.setTypeLibMinorVersion(vbcrd.tlbVerMinor);

	if (!visualBasicInfo.hasProjectName() && vbcrd.projNameOffset != 0)
	{
		projName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
							structureOffset + vbcrd.projNameOffset, VB_MAX_STRING_LEN, true);
	}
	if (!visualBasicInfo.hasProjectHelpFile() && vbcrd.helpFileOffset != 0)
	{
		helpFile = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
							structureOffset + vbcrd.helpFileOffset, VB_MAX_STRING_LEN, true);
	}
	if (!visualBasicInfo.hasProjectDescription() && vbcrd.projDescOffset != 0)
	{
		projDesc = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
							structureOffset + vbcrd.projDescOffset, VB_MAX_STRING_LEN, true);
	}

	visualBasicInfo.setTypeLibCLSID(vbcrd.projCLSID);

	if (vbcrd.regInfoOffset != 0)
	{
		parseVisualBasicComRegistrationInfo(structureOffset + vbcrd.regInfoOffset, structureOffset);
	}

	return true;
}

/**
 * Parse visual basic COM registration info
 * @param structureOffset Offset in file where the structure starts
 * @param comRegDataOffset Offset in file where the com registration data structure starts
 * @return @c true if COM registration info was successfuly parsed, @c false otherwise
 */
bool PeFormat::parseVisualBasicComRegistrationInfo(std::size_t structureOffset,
													std::size_t comRegDataOffset)
{
	const auto &allBytes = getBytes();
	std::vector<std::uint8_t> bytes;
	std::size_t offset = 0;
	struct VBCOMRInfo vbcri;
	std::string COMObjectName;
	std::string COMObjectDesc;

	if (!getBytes(bytes, structureOffset, vbcri.structureSize()) || bytes.size() != vbcri.structureSize())
	{
		return false;
	}

	DynamicBuffer structContent(bytes, retdec::utils::Endianness::LITTLE);
	vbcri.ifInfoOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.ifInfoOffset);
	vbcri.objNameOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.objNameOffset);
	vbcri.objDescOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.objDescOffset);
	vbcri.instancing = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.instancing);
	vbcri.objID = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.objID);
	std::memcpy(&vbcri.objCLSID, static_cast<void *>(&bytes.data()[offset]), sizeof(vbcri.objCLSID)); offset += sizeof(vbcri.objCLSID);
	vbcri.isInterfaceFlag = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.isInterfaceFlag);
	vbcri.ifCLSIDOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.ifCLSIDOffset);
	vbcri.eventCLSIDOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.eventCLSIDOffset);
	vbcri.hasEvents = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.hasEvents);
	vbcri.olemicsFlags = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.olemicsFlags);
	vbcri.classType = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.classType);
	vbcri.objectType = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.objectType);
	vbcri.toolboxBitmap32 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.toolboxBitmap32);
	vbcri.defaultIcon = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.defaultIcon);
	vbcri.isDesignerFlag = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.isDesignerFlag);
	vbcri.designerDataOffset = structContent.read<std::uint32_t>(offset); offset += sizeof(vbcri.designerDataOffset);

	if (vbcri.objNameOffset != 0)
	{
		COMObjectName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
										comRegDataOffset + vbcri.objNameOffset, VB_MAX_STRING_LEN, true);
		visualBasicInfo.setCOMObjectName(COMObjectName);
	}
	if (vbcri.objDescOffset != 0)
	{
		COMObjectDesc = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
										comRegDataOffset + vbcri.objDescOffset, VB_MAX_STRING_LEN, true);
		visualBasicInfo.setCOMObjectDescription(COMObjectDesc);
	}

	visualBasicInfo.setCOMObjectCLSID(vbcri.objCLSID);
	visualBasicInfo.setCOMObjectType(vbcri.objectType);

	if (vbcri.isInterfaceFlag != 0 && vbcri.ifCLSIDOffset != 0 &&
		getBytes(bytes, comRegDataOffset + vbcri.ifCLSIDOffset, 16) && bytes.size() == 16)
	{
		visualBasicInfo.setCOMObjectInterfaceCLSID(bytes.data());
	}

	if (vbcri.hasEvents != 0 && vbcri.eventCLSIDOffset != 0 &&
		getBytes(bytes, comRegDataOffset + vbcri.eventCLSIDOffset, 16) && bytes.size() == 16)
	{
		visualBasicInfo.setCOMObjectEventsCLSID(bytes.data());
	}

	return true;
}

/**
 * Parse visual basic project info
 * @param structureOffset Offset in file where the structure starts
 * @return @c true if project info was successfuly parsed, @c false otherwise
 */
bool PeFormat::parseVisualBasicProjectInfo(std::size_t structureOffset)
{
	std::vector<std::uint8_t> bytes;
	unsigned long long vbExternTableOffset = 0;
	unsigned long long vbObjectTableOffset = 0;
	std::string projPath;
	std::size_t offset = 0;
	struct VBProjInfo vbpi;

	if (!getBytes(bytes, structureOffset, vbpi.structureSize()) || bytes.size() != vbpi.structureSize())
	{
		return false;
	}

	DynamicBuffer structContent(bytes, retdec::utils::Endianness::LITTLE);
	vbpi.version = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.version);
	vbpi.objectTableAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.objectTableAddr);
	vbpi.null = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.null);
	vbpi.codeStartAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.codeStartAddr);
	vbpi.codeEndAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.codeEndAddr);
	vbpi.dataSize = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.dataSize);
	vbpi.threadSpaceAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.threadSpaceAddr);
	vbpi.exHandlerAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.exHandlerAddr);
	vbpi.nativeCodeAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.nativeCodeAddr);
	std::memcpy(&vbpi.pathInformation, static_cast<void *>(&bytes.data()[offset]), sizeof(vbpi.pathInformation)); offset += sizeof(vbpi.pathInformation);
	vbpi.externalTableAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.externalTableAddr);
	vbpi.nExternals = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpi.nExternals);

	projPath = retdec::utils::unicodeToAscii(vbpi.pathInformation, sizeof(vbpi.pathInformation));
	visualBasicInfo.setProjectPath(projPath);
	visualBasicInfo.setPcode(vbpi.nativeCodeAddr == 0);

	if (getOffsetFromAddress(vbExternTableOffset, vbpi.externalTableAddr))
	{
		parseVisualBasicExternTable(vbExternTableOffset, vbpi.nExternals);
	}

	if (getOffsetFromAddress(vbObjectTableOffset, vbpi.objectTableAddr))
	{
		parseVisualBasicObjectTable(vbObjectTableOffset);
	}

	return true;
}

/**
 * Parse visual basic extern table
 * @param structureOffset Offset in file where the structure starts
 * @param nEntries Number of entries in table
 * @return @c true if extern table was successfuly parsed, @c false otherwise
 */
bool PeFormat::parseVisualBasicExternTable(std::size_t structureOffset, std::size_t nEntries)
{
	const auto &allBytes = getBytes();
	std::vector<std::uint8_t> bytes;
	struct VBExternTableEntry entry;
	struct VBExternTableEntryData entryData;
	unsigned long long vbExternEntryDataOffset = 0;
	std::size_t offset = 0;

	for (std::size_t i = 0; i < nEntries; i++)
	{
		std::string moduleName;
		std::string apiName;

		if (!getBytes(bytes, structureOffset + i * entry.structureSize(), entry.structureSize())
			|| bytes.size() != entry.structureSize())
		{
			break;
		}

		offset = 0;
		DynamicBuffer entryContent(bytes, retdec::utils::Endianness::LITTLE);
		entry.type = entryContent.read<std::uint32_t>(offset); offset += sizeof(entry.type);
		entry.importDataAddr = entryContent.read<std::uint32_t>(offset); offset += sizeof(entry.importDataAddr);

		if (entry.type != static_cast<std::uint32_t>(VBExternTableEntryType::external))
		{
			continue;
		}

		if (!getOffsetFromAddress(vbExternEntryDataOffset, entry.importDataAddr))
		{
			continue;
		}

		if (!getBytes(bytes, vbExternEntryDataOffset, entryData.structureSize())
			|| bytes.size() != entryData.structureSize())
		{
			continue;
		}

		offset = 0;
		DynamicBuffer entryDataContent(bytes, retdec::utils::Endianness::LITTLE);
		entryData.moduleNameAddr = entryDataContent.read<std::uint32_t>(offset); offset += sizeof(entryData.moduleNameAddr);
		entryData.apiNameAddr = entryDataContent.read<std::uint32_t>(offset); offset += sizeof(entryData.apiNameAddr);

		unsigned long long moduleNameOffset;
		if (getOffsetFromAddress(moduleNameOffset, entryData.moduleNameAddr))
		{
			moduleName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
														moduleNameOffset, VB_MAX_STRING_LEN, true);
		}

		unsigned long long apiNameOffset;
		if (getOffsetFromAddress(apiNameOffset, entryData.apiNameAddr))
		{
			apiName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(),
														apiNameOffset, VB_MAX_STRING_LEN, true);
		}

		if (!moduleName.empty() || !apiName.empty())
		{
			auto ext = std::make_unique<VisualBasicExtern>();
			ext->setModuleName(moduleName);
			ext->setApiName(apiName);
			visualBasicInfo.addExtern(std::move(ext));
		}
	}

	visualBasicInfo.computeExternTableHashes();

	return true;
}

/**
 * Parse visual basic object table
 * @param structureOffset Offset in file where the structure starts
 * @return @c true if object table was successfuly parsed, @c false otherwise
 */
bool PeFormat::parseVisualBasicObjectTable(std::size_t structureOffset)
{
	const auto &allBytes = getBytes();
	std::vector<std::uint8_t> bytes;
	std::size_t offset = 0;
	unsigned long long projectNameOffset = 0;
	unsigned long long objectDescriptorsOffset = 0;
	struct VBObjectTable vbot;
	std::string projName;

	if (!getBytes(bytes, structureOffset, vbot.structureSize()) || bytes.size() != vbot.structureSize())
	{
		return false;
	}

	DynamicBuffer structContent(bytes, retdec::utils::Endianness::LITTLE);
	vbot.null1 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.null1);
	vbot.execCOMAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.execCOMAddr);
	vbot.projecInfo2Addr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.projecInfo2Addr);
	vbot.reserved = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.reserved);
	vbot.null2 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.null2);
	vbot.projectObjectAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.projectObjectAddr);
	std::memcpy(&vbot.objectGUID, static_cast<void *>(&bytes.data()[offset]), sizeof(vbot.objectGUID)); offset += sizeof(vbot.objectGUID);
	vbot.flagsCompileState = structContent.read<std::uint16_t>(offset); offset += sizeof(vbot.flagsCompileState);
	vbot.nObjects = structContent.read<std::uint16_t>(offset); offset += sizeof(vbot.nObjects);
	vbot.nCompiledObjects = structContent.read<std::uint16_t>(offset); offset += sizeof(vbot.nCompiledObjects);
	vbot.nUsedObjects = structContent.read<std::uint16_t>(offset); offset += sizeof(vbot.nUsedObjects);
	vbot.objectDescriptorsAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.objectDescriptorsAddr);
	vbot.IDE1 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.IDE1);
	vbot.IDE2 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.IDE2);
	vbot.IDE3 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.IDE3);
	vbot.projectNameAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.projectNameAddr);
	vbot.LCID1 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.LCID1);
	vbot.LCID2 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.LCID2);
	vbot.IDE4 = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.IDE4);
	vbot.templateVesion = structContent.read<std::uint32_t>(offset); offset += sizeof(vbot.templateVesion);

	visualBasicInfo.setProjectPrimaryLCID(vbot.LCID1);
	visualBasicInfo.setProjectSecondaryLCID(vbot.LCID2);
	visualBasicInfo.setObjectTableGUID(vbot.objectGUID);

	if (!visualBasicInfo.hasProjectName() && getOffsetFromAddress(projectNameOffset, vbot.projectNameAddr))
	{
		projName = retdec::utils::readNullTerminatedAscii(allBytes.data(), allBytes.size(), projectNameOffset,
														VB_MAX_STRING_LEN, true);
		visualBasicInfo.setProjectName(projName);
	}

	if (getOffsetFromAddress(objectDescriptorsOffset, vbot.objectDescriptorsAddr))
	{
		parseVisualBasicObjects(objectDescriptorsOffset, vbot.nObjects);
	}

	visualBasicInfo.computeObjectTableHashes();
	return true;
}

/**
 * Parse visual basic objects
 * @param structureOffset Offset in file where the public object descriptors array starts
 * @param nObjects Number of objects in array
 * @return @c true if objects were successfuly parsed, @c false otherwise
 */
bool PeFormat::parseVisualBasicObjects(std::size_t structureOffset, std::size_t nObjects)
{
	const auto &allBytes = getBytes();
	std::vector<std::uint8_t> bytes;
	struct VBPublicObjectDescriptor vbpod;
	std::size_t offset = 0;

	for (std::size_t i = 0; i < nObjects; i++)
	{
		std::unique_ptr<VisualBasicObject> object;
		if (!getBytes(bytes, structureOffset + i * vbpod.structureSize(), vbpod.structureSize())
			|| bytes.size() != vbpod.structureSize())
		{
			break;
		}

		offset = 0;
		DynamicBuffer structContent(bytes, retdec::utils::Endianness::LITTLE);
		vbpod.objectInfoAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.objectInfoAddr);
		vbpod.reserved = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.reserved);
		vbpod.publicBytesAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.publicBytesAddr);
		vbpod.staticBytesAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.staticBytesAddr);
		vbpod.modulePublicAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.modulePublicAddr);
		vbpod.moduleStaticAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.moduleStaticAddr);
		vbpod.objectNameAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.objectNameAddr);
		vbpod.nMethods = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.nMethods);
		vbpod.methodNamesAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.methodNamesAddr);
		vbpod.staticVarsCopyAddr = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.staticVarsCopyAddr);
		vbpod.objectType = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.objectType);
		vbpod.null = structContent.read<std::uint32_t>(offset); offset += sizeof(vbpod.null);

		unsigned long long objectNameOffset;
		if (!getOffsetFromAddress(objectNameOffset, vbpod.objectNameAddr))
		{
			continue;
		}

		std::string objectName = readNullTerminatedAscii(allBytes.data(), allBytes.size(), objectNameOffset,
														VB_MAX_STRING_LEN, true);
		object = std::make_unique<VisualBasicObject>();
		object->setName(objectName);

		unsigned long long methodAddrOffset;
		if (getOffsetFromAddress(methodAddrOffset, vbpod.methodNamesAddr))
		{
			for (std::size_t mIdx = 0; mIdx < vbpod.nMethods; mIdx++)
			{
				if (!getBytes(bytes, methodAddrOffset + mIdx * sizeof(std::uint32_t), sizeof(std::uint32_t))
					|| bytes.size() != sizeof(std::uint32_t))
				{
					break;
				}

				auto methodNameAddr = *reinterpret_cast<std::uint32_t *>(bytes.data());

				if (!isLittleEndian())
				{
					methodNameAddr = byteSwap32(methodNameAddr);
				}

				unsigned long long methodNameOffset;
				if (!getOffsetFromAddress(methodNameOffset, methodNameAddr))
				{
					continue;
				}

				std::string methodName = readNullTerminatedAscii(allBytes.data(), allBytes.size(),
															methodNameOffset, VB_MAX_STRING_LEN, true);

				if (!methodName.empty())
				{
					object->addMethod(methodName);
				}
			}
		}

		if (!objectName.empty() || object->getNumberOfMethods() > 0)
		{
			visualBasicInfo.addObject(std::move(object));
		}
	}

	return true;
}

/**
 * Load information about sections
 */
void PeFormat::loadSections()
{
	for(std::size_t i = 0, e = formatParser->getStoredNumberOfSections(); i < e; ++i)
	{
		auto *section = new PeCoffSection();
		if(!formatParser->getSection(i, *section))
		{
			delete section;
			continue;
		}
		section->computeEntropy();
		sections.push_back(section);
	}
}

/**
 * Load information about symbols
 *
 * Instance method @a loadSections() must be invoked before invocation of this method
 */
void PeFormat::loadSymbols()
{
	const auto symTab = file->coffSymTab();
	auto *table = new SymbolTable();

	for(std::size_t i = 0, e = symTab.getNumberOfStoredSymbols(); i < e; ++i)
	{
		auto symbol = std::make_shared<Symbol>();
		const word link = symTab.getSymbolSectionNumber(i);
		if(!link || link == std::numeric_limits<word>::max() || link == std::numeric_limits<word>::max() - 1)
		{
			symbol->invalidateLinkToSection();
			symbol->invalidateAddress();
		}
		else
		{
			symbol->setLinkToSection(link - 1);
			if(link <= getNumberOfSections() && sections[link - 1])
			{
				const auto a = sections[link - 1]->getAddress() + symTab.getSymbolValue(i);
				symbol->setAddress(a);
				symbol->setIsThumbSymbol(isArm() && a % 2);
			}
			else
			{
				symbol->invalidateAddress();
			}
		}
		symbol->setOriginalName(symTab.getSymbolName(i));
		symbol->setName(symTab.getSymbolName(i));
		symbol->setIndex(symTab.getSymbolIndex(i));
		symbol->setType(getSymbolType(link, symTab.getSymbolValue(i), symTab.getSymbolStorageClass(i)));
		symbol->setUsageType(getSymbolUsageType(symTab.getSymbolStorageClass(i), symTab.getSymbolTypeComplex(i)));
		table->addSymbol(symbol);
	}

	if(table->hasSymbols())
	{
		symbolTables.push_back(table);
	}
	else
	{
		delete table;
	}
}

/**
 * Load information about imports
 */
void PeFormat::loadImports()
{
	std::string libname;
	bool missingDependency;

	// Make sure we have import table initialized on the beginning
	if(importTable == nullptr)
		importTable = new ImportTable();

	for(std::size_t i = 0; formatParser->getImportedLibraryFileName(i, libname); ++i)
	{
		// Check whether the name of the DLL is available
		missingDependency = isMissingDependency(libname);

		importTable->addLibrary(libname, missingDependency);

		std::size_t index = 0;
		while (auto import = formatParser->getImport(i, index))
		{
			importTable->addImport(std::move(import));
			index++;
		}
	}

	for(std::size_t i = 0; formatParser->getDelayImportedLibraryFileName(i, libname); ++i)
	{
		importTable->addLibrary(libname);

		std::size_t index = 0;
		while (auto import = formatParser->getDelayImport(i, index))
		{
			import->setLibraryIndex(importTable->getNumberOfLibraries() - 1);
			importTable->addImport(std::move(import));
			index++;
		}
	}

	loadImpHash();

	for(auto&& addressRange : formatParser->getImportDirectoryOccupiedAddresses())
	{
		nonDecodableRanges.addRange(std::move(addressRange));
	}
}

/**
 * Load informations about exports
 */
void PeFormat::loadExports()
{
	Export newExport;
	exportTable = new ExportTable();

	for(std::size_t i = 0, e = formatParser->getNumberOfExportedFunctions(); i < e; ++i)
	{
		if (!formatParser->getExportedFunction(i, newExport))
			break;

		if(hasNonprintableChars(newExport.getName()))
		{
			newExport.setName("exported_function_" + numToStr(newExport.getAddress(), std::hex));
		}
		exportTable->addExport(newExport);
	}

	loadExpHash();

	for(auto&& addressRange : formatParser->getExportDirectoryOccupiedAddresses())
	{
		nonDecodableRanges.addRange(std::move(addressRange));
	}
}

/**
 * Load information about related PDB file
 */
void PeFormat::loadPdbInfo()
{
	for(std::size_t i = 0, e = formatParser->getNumberOfDebugEntries(); i < e; ++i)
	{
		std::vector<std::uint8_t> data;
		if (!formatParser->getDebugEntryData(i, data))
		{
			continue;
		}

		std::string dataString;
		bytesToString(data, dataString);
		const auto size = std::min(data.size(), dataString.length());
		if(size < 4)
		{
			continue;
		}
		const auto prefix = dataString.substr(0, 4);
		if((prefix != "RSDS" && prefix != "NB10") || (prefix == "RSDS" && size < MINIMAL_PDB_RSDS_INFO_LENGTH) ||
			(prefix == "NB10" && size < MINIMAL_PDB_NB10_INFO_LENGTH))
		{
			continue;
		}

		unsigned long long timestamp;
		if (!formatParser->getDebugEntryTimeDateStamp(i, timestamp))
		{
			continue;
		}

		unsigned long long pointerToRawData;
		if (!formatParser->getDebugEntryPointerToRawData(i, pointerToRawData))
		{
			continue;
		}

		const auto isRsds = (prefix == "RSDS");
		pdbInfo = new PdbInfo();
		pdbInfo->setType(prefix);
		pdbInfo->setTimeStamp(timestamp);
		const auto guidOffset = pointerToRawData + prefix.length() + (isRsds ? 0 : 4);
		std::uint64_t res1;
		if(isRsds)
		{
			std::uint64_t res2, res3, res4, res5;
			if(get4ByteOffset(guidOffset, res1) && get2ByteOffset(guidOffset + 4, res2) &&
				get2ByteOffset(guidOffset + 6, res3) && get2ByteOffset(guidOffset + 8, res4, getInverseEndianness()) &&
				getXByteOffset(guidOffset + 10, 6, res5, getInverseEndianness()))
			{
				pdbInfo->setGuid(toUpper(numToStr(res1, std::hex) + "-" + numToStr(res2, std::hex) + "-" +
					numToStr(res3, std::hex) + "-" + numToStr(res4, std::hex) + "-" + numToStr(res5, std::hex)));
			}
		}
		else if(get4ByteOffset(guidOffset, res1))
		{
			pdbInfo->setGuid(toUpper(numToStr(res1, std::hex)));
		}

		const auto ageOffset = guidOffset + (isRsds ? 16 : 4);
		if(get4ByteOffset(ageOffset, res1))
		{
			pdbInfo->setAge(res1);
		}
		if(getNTBSOffset(ageOffset + 4, dataString))
		{
			pdbInfo->setPath(dataString);
		}
		break;
	}

	for (auto&& addressRange : formatParser->getDebugDirectoryOccupiedAddresses())
	{
		nonDecodableRanges.addRange(std::move(addressRange));
	}
}

/**
 * Load only resource nodes
 * @param nodes Nodes of tree (except root node)
 * @param levels Number of nodes in each level of tree (except root level)
 */
void PeFormat::loadResourceNodes(std::vector<const PeLib::ResourceChild*> &nodes, const std::vector<std::size_t> &levels)
{
	unsigned long long rva = 0, size = 0;
	if(levels.empty() || !getDataDirectoryRelative(PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE, rva, size))
	{
		return;
	}

	resourceTable = new ResourceTable();
	std::size_t firstLeafIndex = 0;

	for(std::size_t i = 0, e = levels.size() - 1; i < e; ++i)
	{
		firstLeafIndex += levels[i];
	}

	for(std::size_t i = 0, e = resourceTree->getNumberOfLeafs(); i < e; ++i)
	{
		auto *leafChild = nodes[firstLeafIndex + i];
		if(!leafChild)
		{
			continue;
		}
		auto *leafChildNode = leafChild->getNode();
		auto *leaf = dynamic_cast<const ResourceLeaf*>(leafChildNode);
		if(!leafChildNode || !leafChildNode->isLeaf() || !leaf)
		{
			continue;
		}
		auto resource = std::make_unique<Resource>();
		resource->setOffset(leaf->getOffsetToData() - rva + formatParser->getResourceDirectoryOffset());
		resource->setSizeInFile(leaf->getSize());
		resource->load(this);
		resourceTable->addResource(std::move(resource));
	}
}

/**
 * Load resources
 */
void PeFormat::loadResources()
{
	size_t iconGroupIDcounter = 0;
	unsigned long long rva = 0, size = 0, imageBase = 0;
	if(!getDataDirectoryRelative(PELIB_IMAGE_DIRECTORY_ENTRY_RESOURCE, rva, size))
	{
		return;
	}

	if(!getImageBaseAddress(imageBase))
	{
		return;
	}

	std::vector<const ResourceChild*> nodes;
	std::vector<std::size_t> levels;
	if(!getResourceNodes(nodes, levels))
	{
		return;
	}
	else if(resourceTree->getNumberOfLevelsWithoutRoot() != 3)
	{
		loadResourceNodes(nodes, levels);
		return;
	}

	std::unique_ptr<Resource> resource;
	resourceTable = new ResourceTable();

	for(std::size_t i = 0, e = levels[0], nSft = 0, lSft = 0; i < e; ++i)
	{
		auto *typeChild = nodes[i];
		if(!typeChild)
		{
			continue;
		}

		bool emptyType = false;
		auto type = typeChild->getName();
		if(type.empty())
		{
			type = mapGetValueOrDefault(resourceTypeMap, typeChild->getOffsetToName(), "");
			emptyType = true;
		}

		nSft += typeChild->getNumberOfChildren();

		for(std::size_t j = 0, f = typeChild->getNumberOfChildren(); j < f; ++j)
		{
			auto *nameChild = nodes[e + j + nSft - f];
			if(!nameChild)
			{
				continue;
			}

			auto name = nameChild->getName();
			lSft += nameChild->getNumberOfChildren();

			for(std::size_t k = 0, g = nameChild->getNumberOfChildren(); k < g; ++k)
			{
				auto *lanChild = nodes[e + levels[1] + k + lSft - g];
				if(!lanChild)
				{
					continue;
				}
				auto *lanChildNode = lanChild->getNode();
				auto *lanLeaf = dynamic_cast<const ResourceLeaf*>(lanChildNode);
				if(!lanChildNode || !lanChildNode->isLeaf() || !lanLeaf)
				{
					continue;
				}

				if (type == "Icon")
				{
					resource = std::make_unique<ResourceIcon>();
					resourceTable->addResourceIcon(static_cast<ResourceIcon *>(resource.get()));
				}
				else if (type == "Icon Group")
				{
					auto iGroup = std::make_unique<ResourceIconGroup>();
					iGroup->setIconGroupID(iconGroupIDcounter);
					resource = std::move(iGroup);
					resourceTable->addResourceIconGroup(static_cast<ResourceIconGroup *>(resource.get()));
					iconGroupIDcounter++;
				}
				else if (type == "Version")
				{
					resource = std::make_unique<Resource>();
					resourceTable->addResourceVersion(resource.get());
				}
				else
				{
					resource = std::make_unique<Resource>();
				}
				resource->setType(type);
				resource->invalidateTypeId();
				if(emptyType)
				{
					resource->setTypeId(typeChild->getOffsetToName());
				}

				resource->setName(name);
				resource->invalidateNameId();
				if(resource->hasEmptyName())
				{
					resource->setNameId(nameChild->getOffsetToName());
				}

				unsigned long long dataOffset;
				getOffsetFromAddress(dataOffset, imageBase + lanLeaf->getOffsetToData());
				resource->setOffset(dataOffset);
				resource->setSizeInFile(lanLeaf->getSize());
				resource->setLanguage(lanChild->getName());
				resource->invalidateLanguageId();
				resource->invalidateSublanguageId();
				if(resource->hasEmptyLanguage())
				{
					const auto lIdAll = lanChild->getOffsetToName();
					const auto lId = lIdAll & 0x3FF;
					resource->setLanguageId(lId);
					resource->setSublanguageId((lIdAll & 0xFC00) >> 10);
					resource->setLanguage(mapGetValueOrDefault(resourceLanguageMap, lId, ""));
				}
				resource->load(this);
				resourceTable->addResource(std::move(resource));
			}
		}
	}

	resourceTable->linkResourceIconGroups();
	resourceTable->parseVersionInfoResources();
	loadResourceIconHash();

	for (auto&& addressRange : formatParser->getResourceDirectoryOccupiedAddresses())
	{
		nonDecodableRanges.addRange(std::move(addressRange));
	}
}

/**
 * Load certificates.
 */
void PeFormat::loadCertificates()
{
	const auto &securityDir = file->securityDir();
	if(securityDir.calcNumberOfCertificates() == 0)
	{
		return;
	}

	// We always take the first one, there are no additional certificate tables in PE
	auto certBytes = securityDir.getCertificate(0);

	BIO *bio = BIO_new(BIO_s_mem());
	if(!bio)
	{
		return;
	}

	if(BIO_reset(bio) != 1)
	{
		BIO_free(bio);
		return;
	}

	if(BIO_write(bio, certBytes.data(), static_cast<int>(certBytes.size())) != static_cast<std::int64_t>(certBytes.size()))
	{
		BIO_free(bio);
		return;
	}

	PKCS7 *p7 = d2i_PKCS7_bio(bio, nullptr);
	if(!p7)
	{
		BIO_free(bio);
		return;
	}

	// Find signer of the application and store its serial number.
	X509 *signerCert = nullptr;
	X509 *counterSignerCert = nullptr;
	STACK_OF(X509) *certs = p7->d.sign->cert;
	STACK_OF(X509) *signers = PKCS7_get0_signers(p7, certs, 0);

	SCOPE_EXIT {
		if (signers != nullptr)
			sk_X509_free(signers);
	};

	if(sk_X509_num(signers) > 0)
	{
		signerCert = sk_X509_value(signers, 0);
	}

	// Try to find countersigner if it exists and store its serial number.
	STACK_OF(PKCS7_SIGNER_INFO) *sinfos = PKCS7_get_signer_info(p7);
	if(sk_PKCS7_SIGNER_INFO_num(sinfos) > 0)
	{
		PKCS7_SIGNER_INFO *sinfo = sk_PKCS7_SIGNER_INFO_value(sinfos, 0);

		// Counter-signer is stored as unsigned attribute and there is no other way to get it but manual parsing
		ASN1_TYPE *counterSig = PKCS7_get_attribute(sinfo, NID_pkcs9_countersignature);
		if(counterSig)
		{
			auto bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new_mem_buf(counterSig->value.sequence->data, counterSig->value.sequence->length), &BIO_free);
			PKCS7_SIGNER_INFO *counterSinfo = reinterpret_cast<PKCS7_SIGNER_INFO*>(ASN1_item_d2i_bio(ASN1_ITEM_rptr(PKCS7_SIGNER_INFO), bio.get(), nullptr));
			if(counterSinfo)
			{
				// From SignerInfo, we get only issuer, but we can lookup by issuer in all certificates and get original counter-signer
				counterSignerCert = X509_find_by_issuer_and_serial(certs, counterSinfo->issuer_and_serial->issuer, counterSinfo->issuer_and_serial->serial);
			}
			ASN1_item_free(reinterpret_cast<ASN1_VALUE*>(counterSinfo), ASN1_ITEM_rptr(PKCS7_SIGNER_INFO));
		}
	}

	// If we have no signer and countersigner, there must be something really bad
	if(!signerCert && !counterSignerCert)
	{
		BIO_free(bio);
		return;
	}

	// Now that we know there is at least a signer or counter-signer, we can
	// verify the signature. Do not try to verify the signature before
	// verifying that there is at least a signer or counter-signer as 'p7' is
	// empty in that case (#87).
	signatureVerified = verifySignature(p7);

	// Create hash table with key-value pair as subject-X509 certificate so we can easily lookup certificates by their subject name
	std::unordered_map<std::string, X509*> subjectToCert;
	for(int j = 0; j < sk_X509_num(certs); ++j)
	{
		X509 *xcert = sk_X509_value(certs, j);
		auto subjectPtr = X509_NAME_oneline(X509_get_subject_name(xcert), nullptr, 0);
		std::string subject = subjectPtr;
		subjectToCert[subject] = xcert;
		OPENSSL_free(subjectPtr);
	}

	// Start with signer certificate which will be always first and continue with its issuer name and use previously constructed hash table to
	// reconstruct chain of certificates
	// When we hit the last certificate in the chain and there is counter-signer, try to reconstruct its chain
	X509* xcert = nullptr;
	bool counterChain = false;
	std::string nextIssuer;
	if(signerCert)
	{
		auto nextIssuerPtr = X509_NAME_oneline(X509_get_subject_name(signerCert), nullptr, 0);
		nextIssuer = nextIssuerPtr;
		OPENSSL_free(nextIssuerPtr);
	}

	// Continue while we have next issuer to process, or there is counter-signer certificate and we haven't processed him yet
	while(!nextIssuer.empty() || (!counterChain && counterSignerCert))
	{
		// Find next issuer in the hash table
		auto itr = subjectToCert.find(nextIssuer);
		if(itr == subjectToCert.end())
		{
			// If we haven't processed counter-signer chain yet and there is counter-signer certificate
			if(!counterChain && counterSignerCert)
			{
				auto nextIssuerPtr = X509_NAME_oneline(X509_get_subject_name(counterSignerCert), nullptr, 0);
				nextIssuer = nextIssuerPtr;
				counterChain = true;
				OPENSSL_free(nextIssuerPtr);
				continue;
			}
			else
			{
				break;
			}
		}
		// Remove certificate from the hash table so we can't get into infinite loops
		else
		{
			xcert = itr->second;
			subjectToCert.erase(itr);
		}

		if(!certificateTable)
		{
			certificateTable = new CertificateTable();
		}

		Certificate cert(xcert);
		certificateTable->addCertificate(cert);

		// Check if we are at signer or counter-signer certificate and let the certificate table known indices.
		if(xcert == signerCert)
		{
			certificateTable->setSignerCertificateIndex(certificateTable->getNumberOfCertificates() - 1);
		}
		else if(xcert == counterSignerCert)
		{
			certificateTable->setCounterSignerCertificateIndex(certificateTable->getNumberOfCertificates() - 1);
		}

		// Continue with next issuer
		nextIssuer = cert.getRawIssuer();
	}

	PKCS7_free(p7);
	BIO_free(bio);
}

/**
 * Load thread-local storage information
 */
void PeFormat::loadTlsInformation()
{
	unsigned long long rva = 0, size = 0;
	if (!getDataDirectoryRelative(PELIB_IMAGE_DIRECTORY_ENTRY_TLS, rva, size) || size == 0)
	{
		return;
	}

	tlsInfo = new TlsInfo();
	tlsInfo->setRawDataStartAddr(formatParser->getTlsStartAddressOfRawData());
	tlsInfo->setRawDataEndAddr(formatParser->getTlsEndAddressOfRawData());
	tlsInfo->setIndexAddr(formatParser->getTlsAddressOfIndex());
	tlsInfo->setZeroFillSize(formatParser->getTlsSizeOfZeroFill());
	tlsInfo->setCharacteristics(formatParser->getTlsCharacteristics());

	auto callBacksAddr = formatParser->getTlsAddressOfCallBacks();
	tlsInfo->setCallBacksAddr(callBacksAddr);

	const auto &allBytes = getBytes();
	DynamicBuffer structContent(allBytes);

	unsigned long long callBacksOffset;
	if (getOffsetFromAddress(callBacksOffset, callBacksAddr))
	{
		while (allBytes.size() >= callBacksOffset + sizeof(std::uint32_t))
		{
			auto cbAddr = structContent.read<std::uint32_t>(callBacksOffset);
			callBacksOffset += sizeof(std::uint32_t);

			if (cbAddr == 0)
			{
				break;
			}

			tlsInfo->addCallBack(cbAddr);
		}
	}
}

/**
 * Load .NET headers.
 */
void PeFormat::loadDotnetHeaders()
{
	std::uint64_t metadataHeaderAddress = 0;

	// If our file contains CLR header, then use it
	unsigned long long comHeaderAddress, comHeaderSize;
	if(getDataDirectoryRelative(PELIB_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, comHeaderAddress, comHeaderSize) && comHeaderSize)
	{
		clrHeader = formatParser->getClrHeader();
		metadataHeaderAddress = formatParser->getImageBaseAddress() + clrHeader->getMetadataDirectoryAddress();
	}
	// If not, then try to guess whether the file could possibly be .NET file based on imports and try to search for metadata header
	else
	{
		if (importTable && importTable->getNumberOfImportsInLibraryCaseInsensitive("mscoree.dll"))
		{
			metadataHeaderAddress = detectPossibleMetadataHeaderAddress();
			if (metadataHeaderAddress == 0)
				return;
		}
		else
		{
			return;
		}
	}

	// This explicit initialization needs to be here, because clang 4.0 has bug in optimizer and it causes problem in valgrind.
	std::uint64_t signature = 0;
	if (!get4Byte(metadataHeaderAddress, signature) || signature != MetadataHeaderSignature)
	{
		return;
	}

	std::uint64_t majorVersion, minorVersion, versionLength;
	if (!get2Byte(metadataHeaderAddress + 4, majorVersion)
		|| !get2Byte(metadataHeaderAddress + 6, minorVersion)
		|| !get2Byte(metadataHeaderAddress + 12, versionLength))
	{
		return;
	}

	std::string version;
	if (!getNTBS(metadataHeaderAddress + 16, version, versionLength))
	{
		return;
	}

	auto metadataHeaderStreamsHeader = metadataHeaderAddress + 16 + versionLength;

	std::uint64_t flags, streamCount;
	if (!get1Byte(metadataHeaderStreamsHeader, flags)
		|| !get2Byte(metadataHeaderStreamsHeader + 2, streamCount))
	{
		return;
	}

	metadataHeader = std::make_unique<MetadataHeader>();
	metadataHeader->setAddress(metadataHeaderAddress - formatParser->getImageBaseAddress());
	metadataHeader->setMajorVersion(majorVersion);
	metadataHeader->setMinorVersion(minorVersion);
	metadataHeader->setVersion(version);
	metadataHeader->setFlags(flags);

	auto currentAddress = metadataHeaderStreamsHeader + 4;
	for (std::uint64_t i = 0; i < streamCount; ++i)
	{
		std::uint64_t streamOffset, streamSize;
		std::string streamName;

		if (!get4Byte(currentAddress, streamOffset)
			|| !get4Byte(currentAddress + 4, streamSize)
			|| !getNTBS(currentAddress + 8, streamName))
		{
			return;
		}

		if (streamName == "#~" || streamName == "#-")
			parseMetadataStream(metadataHeaderAddress, streamOffset, streamSize);
		else if (streamName == "#Blob")
			parseBlobStream(metadataHeaderAddress, streamOffset, streamSize);
		else if (streamName == "#GUID")
			parseGuidStream(metadataHeaderAddress, streamOffset, streamSize);
		else if (streamName == "#Strings")
			parseStringStream(metadataHeaderAddress, streamOffset, streamSize);
		else if (streamName == "#US")
			parseUserStringStream(metadataHeaderAddress, streamOffset, streamSize);

		// Round-up to the nearest higher multiple of 4
		currentAddress += 8 + ((streamName.length() + 4) & ~3);
	}

	detectModuleVersionId();
	detectTypeLibId();
	detectDotnetTypes();
}

/**
 * Verifies signature of PE file using PKCS7.
 * @param p7 PKCS7 structure.
 * @return @c true if signature is valid, otherwise @c false.
 */
bool PeFormat::verifySignature(PKCS7 *p7)
{
	// At first, verify that there are data in place where Microsoft Code Signing should be present
	if (!p7->d.sign->contents->d.other)
		return false;

	// We need this because PKCS7_verify() looks up algorithms and without this, tables are empty
	OpenSSL_add_all_algorithms();
	SCOPE_EXIT {
		EVP_cleanup();
	};

	// First, check whether the hash written in ContentInfo matches the hash of the whole file
	auto contentInfoPtr = p7->d.sign->contents->d.other->value.sequence->data;
	auto contentInfoLen = p7->d.sign->contents->d.other->value.sequence->length;
	std::vector<std::uint8_t> contentInfoData(contentInfoPtr, contentInfoPtr + contentInfoLen);
	auto contentInfo = Asn1Item::parse(contentInfoData);
	if (contentInfo == nullptr)
		return false;
	if (!contentInfo->isSequence())
		return false;

	auto digest = std::static_pointer_cast<Asn1Sequence>(contentInfo)->getElement(1);
	if (digest == nullptr || !digest->isSequence())
		return false;

	auto digestSeq = std::static_pointer_cast<Asn1Sequence>(digest);
	if (digestSeq->getNumberOfElements() != 2)
		return false;

	auto digestAlgo = digestSeq->getElement(0);
	auto digestValue = digestSeq->getElement(1);
	if (!digestAlgo->isSequence() || !digestValue->isOctetString())
		return false;

	auto digestAlgoSeq = std::static_pointer_cast<Asn1Sequence>(digestAlgo);
	if (digestAlgoSeq->getNumberOfElements() == 0)
		return false;

	auto digestAlgoOID = digestAlgoSeq->getElement(0);
	if (!digestAlgoOID->isObject())
		return false;

	auto digestAlgoOIDStr = std::static_pointer_cast<Asn1Object>(digestAlgoOID)->getIdentifier();

	retdec::crypto::HashAlgorithm algorithm;
	if (digestAlgoOIDStr == DigestAlgorithmOID_Sha1)
		algorithm = retdec::crypto::HashAlgorithm::Sha1;
	else if (digestAlgoOIDStr == DigestAlgorithmOID_Sha256)
		algorithm = retdec::crypto::HashAlgorithm::Sha256;
	else if (digestAlgoOIDStr == DigestAlgorithmOID_Md5)
		algorithm = retdec::crypto::HashAlgorithm::Md5;
	else
	{
		EVP_cleanup();
		return false;
	}

	auto storedHash = std::static_pointer_cast<Asn1OctetString>(digestValue)->getString();
	auto calculatedHash = calculateDigest(algorithm);
	if (storedHash != calculatedHash)
	{
		EVP_cleanup();
		return false;
	}

	auto contentData = contentInfo->getContentData();
	auto contentBio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new_mem_buf(contentData.data(), contentData.size()), &BIO_free);
	auto emptyTrustStore = std::unique_ptr<X509_STORE, decltype(&X509_STORE_free)>(X509_STORE_new(), &X509_STORE_free);
	if (PKCS7_verify(p7, p7->d.sign->cert, emptyTrustStore.get(), contentBio.get(), nullptr, PKCS7_NOVERIFY) == 0)
		return false;

	return true;
}

/**
 * Returns ranges that are used for digest calculation. This digest is used for signature verification.
 * Range is represented in form of tuple where first element is pointer to the beginning of the range and second is size of the range.
 * @return Ranges used for digest process.
 */
std::vector<std::tuple<const std::uint8_t*, std::size_t>> PeFormat::getDigestRanges() const
{
	std::vector<std::tuple<const std::uint8_t*, std::size_t>> result;
	std::size_t checksumFileOffset = formatParser->getChecksumFileOffset();
	std::size_t secDirFileOffset = formatParser->getSecurityDirFileOffset(); // offset of security directory record in PE header
	std::size_t secDirOffset = formatParser->getSecurityDirRva();
	std::size_t secDirSize = formatParser->getSecurityDirSize();

	// To prevent crashes on unordinary binaries, we need to sort these offsets (together with sizes, but they are unimportant for sorting)
	// Usually, checksum is first, then security directory header and then security directory
	// There are a few binaries where this order is not followed
	std::vector<std::pair<std::size_t, std::size_t>> offsets = { std::make_pair(checksumFileOffset, 4), std::make_pair(secDirFileOffset, 8), std::make_pair(secDirOffset, secDirSize) };
	std::sort(offsets.begin(), offsets.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.first < rhs.first;
		});

	std::size_t lastOffset = 0;
	for (auto& offsetSize : offsets)
	{
		// If the length of the range is bigger than the amount of data we have available, then sanitize the length
		if (offsetSize.second > bytes.size())
			offsetSize.second = bytes.size();

		// If the range overlaps the end of the file, then sanitize the length
		if (offsetSize.first + offsetSize.second > bytes.size())
			offsetSize.second = bytes.size() - offsetSize.first;

		// This offsetSize is completely covered by the last offset so ignore it
		if (offsetSize.first + offsetSize.second <= lastOffset)
			continue;

		// This offsetSize is partially covered by the last offset, so shrink it
		// Shrunk offsetSize begins where the last offset ended
		if (offsetSize.first <= lastOffset)
		{
			offsetSize.second = lastOffset - offsetSize.first;
			offsetSize.first = lastOffset;
		}

		result.emplace_back(bytes.data() + lastOffset, offsetSize.first - lastOffset);
		lastOffset = offsetSize.first + offsetSize.second;
	}

	// Finish off the data if the last offset didn't end at the end of all data
	if (lastOffset != bytes.size())
		result.emplace_back(bytes.data() + lastOffset, bytes.size() - lastOffset);

	return result;
}

/**
 * Calculates the digest using selected hash algorithm.
 * @param hashType Algorithm to use.
 * @return Hex string of hash.
 */
std::string PeFormat::calculateDigest(retdec::crypto::HashAlgorithm hashType) const
{
	retdec::crypto::HashContext hashCtx;
	if (!hashCtx.init(hashType))
		return {};

	auto digestRanges = getDigestRanges();
	for (const auto& range : digestRanges)
	{
		const std::uint8_t* data = std::get<0>(range);
		std::size_t size = std::get<1>(range);

		if (!hashCtx.addData(data, size))
			return {};
	}

	return hashCtx.getHash();
}

/**
 * Parses .NET metadata stream.
 * @param baseAddress Base address of .NET metadata header.
 * @param offset Offset of metadata stream.
 * @param size Size of stream.
 */
void PeFormat::parseMetadataStream(std::uint64_t baseAddress, std::uint64_t offset, std::uint64_t size)
{
	const std::uint64_t metadataStreamHeaderSize = 24;
	if (size <= metadataStreamHeaderSize)
	{
		return;
	}

	metadataStream = std::make_unique<MetadataStream>(offset, size);
	auto address = baseAddress + offset;

	std::uint64_t majorVersion, minorVersion, heapOffsetSizes, valid, sorted;
	if (!get1Byte(address + 4, majorVersion)
		|| !get1Byte(address + 5, minorVersion)
		|| !get1Byte(address + 6, heapOffsetSizes)
		|| !get8Byte(address + 8, valid)
		|| !get8Byte(address + 16, sorted))
	{
		return;
	}

	metadataStream->setMajorVersion(majorVersion);
	metadataStream->setMinorVersion(minorVersion);

	// 'heapOffsetSizes' define whether we should use word or dword for indexes into different streams
	metadataStream->setStringStreamIndexSize(heapOffsetSizes & 0x01 ? 4 : 2);
	metadataStream->setGuidStreamIndexSize(heapOffsetSizes & 0x02 ? 4 : 2);
	metadataStream->setBlobStreamIndexSize(heapOffsetSizes & 0x04 ? 4 : 2);

	// Tables that are present in the stream are marked with bit 1 in 'valid' attribute
	// At first we need to create all tables with their sizes because we don't know how many of them is there,
	// so we don't know where to start to parse table content.
	std::uint64_t currentAddress = address + metadataStreamHeaderSize;
	for (std::size_t i = 0; i < 64; ++i)
	{
		if ((valid >> i) & 1)
		{
			std::uint64_t tableSize;
			if (!get4Byte(currentAddress, tableSize))
			{
				return;
			}

			// If the size of the metadata table would be larger than there are data available, we just end. This must be corrupted.
			if (tableSize > getLoadedFileLength())
			{
				return;
			}

			metadataStream->addMetadataTable(static_cast<MetadataTableType>(i), tableSize);
			currentAddress += 4;
		}
	}

	for (std::size_t i = 0; i < 64; ++i)
	{
		auto table = metadataStream->getMetadataTable(static_cast<MetadataTableType>(i));
		if (table == nullptr)
		{
			continue;
		}

		switch (table->getType())
		{
			case MetadataTableType::Module:
				parseMetadataTable<DotnetModule>(table, currentAddress);
				break;
			case MetadataTableType::TypeRef:
				parseMetadataTable<TypeRef>(table, currentAddress);
				break;
			case MetadataTableType::TypeDef:
				parseMetadataTable<TypeDef>(table, currentAddress);
				break;
			case MetadataTableType::FieldPtr:
				parseMetadataTable<FieldPtr>(table, currentAddress);
				break;
			case MetadataTableType::Field:
				parseMetadataTable<Field>(table, currentAddress);
				break;
			case MetadataTableType::MethodPtr:
				parseMetadataTable<MethodPtr>(table, currentAddress);
				break;
			case MetadataTableType::MethodDef:
				parseMetadataTable<MethodDef>(table, currentAddress);
				break;
			case MetadataTableType::ParamPtr:
				parseMetadataTable<ParamPtr>(table, currentAddress);
				break;
			case MetadataTableType::Param:
				parseMetadataTable<Param>(table, currentAddress);
				break;
			case MetadataTableType::InterfaceImpl:
				parseMetadataTable<InterfaceImpl>(table, currentAddress);
				break;
			case MetadataTableType::MemberRef:
				parseMetadataTable<MemberRef>(table, currentAddress);
				break;
			case MetadataTableType::Constant:
				parseMetadataTable<Constant>(table, currentAddress);
				break;
			case MetadataTableType::CustomAttribute:
				parseMetadataTable<CustomAttribute>(table, currentAddress);
				break;
			case MetadataTableType::FieldMarshal:
				parseMetadataTable<FieldMarshal>(table, currentAddress);
				break;
			case MetadataTableType::DeclSecurity:
				parseMetadataTable<DeclSecurity>(table, currentAddress);
				break;
			case MetadataTableType::ClassLayout:
				parseMetadataTable<ClassLayout>(table, currentAddress);
				break;
			case MetadataTableType::FieldLayout:
				parseMetadataTable<FieldLayout>(table, currentAddress);
				break;
			case MetadataTableType::StandAloneSig:
				parseMetadataTable<StandAloneSig>(table, currentAddress);
				break;
			case MetadataTableType::EventMap:
				parseMetadataTable<EventMap>(table, currentAddress);
				break;
			case MetadataTableType::Event:
				parseMetadataTable<Event>(table, currentAddress);
				break;
			case MetadataTableType::PropertyMap:
				parseMetadataTable<PropertyMap>(table, currentAddress);
				break;
			case MetadataTableType::PropertyPtr:
				parseMetadataTable<PropertyPtr>(table, currentAddress);
				break;
			case MetadataTableType::Property:
				parseMetadataTable<Property>(table, currentAddress);
				break;
			case MetadataTableType::MethodSemantics:
				parseMetadataTable<MethodSemantics>(table, currentAddress);
				break;
			case MetadataTableType::MethodImpl:
				parseMetadataTable<MethodImpl>(table, currentAddress);
				break;
			case MetadataTableType::ModuleRef:
				parseMetadataTable<ModuleRef>(table, currentAddress);
				break;
			case MetadataTableType::TypeSpec:
				parseMetadataTable<TypeSpec>(table, currentAddress);
				break;
			case MetadataTableType::ImplMap:
				parseMetadataTable<ImplMap>(table, currentAddress);
				break;
			case MetadataTableType::FieldRVA:
				parseMetadataTable<FieldRVA>(table, currentAddress);
				break;
			case MetadataTableType::ENCLog:
				parseMetadataTable<ENCLog>(table, currentAddress);
				break;
			case MetadataTableType::ENCMap:
				parseMetadataTable<ENCMap>(table, currentAddress);
				break;
			case MetadataTableType::Assembly:
				parseMetadataTable<Assembly>(table, currentAddress);
				break;
			case MetadataTableType::AssemblyProcessor:
				parseMetadataTable<AssemblyProcessor>(table, currentAddress);
				break;
			case MetadataTableType::AssemblyOS:
				parseMetadataTable<AssemblyOS>(table, currentAddress);
				break;
			case MetadataTableType::AssemblyRef:
				parseMetadataTable<AssemblyRef>(table, currentAddress);
				break;
			case MetadataTableType::AssemblyRefProcessor:
				parseMetadataTable<AssemblyRefProcessor>(table, currentAddress);
				break;
			case MetadataTableType::AssemblyRefOS:
				parseMetadataTable<AssemblyRefOS>(table, currentAddress);
				break;
			case MetadataTableType::File:
				parseMetadataTable<File>(table, currentAddress);
				break;
			case MetadataTableType::ExportedType:
				parseMetadataTable<ExportedType>(table, currentAddress);
				break;
			case MetadataTableType::ManifestResource:
				parseMetadataTable<ManifestResource>(table, currentAddress);
				break;
			case MetadataTableType::NestedClass:
				parseMetadataTable<NestedClass>(table, currentAddress);
				break;
			case MetadataTableType::GenericParam:
				parseMetadataTable<GenericParam>(table, currentAddress);
				break;
			case MetadataTableType::GenericParamContstraint:
				parseMetadataTable<GenericParamContstraint>(table, currentAddress);
				break;
			default:
				break;
		}
	}
}

/**
 * Parses .NET blob stream.
 * @param baseAddress Base address of .NET metadata header.
 * @param offset Offset of blob stream.
 * @param size Size of stream.
 */
void PeFormat::parseBlobStream(std::uint64_t baseAddress, std::uint64_t offset, std::uint64_t size)
{
	blobStream = std::make_unique<BlobStream>(offset, size);
	auto address = baseAddress + offset;

	std::vector<std::uint8_t> elementData;
	std::uint64_t length, lengthSize;

	std::size_t inStreamOffset = 0;
	while (inStreamOffset < size)
	{
		// First byte is length of next element in the blob
		lengthSize = 1;
		if (!get1Byte(address + inStreamOffset, length))
		{
			return;
		}

		// 2-byte length encoding if the length is 10xxxxxx
		if ((length & 0xC0) == 0x80)
		{
			if (!get2Byte(address + inStreamOffset, length, Endianness::BIG))
			{
				return;
			}

			length &= ~0xC000;
			lengthSize = 2;
		}
		// 4-byte length encoding if the length is 110xxxxx
		else if ((length & 0xE0) == 0xC0)
		{
			if (!get4Byte(address + inStreamOffset, length, Endianness::BIG))
			{
				return;
			}

			length &= ~0xE0000000;
			lengthSize = 4;
		}

		// Read only if length is greater than 0
		elementData.clear();
		if (length > 0 && !getXBytes(address + inStreamOffset + lengthSize, length, elementData))
		{
			return;
		}

		blobStream->addElement(inStreamOffset, elementData);
		inStreamOffset += lengthSize + length;
	}
}

/**
 * Parses .NET GUID stream.
 * @param baseAddress Base address of .NET metadata header.
 * @param offset Offset of GUID stream.
 * @param size Size of stream.
 */
void PeFormat::parseGuidStream(std::uint64_t baseAddress, std::uint64_t offset, std::uint64_t size)
{
	std::vector<std::uint8_t> guids;
	if (!getXBytes(baseAddress + offset, size, guids))
	{
		return;
	}

	guidStream = std::make_unique<GuidStream>(offset, size);
	guidStream->addGuids(guids);
}

/**
 * Parses .NET string stream.
 * @param baseAddress Base address of .NET metadata header.
 * @param offset Offset of string stream.
 * @param size Size of stream.
 */
void PeFormat::parseStringStream(std::uint64_t baseAddress, std::uint64_t offset, std::uint64_t size)
{
	stringStream = std::make_unique<StringStream>(offset, size);
	auto address = baseAddress + offset;

	// First string is always empty, so we just skip it
	std::uint64_t currentOffset = 1;
	while (currentOffset < size)
	{
		std::string string;
		if (!getNTBS(address + currentOffset, string))
		{
			currentOffset += 1;
			continue;
		}

		stringStream->addString(currentOffset, string);

		// +1 for null-terminator
		currentOffset += 1 + string.length();
	}
}

/**
 * Parses .NET user string stream.
 * @param baseAddress Base address of .NET metadata header.
 * @param offset Offset of user string stream.
 * @param size Size of stream.
 */
void PeFormat::parseUserStringStream(std::uint64_t/* baseAddress*/, std::uint64_t offset, std::uint64_t size)
{
	userStringStream = std::make_unique<UserStringStream>(offset, size);
}

/**
 * Parses single metadata table from metadata stream.
 * @param table Table where to insert data.
 * @param address Address of table data.
 */
template <typename T>
void PeFormat::parseMetadataTable(BaseMetadataTable* table, std::uint64_t& address)
{
	auto specTable = static_cast<MetadataTable<T>*>(table);
	for (std::size_t i = 0; i < table->getSize(); ++i)
	{
		try
		{
			T row;
			row.load(this, metadataStream.get(), address);
			specTable->addRow(std::move(row));
		}
		catch (const InvalidDotnetRecordError&)
		{
			break;
		}
	}
}

/**
 * Detects Module Version ID (GUID) out of .NET tables.
 */
void PeFormat::detectModuleVersionId()
{
	moduleVersionId.clear();

	if (!metadataStream || !guidStream || !metadataStream->hasTable(MetadataTableType::Module))
	{
		return;
	}

	auto moduleTable = static_cast<const MetadataTable<DotnetModule>*>(metadataStream->getMetadataTable(MetadataTableType::Module));
	if (!moduleTable || moduleTable->getNumberOfRows() < 1)
	{
		return;
	}

	auto row = moduleTable->getRow(1);
	moduleVersionId = guidStream->getGuidString(row->mvId.getIndex());
}

/**
 * Detects TypeLib ID (GUID) out of .NET tables.
 */
void PeFormat::detectTypeLibId()
{
	typeLibId.clear();
	if (!metadataStream || !stringStream || !blobStream)
	{
		return;
	}

	auto typeRefTable = static_cast<const MetadataTable<TypeRef>*>(metadataStream->getMetadataTable(MetadataTableType::TypeRef));
	auto memberRefTable = static_cast<const MetadataTable<MemberRef>*>(metadataStream->getMetadataTable(MetadataTableType::MemberRef));
	auto customAttributeTable = static_cast<const MetadataTable<CustomAttribute>*>(metadataStream->getMetadataTable(MetadataTableType::CustomAttribute));
	auto assemblyRefTable = static_cast<const MetadataTable<AssemblyRef>*>(metadataStream->getMetadataTable(MetadataTableType::AssemblyRef));
	if (!typeRefTable || !memberRefTable || !customAttributeTable || !assemblyRefTable)
	{
		return;
	}

	// First find the index of GuidAttribute type reference in TypeRef table, which has ResolutionScope as mscorlib
	std::size_t guidTypeRef = typeRefTable->getNumberOfRows();
	for (std::size_t i = 1; i <= typeRefTable->getNumberOfRows(); ++i)
	{
		auto typeRefRow = typeRefTable->getRow(i);
		auto assemblyRef = assemblyRefTable->getRow(typeRefRow->resolutionScope.getIndex());
		if (!assemblyRef)
		{
			continue;
		}

		std::string assemblyName;
		if (!stringStream->getString(assemblyRef->name.getIndex(), assemblyName) || assemblyName != "mscorlib")
		{
			continue;
		}

		std::string typeName;
		if (stringStream->getString(typeRefRow->typeName.getIndex(), typeName) && typeName == "GuidAttribute")
		{
			guidTypeRef = i;
			break;
		}
	}

	// No GuidAttribute type reference, that means no TypeLib
	if (guidTypeRef == typeRefTable->getNumberOfRows())
	{
		return;
	}

	// Then try to find the MemberRef which refers to this TypeRef
	std::size_t guidMemberRef = memberRefTable->getNumberOfRows();
	for (std::size_t i = 1; i <= memberRefTable->getNumberOfRows(); ++i)
	{
		auto memberRefRow = memberRefTable->getRow(i);
		if (memberRefRow->classType.getIndex() == guidTypeRef)
		{
			guidMemberRef = i;
			break;
		}
	}

	// No MemberRef reference to type reference, that means no TypeLib
	if (guidMemberRef == memberRefTable->getNumberOfRows())
	{
		return;
	}

	std::regex guidRegex("[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}", std::regex::icase | std::regex::ECMAScript);

	// As last, try to find CustomAttribute with type referring to MemberRef
	for (std::size_t i = 1; i <= customAttributeTable->getNumberOfRows(); ++i)
	{
		auto customAttributeRow = customAttributeTable->getRow(i);
		if (customAttributeRow->type.getIndex() == guidMemberRef)
		{
			// Its value is the TypeLib we are looking for
			auto typeLibData = blobStream->getElement(customAttributeRow->value.getIndex());
			if (typeLibData.size() < 3)
			{
				continue;
			}

			// Custom attributes contain one word 0x0001 at the beginning so we skip it,
			// followed by length of the string, which is GUID we are looking for
			auto length = typeLibData[2];
			typeLibId = retdec::utils::toLower(std::string(reinterpret_cast<const char*>(typeLibData.data() + 3), length));
			if (!std::regex_match(typeLibId, guidRegex))
			{
				typeLibId.clear();
				continue;
			}

			break;
		}
	}
}

/**
 * Detects and reconstructs .NET types such as classes, methods, fields, properties etc.
 */
void PeFormat::detectDotnetTypes()
{
	DotnetTypeReconstructor reconstructor(metadataStream.get(), stringStream.get(), blobStream.get());

	definedClasses.clear();
	importedClasses.clear();
	if (reconstructor.reconstruct())
	{
		definedClasses = reconstructor.getDefinedClasses();
		importedClasses = reconstructor.getReferencedClasses();
	}

	computeTypeRefHashes();
}

/**
 * Detects possible metadata header structure. It first searches for metadata header signature
 * `0x424A5342`. If it finds this signature, it then tries to look further for possible stream names.
 *
 * @return Possible metadata header address, 0 if unsuccessful.
 */
std::uint64_t PeFormat::detectPossibleMetadataHeaderAddress() const
{
	const auto possibleStreamNames = { "#~", "#-", "#Strings", "#Blob", "#GUID", "#US" };

	bool metadataHeaderFound = false;
	std::uint64_t address = 0;
	std::uint64_t signature = 0;
	for (const auto* sec : sections)
	{
		address = sec->getAddress();

		for (std::uint64_t inSecAddress = address; !metadataHeaderFound && inSecAddress < sec->getEndAddress(); ++inSecAddress)
		{
			if (!get4Byte(inSecAddress, signature))
				break;

			if (signature == MetadataHeaderSignature)
			{
				std::uint64_t versionLength = 0;
				if (!get2Byte(inSecAddress + 12, versionLength))
					break;

				auto firstStreamNameAddress = inSecAddress
					+ 16 // skip metadata header fields
					+ versionLength // skip version string
					+ 4 // skip stream count
					+ 8; // skip offset and size of the first stream
				std::string streamName;
				if (!getNTBS(firstStreamNameAddress, streamName))
					break;

				if (std::any_of(possibleStreamNames.begin(), possibleStreamNames.end(),
							[&streamName](const auto& possibleStreamName) {
								return streamName == possibleStreamName;
							}))
				{
					metadataHeaderFound = true;
					address = inSecAddress;
					break;
				}
			}
		}

		if (metadataHeaderFound)
			break;
	}

	return metadataHeaderFound ? address : 0;
}

/**
 * Compute typeref hashes - CRC32, MD5, SHA256.
 */
void PeFormat::computeTypeRefHashes()
{
	if (!metadataStream || !stringStream)
	{
		return;
	}

	std::vector<std::uint8_t> typeRefHashBytes;
	std::string typeName;
	std::string nameSpace;
	std::string referencedName;
	MetadataTableType resolutionScopeType;

	auto typeRefTable = static_cast<const MetadataTable<TypeRef>*>(metadataStream->getMetadataTable(MetadataTableType::TypeRef));
	auto moduleTable = static_cast<const MetadataTable<DotnetModule>*>(metadataStream->getMetadataTable(MetadataTableType::Module));
	auto moduleRefTable = static_cast<const MetadataTable<ModuleRef>*>(metadataStream->getMetadataTable(MetadataTableType::ModuleRef));
	auto assemblyRefTable = static_cast<const MetadataTable<AssemblyRef>*>(metadataStream->getMetadataTable(MetadataTableType::AssemblyRef));

	if (!typeRefTable)
	{
		return;
	}

	for (std::size_t i = 1; i <= typeRefTable->getNumberOfRows(); ++i)
	{
		bool validTypeName = false;
		bool validNameSpace = false;
		bool validReferencedName = false;

		auto typeRefRow = typeRefTable->getRow(i);

		if (stringStream->getString(typeRefRow->typeName.getIndex(), typeName) && !typeName.empty())
		{
			validTypeName = true;
		}
		if (stringStream->getString(typeRefRow->typeNamespace.getIndex(), nameSpace) && !nameSpace.empty())
		{
			validNameSpace = true;
		}

		if (typeRefRow->resolutionScope.getTable(resolutionScopeType))
		{
			switch (resolutionScopeType)
			{
				case MetadataTableType::TypeRef:
				{
					auto typeRef = typeRefTable->getRow(typeRefRow->resolutionScope.getIndex());
					if (typeRef && stringStream->getString(typeRef->typeName.getIndex(), referencedName) && !referencedName.empty())
					{
						referencedName += "TR";
						validReferencedName = true;
					}
					break;
				}
				case MetadataTableType::Module:
				{
					if (moduleTable)
					{
						auto module = moduleTable->getRow(typeRefRow->resolutionScope.getIndex());
						if (module && stringStream->getString(module->name.getIndex(), referencedName) && !referencedName.empty())
						{
							referencedName += "M";
							validReferencedName = true;
						}
					}
					break;
				}
				case MetadataTableType::ModuleRef:
				{
					if (moduleRefTable)
					{
						auto moduleRef = moduleRefTable->getRow(typeRefRow->resolutionScope.getIndex());
						if (moduleRef && stringStream->getString(moduleRef->name.getIndex(), referencedName) && !referencedName.empty())
						{
							referencedName += "MR";
							validReferencedName = true;
						}
					}
					break;
				}
				case MetadataTableType::AssemblyRef:
				{
					if (assemblyRefTable)
					{
						auto assemblyRef = assemblyRefTable->getRow(typeRefRow->resolutionScope.getIndex());
						if (assemblyRef && stringStream->getString(assemblyRef->name.getIndex(), referencedName) && !referencedName.empty())
						{
							referencedName += "AR";
							validReferencedName = true;
						}
					}
					break;
				}
				default:
					break;
			}

			if (!typeRefHashBytes.empty())
			{
				typeRefHashBytes.push_back(static_cast<unsigned char>(','));
			}

			std::string fullName;
			if (validTypeName)
			{
				fullName = typeName;
			}
			if (validNameSpace)
			{
				if (!fullName.empty())
				{
					fullName += ".";
				}

				fullName += nameSpace;
			}
			if (validReferencedName)
			{
				if (!fullName.empty())
				{
					fullName += ".";
				}

				fullName += referencedName;
			}

			for(const auto c : fullName)
			{
				typeRefHashBytes.push_back(static_cast<uint8_t>(c));
			}
		}
	}

	typeRefHashCrc32 = retdec::crypto::getCrc32(typeRefHashBytes.data(), typeRefHashBytes.size());
	typeRefHashMd5 = retdec::crypto::getMd5(typeRefHashBytes.data(), typeRefHashBytes.size());
	typeRefHashSha256 = retdec::crypto::getSha256(typeRefHashBytes.data(), typeRefHashBytes.size());
}

retdec::utils::Endianness PeFormat::getEndianness() const
{
	switch(formatParser->getMachineType())
	{
		case PELIB_IMAGE_FILE_MACHINE_I386:
		case PELIB_IMAGE_FILE_MACHINE_I486:
		case PELIB_IMAGE_FILE_MACHINE_PENTIUM:
		case PELIB_IMAGE_FILE_MACHINE_AMD64:
		case PELIB_IMAGE_FILE_MACHINE_R3000_LITTLE:
		case PELIB_IMAGE_FILE_MACHINE_R4000:
		case PELIB_IMAGE_FILE_MACHINE_R10000:
		case PELIB_IMAGE_FILE_MACHINE_WCEMIPSV2:
		case PELIB_IMAGE_FILE_MACHINE_MIPS16:
		case PELIB_IMAGE_FILE_MACHINE_MIPSFPU:
		case PELIB_IMAGE_FILE_MACHINE_MIPSFPU16:
		case PELIB_IMAGE_FILE_MACHINE_ARM:
		case PELIB_IMAGE_FILE_MACHINE_THUMB:
		case PELIB_IMAGE_FILE_MACHINE_ARMNT:
		case PELIB_IMAGE_FILE_MACHINE_ARM64:
		case PELIB_IMAGE_FILE_MACHINE_POWERPC:
		case PELIB_IMAGE_FILE_MACHINE_POWERPCFP:
			return Endianness::LITTLE;
		case PELIB_IMAGE_FILE_MACHINE_R3000_BIG:
			return Endianness::BIG;
		default:
			return Endianness::UNKNOWN;
	}
}

std::size_t PeFormat::getBytesPerWord() const
{
	switch(formatParser->getMachineType())
	{
		// Architecture::X86
		case PELIB_IMAGE_FILE_MACHINE_I386:
		case PELIB_IMAGE_FILE_MACHINE_I486:
		case PELIB_IMAGE_FILE_MACHINE_PENTIUM:
			return 4;

		// Architecture::X86_64
		case PELIB_IMAGE_FILE_MACHINE_AMD64:
			return 8;

		// Architecture::MIPS
		case PELIB_IMAGE_FILE_MACHINE_R3000_BIG:
		case PELIB_IMAGE_FILE_MACHINE_R3000_LITTLE:
			return 4;
		case PELIB_IMAGE_FILE_MACHINE_R4000:
			return (peClass == PEFILE64 ? 8 : 4);
		case PELIB_IMAGE_FILE_MACHINE_R10000:
			return 8;
		case PELIB_IMAGE_FILE_MACHINE_WCEMIPSV2:
			return 4;
		case PELIB_IMAGE_FILE_MACHINE_MIPS16:
			return 2;
		case PELIB_IMAGE_FILE_MACHINE_MIPSFPU:
			return 8;
		case PELIB_IMAGE_FILE_MACHINE_MIPSFPU16:
			return 2;

		// Architecture::ARM
		case PELIB_IMAGE_FILE_MACHINE_ARM:
		case PELIB_IMAGE_FILE_MACHINE_THUMB:
		case PELIB_IMAGE_FILE_MACHINE_ARMNT:
			return 4;
		case PELIB_IMAGE_FILE_MACHINE_ARM64:
			return 8;

		// Architecture::POWERPC
		case PELIB_IMAGE_FILE_MACHINE_POWERPC:
		case PELIB_IMAGE_FILE_MACHINE_POWERPCFP:
			return (peClass == PEFILE64 ? 8 : 4);

		// unsupported architecture
		default:
			return 0;
	}
}

bool PeFormat::hasMixedEndianForDouble() const
{
	return false;
}

/**
 * Get declared length of file. This length may be shorter or longer than real length of file.
 * @return Declared length of file
 */
std::size_t PeFormat::getDeclaredFileLength() const
{
	std::size_t declSize = FileFormat::getDeclaredFileLength();
	if(getNumberOfCoffSymbols() && getCoffSymbolTableOffset())
	{
		const std::size_t symTabMaxOffset = getCoffSymbolTableOffset() + (getNumberOfCoffSymbols() * PELIB_IMAGE_SIZEOF_COFF_SYMBOL);
		declSize = std::max(declSize, symTabMaxOffset);
	}

	return declSize + getSizeOfStringTable();
}

bool PeFormat::areSectionsValid() const
{
	return true;
}

bool PeFormat::isObjectFile() const
{
	return false;
}

bool PeFormat::isDll() const
{
	return formatParser->isDll();
}

bool PeFormat::isExecutable() const
{
	return !isDll();
}

bool PeFormat::getMachineCode(unsigned long long &result) const
{
	result = formatParser->getMachineType();
	return true;
}

bool PeFormat::getAbiVersion(unsigned long long &result) const
{
	// not in PE files
	static_cast<void>(result);
	return false;
}

bool PeFormat::getImageBaseAddress(unsigned long long &imageBase) const
{
	imageBase = formatParser->getImageBaseAddress();
	return true;
}

bool PeFormat::getEpAddress(unsigned long long &result) const
{
	return formatParser->getEpAddress(result);
}

bool PeFormat::getEpOffset(unsigned long long &epOffset) const
{
	return formatParser->getEpOffset(epOffset);
}

Architecture PeFormat::getTargetArchitecture() const
{
	switch(formatParser->getMachineType())
	{
		case PELIB_IMAGE_FILE_MACHINE_I386:
		case PELIB_IMAGE_FILE_MACHINE_I486:
		case PELIB_IMAGE_FILE_MACHINE_PENTIUM:
			return Architecture::X86;
		case PELIB_IMAGE_FILE_MACHINE_AMD64:
			return Architecture::X86_64;
		case PELIB_IMAGE_FILE_MACHINE_R3000_BIG:
		case PELIB_IMAGE_FILE_MACHINE_R3000_LITTLE:
		case PELIB_IMAGE_FILE_MACHINE_R4000:
		case PELIB_IMAGE_FILE_MACHINE_R10000:
		case PELIB_IMAGE_FILE_MACHINE_WCEMIPSV2:
		case PELIB_IMAGE_FILE_MACHINE_MIPS16:
		case PELIB_IMAGE_FILE_MACHINE_MIPSFPU:
		case PELIB_IMAGE_FILE_MACHINE_MIPSFPU16:
			return Architecture::MIPS;
		case PELIB_IMAGE_FILE_MACHINE_ARM:
		case PELIB_IMAGE_FILE_MACHINE_THUMB:
		case PELIB_IMAGE_FILE_MACHINE_ARMNT:
		case PELIB_IMAGE_FILE_MACHINE_ARM64:
			return Architecture::ARM;
		case PELIB_IMAGE_FILE_MACHINE_POWERPC:
		case PELIB_IMAGE_FILE_MACHINE_POWERPCFP:
			return Architecture::POWERPC;
		default:
			return Architecture::UNKNOWN;
	}
}

std::size_t PeFormat::getDeclaredNumberOfSections() const
{
	return formatParser->getDeclaredNumberOfSections();
}

std::size_t PeFormat::getDeclaredNumberOfSegments() const
{
	return 0;
}

std::size_t PeFormat::getSectionTableOffset() const
{
	std::size_t res = getPeHeaderOffset() + formatParser->getSizeOfPeSignature() + PELIB_IMAGE_FILE_HEADER::size() + getOptionalHeaderSize();
	if(res >= getFileLength())
	{
		res = getPeHeaderOffset() + formatParser->getLoadedSizeOfNtHeaders();
	}

	return res;
}

std::size_t PeFormat::getSectionTableEntrySize() const
{
	return PELIB_IMAGE_SECTION_HEADER::size();
}

std::size_t PeFormat::getSegmentTableOffset() const
{
	return 0;
}

std::size_t PeFormat::getSegmentTableEntrySize() const
{
	return 0;
}

/**
 * Get size of MZ header
 * @return Size of MZ header
 */
std::size_t PeFormat::getMzHeaderSize() const
{
	return mzHeader.size();
}

/**
 * Get size of optional header
 * @return Size of optional header
 */
std::size_t PeFormat::getOptionalHeaderSize() const
{
	return formatParser->getOptionalHeaderSize();
}

/**
 * Get offset of PE signature
 * @return Offset of PE signature
 *
 * In some cases (e.g. FSG packer), offset of PE signature may be inside MZ header and
 * therefore this method may return lesser number that method @a getMzHeaderSize().
 */
std::size_t PeFormat::getPeHeaderOffset() const
{
	return mzHeader.getAddressOfPeHeader();
}

/**
 * Get offset of COFF symbol table
 * @return Offset of COFF symbol table
 */
std::size_t PeFormat::getCoffSymbolTableOffset() const
{
	return formatParser->getCoffSymbolTableOffset();
}

/**
 * Get number of symbols in COFF symbol table
 * @return Number of symbols in COFF symbol table
 */
std::size_t PeFormat::getNumberOfCoffSymbols() const
{
	return formatParser->getNumberOfCoffSymbols();
}

/**
 * Get size in bytes of string table
 * @return Size in bytes of string table
 */
std::size_t PeFormat::getSizeOfStringTable() const
{
	return file->coffSymTab().getSizeOfStringTable();
}

/**
 * Get major version of used linker
 * @return Major version of used linker
 */
std::size_t PeFormat::getMajorLinkerVersion() const
{
	return formatParser->getMajorLinkerVersion();
}

/**
 * Get minor version of used linker
 * @return Minor version of used linker
 */
std::size_t PeFormat::getMinorLinkerVersion() const
{
	return formatParser->getMinorLinkerVersion();
}

/**
 * Get file flags
 * @return File flags as number
 */
std::size_t PeFormat::getFileFlags() const
{
	return formatParser->getFileFlags();
}

/**
 * Get time stamp
 * @return File time stamp
 */
std::size_t PeFormat::getTimeStamp() const
{
	return formatParser->getTimeStamp();
}

/**
 * Get file alignment
 * @return File alignment
 */
std::size_t PeFormat::getFileAlignment() const
{
	return formatParser->getFileAlignment();
}

/**
 * Get section alignment
 * @return Section alignment
 */
std::size_t PeFormat::getSectionAlignment() const
{
	return formatParser->getSectionAlignment();
}

/**
 * Get size of image
 * @return Size of image
 */
std::size_t PeFormat::getSizeOfImage() const
{
	return formatParser->getSizeOfImage();
}

/**
 * Get file checksum
 * @return File checksum
 */
std::size_t PeFormat::getChecksum() const
{
	return formatParser->getChecksum();
}

/**
 * Get size of the stack to reserve
 * @return Size of the stack to reserve
 */
std::size_t PeFormat::getSizeOfStackReserve() const
{
	return formatParser->getSizeOfStackReserve();
}

/**
 * Get size of the stack to commit
 * @return Size of the stack to commit
 */
std::size_t PeFormat::getSizeOfStackCommit() const
{
	return formatParser->getSizeOfStackCommit();
}

/**
 * Get size of the local heap space to reserve
 * @return Size of the local heap space to reserve
 */
std::size_t PeFormat::getSizeOfHeapReserve() const
{
	return formatParser->getSizeOfHeapReserve();
}

/**
 * Get size of the local heap space to commit
 * @return Size of the local heap space to commit
 */
std::size_t PeFormat::getSizeOfHeapCommit() const
{
	return formatParser->getSizeOfHeapCommit();
}

/**
 * Get number of data-directory entries in input file
 * @return Number of data-directory entries in input file
 */
std::size_t PeFormat::getNumberOfDataDirectories() const
{
	return formatParser->getStoredNumberOfDataDirectories();
}

/**
 * Get number of data-directory entries declared in the optional header
 * @return Number of data-directory entries declared in the optional header
 */
std::size_t PeFormat::getDeclaredNumberOfDataDirectories() const
{
	return formatParser->getDeclaredNumberOfDataDirectories();
}

bool PeFormat::isMissingDependency(std::string dllName) const
{
	// Always convert the name to lowercase
	std::transform(dllName.begin(), dllName.end(), dllName.begin(), ::tolower);

	// In Windows 7+, DLLs beginning with name "api-" or "ext-" are resolved by API set;
	// We consider them always existing
	if ((dllName.length() > 4) && (!strncmp(dllName.c_str(), "api-", 4) || !strncmp(dllName.c_str(), "ext-", 4)))
		return false;

	// If we have overriden set, use that one.
	// Otherwise, use the default DLL set
	const std::unordered_set<std::string> & depsDllList = (dllList.size() != 0) ? dllList : defDllList;
	return (depsDllList.count(dllName) == 0);
}

/**
 * Returns a flag whether the given DLL list has failed to load.
 * @return true: Failed to load the DLL list
 */
bool PeFormat::dllListFailedToLoad() const
{
	return errorLoadingDllList;
}

bool PeFormat::initDllList(const std::string & dllListFile)
{
	// Do nothing if the DLL list is empty
	if (dllListFile.length())
	{
		std::ifstream stream(dllListFile, std::ifstream::in);
		std::string oneLine;

		// Do nothing if the DLL list file cannot be open
		if (!stream)
		{
			errorLoadingDllList = true;
			return false;
		}

		while(stream)
		{
			std::getline(stream, oneLine);
			std::transform(oneLine.begin(), oneLine.end(), oneLine.begin(), ::tolower);
			dllList.insert(oneLine);
		}
	}

	// Sanity check
//	assert(isMissingDependency("kernel32.dll") == false);
	return true;
}

/**
 * Get class of PE file
 * @return PeLib::PEFILE32 if file is 32-bit PE file, PeLib::PEFILE64 if file is
 *    64-bit PE file or any other value otherwise
 */
int PeFormat::getPeClass() const
{
	return peClass;
}

/**
 * Check if input file contains CIL/.NET code
 * @return @c true if input file contains CIL/.NET code, @c false otherwise
 */
bool PeFormat::isDotNet() const
{
	return clrHeader != nullptr || metadataHeader != nullptr;
}

/**
 * Check if input file contains packed CIL/.NET code
 * @return @c true if input file contains packed CIL/.NET code, @c false otherwise
 */
bool PeFormat::isPackedDotNet() const
{
	if(isDotNet())
	{
		return false;
	}

	return importTable
		&& importTable->getNumberOfLibraries() == 1
		&& importTable->getNumberOfImportsInLibraryCaseInsensitive("mscoree.dll");
}

/**
 * Check if input file original language is Visual Basic
 * @param version Into this parameter is stored version of Visual Basic, or @c 0 if
 *    version was not detected
 * @return @c true if input file original language is Visual Basic, @c false otherwise
 */
bool PeFormat::isVisualBasic(unsigned long long &version) const
{
	version = 0;
	return importTable && std::any_of(visualBasicLibrariesMap.begin(), visualBasicLibrariesMap.end(),
		[&] (const auto &item)
		{
			if(this->importTable->getNumberOfImportsInLibraryCaseInsensitive(item.first))
			{
				version = item.second;
				return true;
			}

			return false;
		}
	);
}

/**
 * Get DLL flags
 * @param dllFlags Into this parameter DLL flags will be stored
 * @return @c true if file is DLL and flags are successfully detected, @c false otherwise
 */
bool PeFormat::getDllFlags(unsigned long long &dllFlags) const
{
	return formatParser->getDllFlags(dllFlags);
}

/**
 * Get number of base relocation blocks
 * @param relocs Into this parameter the number of blocks is stored
 * @return @c true if number of blocks is successfully detected, @c false otherwise
 *
 * If function returns @c false, @a relocs is left unchanged
 */
bool PeFormat::getNumberOfBaseRelocationBlocks(unsigned long long &relocs) const
{
	unsigned long long addr, size;
	if(!getDataDirectoryRelative(PELIB_IMAGE_DIRECTORY_ENTRY_BASERELOC, addr, size) || !addr)
	{
		return false;
	}

	relocs = formatParser->getNumberOfRelocations();
	return true;
}

/**
 * Get number of relocations
 * @param relocs Into this parameter the number of relocations is stored
 * @return @c true if number of relocations is successfully detected, @c false otherwise
 *
 * If function returns @c false, @a relocs is left unchanged
 */
bool PeFormat::getNumberOfRelocations(unsigned long long &relocs) const
{
	unsigned long long blocks = 0;
	if(!getNumberOfBaseRelocationBlocks(blocks))
	{
		return false;
	}
	relocs = 0;

	for(unsigned long long i = 0; i < blocks; ++i)
	{
		relocs += formatParser->getNumberOfRelocationData(i);
	}

	return true;
}

/**
 * Get data directory
 * @param index Index of selected directory
 * @param relAddr Into this parameter is stored relative virtual address of directory
 * @param size Into this parameter is stored size of directory
 * @return @c true if index of selected directory is valid, @c false otherwise
 *
 * If method returns @c false, @a relAddr and @a size are left unchanged.
 */
bool PeFormat::getDataDirectoryRelative(unsigned long long index, unsigned long long &relAddr, unsigned long long &size) const
{
	return formatParser->getDataDirectoryRelative(index, relAddr, size);
}

/**
 * Get data directory
 * @param index Index of selected directory
 * @param absAddr Into this parameter is stored absolute virtual address of directory
 * @param size Into this parameter is stored size of directory
 * @return @c true if index of selected directory is valid, @c false otherwise
 *
 * If method returns @c false, @a absAddr and @a size are left unchanged.
 */
bool PeFormat::getDataDirectoryAbsolute(unsigned long long index, unsigned long long &absAddr, unsigned long long &size) const
{
	return formatParser->getDataDirectoryAbsolute(index, absAddr, size);
}

/**
 * Get information about section with name @a secName
 * @param secName Name of section
 * @return Pointer to section or @c nullptr if section was not found
 *
 * If file has more sections with name equal to @a secName, then is returned first such section.
 */
const PeCoffSection* PeFormat::getPeSection(const std::string &secName) const
{
	return dynamic_cast<const PeCoffSection*>(getSection(secName));
}

/**
 * Get information about section with index @a secIndex
 * @param secIndex Index of section (indexed from 0)
 * @return Pointer to section or @c nullptr if section was not detected
 */
const PeCoffSection* PeFormat::getPeSection(unsigned long long secIndex) const
{
	return dynamic_cast<const PeCoffSection*>(getSection(secIndex));
}

const CLRHeader* PeFormat::getCLRHeader() const
{
	return clrHeader.get();
}

const MetadataHeader* PeFormat::getMetadataHeader() const
{
	return metadataHeader.get();
}

const MetadataStream* PeFormat::getMetadataStream() const
{
	return metadataStream.get();
}

const StringStream* PeFormat::getStringStream() const
{
	return stringStream.get();
}

const BlobStream* PeFormat::getBlobStream() const
{
	return blobStream.get();
}

const GuidStream* PeFormat::getGuidStream() const
{
	return guidStream.get();
}

const UserStringStream* PeFormat::getUserStringStream() const
{
	return userStringStream.get();
}

const std::string& PeFormat::getModuleVersionId() const
{
	return moduleVersionId;
}

const std::string& PeFormat::getTypeLibId() const
{
	return typeLibId;
}

const std::vector<std::shared_ptr<DotnetClass>>& PeFormat::getDefinedDotnetClasses() const
{
	return definedClasses;
}

const std::vector<std::shared_ptr<DotnetClass>>& PeFormat::getImportedDotnetClasses() const
{
	return importedClasses;
}

const std::string& PeFormat::getTypeRefhashCrc32() const
{
	return typeRefHashCrc32;
}

const std::string& PeFormat::getTypeRefhashMd5() const
{
	return typeRefHashMd5;
}

const std::string& PeFormat::getTypeRefhashSha256() const
{
	return typeRefHashSha256;
}

const VisualBasicInfo* PeFormat::getVisualBasicInfo() const
{
	return &visualBasicInfo;
}

/**
 * Scan for file format anomalies
 */
void PeFormat::scanForAnomalies()
{
	// anomalies.emplace_back(std::make_pair<std::string, std::string>("hello", "world"));
	std::cerr << "=============\n";
	scanForSectionAnomalies();
	scanForResourceAnomalies();
	scanForImportAnomalies();
	scanForExportAnomalies();
	scanForOptHeaderAnomalies();
}

/**
 * Scan for section anomalies
 */
void PeFormat::scanForSectionAnomalies()
{
	std::size_t nSecs = getDeclaredNumberOfSections();
	
	const PeCoffSection *epSec = dynamic_cast<const PeCoffSection*>(getEpSection());
	if (epSec)
	{
		// scan EP in last section
		const PeCoffSection *lastSec = (nSecs) ? getPeSection(nSecs - 1) : nullptr;
		if (epSec == lastSec)
		{
			std::cerr << "EP in last section\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("epInLastSec",
				"Entry point in last section"));
		}

		// scan EP in writable section
		if (epSec->getPeCoffFlags() & PELIB_IMAGE_SCN_MEM_WRITE)
		{
			std::cerr << "EP in writable section\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("epInWritableSec",
				"Entry point in writable section"));
		}

		// scan EP in nonexecutable section
		if (!(epSec->getPeCoffFlags() & PELIB_IMAGE_SCN_MEM_EXECUTE))
		{
			std::cerr << "EP in nonexecutable section\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("epInNonExecSec",
				"Entry point in nonexecutable section"));
		}
	}
	else
	{
		// scan EP outside mapped sections
		std::cerr << "EP outside mapped sections\n";
		anomalies.emplace_back(std::make_pair<std::string, std::string>("epOutsideSecs",
			"Entry point is outside of mapped sections"));
	}

	for (std::size_t i = 0; i < nSecs; i++)
	{
		auto sec = getPeSection(i);
		if (!sec)
		{
			continue;
		}

		const auto name = sec->getName();
		const std::string msgName = (name.empty()) ? numToStr(sec->getIndex()) : name;
		const auto flags = sec->getPeCoffFlags();
		if (!name.empty())
		{
			// scan for unusual section names
			if (std::find(usualSectionNames.begin(), usualSectionNames.end(), name) == usualSectionNames.end())
			{
				std::cerr << "weird section name: " << name << "\n";
				auto p = std::make_pair<std::string, std::string>("unusualSecName", "Unusual section name: " + name);
				anomalies.emplace_back(std::move(p));
			}

			// scan for packer section names
			if (std::find(usualPackerSections.begin(), usualPackerSections.end(), name) != usualPackerSections.end())
			{
				std::cerr << "packer section name: " << name << "\n";
				auto p = std::make_pair<std::string, std::string>("packedSecName", "Packer section name: " + name);
				anomalies.emplace_back(std::move(p));
			}

			// scan for unexpected characteristics
			auto characIt = usualSectionCharacteristics.find(name);
			if (characIt != usualSectionCharacteristics.end() && characIt->second != flags)
			{
				std::cerr << "unusual characteristics: " << name << "\n";
				anomalies.emplace_back(std::make_pair<std::string, std::string>("unusualSecChar",
					"Section " + name + " has unusual characteristics"));
			}

		}

		// scan size over 100MB
		if (sec->getSizeInFile() >= 100000000UL)
		{
			std::cerr << "Section " << msgName << " has size over 100MB\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("largeSec",
				"Section " + msgName + " has size over 100MB"));
		}

		// scan section marked uninitialized but contains data
		if ((flags & PELIB_IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
			(sec->getOffset() != 0 || sec->getSizeInFile() != 0))
		{
			std::cerr << "Section " << msgName << " marked uninitialized but contains data\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("uninitSecHasData", 
				"Section " + msgName + " is marked uninitialized but contains data"));
		}

		// scan sizeOfRawData of section is 0
		if (sec->getSizeInFile() == 0)
		{
			std::cerr << "Zero SizeOfRawData: " << msgName << '\n';
			anomalies.emplace_back(std::make_pair<std::string, std::string>("noRawDataSec", 
				"Section " + msgName + " has zero SizeOfRawData"));
		}

		for (std::size_t j = i + 1; j < nSecs; j++)
		{
			auto cmpSec = getSection(j);
			if (!cmpSec)
			{
				continue;
			}

			// scan for duplicit section names
			const auto cmpName = cmpSec->getName();
			if (!name.empty() && name == cmpName)
			{
				std::cerr << "Duplicit section names\n";
				anomalies.emplace_back(std::make_pair<std::string, std::string>("duplSecNames",
					"Sections " + numToStr(sec->getIndex()) + " and " + numToStr(cmpSec->getIndex()) +
					" have the same name " + name));
			}

			// scan for overlapping sections
			auto secStart = sec->getOffset();
			auto secEnd = secStart + sec->getSizeInFile();
			auto cmpSecStart = cmpSec->getOffset();
			auto cmpSecEnd = cmpSecStart + cmpSec->getSizeInFile();
			if ((secStart <= cmpSecStart && cmpSecStart < secEnd) ||
				(cmpSecStart <= secStart && secStart < cmpSecEnd))
			{
				const std::string cmpMsgName = (cmpName.empty()) ? numToStr(cmpSec->getIndex()) : cmpName;
				std::cerr << "Sections " << msgName << " and " << cmpMsgName << " overlap\n";
				anomalies.emplace_back(std::make_pair<std::string, std::string>("overlappingSecs",
					"Sections " + msgName + " and " + cmpMsgName + " overlap"));
			}
		}
	}
}

/**
 * Scan for section anomalies
 */
void PeFormat::scanForResourceAnomalies()
{
	if (!resourceTable)
	{
		return;
	}

	for (std::size_t i = 0; i < resourceTable->getNumberOfResources(); i++)
	{
		auto res = resourceTable->getResource(i);
		if (!res)
		{
			continue;
		}

		std::size_t nameId;
		std::string msgName = (res->getNameId(nameId)) ? numToStr(nameId) : "<unknown>";

		// scan for resource size over 100MB
		if (res->getSizeInFile() >= 100000000UL)
		{
			std::cerr << "Resource " << msgName << " has size over 100MB\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("largeRes",
				"Resource " + msgName + " has size over 100MB"));
		}

		// scan for resource stretched over multiple sections
		unsigned long long resAddr;
		if (getAddressFromOffset(resAddr, res->getOffset()) &&
			isObjectStretchedOverSections(resAddr, res->getSizeInFile()))
		{
			std::cerr << "Resource " << msgName << " is stretched over multiple sections\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("stretchedRes",
				"Resource " + msgName + " is stretched over multiple sections"));
		}
	}
}

/**
 * Scan for import anomalies
 */
void PeFormat::scanForImportAnomalies()
{
	// scan for import stretched over multiple sections
	for(auto&& impRange : formatParser->getImportDirectoryOccupiedAddresses())
	{
		unsigned long long impAddr;
		if (getAddressFromOffset(impAddr, impRange.getStart()) &&
			isObjectStretchedOverSections(impAddr, impRange.getSize()))
		{
			std::string msgName;
			auto imp = getImport(impAddr);
			if (!imp)
			{
				msgName = "<unknown>";
			}
			else
			{
				if (imp->hasEmptyName())
				{
					unsigned long long ordNum;
					if (!imp->getOrdinalNumber(ordNum))
					{
						msgName = "<unknown>";
					}
					else
					{
						msgName = numToStr(ordNum);
					}

				}
				else
				{
					msgName = imp->getName();
				}
			}

			std::cerr << "Import at " << impRange.getStart() << " " << msgName << " is stretched over multiple sections\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("stretchedImp",
				"Import " + msgName + " is stretched over multiple sections"));
		}
	}
}

/**
 * Scan for export anomalies
 */
void PeFormat::scanForExportAnomalies()
{
	// scan for export stretched over multiple sections
	for(auto&& expRange : formatParser->getExportDirectoryOccupiedAddresses())
	{
		unsigned long long expAddr;
		if (getAddressFromOffset(expAddr, expRange.getStart()) &&
			isObjectStretchedOverSections(expAddr, expRange.getSize()))
		{
			std::string msgName;
			auto exp = getExport(expAddr);
			if (!exp)
			{
				msgName = "<unknown>";
			}
			else
			{
				if (exp->hasEmptyName())
				{
					unsigned long long ordNum;
					if (!exp->getOrdinalNumber(ordNum))
					{
						msgName = "<unknown>";
					}
					else
					{
						msgName = numToStr(ordNum);
					}

				}
				else
				{
					msgName = exp->getName();
				}
			}

			std::cerr << "Export at " << expRange.getStart() << " " << msgName << " is stretched over multiple sections\n";
			anomalies.emplace_back(std::make_pair<std::string, std::string>("stretchedExp",
				"Export " + msgName + " is stretched over multiple sections"));
		}
	}
}

/**
 * Scan for optional header anomalies
 */
void PeFormat::scanForOptHeaderAnomalies()
{
	// scan for missalignment
	if (!formatParser->isIsSizeOfHeaderMultipleOfFileAlignment())
	{
		std::cerr << "OptHeader SizeOfHeaders not aligned to multiple of FileAlignment\n";
		anomalies.emplace_back(std::make_pair<std::string, std::string>("sizeOfHeadersNotAligned",
			"SizeOfHeaders is not aligned to multiple of FileAlignment"));
	}
}

} // namespace fileformat
} // namespace retdec
