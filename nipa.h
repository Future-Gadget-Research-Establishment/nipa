#include <tchar.h>
#define CODEPAGE_SHIFT_JIS 932

int crypt(int, int);
int crypt2(int, char*);
int newid();
int parsevars(int, char **);

void addentry(char *, char *, int, int, int);
void createnpa(int, char**);
void extractnpa(int, int);
void parsedir(char*);
void parsenpa(char *, int);

typedef struct _npahead
{
	char head[7];        /* MUST start with NPA\x01\x00 or it gets flagged as not an NPA archive. The extra 2 are just nulls. */
	int key1;        /* Keys required for decryption purposes */
	int key2;            
	int encrypt;        /* 1 if encrypted */
	int compress;        /* 1 if compressed */
	int filecount;        /* Before folders */
	int foldercount;    
	int totalcount;        /* File count + folder count */
	long null;        /* Almost certain these 8 bytes do nothing. Set some BPs on the spots in the memory and they are never touched. */
	int start;        /* Starting offset for data. Filetable - 0x29 (header) usually */
	int gameid;        /* Not set in the header itself but it's an appropriate place to store the the game ID used for encryption */
} NPAHEAD;

typedef struct _npaentry
{
	int nlength;        /* Name length */
	char *filename;     /* Size based on nlength */
	int type;         /* 1 = folder, 2 = file */
	int fileid;             
	int offset;         
	int compsize;         /* Orig filesize */
	int origsize;         /* Compressed filesize */
} NPAENTRY;

/*
* All keys listed are the generated form and not the original form.
* Finding the original keys is not hard but I don't see the need to waste
* cycles to generate them each time you run the program.
*/

TCHAR games[][20] = { _T("ChaosHead"), _T("ChaosHeadTr1"), _T("ChaosHeadTr2"), _T("MuramasaTr"), _T("Muramasa"), _T("Sumaga"), _T("Django"), _T("DjangoTr"),
	_T("Lamento"), _T("LamentoTr"), _T("sweetpool"), _T("SumagaSP"), _T("Demonbane"), _T("MuramasaAD"), _T("Axanael"), _T("Kikokugai"), _T("SonicomiTr2"), _T("Sumaga3P"), _T("Sonicomi"), _T("LostXTrailer"), _T("\0") };
enum { CHAOSHEAD = 0, CHAOSHEADTR1, CHAOSHEADTR2, MURAMASATR, MURAMASA, SUMAGA, DJANGO, DJANGOTR,
	LAMENTO, LAMENTOTR, SWEETPOOL, SUMAGASP, DEMONBANE, MURAMASAAD, AXANAEL, KIKOKUGAI, SONICOMITR2, SUMAGA3P, SONICOMI, LOSTXTRAILER };
