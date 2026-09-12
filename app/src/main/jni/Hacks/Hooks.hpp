//
// Created by rosetta on 05/07/2024.
//

#ifndef HOOKS_H
#define HOOKS_H

#include "Vars.h"
#include "UnityResolve/UnityResolve.hpp"
#include "Includes/ObscuredTypes.hpp"
#include "Includes/obfuscate.h"
#include "Hacks/Vars.h"
#include "Includes/Logger.h"

#include <string>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <cstring>

class Hooks {
public:
    static void InitHooks();

    // ====== SAYAÇLAR ======
    static std::unordered_map<std::string, int> renkSayaclari;
    static int toplamUretim;
    static std::mutex sayacMutex;

    static std::unordered_map<std::string, int> GetRenkSayaclari();
    static int GetToplamUretim();
    static void ResetSayaclar();

    // ====== HOOK METOTLARI ======
    // Hook 1: ItemCreator::CreateItemForFillingCellAt (renk değiştirme + sayaç)
    static void* Hook_CreateItemForFillingCellAt(
            void* pInstance,
            void* fillingCell,
            void* position
    );

    static void* (*orig_CreateItemForFillingCellAt)(
            void* pInstance,
            void* fillingCell,
            void* position
    );

    // Hook 2: EventWriter::AddEventData (normalize + manipülasyon)  ← YENİ
    static void Hook_AddEventData(
            void* pInstance,
            void* eventData
    );

    static void (*orig_AddEventData)(
            void* pInstance,
            void* eventData
    );

};

#endif // HOOKS_H