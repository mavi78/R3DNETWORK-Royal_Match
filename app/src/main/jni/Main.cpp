#include <list>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <cstring>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include <numbers>

#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "dobby/dobby.h"
#include "Menu/FeatureModule.hpp"
#include "UnityResolve/UnityResolve.hpp"
#include "Includes/Utils.hpp"
#include "Includes/RemapTools.h"
#include "KittyMemory/MemoryPatch.h"
#include "Includes/ObscuredTypes.hpp"
#include "Includes/ESPManager.h"
#include "Menu/JNILoader.hpp"
#include "Menu/Setup.hpp"
#include "Hacks/Hooks.hpp"

// Target lib here
#define IL2CPP_MODULE OBFUSCATE("libil2cpp.so")

void *hack_thread(void *) {
    LOGI(OBFUSCATE("pthread created"));

    espManager = new ESPManager();
    LOGI(OBFUSCATE("ESPManager initialized"));

    // ============================================
    // 1. ADIM: libil2cpp.so'yu bekle
    // (Artık KittyMemory dl_iterate_phdr kullandığı için çalışacak)
    // ============================================
    int sayac = 0;
    do {
        sleep(1);
        sayac++;
        LOGI(OBFUSCATE("IL2CPP bekleniyor... (%d)"), sayac);

        if (sayac > 30) {
            LOGE(OBFUSCATE("Timeout!"));
            return NULL;
        }
    } while (!KittyMemory::getLibraryMap(IL2CPP_MODULE).isValid());

    LOGI(OBFUSCATE("✅ libil2cpp.so bulundu!"));

    // ============================================
    // 2. ADIM: dlopen ile handle al
    // ============================================
    sleep(5);

    auto p_handler = dlopen(IL2CPP_MODULE, RTLD_NOW);
    if (!p_handler) {
        LOGE(OBFUSCATE("❌ dlopen başarısız: %s"), dlerror());
        return NULL;
    }
    LOGI(OBFUSCATE("✅ dlopen başarılı: %p"), p_handler);

    // ============================================
    // 3. ADIM: UnityResolve'u başlat
    // ============================================
    UnityResolve::Init(p_handler);
    LOGI(OBFUSCATE("✅ UnityResolve başlatıldı"));

    // ============================================
    // 4. ADIM: Hook'ları başlat
    // ============================================
    LOGI(OBFUSCATE("🚀 Starting hooks..."));
    Hooks::InitHooks();
    LOGI(OBFUSCATE("✅ Hooks tamamlandı"));

    return NULL;
}

__attribute__((constructor))
void init() {
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
    RemapTools::RemapLibrary(OBFUSCATE("libLoader.so"));
}

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);

    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    if (JNILoader::RegisterAll(env) != JNI_OK)
        return JNI_ERR;

    if (RegisterMenu(env) != 0)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}