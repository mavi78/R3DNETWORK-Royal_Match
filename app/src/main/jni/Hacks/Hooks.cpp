//
// Created by rosetta on 05/07/2024.
//

#include "Hooks.hpp"
#include <cstdint>

// ============================================================
// YARDIMCI: Unity String'i std::string'e çevir (GÜVENLİ)
// wstring_convert KULLANMA — NDK 27'de bozuk
// ============================================================
static std::string UnityStringToStd(void* unityStringObj) {
    if (!unityStringObj) return "";

    auto* str = reinterpret_cast<UnityResolve::UnityType::String*>(unityStringObj);

    // m_stringLength = karakter sayısı
    int32_t length = str->m_stringLength;
    if (length <= 0 || length > 1000) return "";  // güvenlik sınırı

    // m_firstChar = UTF-16 karakter dizisi
    const char16_t* chars = reinterpret_cast<const char16_t*>(str->m_firstChar);

    std::string result;
    result.reserve(length);

    for (int32_t i = 0; i < length; i++) {
        char16_t c = chars[i];

        // ASCII (0-127)
        if (c < 0x80) {
            result += static_cast<char>(c);
        }
            // 2-byte UTF-8 (128-2047)
        else if (c < 0x800) {
            result += static_cast<char>(0xC0 | (c >> 6));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
            // 3-byte UTF-8 (2048-65535)
        else {
            result += static_cast<char>(0xE0 | (c >> 12));
            result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (c & 0x3F));
        }
    }

    return result;
}

// ============================================================
// GLOBAL DEĞİŞKEN TANIMLARI
// ============================================================
std::unordered_map<std::string, int> Hooks::renkSayaclari = {
        {"blue",   0},
        {"green",  0},
        {"red",    0},
        {"yellow", 0},
        {"pink",   0},
        {"orange", 0},
};
int Hooks::toplamUretim = 0;
std::mutex Hooks::sayacMutex;

// ============================================================
// SAYAÇ ERİŞİM FONKSİYONLARI
// ============================================================
std::unordered_map<std::string, int> Hooks::GetRenkSayaclari() {
    std::lock_guard<std::mutex> lock(sayacMutex);
    return renkSayaclari;
}

int Hooks::GetToplamUretim() {
    std::lock_guard<std::mutex> lock(sayacMutex);
    return toplamUretim;
}

void Hooks::ResetSayaclar() {
    std::lock_guard<std::mutex> lock(sayacMutex);
    for (auto& pair : renkSayaclari) pair.second = 0;
    toplamUretim = 0;
}

// ============================================================
// ENUM YARDIMCILARI
// ============================================================
std::string EnumToString(const char* enumClassName, int32_t value) {
    static const std::unordered_map<int32_t, std::string> matchTypeMap = {
            {0, "Blue"},
            {1, "Red"},
            {2, "Green"},
            {3, "Yellow"},
            {4, "Pink"},
            {5, "Orange"},
    };
    auto it = matchTypeMap.find(value);
    return (it != matchTypeMap.end()) ? it->second : "";
}

int32_t GetMatchTypeValue(const std::string& renkAdi) {
    static const std::unordered_map<std::string, int32_t> reverseMap = {
            {"Blue",   0},
            {"Red",    1},
            {"Green",  2},
            {"Yellow", 3},
            {"Pink",   4},
            {"Orange", 5},
    };
    auto it = reverseMap.find(renkAdi);
    return (it != reverseMap.end()) ? it->second : 0;
}

