// Inigo Quilez - 2020
// The MIT License: Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// This file is where all the EFI related stuff is, including the main entry point. All that happens here
// is that the graphics mode of the screen is queried, a backbuffer is allocated for rendering, a timer
// callback is created, the demo is inited, and we enter the render loop. I also initcialize the mlibc.h.cpp
// module which I created to provide basic libc functions (memset, malloc, fsqrt, fsin, fract, etc) that
// otherwise we no longer have available. It also helps decouple EFI form the demo stuff, which is in
// demo.h.cpp
//
// To install in an actual USB flash drive, copy the generated image/efi/boot/bootx64.efi file and copy it to your
// USB stick as /efi/boot/bootx64.efi.
//
// Some EFI documentation:
//    https://edk2-docs.gitbook.io/edk-ii-uefi-driver-writer-s-guide/5_uefi_services/readme.2/527_gettime
//    https://www.intel.com/content/dam/doc/guide/efi-driver-writers-v1-10-guide.pdf
//

#include <efi.h>

#include "mlibc.h"
#include "demo.h"

extern "C" int _fltused = 0;

static EFI_GRAPHICS_OUTPUT_PROTOCOL* iGfxInit(EFI_SYSTEM_TABLE* sys)
{
    EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    // get from ConsoleOutHandle
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gfx = nullptr;
    EFI_STATUS sta = sys->BootServices->HandleProtocol(sys->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID**)&gfx);
    if (EFI_ERROR(sta))
    {
        // try locating directly
        sta = sys->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gfx);
        if (EFI_ERROR(sta) || gfx == NULL)
        {
            // try locating by handle
            UINTN num = 0;
            EFI_HANDLE* HandleBuffer = nullptr;
            sta = sys->BootServices->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, nullptr, &num, &HandleBuffer);
            if (EFI_ERROR(sta))
            {
                gfx = nullptr;
            }
            else
            {
                for (int i = 0; i < num; i++)
                {
                    sta = sys->BootServices->HandleProtocol(HandleBuffer[i], &gEfiGraphicsOutputProtocolGuid, (void**)&gfx);
                    if (!EFI_ERROR(sta))
                        break;
                }
                sys->BootServices->FreePool(HandleBuffer);
            }
        }
    }

    return gfx;
}

static void iTimerHandler(EFI_EVENT evt, void* context)
{
    uint64_t* iTimeCounter = (uint64_t*)context;
    (*iTimeCounter)++;
}

static void* iMalloc(void* sys, uint64_t amount)
{
#if 1
    void* res = nullptr;
    EFI_STATUS status = ((EFI_SYSTEM_TABLE*)sys)->BootServices->AllocatePool(EfiBootServicesData, amount, &res);
    if (EFI_ERROR(status))
        return nullptr;
    return res;
#else
    EFI_PHYSICAL_ADDRESS physicalRes;
    UINTN pages = EFI_SIZE_TO_PAGES(amount);
    EFI_STATUS status = ((EFI_SYSTEM_TABLE*)sys)->BootServices->AllocatePages(AllocateAnyPages, EfiBootServicesData, pages, &physicalRes);
    if (EFI_ERROR(status))
        return nullptr;
    return (void*)(UINTN)physicalRes;
#endif
}
static void iFree(void* sys, void* ptr)
{
    ((EFI_SYSTEM_TABLE*)sys)->BootServices->FreePool(ptr);
    //((EFI_SYSTEM_TABLE*)sys)->BootServices->FreePages(ptr);

}
static void iMemcpy(void* sys, void* dst, void* src, uint64_t len)
{
    ((EFI_SYSTEM_TABLE*)sys)->BootServices->CopyMem(dst, src, len);
}
static void iMemset(void* sys, void* dst, uint8_t val, uint64_t len)
{
    ((EFI_SYSTEM_TABLE*)sys)->BootServices->SetMem(dst, len, val);
}
static void iPrintStr(void* sys, wchar_t* str)
{
    ((EFI_SYSTEM_TABLE*)sys)->ConOut->OutputString(((EFI_SYSTEM_TABLE*)sys)->ConOut, (CHAR16*)str);
}
static void iPrintInt(void* sys, int num)
{
    wchar_t str[16] = { 0 };
    wchar_t* p = &str[14];
    *p-- = L'\n';
    *p-- = L'\r';
    for (int i = sizeof(str) - 4; num > 0 && i >= 0; i--) {
        *p-- = L'0' + num % 10;
        num /= 10;
    }
    p++;
    iPrintStr(sys, p);
}
static void iPrintInt(void* sys, UINT64 num){
    iPrintInt(sys, (int) num);
}
EFI_STATUS test(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* sys) {
    // Initialize the EFI library
    //InitializeLib(ImageHandle, SystemTable);

    // Allocate memory for the memory map
    EFI_MEMORY_DESCRIPTOR* MemoryMap = nullptr;
    UINTN MemoryMapSize = 0;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    // Get the required memory map size
    EFI_STATUS Status = sys->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (Status == EFI_BUFFER_TOO_SMALL) {
        MemoryMapSize += 2 * DescriptorSize;  // Increase the size for safety
        Status = sys->BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, (VOID**)&MemoryMap);
        if (EFI_ERROR(Status)) {
            iPrintStr(sys, L"Failed to allocate memory for the memory map\n");
            return Status;
        }
        Status = sys->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    }

    if (EFI_ERROR(Status)) {
        iPrintStr(sys, L"Failed to retrieve the memory map\n");
        if (MemoryMap != NULL) {
            sys->BootServices->FreePool(MemoryMap);
        }
        return Status;
    }

    // Iterate through the memory map and calculate memory capacity
    EFI_MEMORY_DESCRIPTOR* MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINTN)MemoryMap + MemoryMapSize);
    UINT64 TotalMemory = 0;
    UINT64 pc = 0;
    for (EFI_MEMORY_DESCRIPTOR* Desc = MemoryMap; Desc < MemoryMapEnd; Desc = (EFI_MEMORY_DESCRIPTOR*)((UINTN)Desc + DescriptorSize)) {
        if (Desc->Type == EfiConventionalMemory || Desc->Type == EfiBootServicesCode || Desc->Type == EfiBootServicesData) {
            TotalMemory += Desc->NumberOfPages * EFI_PAGE_SIZE;
        }
        pc += Desc->NumberOfPages;
    }
    iPrintStr(sys, L"pages:");
    iPrintInt(sys, pc);

    //Print(L"Total system memory: %lu bytes\n", TotalMemory);
    iPrintStr(sys, L"Total system memory:");
    iPrintInt(sys, TotalMemory);
    // Free the allocated memory for the memory map
    sys->BootServices->FreePool(MemoryMap);

    return EFI_SUCCESS;
}

EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* sys)
{
    test(imageHandle, sys); while (1);
    //---------------------------
    // init graphics mode
    //---------------------------
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gfx = iGfxInit(sys);
    if (gfx == nullptr)
    {
        iPrintStr(sys, L"Could not init graphics");
        while (1);
        return EFI_LOAD_ERROR;
    }

    //---------------------------
    // allocate framebuffer
    //---------------------------
    const int xres = gfx->Mode->Info->HorizontalResolution;
    const int yres = gfx->Mode->Info->VerticalResolution;
    const int pitc = gfx->Mode->Info->PixelsPerScanLine;
    uint32_t* pixels = (uint32_t*)iMalloc(sys, 4 * xres * yres);
    if (!pixels) {
        iPrintStr(sys, L"exit");
        while (1);

        return EFI_LOAD_ERROR;
    }

#if 1
    const EFI_GRAPHICS_PIXEL_FORMAT format = gfx->Mode->Info->PixelFormat;
    if (format == PixelRedGreenBlueReserved8BitPerColor) iPrintStr(sys, L"Format = BGRA\r\n");
    if (format == PixelBlueGreenRedReserved8BitPerColor) iPrintStr(sys, L"Format = RGBA\r\n");
    if (format == PixelBitMask) iPrintStr(sys, L"Format = PixelBitMask\r\n");
    if (format == PixelBltOnly) iPrintStr(sys, L"Format = PixelBltOnly\r\n");
    iPrintStr(sys, L"xres: "); iPrintInt(sys, xres);
    iPrintStr(sys, L"yres: "); iPrintInt(sys, yres);
    iPrintStr(sys, L"pitc: "); iPrintInt(sys, pitc);
#endif

    //---------------------------
    // setup timer
    //---------------------------
    uint64_t iTimeCounter = 0;
    EFI_EVENT evtTimer;
    EFI_STATUS status = sys->BootServices->CreateEvent(EFI_EVENT_TIMER | EFI_EVENT_NOTIFY_SIGNAL, EFI_TPL_CALLBACK, iTimerHandler, &iTimeCounter, &evtTimer);
    if (EFI_ERROR(status)) return EFI_LOAD_ERROR;
    status = sys->BootServices->SetTimer(evtTimer, TimerPeriodic, 10000);
    if (EFI_ERROR(status)) return EFI_LOAD_ERROR;

    //---------------------------
    // init my mini libc module
    //---------------------------
    mlibc_init(sys, iMalloc, iFree, iMemcpy, iMemset, iPrintStr);

    //---------------------------
    // init demo
    //---------------------------
    if (!Demo_Init(xres, yres)) return EFI_LOAD_ERROR;

    //---------------------------
    // reset keyboard
    //---------------------------
    sys->ConIn->Reset(sys->ConIn, FALSE);

    //---------------------------
    // render loop
    //---------------------------
    iPrintStr(sys, L"ren");
    for (int i = 0; i < 1024 * 100; i++)pixels[i] = 0xff;
    EFI_STATUS ret = gfx->Blt(gfx, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)pixels, EfiBltBufferToVideo, 0, 0, 0, 0, xres, yres, 0);
    iPrintStr(sys, L"blt: "); iPrintInt(sys, ret);
    float iTime = 0.0f;
    bool done = false;
    while (!done)
    {
        // get time
        sys->BootServices->Stall(4000); // 4000 microseconds = 4 miliseconds
        //const float iTime = float(iTimeCounter)/(float)(100);
        iTime += 1.0f / 30.0f;

        // render demo
        Demo_Render(pixels, xres, yres, iTime);

        // blit should be more compatible than the memcpy with esoteric framebuffer pixel formats
#if 1
        gfx->Blt(gfx, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)pixels, EfiBltBufferToVideo, 0, 0, 0, 0, xres, yres, 0);
#else
        char* dst = (char*)gfx->Mode->FrameBufferBase;
        char* src = (char*)pixels;
        for (int i = 0; i < yres; i++)
        {
            sys->BootServices->CopyMem(dst, src, xres * 4);
            src += xres * 4;
            dst += pitc * 4;
        }
#endif
        // if ESC, exit
        EFI_INPUT_KEY key;
        status = sys->ConIn->ReadKeyStroke(sys->ConIn, &key);
        done = (key.ScanCode == SCAN_ESC);
    }

    //---------------------------
    // deinit demo
    //---------------------------
    Demo_DeInit();

    //---------------------------
    // free resources
    //---------------------------
    iFree(sys, pixels);
    sys->BootServices->SetTimer(evtTimer, TimerCancel, 0);
    sys->BootServices->CloseEvent(evtTimer);

    //---------------------------
    // switch off machine
    //---------------------------
    sys->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, nullptr);

    return EFI_SUCCESS;
}
