#include "dobby.h"
#include "byopen.h"
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <android/log.h>
#include <unistd.h>
#include <sys/stat.h>
#include <link.h>

#define TAG "XPOSED_SGAMEHOOK"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)


typedef struct Il2CppDomain Il2CppDomain;
typedef struct Il2CppObject Il2CppObject;
typedef struct Il2CppAssembly Il2CppAssembly;
typedef struct Il2CppImage Il2CppImage;
typedef struct Il2CppClass Il2CppClass;
typedef struct Il2CppArray Il2CppArray;
typedef struct Il2CppString Il2CppString;
typedef void (*Il2CppMethodPointer)();
typedef struct MethodInfo MethodInfo;

struct MethodInfo {
    Il2CppMethodPointer methodPointer;
};

const char* G_BASE_PATHS[] = {
    "/data/user/0/com.tencent.tmgp.sgame",
    "/data/user/0/com.tencent.tmgp.sgamece"
};
const char* CUSTOM_TEX_JPG_FILE = "customtexjpg";
const char* CUSTOM_TEX_PNG_FILE = "customtexpng";

static char G_CURRENT_PACKAGE_PATH[256] = {0};

static volatile void* __attribute__((visibility("hidden"))) g_hook_il2cpp_handle = nullptr;

static Il2CppDomain* (*domain_get)(void) = nullptr;
static Il2CppAssembly* (*domain_get_assembly_from_name)(Il2CppDomain*, const char*) = nullptr;
static Il2CppImage* (*assembly_get_image)(Il2CppAssembly*) = nullptr;
static Il2CppClass* (*class_from_name)(Il2CppImage*, const char*, const char*) = nullptr;
static MethodInfo* (*class_get_method_from_name)(Il2CppClass*, const char*, int) = nullptr;
static Il2CppArray* (*array_new)(Il2CppClass*, size_t) = nullptr;

static void* g_encodeTexture2DToJPGPtr = nullptr;
static void* g_textureEncodeToPNGPtr = nullptr;
static void* g_loadImagePtr = nullptr;
static int g_Il2ArrOffset = sizeof(void*) * 4;

using EncodeTexture2DToJPG_t = void* (*)(void* tex, int quality);
EncodeTexture2DToJPG_t orig_EncodeTexture2DToJPG = nullptr;

using TextureEncodeToPNG_t = void* (*)(void* tex);
TextureEncodeToPNG_t orig_TextureEncodeToPNG = nullptr;

using LoadImage_t = bool (*)(void* texture, void* data, bool markNonReadable);
LoadImage_t orig_LoadImage = nullptr;


// copy from frida-il2cpp-bridge
int offsetOf(void* base, std::function<bool(void*)> condition, int depth = 512) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(base);

    if (depth > 0) {
        for (int i = 0; i < depth; i++) {
            if (condition(ptr + i)) {
                return i;
            }
        }
    } else {
        for (int i = 0; i < -depth; i++) {
            if (condition(ptr - i)) {
                return -i;
            }
        }
    }

    return -1;
}

int getIl2ArrOffset(void* il2cpp_handle) {
    Il2CppString* (*il2cpp_string_new)(const char*) =
        (Il2CppString*(*)(const char*))by_dlsym(il2cpp_handle, "il2cpp_string_new");
    Il2CppClass* (*il2cpp_object_get_class)(Il2CppObject*) =
         (Il2CppClass*(*)(Il2CppObject*))by_dlsym(il2cpp_handle, "il2cpp_object_get_class");
    MethodInfo* (*il2cpp_class_get_method_from_name)(Il2CppClass*, const char*, int) =
        (MethodInfo*(*)(Il2CppClass*, const char*, int))by_dlsym(il2cpp_handle, "il2cpp_class_get_method_from_name");

    Il2CppString* Il2Str = il2cpp_string_new("v");
    Il2CppClass* strClass = il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(Il2Str));
    MethodInfo* methodInfo = il2cpp_class_get_method_from_name(strClass, "ToCharArray", 0);

    typedef Il2CppArray* (*ToCharArrayFunc)(Il2CppString*);
    ToCharArrayFunc toCharArrayFunc = (ToCharArrayFunc)methodInfo->methodPointer;
    Il2CppArray* arr = toCharArrayFunc(Il2Str);

    int offset = offsetOf(arr, [](void* ptr) -> bool {
        int16_t value = *reinterpret_cast<int16_t*>(ptr);
        return value == 118;
    });

    if (offset == -1) return g_Il2ArrOffset;
    return offset;
}