// ============================================================
// YARDIMCI: List içinde key ile param bul
// ============================================================
static void* FindParamByKey(void* paramsList, const char* hedefKey) {
    if (!paramsList || !hedefKey) return nullptr;

    // List<T> sınıfını al
    static auto* listCls = UnityResolve::Get(
            OBFUSCATE("mscorlib.dll")
    )->Get(OBFUSCATE("List`1"));

    if (!listCls) {
        LOGI("[FIND] List`1 sınıfı bulunamadı!");
        return nullptr;
    }

    static auto* getCountMethod = listCls->Get<UnityResolve::Method>(
            OBFUSCATE("get_Count")
    );
    static auto* getItemMethod = listCls->Get<UnityResolve::Method>(
            OBFUSCATE("get_Item")
    );

    if (!getCountMethod || !getItemMethod) {
        LOGI("[FIND] get_Count veya get_Item bulunamadı!");
        return nullptr;
    }

    int count = getCountMethod->Invoke<int>(paramsList);
    if (count <= 0) return nullptr;

    // EventDataParameter sınıfı
    static auto* paramCls = UnityResolve::Get(
            OBFUSCATE("Assembly-CSharp.dll")
    )->Get(OBFUSCATE("EventDataParameter"));

    if (!paramCls) {
        LOGI("[FIND] EventDataParameter sınıfı bulunamadı!");
        return nullptr;
    }

    static auto* keyField = paramCls->Get<UnityResolve::Field>(
            OBFUSCATE("key")
    );

    if (!keyField) {
        LOGI("[FIND] EventDataParameter.key field bulunamadı!");
        return nullptr;
    }

    for (int i = 0; i < count; i++) {
        // get_Item(i) → EventDataParameter
        void* param = getItemMethod->Invoke<void*>(paramsList, i);
        if (!param) continue;

        // param.key alanını oku
        void* paramKey = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(param) + keyField->offset
        );

        if (!paramKey) continue;

        // ✅ GÜVENLİ String dönüşümü
        std::string paramKeyStd = UnityStringToStd(paramKey);

        if (paramKeyStd == hedefKey) {
            return param;
        }
    }

    return nullptr;
}

// ============================================================
// YARDIMCI: Field'a int32 yaz
// ============================================================
static void WriteInt32Field(void* obj, UnityResolve::Class* cls, const char* fieldName, int32_t value) {
    if (!obj || !cls) return;

    auto* field = cls->Get<UnityResolve::Field>(fieldName);
    if (!field) return;

    *reinterpret_cast<int32_t*>(
            reinterpret_cast<uintptr_t>(obj) + field->offset
    ) = value;
}

// ============================================================
// YARDIMCI: Field'tan int32 oku
// ============================================================
static int32_t ReadInt32Field(void* obj, UnityResolve::Class* cls, const char* fieldName) {
    if (!obj || !cls) return 0;

    auto* field = cls->Get<UnityResolve::Field>(fieldName);
    if (!field) return 0;

    return *reinterpret_cast<int32_t*>(
            reinterpret_cast<uintptr_t>(obj) + field->offset
    );
}

// ============================================================
// YARDIMCI: Boxed int32 oku/yaz
// ============================================================
static int32_t ReadBoxedInt(void* param, UnityResolve::Field* valueField) {
    if (!param || !valueField) return 0;

    void* boxedObj = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(param) + valueField->offset
    );
    if (!boxedObj) return 0;

    return *reinterpret_cast<int32_t*>(
            reinterpret_cast<uintptr_t>(boxedObj) + 0x10
    );
}

static void WriteBoxedInt(void* param, UnityResolve::Field* valueField, int32_t yeniDeger) {
    if (!param || !valueField) return;

    void* boxedObj = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(param) + valueField->offset
    );
    if (!boxedObj) return;

    *reinterpret_cast<int32_t*>(
            reinterpret_cast<uintptr_t>(boxedObj) + 0x10
    ) = yeniDeger;
}