unsigned char keytbl[][0x100] = {    
	/* Chaos;Head Retail */
	0xF1, 0x71, 0x80, 0x19, 0x17, 0x01, 0x74, 0x7D, 0x90, 0x47, 0xF9, 0x68, 0xDE, 0xB4, 0x24, 0x40,
	0x73, 0x9E, 0x5B, 0x38, 0x4C, 0x3A, 0x2A, 0x0D, 0x2E, 0xB9, 0x5C, 0xE9, 0xCE, 0xE8, 0x3E, 0x39,
	0xA2, 0xF8, 0xA8, 0x5E, 0x1D, 0x1B, 0xD3, 0x23, 0xCB, 0x9B, 0xB0, 0xD5, 0x59, 0xF0, 0x3B, 0x09, 
	0x4D, 0xE4, 0x4A, 0x30, 0x7F, 0x89, 0x44, 0xA0, 0x7A, 0x3C, 0xEE, 0x0E, 0x66, 0xBF, 0xC9, 0x46, 
	0x77, 0x21, 0x86, 0x78, 0x6E, 0x8E, 0xE6, 0x99, 0x33, 0x2B, 0x0C, 0xEA, 0x42, 0x85, 0xD2, 0x8F,
	0x5F, 0x94, 0xDA, 0xAC, 0x76, 0xB7, 0x51, 0xBA, 0x0B, 0xD4, 0x91, 0x28, 0x72, 0xAE, 0xE7, 0xD6, 
	0xBD, 0x53, 0xA3, 0x4F, 0x9D, 0xC5, 0xCC, 0x5D, 0x18, 0x96, 0x02, 0xA5, 0xC2, 0x63, 0xF4, 0x00, 
	0x6B, 0xEB, 0x79, 0x95, 0x83, 0xA7, 0x8C, 0x9A, 0xAB, 0x8A, 0x4E, 0xD7, 0xDB, 0xCA, 0x62, 0x27, 
	0x0A, 0xD1, 0xDD, 0x48, 0xC6, 0x88, 0xB6, 0xA9, 0x41, 0x10, 0xFE, 0x55, 0xE0, 0xD9, 0x06, 0x29, 
	0x65, 0x6A, 0xED, 0xE5, 0x98, 0x52, 0xFF, 0x8D, 0x43, 0xF6, 0xA4, 0xCF, 0xA6, 0xF2, 0x97, 0x13, 
	0x12, 0x04, 0xFD, 0x25, 0x81, 0x87, 0xEF, 0x2F, 0x6C, 0x84, 0x2C, 0xAA, 0xA1, 0xAF, 0x36, 0xCD, 
	0x92, 0x0F, 0x2D, 0x67, 0x45, 0xE2, 0x64, 0xB3, 0x20, 0x50, 0x4B, 0xF3, 0x7B, 0x1F, 0x1C, 0x03, 
	0xC4, 0xC1, 0x16, 0x61, 0x6F, 0xC7, 0xBE, 0x05, 0xAD, 0x22, 0x34, 0xB2, 0x54, 0x37, 0xF7, 0xD0, 
	0xFA, 0x60, 0x8B, 0x14, 0x08, 0xBC, 0xEC, 0xBB, 0x26, 0x9C, 0x57, 0x32, 0x5A, 0x3F, 0x35, 0x6D, 
	0xC8, 0xC3, 0x69, 0x7C, 0x31, 0x58, 0xE3, 0x75, 0xD8, 0xE1, 0xC0, 0x9F, 0x11, 0xB5, 0x93, 0x56, 
	0xF5, 0x1E, 0xB1, 0x1A, 0x70, 0x3D, 0xFB, 0x82, 0xDC, 0xDF, 0x7E, 0x07, 0x15, 0x49, 0xFC, 0xB8, 

	/* Chaos;Head Trial 1 */
	0xE0, 0x60, 0x7F, 0x08, 0x06, 0xF0, 0x63, 0x6C, 0x8F, 0x36, 0xE8, 0x57, 0xCD, 0xA3, 0x13, 0x3F,
	0x62, 0x8D, 0x4A, 0x27, 0x3B, 0x29, 0x19, 0xFC, 0x1D, 0xA8, 0x4B, 0xD8, 0xBD, 0xD7, 0xC1, 0x28,
	0x91, 0xE7, 0x97, 0x4D, 0x0C, 0x0A, 0xC2, 0x12, 0xBA, 0x8A, 0xAF, 0xC4, 0x48, 0xEF, 0x2A, 0xF8,
	0x3C, 0xD3, 0x39, 0x2F, 0x6E, 0x78, 0x33, 0x9F, 0x69, 0x2B, 0xDD, 0xFD, 0x55, 0xAE, 0xB8, 0x35,
	0x66, 0x10, 0x75, 0x67, 0x5D, 0x7D, 0xD5, 0x88, 0x22, 0x1A, 0xFB, 0xD9, 0x31, 0x74, 0x2D, 0x7E,
	0x4E, 0x83, 0xC9, 0x9B, 0x65, 0xA6, 0x40, 0xA9, 0xFA, 0xC3, 0x80, 0x17, 0x61, 0x9D, 0xD6, 0xC5,
	0xAC, 0x42, 0x92, 0x3E, 0x8C, 0xB4, 0x53, 0x4C, 0x07, 0x85, 0xF1, 0x94, 0xB1, 0x52, 0xE3, 0xFF,
	0x5A, 0xDA, 0x68, 0x84, 0x72, 0x96, 0x7B, 0x89, 0x9A, 0x79, 0x3D, 0xC6, 0xCA, 0xB9, 0x51, 0x16,
	0xF9, 0xC0, 0xCC, 0x37, 0xB5, 0x77, 0xA5, 0x98, 0x30, 0x0F, 0xED, 0x44, 0xDF, 0xC8, 0xF5, 0x18,
	0x54, 0x59, 0xDC, 0xD4, 0x87, 0x41, 0xEE, 0x7C, 0x32, 0xE5, 0x93, 0xBE, 0x95, 0xE1, 0x86, 0x02,
	0x01, 0xF3, 0xEC, 0x14, 0x70, 0x76, 0xDE, 0x1E, 0x5B, 0x73, 0x1B, 0x99, 0x90, 0x9E, 0x25, 0xBC,
	0x81, 0xFE, 0x1C, 0x56, 0x34, 0xD1, 0xBB, 0xA2, 0x1F, 0x4F, 0x3A, 0xE2, 0x6A, 0x0E, 0x0B, 0xF2,
	0xB3, 0xB0, 0x05, 0x50, 0x5E, 0xB6, 0xAD, 0xF4, 0x9C, 0x11, 0x23, 0xA1, 0x43, 0x26, 0xE6, 0xCF,
	0xE9, 0x5F, 0x7A, 0x03, 0xF7, 0xAB, 0xDB, 0xAA, 0x15, 0x8B, 0x46, 0x21, 0x49, 0x2E, 0x24, 0x5C,
	0xB7, 0xB2, 0x58, 0x6B, 0x20, 0x47, 0xD2, 0x64, 0xC7, 0xD0, 0xBF, 0x8E, 0x00, 0xA4, 0x82, 0x45,
	0xE4, 0x0D, 0xA0, 0x09, 0x6F, 0x2C, 0xEA, 0x71, 0xCB, 0xCE, 0x6D, 0xF6, 0x04, 0x38, 0xEB, 0xA7,

	/* Chaos;Head Trial 2 */
	0xF1, 0x21, 0x30, 0x69, 0x67, 0x51, 0x24, 0x2D, 0x40, 0x97, 0xF9, 0x18, 0xDE, 0xB4, 0x74, 0x90,
	0x23, 0x4E, 0x0B, 0x88, 0x9C, 0x8A, 0x7A, 0x5D, 0x7E, 0xB9, 0x0C, 0xE9, 0xCE, 0xE8, 0x8E, 0x89,
	0xA2, 0xF8, 0xA8, 0x0E, 0x6D, 0x6B, 0xD3, 0x73, 0xCB, 0x4B, 0xB0, 0xD5, 0x09, 0xF0, 0x8B, 0x59,
	0x9D, 0xE4, 0x9A, 0x80, 0x2F, 0x39, 0x94, 0xA0, 0x2A, 0x8C, 0xEE, 0x5E, 0x16, 0xBF, 0xC9, 0x96,
	0x27, 0x71, 0x36, 0x28, 0x1E, 0x3E, 0xE6, 0x49, 0x83, 0x7B, 0x5C, 0xEA, 0x92, 0x35, 0xD2, 0x3F,
	0x0F, 0x44, 0xDA, 0xAC, 0x26, 0xB7, 0x01, 0xBA, 0x5B, 0xD4, 0x41, 0x78, 0x22, 0xAE, 0xE7, 0xD6,
	0xBD, 0x03, 0xA3, 0x9F, 0x4D, 0xC5, 0xCC, 0x0D, 0x68, 0x46, 0x52, 0xA5, 0xC2, 0x13, 0xF4, 0x50,
	0x1B, 0xEB, 0x29, 0x45, 0x33, 0xA7, 0x3C, 0x4A, 0xAB, 0x3A, 0x9E, 0xD7, 0xDB, 0xCA, 0x12, 0x77,
	0x5A, 0xD1, 0xDD, 0x98, 0xC6, 0x38, 0xB6, 0xA9, 0x91, 0x60, 0xFE, 0x05, 0xE0, 0xD9, 0x56, 0x79,
	0x15, 0x1A, 0xED, 0xE5, 0x48, 0x02, 0xFF, 0x3D, 0x93, 0xF6, 0xA4, 0xCF, 0xA6, 0xF2, 0x47, 0x63,
	0x62, 0x54, 0xFD, 0x75, 0x31, 0x37, 0xEF, 0x7F, 0x1C, 0x34, 0x7C, 0xAA, 0xA1, 0xAF, 0x86, 0xCD,
	0x42, 0x5F, 0x7D, 0x17, 0x95, 0xE2, 0x14, 0xB3, 0x70, 0x00, 0x9B, 0xF3, 0x2B, 0x6F, 0x6C, 0x53,
	0xC4, 0xC1, 0x66, 0x11, 0x1F, 0xC7, 0xBE, 0x55, 0xAD, 0x72, 0x84, 0xB2, 0x04, 0x87, 0xF7, 0xD0,
	0xFA, 0x10, 0x3B, 0x64, 0x58, 0xBC, 0xEC, 0xBB, 0x76, 0x4C, 0x07, 0x82, 0x0A, 0x8F, 0x85, 0x1D,
	0xC8, 0xC3, 0x19, 0x2C, 0x81, 0x08, 0xE3, 0x25, 0xD8, 0xE1, 0xC0, 0x4F, 0x61, 0xB5, 0x43, 0x06,
	0xF5, 0x6E, 0xB1, 0x6A, 0x20, 0x8D, 0xFB, 0x32, 0xDC, 0xDF, 0x2E, 0x57, 0x65, 0x99, 0xFC, 0xB8,

	/* Muramasa Trial */
	0xBF, 0x7F, 0x63, 0x11, 0x19, 0xCF, 0x76, 0x7C, 0x93, 0x49, 0xB1, 0x50, 0xFD, 0xD6, 0x06, 0x43,
	0x75, 0x9D, 0x3A, 0x20, 0x4B, 0x22, 0x02, 0xCC, 0x0D, 0xD1, 0x3B, 0x81, 0xED, 0x80, 0x2D, 0x21,
	0xA4, 0xB0, 0xA0, 0x3D, 0x1C, 0x1A, 0xF5, 0x05, 0xEA, 0x9A, 0xD3, 0xF7, 0x31, 0xB3, 0x2A, 0xC1,
	0x4C, 0x86, 0x42, 0x23, 0x7E, 0x61, 0x46, 0xA3, 0x72, 0x2B, 0x8D, 0xCD, 0x58, 0xDE, 0xE1, 0x48,
	0x79, 0x0F, 0x68, 0x70, 0x5D, 0x6D, 0x88, 0x91, 0x25, 0x0A, 0xCB, 0x82, 0x44, 0x67, 0xF4, 0x6E,
	0x3E, 0x96, 0xF2, 0xAB, 0x78, 0xD9, 0x3F, 0xD2, 0xCA, 0xF6, 0x9F, 0x00, 0x74, 0xAD, 0x89, 0xF8,
	0xDC, 0x35, 0xA5, 0x4E, 0x9C, 0xE7, 0xEB, 0x3C, 0x10, 0x98, 0xC4, 0xA7, 0xE4, 0x55, 0xB6, 0xC3,
	0x5A, 0x8A, 0x71, 0x97, 0x65, 0xA9, 0x6B, 0x92, 0xAA, 0x62, 0x4D, 0xF9, 0xFA, 0xE2, 0x54, 0x09,
	0xC2, 0xFF, 0xFC, 0x40, 0xE8, 0x60, 0xD8, 0xA1, 0x4F, 0x13, 0xBD, 0x37, 0x83, 0xF1, 0xC8, 0x01,
	0x57, 0x52, 0x8C, 0x87, 0x90, 0x34, 0xBE, 0x6C, 0x45, 0xB8, 0xA6, 0xEE, 0xA8, 0xB4, 0x99, 0x15,
	0x14, 0xC6, 0xBC, 0x07, 0x6F, 0x69, 0x8E, 0x0E, 0x5B, 0x66, 0x0B, 0xA2, 0xAF, 0xAE, 0x28, 0xEC,
	0x94, 0xCE, 0x0C, 0x59, 0x47, 0x84, 0x56, 0xD5, 0x03, 0x33, 0x4A, 0xB5, 0x7A, 0x1E, 0x1B, 0xC5,
	0xE6, 0xEF, 0x18, 0x5F, 0x5E, 0xE9, 0xDD, 0xC7, 0xAC, 0x04, 0x26, 0xD4, 0x36, 0x29, 0xB9, 0xF3,
	0xB2, 0x53, 0x6A, 0x16, 0xC0, 0xDB, 0x8B, 0xDA, 0x08, 0x9B, 0x39, 0x24, 0x32, 0x2E, 0x27, 0x5C,
	0xE0, 0xE5, 0x51, 0x7B, 0x2F, 0x30, 0x85, 0x77, 0xF0, 0x8F, 0xE3, 0x9E, 0x1F, 0xD7, 0x95, 0x38,
	0xB7, 0x1D, 0xDF, 0x12, 0x73, 0x2C, 0xBA, 0x64, 0xFB, 0xFE, 0x7D, 0xC9, 0x17, 0x41, 0xBB, 0xD0,

	/* Muramasa Retail */
	0x48, 0xE8, 0xD3, 0x11, 0x1D, 0x58, 0xEA, 0xE5, 0x23, 0xBD, 0x41, 0xC0, 0x86, 0x6A, 0x0A, 0xB3,
	0xE9, 0x26, 0xAE, 0x90, 0xBF, 0x92, 0x02, 0x55, 0x06, 0x61, 0xAF, 0xF1, 0x76, 0xF0, 0x96, 0x91,
	0x34, 0x40, 0x30, 0xA6, 0x15, 0x1E, 0x89, 0x09, 0x7E, 0x2E, 0x63, 0x8B, 0xA1, 0x43, 0x9E, 0x51,
	0xB5, 0xFA, 0xB2, 0x93, 0xE7, 0xD1, 0xBA, 0x33, 0xE2, 0x9F, 0xF6, 0x56, 0xCC, 0x67, 0x71, 0xBC,
	0xED, 0x08, 0xDC, 0xE0, 0xC6, 0xD6, 0xFC, 0x21, 0x99, 0x0E, 0x5F, 0xF2, 0xB4, 0xDB, 0x84, 0xD7,
	0xA7, 0x2A, 0x82, 0x3F, 0xEC, 0x6D, 0xA8, 0x62, 0x5E, 0x8A, 0x28, 0x00, 0xE4, 0x36, 0xFD, 0x8C,
	0x65, 0xA9, 0x39, 0xB7, 0x25, 0x7B, 0x7F, 0xA5, 0x10, 0x2C, 0x54, 0x3B, 0x74, 0xC9, 0x4A, 0x53,
	0xCE, 0xFE, 0xE1, 0x2B, 0xD9, 0x3D, 0xDF, 0x22, 0x3E, 0xD2, 0xB6, 0x8D, 0x8E, 0x72, 0xC4, 0x0D,
	0x52, 0x88, 0x85, 0xB0, 0x7C, 0xD0, 0x6C, 0x31, 0xB8, 0x13, 0x46, 0xAB, 0xF3, 0x81, 0x5C, 0x01,
	0xCB, 0xC2, 0xF5, 0xFB, 0x20, 0xA4, 0x47, 0xD5, 0xB9, 0x4C, 0x3A, 0x77, 0x3C, 0x44, 0x2D, 0x19,
	0x14, 0x5A, 0x45, 0x0B, 0xD8, 0xDD, 0xF7, 0x07, 0xCF, 0xDA, 0x0F, 0x32, 0x38, 0x37, 0x9C, 0x75,
	0x24, 0x57, 0x05, 0xCD, 0xBB, 0xF4, 0xCA, 0x69, 0x03, 0xA3, 0xBE, 0x49, 0xEE, 0x17, 0x1F, 0x59,
	0x7A, 0x78, 0x1C, 0xC8, 0xC7, 0x7D, 0x66, 0x5B, 0x35, 0x04, 0x9A, 0x64, 0xAA, 0x9D, 0x4D, 0x83,
	0x42, 0xC3, 0xDE, 0x1A, 0x50, 0x6F, 0xFF, 0x6E, 0x0C, 0x2F, 0xAD, 0x94, 0xA2, 0x97, 0x9B, 0xC5,
	0x70, 0x79, 0xC1, 0xEF, 0x98, 0xA0, 0xF9, 0xEB, 0x80, 0xF8, 0x73, 0x27, 0x18, 0x6B, 0x29, 0xAC,
	0x4B, 0x16, 0x68, 0x12, 0xE3, 0x95, 0x4E, 0xD4, 0x8F, 0x87, 0xE6, 0x5D, 0x1B, 0xB1, 0x4F, 0x60,

	/* Sumaga Retail */
	0xBF, 0x7F, 0x63, 0x11, 0x19, 0xCF, 0x76, 0x7C, 0x83, 0x39, 0xB1, 0x50, 0xFD, 0xD6, 0x06, 0x33,
	0x75, 0x8D, 0x4A, 0x20, 0x3B, 0x22, 0x02, 0xCC, 0x0D, 0xD1, 0x4B, 0xA1, 0xED, 0xA0, 0x2D, 0x21,
	0x94, 0xB0, 0x90, 0x4D, 0x1C, 0x1A, 0xF5, 0x05, 0xEA, 0x8A, 0xD3, 0xF7, 0x41, 0xB3, 0x2A, 0xC1,
	0x3C, 0xA6, 0x32, 0x23, 0x7E, 0x61, 0x36, 0x93, 0x72, 0x2B, 0xAD, 0xCD, 0x58, 0xDE, 0xE1, 0x38,
	0x79, 0x0F, 0x68, 0x70, 0x5D, 0x6D, 0xA8, 0x81, 0x25, 0x0A, 0xCB, 0xA2, 0x34, 0x67, 0xF4, 0x6E,
	0x4E, 0x86, 0xF2, 0x9B, 0x78, 0xD9, 0x4F, 0xD2, 0xCA, 0xF6, 0x8F, 0x00, 0x74, 0x9D, 0xA9, 0xF8,
	0xDC, 0x45, 0x95, 0x3E, 0x8C, 0xE7, 0xEB, 0x4C, 0x10, 0x88, 0xC4, 0x97, 0xE4, 0x55, 0xB6, 0xC3,
	0x5A, 0xAA, 0x71, 0x87, 0x65, 0x99, 0x6B, 0x82, 0x9A, 0x62, 0x3D, 0xF9, 0xFA, 0xE2, 0x54, 0x09,
	0xC2, 0xFF, 0xFC, 0x30, 0xE8, 0x60, 0xD8, 0x91, 0x3F, 0x13, 0xBD, 0x47, 0xA3, 0xF1, 0xC8, 0x01,
	0x57, 0x52, 0xAC, 0xA7, 0x80, 0x44, 0xBE, 0x6C, 0x35, 0xB8, 0x96, 0xEE, 0x98, 0xB4, 0x89, 0x15,
	0x14, 0xC6, 0xBC, 0x07, 0x6F, 0x69, 0xAE, 0x0E, 0x5B, 0x66, 0x0B, 0x92, 0x9F, 0x9E, 0x28, 0xEC,
	0x84, 0xCE, 0x0C, 0x59, 0x37, 0xA4, 0x56, 0xD5, 0x03, 0x43, 0x3A, 0xB5, 0x7A, 0x1E, 0x1B, 0xC5,
	0xE6, 0xEF, 0x18, 0x5F, 0x5E, 0xE9, 0xDD, 0xC7, 0x9C, 0x04, 0x26, 0xD4, 0x46, 0x29, 0xB9, 0xF3,
	0xB2, 0x53, 0x6A, 0x16, 0xC0, 0xDB, 0xAB, 0xDA, 0x08, 0x8B, 0x49, 0x24, 0x42, 0x2E, 0x27, 0x5C,
	0xE0, 0xE5, 0x51, 0x7B, 0x2F, 0x40, 0xA5, 0x77, 0xF0, 0xAF, 0xE3, 0x8E, 0x1F, 0xD7, 0x85, 0x48,
	0xB7, 0x1D, 0xDF, 0x12, 0x73, 0x2C, 0xBA, 0x64, 0xFB, 0xFE, 0x7D, 0xC9, 0x17, 0x31, 0xBB, 0xD0,



	/*
	* Nitro+ CHiRAL, the BL branch
	* Zoku Satsuriku no Django Retail
	*/
	0xDF, 0x5F, 0x6E, 0xF7, 0xF5, 0xEF, 0x52, 0x5B, 0x7E, 0x25, 0xD7, 0x46, 0xBC, 0x92, 0x02, 0x2E,
	0x51, 0x7C, 0x39, 0x16, 0x2A, 0x18, 0x08, 0xEB, 0x0C, 0x97, 0x3A, 0xC7, 0xAC, 0xC6, 0xB0, 0x17,
	0x80, 0xD6, 0x86, 0x3C, 0xFB, 0xF9, 0xB1, 0x01, 0xA9, 0x79, 0x9E, 0xB3, 0x37, 0xDE, 0x19, 0xE7,
	0x2B, 0xC2, 0x28, 0x1E, 0x5D, 0x67, 0x22, 0x8E, 0x58, 0x1A, 0xCC, 0xEC, 0x44, 0x9D, 0xA7, 0x24,
	0x55, 0x0F, 0x64, 0x56, 0x4C, 0x6C, 0xC4, 0x77, 0x11, 0x09, 0xEA, 0xC8, 0x20, 0x63, 0x1C, 0x6D,
	0x3D, 0x72, 0xB8, 0x8A, 0x54, 0x95, 0x3F, 0x98, 0xE9, 0xB2, 0x7F, 0x06, 0x50, 0x8C, 0xC5, 0xB4,
	0x9B, 0x31, 0x81, 0x2D, 0x7B, 0xA3, 0x42, 0x3B, 0xF6, 0x74, 0xE0, 0x83, 0xA0, 0x41, 0xD2, 0xEE,
	0x49, 0xC9, 0x57, 0x73, 0x61, 0x85, 0x6A, 0x78, 0x89, 0x68, 0x2C, 0xB5, 0xB9, 0xA8, 0x40, 0x05,
	0xE8, 0xBF, 0xBB, 0x26, 0xA4, 0x66, 0x94, 0x87, 0x2F, 0xFE, 0xDC, 0x33, 0xCE, 0xB7, 0xE4, 0x07,
	0x43, 0x48, 0xCB, 0xC3, 0x76, 0x30, 0xDD, 0x6B, 0x21, 0xD4, 0x82, 0xAD, 0x84, 0xD0, 0x75, 0xF1,
	0xF0, 0xE2, 0xDB, 0x03, 0x6F, 0x65, 0xCD, 0x0D, 0x4A, 0x62, 0x0A, 0x88, 0x8F, 0x8D, 0x14, 0xAB,
	0x70, 0xED, 0x0B, 0x45, 0x23, 0xC0, 0xAA, 0x91, 0x0E, 0x3E, 0x29, 0xD1, 0x59, 0xFD, 0xFA, 0xE1,
	0xA2, 0xAF, 0xF4, 0x4F, 0x4D, 0xA5, 0x9C, 0xE3, 0x8B, 0x00, 0x12, 0x90, 0x32, 0x15, 0xD5, 0xBE,
	0xD8, 0x4E, 0x69, 0xF2, 0xE6, 0x9A, 0xCA, 0x99, 0x04, 0x7A, 0x35, 0x10, 0x38, 0x1D, 0x13, 0x4B,
	0xA6, 0xA1, 0x47, 0x5A, 0x1F, 0x36, 0xC1, 0x53, 0xB6, 0xCF, 0xAE, 0x7D, 0xFF, 0x93, 0x71, 0x34,
	0xD3, 0xFC, 0x9F, 0xF8, 0x5E, 0x1B, 0xD9, 0x60, 0xBA, 0xBD, 0x5C, 0xE5, 0xF3, 0x27, 0xDA, 0x96,

	/* Zoku Satsuriku no Django Trial */
	0xEF, 0x5F, 0x6E, 0xF6, 0xF5, 0xDF, 0x52, 0x5B, 0x7E, 0x15, 0xE6, 0x47, 0xBD, 0xA2, 0x02, 0x1E,
	0x51, 0x7D, 0x39, 0x27, 0x1A, 0x28, 0x08, 0xDB, 0x0D, 0xA6, 0x3A, 0xC6, 0x9D, 0xC7, 0xB0, 0x26,
	0x80, 0xE7, 0x87, 0x3D, 0xFB, 0xF9, 0xB1, 0x01, 0x99, 0x79, 0xAE, 0xB3, 0x36, 0xEE, 0x29, 0xD6,
	0x1B, 0xC2, 0x18, 0x2E, 0x5C, 0x66, 0x12, 0x8E, 0x58, 0x2A, 0xCD, 0xDD, 0x44, 0xAC, 0x96, 0x14,
	0x55, 0x0F, 0x64, 0x57, 0x4D, 0x6D, 0xC4, 0x76, 0x21, 0x09, 0xDA, 0xC8, 0x10, 0x63, 0x2D, 0x6C,
	0x3C, 0x72, 0xB8, 0x8A, 0x54, 0xA5, 0x3F, 0xA8, 0xD9, 0xB2, 0x7F, 0x07, 0x50, 0x8D, 0xC5, 0xB4,
	0xAB, 0x31, 0x81, 0x1C, 0x7B, 0x93, 0x42, 0x3B, 0xF7, 0x74, 0xD0, 0x83, 0x90, 0x41, 0xE2, 0xDE,
	0x49, 0xC9, 0x56, 0x73, 0x61, 0x85, 0x6A, 0x78, 0x89, 0x68, 0x1D, 0xB5, 0xB9, 0x98, 0x40, 0x05,
	0xD8, 0xBF, 0xBB, 0x17, 0x94, 0x67, 0xA4, 0x86, 0x1F, 0xFE, 0xED, 0x33, 0xCE, 0xB6, 0xD4, 0x06,
	0x43, 0x48, 0xCB, 0xC3, 0x77, 0x30, 0xEC, 0x6B, 0x11, 0xE4, 0x82, 0x9C, 0x84, 0xE0, 0x75, 0xF1,
	0xF0, 0xD2, 0xEB, 0x03, 0x6F, 0x65, 0xCC, 0x0C, 0x4A, 0x62, 0x0A, 0x88, 0x8F, 0x8C, 0x24, 0x9B,
	0x70, 0xDC, 0x0B, 0x45, 0x13, 0xC0, 0x9A, 0xA1, 0x0E, 0x3E, 0x19, 0xE1, 0x59, 0xFC, 0xFA, 0xD1,
	0x92, 0x9F, 0xF4, 0x4F, 0x4C, 0x95, 0xAD, 0xD3, 0x8B, 0x00, 0x22, 0xA0, 0x32, 0x25, 0xE5, 0xBE,
	0xE8, 0x4E, 0x69, 0xF2, 0xD7, 0xAA, 0xCA, 0xA9, 0x04, 0x7A, 0x35, 0x20, 0x38, 0x2C, 0x23, 0x4B,
	0x97, 0x91, 0x46, 0x5A, 0x2F, 0x37, 0xC1, 0x53, 0xB7, 0xCF, 0x9E, 0x7C, 0xFF, 0xA3, 0x71, 0x34,
	0xE3, 0xFD, 0xAF, 0xF8, 0x5E, 0x2B, 0xE9, 0x60, 0xBA, 0xBC, 0x5D, 0xD5, 0xF3, 0x16, 0xEA, 0xA7,

	/* Lamento -Beyond the Void- Retail */
	0xDF, 0x5F, 0x6E, 0xF7, 0xF5, 0xEF, 0x52, 0x5B, 0x7E, 0x25, 0xD7, 0x46, 0xBC, 0x92, 0x02, 0x2E,
	0x51, 0x7C, 0x39, 0x16, 0x2A, 0x18, 0x08, 0xEB, 0x0C, 0x97, 0x3A, 0xC7, 0xAC, 0xC6, 0xB0, 0x17,
	0x80, 0xD6, 0x86, 0x3C, 0xFB, 0xF9, 0xB1, 0x01, 0xA9, 0x79, 0x9E, 0xB3, 0x37, 0xDE, 0x19, 0xE7,
	0x2B, 0xC2, 0x28, 0x1E, 0x5D, 0x67, 0x22, 0x8E, 0x58, 0x1A, 0xCC, 0xEC, 0x44, 0x9D, 0xA7, 0x24,
	0x55, 0x0F, 0x64, 0x56, 0x4C, 0x6C, 0xC4, 0x77, 0x11, 0x09, 0xEA, 0xC8, 0x20, 0x63, 0x1C, 0x6D,
	0x3D, 0x72, 0xB8, 0x8A, 0x54, 0x95, 0x3F, 0x98, 0xE9, 0xB2, 0x7F, 0x06, 0x50, 0x8C, 0xC5, 0xB4,
	0x9B, 0x31, 0x81, 0x2D, 0x7B, 0xA3, 0x42, 0x3B, 0xF6, 0x74, 0xE0, 0x83, 0xA0, 0x41, 0xD2, 0xEE,
	0x49, 0xC9, 0x57, 0x73, 0x61, 0x85, 0x6A, 0x78, 0x89, 0x68, 0x2C, 0xB5, 0xB9, 0xA8, 0x40, 0x05,
	0xE8, 0xBF, 0xBB, 0x26, 0xA4, 0x66, 0x94, 0x87, 0x2F, 0xFE, 0xDC, 0x33, 0xCE, 0xB7, 0xE4, 0x07,
	0x43, 0x48, 0xCB, 0xC3, 0x76, 0x30, 0xDD, 0x6B, 0x21, 0xD4, 0x82, 0xAD, 0x84, 0xD0, 0x75, 0xF1,
	0xF0, 0xE2, 0xDB, 0x03, 0x6F, 0x65, 0xCD, 0x0D, 0x4A, 0x62, 0x0A, 0x88, 0x8F, 0x8D, 0x14, 0xAB,
	0x70, 0xED, 0x0B, 0x45, 0x23, 0xC0, 0xAA, 0x91, 0x0E, 0x3E, 0x29, 0xD1, 0x59, 0xFD, 0xFA, 0xE1,
	0xA2, 0xAF, 0xF4, 0x4F, 0x4D, 0xA5, 0x9C, 0xE3, 0x8B, 0x00, 0x12, 0x90, 0x32, 0x15, 0xD5, 0xBE,
	0xD8, 0x4E, 0x69, 0xF2, 0xE6, 0x9A, 0xCA, 0x99, 0x04, 0x7A, 0x35, 0x10, 0x38, 0x1D, 0x13, 0x4B,
	0xA6, 0xA1, 0x47, 0x5A, 0x1F, 0x36, 0xC1, 0x53, 0xB6, 0xCF, 0xAE, 0x7D, 0xFF, 0x93, 0x71, 0x34,
	0xD3, 0xFC, 0x9F, 0xF8, 0x5E, 0x1B, 0xD9, 0x60, 0xBA, 0xBD, 0x5C, 0xE5, 0xF3, 0x27, 0xDA, 0x96,

	/* Lamento -Beyond the Void- Trial */
	0xDF, 0x5F, 0x6E, 0xF7, 0xF5, 0xEF, 0x52, 0x5B, 0x7E, 0x25, 0xD7, 0x46, 0xBC, 0x92, 0x02, 0x2E,
	0x51, 0x7C, 0x39, 0x16, 0x2A, 0x18, 0x08, 0xEB, 0x0C, 0x97, 0x3A, 0xC7, 0xAC, 0xC6, 0xB0, 0x17,
	0x80, 0xD6, 0x86, 0x3C, 0xFB, 0xF9, 0xB1, 0x01, 0xA9, 0x79, 0x9E, 0xB3, 0x37, 0xDE, 0x19, 0xE7,
	0x2B, 0xC2, 0x28, 0x1E, 0x5D, 0x67, 0x22, 0x8E, 0x58, 0x1A, 0xCC, 0xEC, 0x44, 0x9D, 0xA7, 0x24,
	0x55, 0x0F, 0x64, 0x56, 0x4C, 0x6C, 0xC4, 0x77, 0x11, 0x09, 0xEA, 0xC8, 0x20, 0x63, 0x1C, 0x6D,
	0x3D, 0x72, 0xB8, 0x8A, 0x54, 0x95, 0x3F, 0x98, 0xE9, 0xB2, 0x7F, 0x06, 0x50, 0x8C, 0xC5, 0xB4,
	0x9B, 0x31, 0x81, 0x2D, 0x7B, 0xA3, 0x42, 0x3B, 0xF6, 0x74, 0xE0, 0x83, 0xA0, 0x41, 0xD2, 0xEE,
	0x49, 0xC9, 0x57, 0x73, 0x61, 0x85, 0x6A, 0x78, 0x89, 0x68, 0x2C, 0xB5, 0xB9, 0xA8, 0x40, 0x05,
	0xE8, 0xBF, 0xBB, 0x26, 0xA4, 0x66, 0x94, 0x87, 0x2F, 0xFE, 0xDC, 0x33, 0xCE, 0xB7, 0xE4, 0x07,
	0x43, 0x48, 0xCB, 0xC3, 0x76, 0x30, 0xDD, 0x6B, 0x21, 0xD4, 0x82, 0xAD, 0x84, 0xD0, 0x75, 0xF1,
	0xF0, 0xE2, 0xDB, 0x03, 0x6F, 0x65, 0xCD, 0x0D, 0x4A, 0x62, 0x0A, 0x88, 0x8F, 0x8D, 0x14, 0xAB,
	0x70, 0xED, 0x0B, 0x45, 0x23, 0xC0, 0xAA, 0x91, 0x0E, 0x3E, 0x29, 0xD1, 0x59, 0xFD, 0xFA, 0xE1,
	0xA2, 0xAF, 0xF4, 0x4F, 0x4D, 0xA5, 0x9C, 0xE3, 0x8B, 0x00, 0x12, 0x90, 0x32, 0x15, 0xD5, 0xBE,
	0xD8, 0x4E, 0x69, 0xF2, 0xE6, 0x9A, 0xCA, 0x99, 0x04, 0x7A, 0x35, 0x10, 0x38, 0x1D, 0x13, 0x4B,
	0xA6, 0xA1, 0x47, 0x5A, 0x1F, 0x36, 0xC1, 0x53, 0xB6, 0xCF, 0xAE, 0x7D, 0xFF, 0x93, 0x71, 0x34,
	0xD3, 0xFC, 0x9F, 0xF8, 0x5E, 0x1B, 0xD9, 0x60, 0xBA, 0xBD, 0x5C, 0xE5, 0xF3, 0x27, 0xDA, 0x96,

	/* sweet pool Retail */
	0x7A, 0x3A, 0x23, 0xD1, 0xDF, 0x8A, 0x3C, 0x37, 0x43, 0xFF, 0x71, 0x10, 0xB8, 0x9C, 0xCC, 0xF3,
	0x3B, 0x48, 0x05, 0xE0, 0xF6, 0xE2, 0xC2, 0x87, 0xC8, 0x91, 0x06, 0x61, 0xA8, 0x60, 0xE8, 0xE1,
	0x54, 0x70, 0x50, 0x08, 0xD7, 0xD5, 0xBB, 0xCB, 0xA5, 0x45, 0x93, 0xBD, 0x01, 0x73, 0xE5, 0x81,
	0xF7, 0x6C, 0xF2, 0xE3, 0x39, 0x21, 0xFC, 0x53, 0x32, 0xE6, 0x68, 0x88, 0x1E, 0x99, 0xA1, 0xFE,
	0x3F, 0xCA, 0x2E, 0x30, 0x18, 0x28, 0x6E, 0x41, 0xEB, 0xC5, 0x86, 0x62, 0xF4, 0x2D, 0xB4, 0x29,
	0x09, 0x4C, 0xB2, 0x56, 0x3E, 0x9F, 0x0A, 0x92, 0x85, 0xBC, 0x4A, 0xC0, 0x34, 0x58, 0x6F, 0xBE,
	0x97, 0x0B, 0x5B, 0xF9, 0x47, 0xAD, 0xA6, 0x07, 0xD0, 0x4E, 0x84, 0x5D, 0xA4, 0x1B, 0x7C, 0x83,
	0x15, 0x65, 0x31, 0x4D, 0x2B, 0x5F, 0x26, 0x42, 0x55, 0x22, 0xF8, 0xBF, 0xB5, 0xA2, 0x14, 0xCF,
	0x82, 0xBA, 0xB7, 0xF0, 0xAE, 0x20, 0x9E, 0x51, 0xFA, 0xD3, 0x78, 0x0D, 0x63, 0xB1, 0x8E, 0xC1,
	0x1D, 0x12, 0x67, 0x6D, 0x40, 0x04, 0x79, 0x27, 0xFB, 0x7E, 0x5C, 0xA9, 0x5E, 0x74, 0x4F, 0xDB,
	0xD4, 0x8C, 0x77, 0xCD, 0x2A, 0x2F, 0x69, 0xC9, 0x16, 0x2C, 0xC6, 0x52, 0x5A, 0x59, 0xEE, 0xA7,
	0x44, 0x89, 0xC7, 0x1F, 0xFD, 0x64, 0x1C, 0x9B, 0xC3, 0x03, 0xF5, 0x7B, 0x35, 0xD9, 0xD6, 0x8B,
	0xAC, 0xAA, 0xDE, 0x1A, 0x19, 0xAF, 0x98, 0x8D, 0x57, 0xC4, 0xEC, 0x94, 0x0C, 0xEF, 0x7F, 0xB3,
	0x72, 0x13, 0x25, 0xDC, 0x80, 0x96, 0x66, 0x95, 0xCE, 0x46, 0x0F, 0xE4, 0x02, 0xE9, 0xED, 0x17,
	0xA0, 0xAB, 0x11, 0x36, 0xEA, 0x00, 0x6B, 0x3D, 0xB0, 0x6A, 0xA3, 0x49, 0xDA, 0x9D, 0x4B, 0x0E,
	0x7D, 0xD8, 0x9A, 0xD2, 0x33, 0xE7, 0x75, 0x24, 0xB6, 0xB9, 0x38, 0x8F, 0xDD, 0xF1, 0x76, 0x90, 

	/* Added after table was made */
	/* Sumaga Special Retail */
	0xA7, 0x67, 0x5A, 0x08, 0x01, 0xB7, 0x6D, 0x64, 0x8A, 0x31, 0xA8, 0x40, 0xE5, 0xCD, 0xFD, 0x3A,
	0x6C, 0x85, 0x22, 0x10, 0x33, 0x19, 0xF9, 0xB4, 0xF5, 0xC8, 0x23, 0x78, 0xD5, 0x70, 0x15, 0x18,
	0x9B, 0xA0, 0x90, 0x25, 0x04, 0x02, 0xEC, 0xFC, 0xD2, 0x82, 0xCA, 0xEE, 0x28, 0xAA, 0x12, 0xB8,
	0x34, 0x7D, 0x39, 0x1A, 0x66, 0x58, 0x3D, 0x9A, 0x69, 0x13, 0x75, 0xB5, 0x4F, 0xC6, 0xD8, 0x3F,
	0x61, 0xF7, 0x5F, 0x60, 0x45, 0x55, 0x7F, 0x88, 0x1C, 0xF2, 0xB3, 0x79, 0x3B, 0x5E, 0xEB, 0x56,
	0x26, 0x8D, 0xE9, 0x93, 0x6F, 0xC1, 0x27, 0xC9, 0xB2, 0xED, 0x87, 0xF0, 0x6B, 0x95, 0x71, 0xEF,
	0xC4, 0x2C, 0x9C, 0x36, 0x84, 0xDE, 0xD3, 0x24, 0x00, 0x8F, 0xBB, 0x9E, 0xDB, 0x4C, 0xAD, 0xBA,
	0x42, 0x72, 0x68, 0x8E, 0x5C, 0x91, 0x53, 0x89, 0x92, 0x59, 0x35, 0xE1, 0xE2, 0xD9, 0x4B, 0xF1,
	0xB9, 0xE7, 0xE4, 0x30, 0xDF, 0x50, 0xCF, 0x98, 0x37, 0x0A, 0xA5, 0x2E, 0x7A, 0xE8, 0xBF, 0xF8,
	0x4E, 0x49, 0x74, 0x7E, 0x80, 0x2B, 0xA6, 0x54, 0x3C, 0xAF, 0x9D, 0xD6, 0x9F, 0xAB, 0x81, 0x0C,
	0x0B, 0xBD, 0xA4, 0xFE, 0x57, 0x51, 0x76, 0xF6, 0x43, 0x5D, 0xF3, 0x99, 0x97, 0x96, 0x1F, 0xD4,
	0x8B, 0xB6, 0xF4, 0x41, 0x3E, 0x7B, 0x4D, 0xCC, 0xFA, 0x2A, 0x32, 0xAC, 0x62, 0x06, 0x03, 0xBC,
	0xDD, 0xD7, 0x0F, 0x47, 0x46, 0xD1, 0xC5, 0xBE, 0x94, 0xFB, 0x1D, 0xCB, 0x2D, 0x11, 0xA1, 0xEA,
	0xA9, 0x4A, 0x52, 0x0D, 0xB0, 0xC3, 0x73, 0xC2, 0xFF, 0x83, 0x21, 0x1B, 0x29, 0x16, 0x1E, 0x44,
	0xD0, 0xDC, 0x48, 0x63, 0x17, 0x20, 0x7C, 0x6E, 0xE0, 0x77, 0xDA, 0x86, 0x07, 0xCE, 0x8C, 0x2F,
	0xAE, 0x05, 0xC7, 0x09, 0x6A, 0x14, 0xA2, 0x5B, 0xE3, 0xE6, 0x65, 0xB1, 0x0E, 0x38, 0xA3, 0xC0,

	/* Demonbane The Best */
	0x5C, 0x1C, 0x09, 0xA5, 0xA1, 0x6C, 0x1D, 0x14, 0x39, 0xD1, 0x55, 0x20, 0x8A, 0xED, 0x9D, 0xD9,
	0x17, 0x3A, 0xC2, 0xB0, 0xD3, 0xB8, 0x98, 0x64, 0x9A, 0xE5, 0xC3, 0x75, 0xFA, 0x70, 0xBA, 0xB5,
	0x46, 0x50, 0x40, 0xCA, 0xA4, 0xA2, 0x87, 0x97, 0xF2, 0x32, 0xE9, 0x8E, 0xC5, 0x59, 0xB2, 0x65,
	0xD4, 0x7D, 0xD8, 0xB9, 0x1B, 0x05, 0xDD, 0x49, 0x18, 0xB3, 0x7A, 0x6A, 0x2F, 0xEB, 0xF5, 0xDF,
	0x11, 0x9C, 0x0F, 0x10, 0x2A, 0x0A, 0x7F, 0x35, 0xB7, 0x92, 0x63, 0x78, 0xD6, 0x0E, 0x86, 0x0B,
	0xCB, 0x3D, 0x88, 0x43, 0x1F, 0xE1, 0xCC, 0xE8, 0x62, 0x8D, 0x3C, 0x90, 0x16, 0x4A, 0x71, 0x8F,
	0xE4, 0xC7, 0x47, 0xDB, 0x34, 0xFE, 0xF3, 0xC4, 0xA0, 0x3F, 0x66, 0x4E, 0xF6, 0x27, 0x5D, 0x69,
	0x22, 0x72, 0x15, 0x3E, 0x07, 0x41, 0x03, 0x38, 0x42, 0x08, 0xDA, 0x81, 0x82, 0xF8, 0x26, 0x91,
	0x68, 0x8C, 0x84, 0xD0, 0xFF, 0x00, 0xEF, 0x45, 0xDC, 0xA9, 0x5A, 0xCE, 0x79, 0x85, 0x6F, 0x95,
	0x2E, 0x28, 0x74, 0x7E, 0x30, 0xC6, 0x5B, 0x04, 0xD7, 0x5F, 0x4D, 0xFB, 0x4F, 0x56, 0x31, 0xA7,
	0xA6, 0x6D, 0x54, 0x9E, 0x0C, 0x01, 0x7B, 0x9B, 0x23, 0x0D, 0x93, 0x48, 0x4C, 0x4B, 0xBF, 0xF4,
	0x36, 0x6B, 0x94, 0x21, 0xDE, 0x76, 0x2D, 0xE7, 0x99, 0xC9, 0xD2, 0x57, 0x12, 0xAB, 0xA3, 0x67,
	0xFD, 0xFC, 0xAF, 0x2C, 0x2B, 0xF1, 0xEA, 0x6E, 0x44, 0x96, 0xBD, 0xE6, 0xCD, 0xB1, 0x51, 0x89,
	0x58, 0x29, 0x02, 0xAD, 0x60, 0xE3, 0x73, 0xE2, 0x9F, 0x33, 0xC1, 0xB6, 0xC8, 0xBB, 0xBE, 0x24,
	0xF0, 0xF7, 0x25, 0x13, 0xBC, 0xC0, 0x77, 0x1E, 0x80, 0x7C, 0xF9, 0x3B, 0xAC, 0xEE, 0x37, 0xCF,
	0x5E, 0xAA, 0xEC, 0xA8, 0x19, 0xB4, 0x52, 0x06, 0x83, 0x8B, 0x1A, 0x61, 0xAE, 0xD5, 0x53, 0xE0,

	/* MuramasaAD */
	0xA1, 0xF1, 0x70, 0x5F, 0x5D, 0x01, 0xFA, 0xF6, 0x80, 0xCD, 0xAF, 0xEE, 0x37, 0x1A, 0x6A, 0xC0,
	0xF9, 0x87, 0xD4, 0xBE, 0xC5, 0xB3, 0x63, 0x06, 0x67, 0x1F, 0xD5, 0x4F, 0x27, 0x4E, 0xB7, 0xBF,
	0x92, 0xAE, 0x9E, 0xD7, 0x56, 0x54, 0x39, 0x69, 0x24, 0x84, 0x10, 0x3B, 0xDF, 0xA0, 0xB4, 0x0F,
	0xC6, 0x4A, 0xC3, 0xB0, 0xF8, 0x7F, 0xCA, 0x90, 0xF3, 0xB5, 0x47, 0x07, 0xEC, 0x18, 0x2F, 0xCC,
	0xFD, 0x61, 0x7C, 0xFE, 0xE7, 0x77, 0x4C, 0x8F, 0xB9, 0x64, 0x05, 0x43, 0xC2, 0x7B, 0x32, 0x78,
	0xD8, 0x8A, 0x33, 0x95, 0xFC, 0x1D, 0xD1, 0x13, 0x04, 0x3A, 0x81, 0x6E, 0xF2, 0x97, 0x4D, 0x3C,
	0x16, 0xD9, 0x99, 0xC8, 0x86, 0x2B, 0x25, 0xD6, 0x5E, 0x8C, 0x02, 0x9B, 0x22, 0xE9, 0xAA, 0x00,
	0xE4, 0x44, 0xFF, 0x8B, 0x79, 0x9D, 0x75, 0x83, 0x94, 0x73, 0xC7, 0x3D, 0x34, 0x23, 0xE2, 0x6D,
	0x03, 0x31, 0x36, 0xCE, 0x2C, 0x7E, 0x1C, 0x9F, 0xC1, 0x50, 0xA7, 0xDB, 0x40, 0x3F, 0x0C, 0x6F,
	0xEB, 0xE3, 0x46, 0x4B, 0x8E, 0xD2, 0xA8, 0x76, 0xC9, 0xAC, 0x9A, 0x28, 0x9C, 0xA2, 0x8D, 0x59,
	0x52, 0x0A, 0xA6, 0x6B, 0x71, 0x7D, 0x48, 0x68, 0xE5, 0x7A, 0x65, 0x93, 0x91, 0x98, 0xBC, 0x26,
	0x82, 0x08, 0x66, 0xED, 0xCB, 0x42, 0xEA, 0x19, 0x60, 0xD0, 0xC4, 0xA9, 0xF4, 0x58, 0x55, 0x09,
	0x2A, 0x21, 0x5C, 0xE1, 0xE8, 0x2D, 0x17, 0x0B, 0x96, 0x62, 0xBA, 0x12, 0xDA, 0xBD, 0xAD, 0x30,
	0xA3, 0xE0, 0x74, 0x5A, 0x0E, 0x15, 0x45, 0x14, 0x6C, 0x85, 0xDD, 0xB2, 0xD3, 0xB8, 0xBB, 0xE6,
	0x2E, 0x29, 0xEF, 0xF5, 0xB1, 0xDE, 0x49, 0xFB, 0x3E, 0x41, 0x20, 0x88, 0x51, 0x1B, 0x89, 0xDC,
	0xAB, 0x57, 0x11, 0x53, 0xF0, 0xB6, 0xA4, 0x72, 0x35, 0x38, 0xF7, 0x0D, 0x5B, 0xCF, 0xA5, 0x1E,

	/* Axanael + Trial */
	0x21, 0x71, 0xF0, 0xD8, 0xD6, 0x81, 0x73, 0x7C, 0x00, 0x46, 0x28, 0x67, 0xBD, 0x93, 0xE3, 0x40,
	0x7F, 0x0D, 0x5A, 0x37, 0x4B, 0x39, 0xE9, 0x8C, 0xED, 0x98, 0x5B, 0xC8, 0xAD, 0xC7, 0x3D, 0x38,
	0x12, 0x27, 0x17, 0x5D, 0xDC, 0xDA, 0xBF, 0xEF, 0xAA, 0x0A, 0x90, 0xB4, 0x58, 0x20, 0x3A, 0x88,
	0x4C, 0xC3, 0x49, 0x30, 0x7E, 0xF8, 0x43, 0x10, 0x79, 0x3B, 0xCD, 0x8D, 0x65, 0x9E, 0xA8, 0x45,
	0x76, 0xE1, 0xF5, 0x77, 0x6D, 0xFD, 0xC5, 0x08, 0x3F, 0xEA, 0x8B, 0xC9, 0x42, 0xF4, 0xB2, 0xFE,
	0x5E, 0x03, 0xB9, 0x1B, 0x75, 0x96, 0x51, 0x99, 0x8A, 0xB3, 0x01, 0xE7, 0x72, 0x1D, 0xC6, 0xB5,
	0x9C, 0x5F, 0x1F, 0x4E, 0x0C, 0xA4, 0xAB, 0x5C, 0xD7, 0x05, 0x82, 0x14, 0xA2, 0x6F, 0x23, 0x80,
	0x6A, 0xCA, 0x78, 0x04, 0xFF, 0x16, 0xFB, 0x09, 0x1A, 0xF9, 0x4D, 0xB6, 0xBA, 0xA9, 0x62, 0xE6,
	0x89, 0xB1, 0xBC, 0x47, 0xA5, 0xF7, 0x95, 0x18, 0x41, 0xD0, 0x2D, 0x54, 0xC0, 0xB8, 0x85, 0xE8,
	0x64, 0x69, 0xCC, 0xC4, 0x07, 0x52, 0x2E, 0xFC, 0x4F, 0x25, 0x13, 0xAE, 0x15, 0x22, 0x06, 0xDF,
	0xD2, 0x83, 0x2C, 0xE4, 0xF1, 0xF6, 0xCE, 0xEE, 0x6B, 0xF3, 0xEB, 0x19, 0x11, 0x1E, 0x35, 0xAC,
	0x02, 0x8E, 0xEC, 0x66, 0x44, 0xC2, 0x63, 0x9F, 0xE0, 0x50, 0x4A, 0x2F, 0x7A, 0xDE, 0xDB, 0x8F,
	0xA3, 0xA1, 0xD5, 0x61, 0x6E, 0xA6, 0x9D, 0x84, 0x1C, 0xE2, 0x33, 0x92, 0x53, 0x36, 0x26, 0xB0,
	0x29, 0x60, 0xFA, 0xD3, 0x87, 0x9B, 0xCB, 0x9A, 0xE5, 0x0B, 0x56, 0x32, 0x59, 0x3E, 0x34, 0x6C,
	0xA7, 0xAF, 0x68, 0x7B, 0x31, 0x57, 0xCF, 0x74, 0xB7, 0xC1, 0xA0, 0x0E, 0xD1, 0x94, 0x0F, 0x55,
	0x24, 0xDD, 0x91, 0xD9, 0x70, 0x3C, 0x2A, 0xF2, 0xBB, 0xBE, 0x7D, 0x86, 0xD4, 0x48, 0x2B, 0x97,

	/* Updated Kikokugai on N2System */
	0x21, 0xE1, 0xA0, 0x8D, 0x86, 0xF1, 0xE3, 0xE9, 0x00, 0xB6, 0x2D, 0xD7, 0x6A, 0x43, 0x93, 0xB0,
	0xEC, 0x0A, 0xCF, 0x37, 0xB8, 0x3E, 0x9E, 0xF9, 0x9A, 0x4D, 0xC8, 0x7D, 0x5A, 0x77, 0x3A, 0x3D, 
	0x12, 0x27, 0x17, 0xCA, 0x89, 0x8F, 0x6C, 0x9C, 0x5F, 0x0F, 0x40, 0x64, 0xCD, 0x20, 0x3F, 0xFD, 
	0xB9, 0x73, 0xBE, 0x30, 0xEB, 0xAD, 0xB3, 0x10, 0xEE, 0x38, 0x7A, 0xFA, 0xD5, 0x4B, 0x5D, 0xB5, 
	0xE6, 0x91, 0xA5, 0xE7, 0xDA, 0xAA, 0x75, 0x0D, 0x3C, 0x9F, 0xF8, 0x7E, 0xB2, 0xA4, 0x62, 0xAB, 
	0xCB, 0x03, 0x6E, 0x18, 0xE5, 0x46, 0xC1, 0x4E, 0xFF, 0x63, 0x01, 0x97, 0xE2, 0x1A, 0x76, 0x65, 
	0x49, 0xCC, 0x1C, 0xBB, 0x09, 0x54, 0x58, 0xC9, 0x87, 0x05, 0xF2, 0x14, 0x52, 0xDC, 0x23, 0xF0, 
	0xDF, 0x7F, 0xED, 0x04, 0xAC, 0x16, 0xA8, 0x0E, 0x1F, 0xAE, 0xBA, 0x66, 0x6F, 0x5E, 0xD2, 0x96, 
	0xFE, 0x61, 0x69, 0xB7, 0x55, 0xA7, 0x45, 0x1D, 0xB1, 0x80, 0x2A, 0xC4, 0x70, 0x6D, 0xF5, 0x9D, 
	0xD4, 0xDE, 0x79, 0x74, 0x07, 0xC2, 0x2B, 0xA9, 0xBC, 0x25, 0x13, 0x5B, 0x15, 0x22, 0x06, 0x8C, 
	0x82, 0xF3, 0x29, 0x94, 0xA1, 0xA6, 0x7B, 0x9B, 0xD8, 0xA3, 0x98, 0x1E, 0x11, 0x1B, 0x35, 0x59, 
	0x02, 0xFB, 0x99, 0xD6, 0xB4, 0x72, 0xD3, 0x4C, 0x90, 0xC0, 0xBF, 0x2C, 0xEF, 0x8B, 0x88, 0xFC, 
	0x53, 0x51, 0x85, 0xD1, 0xDB, 0x56, 0x4A, 0xF4, 0x19, 0x92, 0x33, 0x42, 0xC3, 0x36, 0x26, 0x60, 
	0x2E, 0xD0, 0xAF, 0x83, 0xF7, 0x48, 0x78, 0x4F, 0x95, 0x08, 0xC6, 0x32, 0xCE, 0x3B, 0x34, 0xD9, 
	0x57, 0x5C, 0xDD, 0xE8, 0x31, 0xC7, 0x7C, 0xE4, 0x67, 0x71, 0x50, 0x0B, 0x81, 0x44, 0x0C, 0xC5, 
	0x24, 0x8A, 0x41, 0x8E, 0xE0, 0x39, 0x2F, 0xA2, 0x68, 0x6B, 0xEA, 0xF6, 0x84, 0xBD, 0x28, 0x47,

	/* Sonicomi Trial 2 */
	0x21, 0x71, 0xD0, 0xB8, 0xBC, 0x81, 0x73, 0x7F, 0x00, 0xEC, 0x28, 0x6D, 0x95, 0x43, 0xC3, 0xE0,
	0x77, 0x05, 0xFA, 0x3D, 0xEE, 0x39, 0xC9, 0x8F, 0xC5, 0x48, 0xFE, 0xA8, 0x55, 0xAD, 0x35, 0x38,
	0x12, 0x2D, 0x1D, 0xF5, 0xBF, 0xBA, 0x97, 0xC7, 0x5A, 0x0A, 0x40, 0x94, 0xF8, 0x20, 0x3A, 0x88,
	0xEF, 0xA3, 0xE9, 0x30, 0x76, 0xD8, 0xE3, 0x10, 0x79, 0x3E, 0xA5, 0x85, 0x6B, 0x46, 0x58, 0xEB,
	0x7C, 0xC1, 0xDB, 0x7D, 0x65, 0xD5, 0xAB, 0x08, 0x37, 0xCA, 0x8E, 0xA9, 0xE2, 0xD4, 0x92, 0xD6,
	0xF6, 0x03, 0x99, 0x1E, 0x7B, 0x4C, 0xF1, 0x49, 0x8A, 0x93, 0x01, 0xCD, 0x72, 0x15, 0xAC, 0x9B,
	0x4F, 0xF7, 0x17, 0xE6, 0x0F, 0x54, 0x5E, 0xFF, 0xBD, 0x0B, 0x82, 0x14, 0x52, 0x67, 0x23, 0x80,
	0x6A, 0xAA, 0x78, 0x04, 0xD7, 0x1C, 0xDE, 0x09, 0x1A, 0xD9, 0xE5, 0x9C, 0x9A, 0x59, 0x62, 0xCC,
	0x89, 0x91, 0x9F, 0xED, 0x5B, 0xDD, 0x4B, 0x18, 0xE1, 0xB0, 0x25, 0xF4, 0xA0, 0x98, 0x8B, 0xC8,
	0x64, 0x69, 0xAF, 0xA4, 0x0D, 0xF2, 0x26, 0xDF, 0xE7, 0x2B, 0x13, 0x56, 0x1B, 0x22, 0x0C, 0xB7,
	0xB2, 0x83, 0x2F, 0xC4, 0xD1, 0xDC, 0xA6, 0xC6, 0x6E, 0xD3, 0xCE, 0x19, 0x11, 0x16, 0x3B, 0x5F,
	0x02, 0x86, 0xCF, 0x6C, 0xE4, 0xA2, 0x63, 0x47, 0xC0, 0xF0, 0xEA, 0x27, 0x7A, 0xB6, 0xBE, 0x87,
	0x53, 0x51, 0xBB, 0x61, 0x66, 0x5C, 0x45, 0x84, 0x1F, 0xC2, 0x33, 0x42, 0xF3, 0x3C, 0x2C, 0x90,
	0x29, 0x60, 0xDA, 0xB3, 0x8D, 0x4E, 0xAE, 0x4A, 0xCB, 0x0E, 0xFC, 0x32, 0xF9, 0x36, 0x34, 0x6F,
	0x5D, 0x57, 0x68, 0x7E, 0x31, 0xFD, 0xA7, 0x74, 0x9D, 0xA1, 0x50, 0x06, 0xB1, 0x44, 0x07, 0xFB,
	0x24, 0xB5, 0x41, 0xB9, 0x70, 0x3F, 0x2A, 0xD2, 0x9E, 0x96, 0x75, 0x8C, 0xB4, 0xE8, 0x2E, 0x4D,

	/* Sumaga 3% Trial */	
	0xEF, 0x6F, 0x70, 0x08, 0x06, 0xFF, 0x63, 0x6C, 0x80, 0x36, 0xE8, 0x57, 0xCD, 0xA3, 0x13, 0x30,
	0x62, 0x8D, 0x4A, 0x27, 0x3B, 0x29, 0x19, 0xFC, 0x1D, 0xA8, 0x4B, 0xD8, 0xBD, 0xD7, 0x2D, 0x28,
	0x91, 0xE7, 0x97, 0x4D, 0x0C, 0x0A, 0xC2, 0x12, 0xBA, 0x8A, 0xA0, 0xC4, 0x48, 0xE0, 0x2A, 0xF8,
	0x3C, 0xD3, 0x39, 0x20, 0x6E, 0x78, 0x33, 0x90, 0x69, 0x2B, 0xDD, 0xFD, 0x55, 0xAE, 0xB8, 0x35,
	0x66, 0x1F, 0x75, 0x67, 0x5D, 0x7D, 0xD5, 0x88, 0x22, 0x1A, 0xFB, 0xD9, 0x31, 0x74, 0xC1, 0x7E,
	0x4E, 0x83, 0xC9, 0x9B, 0x65, 0xA6, 0x4F, 0xA9, 0xFA, 0xC3, 0x8F, 0x17, 0x61, 0x9D, 0xD6, 0xC5,
	0xAC, 0x42, 0x92, 0x3E, 0x8C, 0xB4, 0xBB, 0x4C, 0x07, 0x85, 0xF1, 0x94, 0xB1, 0x52, 0xE3, 0xF0,
	0x5A, 0xDA, 0x68, 0x84, 0x72, 0x96, 0x7B, 0x89, 0x9A, 0x79, 0x3D, 0xC6, 0xCA, 0xB9, 0x51, 0x16,
	0xF9, 0xCF, 0xCC, 0x37, 0xB5, 0x77, 0xA5, 0x98, 0x3F, 0x00, 0xED, 0x44, 0xD0, 0xC8, 0xF5, 0x18,
	0x54, 0x59, 0xDC, 0xD4, 0x87, 0x41, 0xEE, 0x7C, 0x32, 0xE5, 0x93, 0xBE, 0x95, 0xE1, 0x86, 0x02,
	0x01, 0xF3, 0xEC, 0x14, 0x7F, 0x76, 0xDE, 0x1E, 0x5B, 0x73, 0x1B, 0x99, 0x9F, 0x9E, 0x25, 0xBC,
	0x81, 0xFE, 0x1C, 0x56, 0x34, 0xD1, 0x53, 0xA2, 0x10, 0x40, 0x3A, 0xE2, 0x6A, 0x0E, 0x0B, 0xF2,
	0xB3, 0xBF, 0x05, 0x5F, 0x5E, 0xB6, 0xAD, 0xF4, 0x9C, 0x11, 0x23, 0xA1, 0x43, 0x26, 0xE6, 0xC0,
	0xE9, 0x50, 0x7A, 0x03, 0xF7, 0xAB, 0xDB, 0xAA, 0x15, 0x8B, 0x46, 0x21, 0x49, 0x2E, 0x24, 0x5C,
	0xB7, 0xB2, 0x58, 0x6B, 0x2F, 0x47, 0xD2, 0x64, 0xC7, 0xDF, 0xB0, 0x8E, 0x0F, 0xA4, 0x82, 0x45,
	0xE4, 0x0D, 0xAF, 0x09, 0x60, 0x2C, 0xEA, 0x71, 0xCB, 0xCE, 0x6D, 0xF6, 0x04, 0x38, 0xEB, 0xA7,	
	
	/* Sonicomi Version 1.0 */
	0x91, 0xD1, 0x10, 0xCB, 0xCF, 0xE1, 0xD6, 0xD5, 0x70, 0x2F, 0x9B, 0x63, 0xF8, 0x46, 0x06, 0x20,
	0xDA, 0x78, 0x3D, 0xA3, 0x24, 0xAC, 0x0C, 0xE5, 0x08, 0x4B, 0x34, 0xBB, 0x58, 0xB3, 0xA8, 0xAB, 
	0x82, 0x93, 0x83, 0x38, 0xC5, 0xCD, 0xFA, 0x0A, 0x5D, 0x7D, 0x40, 0xF7, 0x3B, 0x90, 0xAD, 0xEB, 
	0x25, 0xB6, 0x2C, 0xA0, 0xD9, 0x1B, 0x26, 0x80, 0xDC, 0xA4, 0xB8, 0xE8, 0x6E, 0x49, 0x5B, 0x2E, 
	0xDF, 0x01, 0x1E, 0xD3, 0x68, 0x18, 0xBE, 0x7B, 0xAA, 0x0D, 0xE4, 0xBC, 0x22, 0x17, 0xF2, 0x19, 
	0x39, 0x76, 0xFC, 0x84, 0xDE, 0x4F, 0x31, 0x4C, 0xED, 0xF6, 0x71, 0x03, 0xD2, 0x88, 0xBF, 0xFE, 
	0x45, 0x3A, 0x8A, 0x29, 0x75, 0x57, 0x54, 0x35, 0xC3, 0x7E, 0xE2, 0x87, 0x52, 0x6A, 0x96, 0xE0, 
	0x6D, 0xBD, 0xDB, 0x77, 0x1A, 0x8F, 0x14, 0x7C, 0x8D, 0x1C, 0x28, 0xFF, 0xFD, 0x5C, 0x62, 0x0F, 
	0xEC, 0xF1, 0xF5, 0x23, 0x5E, 0x13, 0x4E, 0x8B, 0x21, 0xC0, 0x98, 0x37, 0xB0, 0xFB, 0xEE, 0x0B, 
	0x67, 0x6C, 0xB5, 0xB7, 0x73, 0x32, 0x99, 0x15, 0x2A, 0x9E, 0x86, 0x59, 0x8E, 0x92, 0x7F, 0xCA, 
	0xC2, 0xE6, 0x95, 0x07, 0x11, 0x1F, 0xB9, 0x09, 0x64, 0x16, 0x04, 0x8C, 0x81, 0x89, 0xAE, 0x55, 
	0x72, 0xE9, 0x05, 0x6F, 0x27, 0xB2, 0x66, 0x4A, 0x00, 0x30, 0x2D, 0x9A, 0xDD, 0xC9, 0xC4, 0xEA, 
	0x56, 0x51, 0xCE, 0x61, 0x69, 0x5F, 0x48, 0xE7, 0x85, 0x02, 0xA6, 0x42, 0x36, 0xAF, 0x9F, 0xF0, 
	0x9C, 0x60, 0x1D, 0xC6, 0xE3, 0x44, 0xB4, 0x4D, 0x0E, 0x74, 0x3F, 0xA2, 0x3C, 0xA9, 0xA7, 0x65, 
	0x53, 0x5A, 0x6B, 0xD4, 0xA1, 0x33, 0xBA, 0xD7, 0xF3, 0xB1, 0x50, 0x79, 0xC1, 0x47, 0x7A, 0x3E, 
	0x97, 0xC8, 0x41, 0xCC, 0xD0, 0xA5, 0x9D, 0x12, 0xF4, 0xF9, 0xD8, 0xEF, 0xC7, 0x2B, 0x94, 0x43,
	
	/* Guilty Crown Lost Xmas Trailer */
	0xC8, 0x38, 0xF3, 0xB9, 0xBC, 0x48, 0x35, 0x30, 0x83, 0x5C, 0xC9, 0x0A, 0x61, 0x15, 0xD5, 0x53,
	0x3F, 0x81, 0xA7, 0xEA, 0x52, 0xE6, 0xD6, 0x40, 0xD1, 0x19, 0xA2, 0x79, 0x21, 0x7A, 0xE1, 0xE9,
	0x9D, 0xCA, 0x9A, 0xA1, 0xB0, 0xB7, 0x6F, 0xDF, 0x27, 0x87, 0x13, 0x6B, 0xA9, 0xC3, 0xE7, 0x49,
	0x50, 0x75, 0x56, 0xE3, 0x3E, 0xF9, 0x55, 0x93, 0x36, 0xE2, 0x71, 0x41, 0x04, 0x1E, 0x29, 0x54,
	0x3C, 0xD8, 0xF4, 0x3A, 0x01, 0xF1, 0x74, 0x89, 0xEF, 0xD7, 0x42, 0x76, 0x5D, 0xFB, 0x6D, 0xFE,
	0xAE, 0x85, 0x66, 0x92, 0x34, 0x1C, 0xA8, 0x16, 0x47, 0x65, 0x88, 0xDA, 0x3D, 0x91, 0x7C, 0x64,
	0x10, 0xAF, 0x9F, 0x5E, 0x80, 0x2B, 0x22, 0xA0, 0xBA, 0x84, 0x4D, 0x9B, 0x2D, 0x0F, 0xC5, 0x43,
	0x07, 0x77, 0x39, 0x8B, 0xFF, 0x9C, 0xF2, 0x86, 0x97, 0xF6, 0x51, 0x6C, 0x67, 0x26, 0x0D, 0xDC,
	0x46, 0x68, 0x60, 0x5A, 0x24, 0xFA, 0x14, 0x99, 0x58, 0xB3, 0xC1, 0xAB, 0x73, 0x69, 0x44, 0xD9,
	0x0B, 0x06, 0x70, 0x7B, 0x8A, 0xAD, 0xCE, 0xF0, 0x5F, 0xC4, 0x95, 0x2E, 0x94, 0xCD, 0x8C, 0xBF,
	0xBD, 0x45, 0xC0, 0xDB, 0xF8, 0xFC, 0x7E, 0xDE, 0x02, 0xF5, 0xD2, 0x96, 0x98, 0x9E, 0xE4, 0x20,
	0x8D, 0x4E, 0xD0, 0x0C, 0x5B, 0x7D, 0x05, 0x1F, 0xD3, 0xA3, 0x57, 0xCF, 0x37, 0xBE, 0xB2, 0x4F,
	0x25, 0x28, 0xB4, 0x08, 0x0E, 0x2C, 0x11, 0x4B, 0x90, 0xDD, 0xE5, 0x1D, 0xA5, 0xEC, 0xCC, 0x63,
	0xC6, 0x03, 0xF7, 0xB5, 0x4A, 0x12, 0x72, 0x17, 0xD4, 0x82, 0xAC, 0xED, 0xA6, 0xEE, 0xEB, 0x00,
	0x2A, 0x2F, 0x09, 0x32, 0xE8, 0xAA, 0x7F, 0x3B, 0x6A, 0x78, 0x23, 0x8E, 0xB8, 0x1B, 0x8F, 0xA4,
	0xCB, 0xB1, 0x18, 0xB6, 0x33, 0xE0, 0xC7, 0xFD, 0x62, 0x6E, 0x31, 0x4C, 0xBB, 0x59, 0xC2, 0x1A,

	0
};