void init_il2cpp_api(void* il2cpp_handle) {
    *(void**)(&domain_get) = by_dlsym(il2cpp_handle, "il2cpp_domain_get");
    *(void**)(&domain_get_assembly_from_name) = by_dlsym(il2cpp_handle, "il2cpp_domain_assembly_open");
    *(void**)(&assembly_get_image) = by_dlsym(il2cpp_handle, "il2cpp_assembly_get_image");
    *(void**)(&class_from_name) = by_dlsym(il2cpp_handle, "il2cpp_class_from_name");
    *(void**)(&class_get_method_from_name) = by_dlsym(il2cpp_handle, "il2cpp_class_get_method_from_name");
    *(void**)(&array_new) = by_dlsym(il2cpp_handle, "il2cpp_array_new");

    void (*il2cpp_thread_attach)(Il2CppDomain*) = (void(*)(Il2CppDomain*))by_dlsym(il2cpp_handle, "il2cpp_thread_attach");
    // 调用il2cpp_thread_attach 不然可能游戏崩溃
    il2cpp_thread_attach(domain_get());

    // 在体验服可能偏移不一致 动态获取
    g_Il2ArrOffset = getIl2ArrOffset(il2cpp_handle);
    LOGI("Il2Arr offset: %d\n", g_Il2ArrOffset);
    LOGI("[+] il2cpp API resolved successfully.");
}

void* get_method_ptr(const char* asmName, const char* ns, const char* cls, const char* method, int paramCount) {
    Il2CppDomain* domain = domain_get();
    Il2CppAssembly* il2cpp_asm = domain_get_assembly_from_name(domain, asmName);
    Il2CppImage* img = assembly_get_image(il2cpp_asm);
    Il2CppClass* klass = class_from_name(img, ns, cls);
    MethodInfo* m = class_get_method_from_name(klass, method, paramCount);
    return (void*)m->methodPointer;
}

static bool check_path_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static const char* get_current_package_name() {
    static char pkg_name[256];
    for (size_t i = 0; i < sizeof(G_BASE_PATHS) / sizeof(G_BASE_PATHS[0]); ++i) {
        const char* basePath = G_BASE_PATHS[i];
        if (check_path_exists(basePath)) {
            sscanf(basePath, "/data/user/0/%255[^/]", pkg_name);
            strncpy(G_CURRENT_PACKAGE_PATH, basePath, sizeof(G_CURRENT_PACKAGE_PATH) - 1);
            G_CURRENT_PACKAGE_PATH[sizeof(G_CURRENT_PACKAGE_PATH) - 1] = '\0';
            LOGI("[+] Target package detected: %s", pkg_name);
            return pkg_name;
        }
    }
    LOGE("[-] No target package path found.");
    return nullptr;
}

void* create_il2cpp_byte_array(const uint8_t* data, size_t size) {
    Il2CppDomain* domain = domain_get();
    Il2CppAssembly* asm_mscorlib = domain_get_assembly_from_name(domain, "mscorlib");
    Il2CppImage* img = assembly_get_image(asm_mscorlib);
    Il2CppClass* byteClass = class_from_name(img, "System", "Byte");
    Il2CppArray* arr = array_new(byteClass, size);
    memcpy(reinterpret_cast<uint8_t*>(arr) + g_Il2ArrOffset, data, size);
    return arr;
}

static bool is_target_package() {
    const char* pkg_name = get_current_package_name();
    return pkg_name && (strstr(pkg_name, "com.tencent.tmgp.sgame") != nullptr);
}