// ============================================================
// HOOK: EventWriter::AddEventData
// ============================================================
void Hooks::Hook_AddEventData(void* pInstance, void* eventData) {
    // Toggle kontrolü
    if (!Vars::PlayerData.sabitRenkAktif) {
        orig_AddEventData(pInstance, eventData);
        return;
    }

    try {
        if (!eventData) {
            orig_AddEventData(pInstance, eventData);
            return;
        }

        // ====== EventData.name field'ını oku ======
        auto* eventDataCls = UnityResolve::Get(
                OBFUSCATE("Assembly-CSharp.dll")
        )->Get(OBFUSCATE("EventData"));

        if (!eventDataCls) {
            LOGI("[NORMALIZE] EventData sınıfı bulunamadı");
            orig_AddEventData(pInstance, eventData);
            return;
        }

        auto* nameField = eventDataCls->Get<UnityResolve::Field>(
                OBFUSCATE("name")
        );

        if (!nameField) {
            LOGI("[NORMALIZE] name field bulunamadı");
            orig_AddEventData(pInstance, eventData);
            return;
        }

        void* nameObj = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(eventData) + nameField->offset
        );

        if (!nameObj) {
            orig_AddEventData(pInstance, eventData);
            return;
        }

        // ✅ GÜVENLİ String dönüşümü
        std::string nameStd = UnityStringToStd(nameObj);

        // Filtreleme: LevelEnd içermeli ve Reward içermemeli
        if (nameStd.find("LevelEnd") == std::string::npos ||
            nameStd.find("Reward") != std::string::npos) {
            orig_AddEventData(pInstance, eventData);
            return;
        }

        LOGI("[NORMALIZE] Event name: %s", nameStd.c_str());

        // ====== parameters alanını al ======
        auto* parametersField = eventDataCls->Get<UnityResolve::Field>(
                OBFUSCATE("parameters")
        );

        if (!parametersField) {
            LOGI("[NORMALIZE] parameters field bulunamadı");
            orig_AddEventData(pInstance, eventData);
            return;
        }

        void* params = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(eventData) + parametersField->offset
        );

        if (!params) {
            LOGI("[NORMALIZE] parameters null");
            orig_AddEventData(pInstance, eventData);
            return;
        }

        LOGI("[NORMALIZE] ========== NORMALIZE WITH COUNTERS ==========");

        auto sayaclar = GetRenkSayaclari();
        LOGI("[NORMALIZE] Sayaclar: blue=%d, green=%d, red=%d, yellow=%d, pink=%d",
             sayaclar["blue"], sayaclar["green"], sayaclar["red"],
             sayaclar["yellow"], sayaclar["pink"]);

        // EventDataParameter sınıfı
        auto* paramCls = UnityResolve::Get(
                OBFUSCATE("Assembly-CSharp.dll")
        )->Get(OBFUSCATE("EventDataParameter"));

        if (!paramCls) {
            LOGI("[NORMALIZE] EventDataParameter sınıfı bulunamadı");
            orig_AddEventData(pInstance, eventData);
            return;
        }

        auto* valueField = paramCls->Get<UnityResolve::Field>(
                OBFUSCATE("value")
        );

        if (!valueField) {
            LOGI("[NORMALIZE] EventDataParameter.value bulunamadı");
            orig_AddEventData(pInstance, eventData);
            return;
        }

        // ============================================================
        // 1. MATCH_ITEMS
        // ============================================================
        void* matchItemsParam = FindParamByKey(params, "match_items");
        if (matchItemsParam) {
            void* value = *reinterpret_cast<void**>(
                    reinterpret_cast<uintptr_t>(matchItemsParam) + valueField->offset
            );

            if (value) {
                auto* matchStatsCls = UnityResolve::Get(
                        OBFUSCATE("Assembly-CSharp.dll")
                )->Get(OBFUSCATE("MatchItemsStats"));

                if (matchStatsCls) {
                    int32_t eskiBlue   = ReadInt32Field(value, matchStatsCls, "blue");
                    int32_t eskiGreen  = ReadInt32Field(value, matchStatsCls, "green");
                    int32_t eskiRed    = ReadInt32Field(value, matchStatsCls, "red");
                    int32_t eskiYellow = ReadInt32Field(value, matchStatsCls, "yellow");
                    int32_t eskiPink   = ReadInt32Field(value, matchStatsCls, "pink");

                    LOGI("[NORMALIZE] MATCH Eski: blue=%d, green=%d, red=%d, yellow=%d, pink=%d",
                         eskiBlue, eskiGreen, eskiRed, eskiYellow, eskiPink);

                    int32_t toplamEslesme = eskiBlue + eskiGreen + eskiRed + eskiYellow + eskiPink;
                    int32_t sayacToplam = sayaclar["blue"] + sayaclar["green"] +
                                          sayaclar["red"] + sayaclar["yellow"] + sayaclar["pink"];

                    if (sayacToplam > 0 && toplamEslesme > 0) {
                        int32_t yeniBlue   = (int32_t)std::round((double)toplamEslesme * sayaclar["blue"]   / sayacToplam);
                        int32_t yeniGreen  = (int32_t)std::round((double)toplamEslesme * sayaclar["green"]  / sayacToplam);
                        int32_t yeniRed    = (int32_t)std::round((double)toplamEslesme * sayaclar["red"]    / sayacToplam);
                        int32_t yeniYellow = (int32_t)std::round((double)toplamEslesme * sayaclar["yellow"] / sayacToplam);
                        int32_t yeniPink   = (int32_t)std::round((double)toplamEslesme * sayaclar["pink"]   / sayacToplam);

                        int32_t yeniToplam = yeniBlue + yeniGreen + yeniRed + yeniYellow + yeniPink;
                        int32_t fark = toplamEslesme - yeniToplam;
                        if (fark != 0) yeniBlue += fark;

                        WriteInt32Field(value, matchStatsCls, "blue",   yeniBlue);
                        WriteInt32Field(value, matchStatsCls, "green",  yeniGreen);
                        WriteInt32Field(value, matchStatsCls, "red",    yeniRed);
                        WriteInt32Field(value, matchStatsCls, "yellow", yeniYellow);
                        WriteInt32Field(value, matchStatsCls, "pink",   yeniPink);

                        LOGI("[NORMALIZE] MATCH Yeni: blue=%d, green=%d, red=%d, yellow=%d, pink=%d",
                             yeniBlue, yeniGreen, yeniRed, yeniYellow, yeniPink);
                    }
                }
            }
        }

        // ============================================================
        // 2. SPECIAL_ITEMS
        // ============================================================
        void* specialItemsParam = FindParamByKey(params, "special_items");
        if (specialItemsParam) {
            void* value = *reinterpret_cast<void**>(
                    reinterpret_cast<uintptr_t>(specialItemsParam) + valueField->offset
            );

            if (value) {
                auto* specialStatsCls = UnityResolve::Get(
                        OBFUSCATE("Assembly-CSharp.dll")
                )->Get(OBFUSCATE("SpecialItemsStats"));

                if (specialStatsCls) {
                    // Creation
                    int32_t rocket_creation    = ReadInt32Field(value, specialStatsCls, "rocket_creation");
                    int32_t tnt_creation       = ReadInt32Field(value, specialStatsCls, "tnt_creation");
                    int32_t lb_creation        = ReadInt32Field(value, specialStatsCls, "lb_creation");
                    int32_t propeller_creation = ReadInt32Field(value, specialStatsCls, "propeller_creation");

                    LOGI("[NORMALIZE] SPECIAL Eski: rocket=%d, tnt=%d, lb=%d, prop=%d",
                         rocket_creation, tnt_creation, lb_creation, propeller_creation);

                    int32_t toplamCreation = rocket_creation + tnt_creation + lb_creation + propeller_creation;
                    const int32_t maxToplam = 20;

                    if (toplamCreation > maxToplam) {
                        double oran = (double)maxToplam / toplamCreation;

                        int32_t yeniRocket = (int32_t)std::round(rocket_creation * oran);
                        int32_t yeniTnt    = (int32_t)std::round(tnt_creation * oran);
                        int32_t yeniLb     = (int32_t)std::round(lb_creation * oran);
                        int32_t yeniProp   = (int32_t)std::round(propeller_creation * oran);

                        WriteInt32Field(value, specialStatsCls, "rocket_creation",    yeniRocket);
                        WriteInt32Field(value, specialStatsCls, "tnt_creation",       yeniTnt);
                        WriteInt32Field(value, specialStatsCls, "lb_creation",        yeniLb);
                        WriteInt32Field(value, specialStatsCls, "propeller_creation", yeniProp);

                        LOGI("[NORMALIZE] SPECIAL Yeni: rocket=%d, tnt=%d, lb=%d, prop=%d",
                             yeniRocket, yeniTnt, yeniLb, yeniProp);
                    }

                    // Use
                    int32_t rocket_use    = ReadInt32Field(value, specialStatsCls, "rocket_use");
                    int32_t tnt_use       = ReadInt32Field(value, specialStatsCls, "tnt_use");
                    int32_t lb_use        = ReadInt32Field(value, specialStatsCls, "lb_use");
                    int32_t propeller_use = ReadInt32Field(value, specialStatsCls, "propeller_use");

                    LOGI("[NORMALIZE] SPECIAL USE Eski: rocket=%d, tnt=%d, lb=%d, prop=%d",
                         rocket_use, tnt_use, lb_use, propeller_use);

                    int32_t toplamUse = rocket_use + tnt_use + lb_use + propeller_use;
                    const int32_t maxUse = 10;

                    if (toplamUse > maxUse) {
                        double oran = (double)maxUse / toplamUse;

                        int32_t yeniRocketUse = (int32_t)std::round(rocket_use * oran);
                        int32_t yeniTntUse    = (int32_t)std::round(tnt_use * oran);
                        int32_t yeniLbUse     = (int32_t)std::round(lb_use * oran);
                        int32_t yeniPropUse   = (int32_t)std::round(propeller_use * oran);

                        WriteInt32Field(value, specialStatsCls, "rocket_use",    yeniRocketUse);
                        WriteInt32Field(value, specialStatsCls, "tnt_use",       yeniTntUse);
                        WriteInt32Field(value, specialStatsCls, "lb_use",        yeniLbUse);
                        WriteInt32Field(value, specialStatsCls, "propeller_use", yeniPropUse);

                        LOGI("[NORMALIZE] SPECIAL USE Yeni: rocket=%d, tnt=%d, lb=%d, prop=%d",
                             yeniRocketUse, yeniTntUse, yeniLbUse, yeniPropUse);
                    }
                }
            }
        }

        // ============================================================
        // 3. HAMLE SAYILARI
        // ============================================================
        void* movesGivenParam = FindParamByKey(params, "moves_given");
        void* movesMadeParam  = FindParamByKey(params, "moves_made");
        void* movesLeftParam  = FindParamByKey(params, "moves_left");

        if (movesGivenParam && movesMadeParam && movesLeftParam) {
            int32_t movesGiven = ReadBoxedInt(movesGivenParam, valueField);
            int32_t movesMade  = ReadBoxedInt(movesMadeParam, valueField);
            int32_t movesLeft  = ReadBoxedInt(movesLeftParam, valueField);

            LOGI("[NORMALIZE] HAMLE Eski: made=%d, left=%d, given=%d",
                 movesMade, movesLeft, movesGiven);

            int32_t yeniMovesLeft = 1;
            int32_t yeniMovesMade = movesGiven - yeniMovesLeft;

            WriteBoxedInt(movesMadeParam, valueField, yeniMovesMade);
            WriteBoxedInt(movesLeftParam, valueField, yeniMovesLeft);

            LOGI("[NORMALIZE] HAMLE Yeni: made=%d, left=%d, given=%d",
                 yeniMovesMade, yeniMovesLeft, movesGiven);
        }

        // ============================================================
        // 4. TIME_SPENT
        // ============================================================
        void* timeSpentParam = FindParamByKey(params, "time_spent");
        if (timeSpentParam) {
            int32_t eskiTime = ReadBoxedInt(timeSpentParam, valueField);

            int32_t movesMadeForTime = 20;
            if (movesMadeParam) {
                movesMadeForTime = ReadBoxedInt(movesMadeParam, valueField);
            }

            int32_t yeniTime = (int32_t)std::floor(
                    movesMadeForTime * (3 + ((double)rand() / RAND_MAX) * 4)
            );

            WriteBoxedInt(timeSpentParam, valueField, yeniTime);

            LOGI("[NORMALIZE] TIME Eski: %d, Yeni: %d", eskiTime, yeniTime);
        }

        // ============================================================
        // 5. SAYAÇLARI SIFIRLA
        // ============================================================
        ResetSayaclar();
        LOGI("[NORMALIZE] Sayaçlar sıfırlandı");

    } catch (const std::exception& e) {
        LOGI("[NORMALIZE] std::exception: %s", e.what());
    } catch (...) {
        LOGI("[NORMALIZE] Bilinmeyen exception");
    }

    // Orijinal fonksiyonu çağır
    orig_AddEventData(pInstance, eventData);
}

