#include "xr_alloc.h"
#include "lj_def.h"
#include "lj_arch.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef long (*PNTAVM)(HANDLE handle, void **addr, ULONG zbits,
		       size_t *size, ULONG alloctype, ULONG prot);
extern PNTAVM ntavm;
/* Number of top bits of the lower 32 bits of an address that must be zero.
** Apparently 0 gives us full 64 bit addresses and 1 gives us the lower 2GB.
*/
#define NTAVM_ZEROBITS		1

#define MAX_SIZE_T		(~(size_t)0)
#define MFAIL			((void *)(MAX_SIZE_T))

// Луаджит выделяет память кусками, кратными 128К
// Поэтому сделаю два пула по эти размеры
#define CHUNK_SIZE (64 * 1024)
#define CHUNK_COUNT 4096

#define CHUNKS_FROM_SIZE(x) ((x + CHUNK_SIZE - 1) / CHUNK_SIZE)

static int inited = 0;
void* g_heap;
char g_heapMap[CHUNK_COUNT + 1];
char* g_firstFreeChunk;
char* find_free(int size);

//#define DEBUG_MEM
#ifdef DEBUG_MEM
static char buf[100];
void dump_map(void* ptr, size_t size, char c);
#endif

void XR_INIT()
{
	if (inited)
		return;
	g_heap = NULL;
	size_t size = CHUNK_SIZE * CHUNK_COUNT;
	long st = ntavm(INVALID_HANDLE_VALUE, &g_heap, NTAVM_ZEROBITS, &size,
	                MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	for (int i = 0; i < CHUNK_COUNT; i++)
		g_heapMap[i] = 'x';
	g_heapMap[CHUNK_COUNT] = 0;
	g_firstFreeChunk = g_heapMap;

#ifdef DEBUG_MEM	
	sprintf(buf, "XR_INIT create_block %p result=%X\r\n", g_heap, st);
	OutputDebugString(buf);
#endif
	inited = 1;
}

void* XR_MMAP(size_t size)
{
#ifdef DEBUG_MEM
	sprintf(buf, "XR_MMAP(%Iu)", size);
	OutputDebugString(buf);
#endif
	int chunks = CHUNKS_FROM_SIZE(size);
	char* s = find_free(chunks);
	void* ptr = MFAIL;
	if (s != NULL) {
		ptr = (char*)g_heap + CHUNK_SIZE * (s - g_heapMap);
		for (int i = 0; i < chunks; i++)
			s[i] = 'a';
		if (s == g_firstFreeChunk)
			g_firstFreeChunk = find_free(1);
	}
#ifdef DEBUG_MEM
	sprintf(buf, " ptr=%p chunks %d\r\n", ptr, chunks);
	OutputDebugString(buf);
	dump_map(ptr, size, 'U');
#endif
	return ptr;
}

// Судя по комментарию к CALL_MUNMAP, луаджит может объединять выделенные ему чанки
// и освобождать их как один. Надеюсь он не слепит вместе чанки из разных пулов
void XR_DESTROY(void* ptr, size_t size)
{
#ifdef DEBUG_MEM
	sprintf(buf, "XR_DESTROY(ptr=%p, size=%Iu)", ptr, size);
	OutputDebugString(buf);
#endif
	char* s = g_heapMap + ((char*)ptr - (char*)g_heap) / CHUNK_SIZE;
	int count = CHUNKS_FROM_SIZE(size);
	for (int i = 0; i < count; i++)
		s[i] = 'x';
	if (s < g_firstFreeChunk || !g_firstFreeChunk)
		g_firstFreeChunk = s;
#ifdef DEBUG_MEM	
	dump_map(ptr, size, 'X');
#endif
}

// Находит подряд идущие свободные чанки количеством size, начиная с первого свободного
// Возвращает указатель на группу из heapMap или NULL, если найти не удалось
char* find_free(int size)
{
	char* p = g_firstFreeChunk;
	if (!p) return NULL;

	int count = 0;
	while (*p != '\0') {
		if (*p == 'x')
			count++;
		else
			count = 0;
		p++;
		if (count >= size)
			return p - count;
	}
	return NULL;
}

static 	char temp[1025];
void dump_map(void* ptr, size_t size, char c)
{
#ifdef DEBUG_MEM
	OutputDebugString("heap:\r\n|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------|-------\r\n");
	strcpy(temp, g_heapMap);
	char *cur = temp + ((char*)ptr - (char*)g_heap) / CHUNK_SIZE;
	for (int i = 0; i < size / CHUNK_SIZE; i++)
		cur[i] = c;
	
	for (int i = 0; i < 8; i++)
	{
		char a = temp[i * 128 + 128];
		temp[i * 128 + 128] = '\0';
		OutputDebugString(temp + i * 128);
		temp[i * 128 + 128] = a;
		OutputDebugString("\r\n");
	}
	OutputDebugString("--------------------------------------------------------------------------------------------------------------------------------\r\n");
#endif
}