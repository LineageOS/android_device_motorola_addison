#include "SkBitmap.h"
#include "SkColorTable.h"

struct LockRec {
    LockRec() : fPixels(NULL), fColorTable(NULL) {}

    void*           fPixels;
    SkColorTable*   fColorTable;
    size_t          fRowBytes;

    void zero() { sk_bzero(this, sizeof(*this)); }

    bool isZero() const {
        return NULL == fPixels && NULL == fColorTable && 0 == fRowBytes;
    }
};

extern "C" void _ZN8SkBitmap14tryAllocPixelsEPNS_9AllocatorE(SkBitmap::Allocator* allocator);

extern "C" void _ZN8SkBitmap14tryAllocPixelsEPNS_9AllocatorEP12SkColorTable(SkBitmap::Allocator* allocator, SkColorTable* ctable){
    return _ZN8SkBitmap14tryAllocPixelsEPNS_9AllocatorE(allocator);
}

extern "C" bool _ZNK8SkBitmap10lockPixelsEv(LockRec* rec){
    return true;
}

extern "C" void _ZNK8SkBitmap12unlockPixelsEv(){
}

extern "C" void _ZN14SkImageEncoder10EncodeFileEPKcRK8SkBitmapNS_4TypeEi(){
}