void* read_custom_texture_file(const char* filename) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/files/%s", G_CURRENT_PACKAGE_PATH, filename);
    LOGI("[+] Reading custom texture file: %s", filepath);

    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        LOGE("[-] Failed to open: %s", filepath);
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    if (size == 0) {
        fclose(fp);
        LOGE("[-] File is empty: %s", filepath);
        return nullptr;
    }
    uint8_t* buffer = static_cast<uint8_t*>(malloc(size));
    fread(buffer, 1, size, fp);
    fclose(fp);

    void* arr = create_il2cpp_byte_array(buffer, size);
    free(buffer);
    return arr;
}

static const char* get_custom_texture_filename(bool is_png) {
    return is_png ? CUSTOM_TEX_PNG_FILE : CUSTOM_TEX_JPG_FILE;
}

void* hook_EncodeTexture2DToJPG(void* tex, int quality) {
    void* custom = read_custom_texture_file(get_custom_texture_filename(false));
    if (custom && orig_LoadImage) {
        LOGI("[+] Replacing JPG texture");
        orig_LoadImage(tex, custom, false);
    }
    return orig_EncodeTexture2DToJPG(tex, quality);
}

void* hook_TextureEncodeToPNG(void* tex) {
    void* custom = read_custom_texture_file(get_custom_texture_filename(true));
    if (custom && orig_LoadImage) {
        LOGI("jjj custom pointer: %p", custom);
        LOGI("[+] Replacing PNG texture");
        orig_LoadImage(tex, custom, false);
    }
    return orig_TextureEncodeToPNG(tex);
}

void* il2cpp_hook(void*) {
    LOGI("[*] hooking...");
    LOGI("[*] Waiting for libil2cpp.so to load...");
    while (g_hook_il2cpp_handle == nullptr) {
        g_hook_il2cpp_handle = by_dlopen("libil2cpp.so", BY_RTLD_NOW);
        sleep(1);
    }

    LOGI("[+] libil2cpp.so loaded and initialized. Proceeding.");

    // 等待5秒让il2cpp加载完毕
    sleep(5);
    init_il2cpp_api(const_cast<void*>(g_hook_il2cpp_handle));

    if (!is_target_package()) {
        LOGI("[~] Not target package. Skipping hooks.");
        return nullptr;
    }

    g_loadImagePtr = get_method_ptr("UnityEngine.ImageConversionModule", "UnityEngine", "ImageConversion", "LoadImage", 3);
    orig_LoadImage = (LoadImage_t)g_loadImagePtr;

    g_encodeTexture2DToJPGPtr = get_method_ptr("Scripts.Base", "Assets.Scripts.UI", "CUIUtility", "EncodeTexture2DToJPG", 2);
    if (g_encodeTexture2DToJPGPtr) {
        DobbyHook(g_encodeTexture2DToJPGPtr,
                  (dobby_dummy_func_t)hook_EncodeTexture2DToJPG,
                  (dobby_dummy_func_t*)&orig_EncodeTexture2DToJPG);
        LOGI("[+] Hooked EncodeTexture2DToJPG");
    }

    if (strstr(G_CURRENT_PACKAGE_PATH, "com.tencent.tmgp.sgamece")) {
        g_textureEncodeToPNGPtr = get_method_ptr("Scripts.Base", "Assets.Scripts.UI", "CUIUtility", "TextureEncodeToPNG", 1);
        if (g_textureEncodeToPNGPtr) {
            DobbyHook(g_textureEncodeToPNGPtr,
                      (dobby_dummy_func_t)hook_TextureEncodeToPNG,
                      (dobby_dummy_func_t*)&orig_TextureEncodeToPNG);
            LOGI("[+] Hooked TextureEncodeToPNG (sgamece)");
        }
    }

    LOGI("hook done!");
    return nullptr;
}

__attribute__((constructor))
void entry() {
    pthread_t ptid;
    pthread_create(&ptid, nullptr, il2cpp_hook, nullptr);
}