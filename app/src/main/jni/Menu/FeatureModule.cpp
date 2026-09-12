//
// Created by rosetta on 13/09/2024.
//

#include "FeatureModule.hpp"


void FeatureModule::OnDraw(JNIEnv *env, jclass clazz, jobject draw_view, jobject canvas) {
    Draw draw(env, draw_view, canvas);
    if (draw.isValid()) {
        Visuals::Update(draw, draw.getWidth(), draw.getHeight());
    } else {
        LOGE(OBFUSCATE("Draw is not valid in OnDraw"));
    }
}

jstring FeatureModule::GetFeatureList(JNIEnv *env, jobject context) {
    RegisterFeatures();

    std::string json = Widget::ToJsonString();
    return env->NewStringUTF(json.c_str());
}


/** Here you can add your features using the Widget system
 * It's better to keep the IDs unique for each widget
 * And now we have various widgets available like ICheckBox, ISwitch, ISlider, ISpinner, ICollapse, ICategory, IRadioButton, IInputText, IInputInt, IButtonLink
 * Some feature are support some HTML tags like <font color=#FF0000>red text</font>
 *Refer to the WidgetExport.hpp for all available widgets
 **/
void FeatureModule::RegisterFeatures() {
    std::string toplamStr = "Toplam Üretim: " + std::to_string(Hooks::GetToplamUretim());

    Widget::Add(ICategory(OBFUSCATE("SABİT RENK")));
    Widget::Add(ISwitch(0, OBFUSCATE("Sabit Renk"), OBFUSCATE("Aktif olduğunda seçilen renk gelir.<br>Seçilmediği taktirde Mavi renk sabit gelir"),Vars::PlayerData.sabitRenkAktif));
    Widget::Add(ICategory(OBFUSCATE("RENK SEÇİMLERİ")));
    Widget::Add(IRadioButton(1, OBFUSCATE("Seçilen: "),{
            OBFUSCATE("🔴 Kırmızı"),
            OBFUSCATE("🟢 Yeşil"),
            OBFUSCATE("🔵 Mavi"),
            OBFUSCATE("🟡 Sarı"),
            OBFUSCATE("🩷 Pembe"),
            OBFUSCATE("🟠 Turuncu")
    }));
    Widget::Add(ICategory(OBFUSCATE("BİLGİ")));
    Widget::Add(ITextView(toplamStr.c_str()));
}

void FeatureModule::OnFeatureChanged(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value, jboolean boolean, jstring str) {

    const char* name = env->GetStringUTFChars(featName, nullptr);
    const char* strVal = str != nullptr ? env->GetStringUTFChars(str, nullptr) : "";

    LOGD(OBFUSCATE("Feature changed: [%d] %s | value=%d | bool=%d | str=%s"),
         featNum, name, value, boolean, strVal);

    switch (featNum) {
        case 0:
            Vars::PlayerData.sabitRenkAktif = boolean;
            break;
        case 1:
            switch (value) {
                case 1:
                    Vars::PlayerData.hedefRenk = "Red";
                    break;
                case 2:
                    Vars::PlayerData.hedefRenk = "Green";
                    break;
                case 3:
                    Vars::PlayerData.hedefRenk = "Blue";
                    break;
                case 4:
                    Vars::PlayerData.hedefRenk = "Yellow";
                    break;
                case 5:
                    Vars::PlayerData.hedefRenk = "Pink";
                    break;
                case 6:
                    Vars::PlayerData.hedefRenk = "Orange";
                    break;
            }
            break;
        default: break;
    }

    env->ReleaseStringUTFChars(featName, name);
    if (str != nullptr) env->ReleaseStringUTFChars(str, strVal);
}