//
// Created by rosetta on 01/06/2024.
//

// Here you can store the variables of your features

#pragma once

#include <string>

namespace Vars {
    struct PlayerData_t {
        std::string RenkKodu = "";
        bool sabitRenkAktif = false;   // Frida'da hook kurulunca aktif oluyordu
        std::string hedefRenk = "Blue"; // Kullanıcının seçtiği hedef renk

        // ← YENİ EKLENEN ALANLAR (Visuals.cpp için)
        bool ESPCrosshair     = false;   // Crosshair açık/kapalı
        int  CrosshairColor   = 0;       // 0=Kırmızı, 1=Yeşil, vs.
        int  CrosshairSize    = 5;       // Crosshair boyutu
    };

    inline PlayerData_t PlayerData;
}