// Orijinal fonksiyon pointer'ı
void (*Hooks::orig_AddEventData)(void*, void*) = nullptr;

// ============================================================
// HOOK: ItemCreator::CreateItemForFillingCellAt
// ============================================================
void* Hooks::Hook_CreateItemForFillingCellAt(
        void* pInstance,
        void* fillingCell,
        void* position
) {
    // 1) Orijinal fonksiyonu çağır
    void* result = orig_CreateItemForFillingCellAt(
            pInstance, fillingCell, position
    );

    if (!result) return result;

    // 2) Toggle kontrolü
    if (!Vars::PlayerData.sabitRenkAktif) {
        return result;
    }

    // 3) MatchItemModel sınıfını al
    auto* matchItemModelCls = UnityResolve::Get(
            OBFUSCATE("Assembly-CSharp.dll")
    )->Get(OBFUSCATE("MatchItemModel"));

    if (!matchItemModelCls) {
        return result;
    }

    // 4) MatchType field'ını al
    auto* matchTypeField = matchItemModelCls->Get<UnityResolve::Field>(
            OBFUSCATE("<MatchType>k__BackingField")
    );

    if (!matchTypeField) {
        return result;
    }

    // ====== 5) SAYAÇ ======
    try {
        int32_t gercekRenkInt = *reinterpret_cast<int32_t*>(
                reinterpret_cast<uintptr_t>(result) + matchTypeField->offset
        );

        std::string gercekRenkStr = EnumToString(
                OBFUSCATE("Royal.Scenes.Game.Mechanics.Matches.MatchType"),
                gercekRenkInt
        );

        std::transform(
                gercekRenkStr.begin(), gercekRenkStr.end(),
                gercekRenkStr.begin(), ::tolower
        );

        if (!gercekRenkStr.empty()) {
            std::lock_guard<std::mutex> lock(Hooks::sayacMutex);

            auto it = Hooks::renkSayaclari.find(gercekRenkStr);
            if (it != Hooks::renkSayaclari.end()) {
                it->second++;
                Hooks::toplamUretim++;

                LOGI("[SAYAC] %s -> %d (toplam: %d)",
                     gercekRenkStr.c_str(),
                     it->second,
                     Hooks::toplamUretim);
            }
        }
    } catch (...) {}

    // ====== 6) RENK DEĞİŞTİRME ======
    try {
        int32_t hedefRenkInt = GetMatchTypeValue(Vars::PlayerData.hedefRenk);

        // MatchType field'ına hedef rengi yaz
        *reinterpret_cast<int32_t*>(
                reinterpret_cast<uintptr_t>(result) + matchTypeField->offset
        ) = hedefRenkInt;

        // ItemView'a erişim
        auto* itemViewField = matchItemModelCls->Get<UnityResolve::Field>(
                OBFUSCATE("<ItemView>k__BackingField")
        );

        if (itemViewField) {
            void* itemView = *reinterpret_cast<void**>(
                    reinterpret_cast<uintptr_t>(result) + itemViewField->offset
            );

            if (itemView) {
                auto* itemViewCls = UnityResolve::Get(
                        OBFUSCATE("Assembly-CSharp.dll")
                )->Get(OBFUSCATE("ItemView"));

                if (itemViewCls) {
                    auto* itemAssetsField = itemViewCls->Get<UnityResolve::Field>(
                            OBFUSCATE("itemAssets")
                    );

                    if (itemAssetsField) {
                        void* itemAssets = *reinterpret_cast<void**>(
                                reinterpret_cast<uintptr_t>(itemView) + itemAssetsField->offset
                        );

                        if (itemAssets) {
                            auto* itemAssetsCls = UnityResolve::Get(
                                    OBFUSCATE("Assembly-CSharp.dll")
                            )->Get(OBFUSCATE("ItemAssets"));

                            if (itemAssetsCls) {
                                auto* getSpriteMethod = itemAssetsCls->Get<UnityResolve::Method>(
                                        OBFUSCATE("GetSprite")
                                );

                                if (getSpriteMethod) {
                                    void* newSprite = getSpriteMethod->Invoke<void*>(
                                            itemAssets, hedefRenkInt
                                    );

                                    auto* baseViewField = itemViewCls->Get<UnityResolve::Field>(
                                            OBFUSCATE("baseView")
                                    );

                                    if (baseViewField && newSprite) {
                                        void* baseView = *reinterpret_cast<void**>(
                                                reinterpret_cast<uintptr_t>(itemView) + baseViewField->offset
                                        );

                                        if (baseView) {
                                            auto* baseViewCls = UnityResolve::Get(
                                                    OBFUSCATE("Assembly-CSharp.dll")
                                            )->Get(OBFUSCATE("BaseView"));

                                            if (baseViewCls) {
                                                auto* setSpriteMethod = baseViewCls->Get<UnityResolve::Method>(
                                                        OBFUSCATE("set_sprite")
                                                );

                                                if (setSpriteMethod) {
                                                    setSpriteMethod->Invoke<void>(
                                                            baseView, newSprite
                                                    );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } catch (...) {
        LOGI("[FIX] HATA yakalandı");
    }

    return result;
}

// Orijinal fonksiyon pointer'ı
void* (*Hooks::orig_CreateItemForFillingCellAt)(
        void*, void*, void*
) = nullptr;

// ============================================================
// HOOK KURULUMU
// ============================================================
void Hooks::InitHooks() {
    // ⚡ AYNI PROCESS'TE TEKRAR ÇAĞRILMASINI ENGELLE
    static bool zatenKuruldu = false;
    if (zatenKuruldu) {
        LOGI("[HOOK] InitHooks zaten çağrıldı, atlanıyor");
        return;
    }
    zatenKuruldu = true;

    LOGI("[HOOK] InitHooks başlatılıyor...");

    // ====== HOOK 1: ItemCreator::CreateItemForFillingCellAt ======
    UnityResolve::Hook(
            OBFUSCATE("ItemCreator"),
            OBFUSCATE("CreateItemForFillingCellAt"),
            {"*", "*"},
            (void*)Hooks::Hook_CreateItemForFillingCellAt,
            (void**)&Hooks::orig_CreateItemForFillingCellAt
    );

    // ====== HOOK 2: EventWriter::AddEventData ======
    UnityResolve::Hook(
            OBFUSCATE("EventWriter"),
            OBFUSCATE("AddEventData"),
            {"*"},
            (void*)Hooks::Hook_AddEventData,
            (void**)&Hooks::orig_AddEventData
    );

    // Varsayılan hedef renk
    Vars::PlayerData.hedefRenk = "Blue";

    LOGI("Final fix kuruldu - eşleşme yap!");
}