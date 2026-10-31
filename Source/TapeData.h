#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace nova
{

//==============================================================================
// Sound character of one cassette. Values match the web prototype 1:1.
struct TapeChar
{
    float sens, k, asym, hp, lp, bumpF, bumpG, midF, midG, pres;
    float hiss, wow, flut, drift, drop, chew, age, og;
    bool mono;
};

struct TapeDef
{
    const char* id;
    const char* name;
    const char* title;
    const char* type;     // shown on the shell stripe and in the info line
    const char* eq;
    const char* len;
    const char* brand;
    const char* hand;     // handwriting on the label
    std::uint32_t shell, label, stripe, ink, pack, win;
    bool warp, torn, small;
    TapeChar p;
};

inline const std::array<TapeDef, 8>& tapes()
{
    static const std::array<TapeDef, 8> t {{
        { "fe60", "FE-60", "Ferro", "TYPE I", "120 us", "C60", "NOVA FE-60", "Sommer '87 - Radio",
          0xff2c2b29, 0xffefe6d0, 0xffe0642c, 0xff26303f, 0xff3b2717, 0xff15110d, false, false, false,
          { 1.0f, 2.4f, .14f, 28, 9500, 85, 4, 900, 0, -1, .9f, .18f, .18f, .2f, .04f, 0, .08f, 0, false } },
        { "cr90", "CR-90", "Chrome", "TYPE II", "70 us", "C90", "NOVA CR-90", "Mixtape fuer Jana",
          0xff3b4855, 0xfff4f4f1, 0xff2f6fb6, 0xff1d2a6b, 0xff231a14, 0xff0f1216, false, false, false,
          { .9f, 1.8f, .07f, 32, 13500, 70, 3, 900, 0, 1.5f, .55f, .12f, .12f, .12f, .03f, 0, .04f, 0, false } },
        { "mx46", "MX-46", "Metal", "TYPE IV", "70 us", "C46", "NOVA MX-46", "MASTER - nicht loeschen!",
          0xff141414, 0xff202020, 0xffc9a24a, 0xffead9a4, 0xff1a1512, 0xff0b0b0b, false, false, false,
          { .75f, 1.25f, .04f, 30, 16000, 62, 2.5f, 900, 0, 2, .45f, .09f, .09f, .08f, .02f, 0, 0, 0, false } },
        { "fecr", "FeCr-60", "Ferrichrom", "TYPE III", "70 us", "C60", "NOVA FeCr-60", "Jazz am Sonntag",
          0xff6b5843, 0xffdccdab, 0xff7b2b2b, 0xff3a2a1a, 0xff2c1e14, 0xff17120d, false, false, false,
          { .95f, 2.0f, .22f, 40, 11500, 95, 3, 800, -4.5f, 2.5f, .75f, .2f, .22f, .2f, .05f, .02f, .15f, 0, false } },
        { "dash", "AUTOBAHN 94", "Sonnenbrand", "TYPE I", "120 us", "C90", "SUPER-TON C90", "Autobahn '94",
          0xffb8a374, 0xffebcbc3, 0xffcf8d82, 0xff5b3a6a, 0xff3d2a1a, 0xff1c160e, true, false, false,
          { 1.0f, 2.8f, .26f, 45, 6500, 105, 5, 1200, 1.5f, -2, 1.0f, .95f, .3f, 1.0f, .12f, .08f, .4f, 0, false } },
        { "dub7", "7. KOPIE", "Kopie von Kopie", "TYPE I", "120 us", "C60", "NOVA FE-60", "Kopie v. Kopie v. Kopie",
          0xff9aa0a6, 0xffe9e7df, 0xff7a7a7a, 0xff5d5d5d, 0xff3b2717, 0xff1a1714, false, false, false,
          { 1.1f, 3.0f, .16f, 70, 4300, 130, 6, 650, 3, -3, 1.9f, .3f, .28f, .35f, .08f, .03f, .5f, 2, false } },
        { "floh", "FLOHMARKT", "Wuehlkiste 50 Pf", "TYPE ?", "?", "C?", "MAXI-SOUND", "??? Mix (Seite A)",
          0xff4b3b5c, 0xffe6daa9, 0xff3d8a4b, 0xff7a1f1f, 0xff4a321e, 0xff18121a, false, true, false,
          { 1.05f, 2.6f, .2f, 50, 7200, 100, 4, 1500, 1, -1, 1.2f, .35f, .45f, .4f, .55f, .45f, .7f, 1.5f, false } },
        { "mc30", "MC-30", "Diktiergeraet", "MICRO", "-", "MC30", "NOVA MC-30", "Diktat 14.03.",
          0xff232629, 0xffd9dee4, 0xffc8342c, 0xff22313f, 0xff2a1f17, 0xff0e1013, false, false, true,
          { 1.2f, 4.5f, .3f, 380, 3400, 900, 3, 1800, 4, 0, 1.4f, .3f, .75f, .3f, .1f, .05f, .25f, 9, true } },
    }};
    return t;
}

// Display strings with umlauts (UTF-8), kept separate so the table above stays ASCII-safe.
inline const char* tapeDescription (int i)
{
    static const char* d[] = {
        u8"Normalband mit Eisenoxid. Warm und dunkel, sättigt früh und weich.",
        u8"Chromdioxid. Heller, straffer Bass, weniger Rauschen. Zerrt später, dafür härter.",
        u8"Reineisen-Partikel. Riesiger Headroom, knackige Transienten, die meisten Höhen.",
        u8"Die vergessene Doppelschicht von 1975. Hohle Mitten, glasige Höhen.",
        u8"Einen Sommer auf dem Armaturenbrett. Verzogenes Gehäuse, eiert wie verrückt.",
        u8"Sieben Mal überspielt. Muffig, mittig, Rauschen auf Rauschen.",
        u8"Vom Flohmarkt für 50 Pfennig. Dropouts, Knistern, frisst sich gern fest.",
        u8"Mikrokassette aus dem Diktiergerät. Mono, Telefonband, krachige Zerre.",
    };
    return d[i];
}

inline const char* tapeHand (int i)
{
    static const char* h[] = {
        u8"Sommer ’87 – Radio", u8"Mixtape für Jana", u8"MASTER – nicht löschen!", u8"Jazz am Sonntag",
        u8"Autobahn ’94", u8"Kopie v. Kopie v. Kopie", u8"??? Mix (Seite A)", u8"Diktat 14.03."
    };
    return h[i];
}

inline const char* tapeTitle (int i)
{
    static const char* t[] = { u8"Ferro", u8"Chrome", u8"Metal", u8"Ferrichrom", u8"Sonnenbrand",
                               u8"Kopie von Kopie", u8"Wühlkiste 50 Pf", u8"Diktiergerät" };
    return t[i];
}

inline const char* tapeEq (int i)
{
    static const char* e[] = { u8"120 µs", u8"70 µs", u8"70 µs", u8"70 µs", u8"120 µs", u8"120 µs", u8"?", u8"–" };
    return e[i];
}

//==============================================================================
struct SpeedChar { float f, lp, wow, flut, hiss, bump, wf; };

inline const SpeedChar& speedChar (int i)
{
    static const SpeedChar s[3] = {
        { 2.0f, 1.55f, .6f, .65f, .72f, 1.6f, 1.5f },  // 9.5 cm/s
        { 1.0f, 1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f }, // 4.8 cm/s
        { .5f,  .55f,  1.7f, 1.4f, 1.3f, .62f, .7f },  // 2.4 cm/s
    };
    return s[i < 0 ? 0 : (i > 2 ? 2 : i)];
}

//==============================================================================
// Factory presets. Values are parameter values in their own units:
// knobs 0..1, "tape" index, "speed" index (0 = 9.5, 1 = 4.8, 2 = 2.4), booleans 0/1, "dpos" 0 = PRE / 1 = POST.
struct FactoryPreset
{
    const char* category;
    const char* name;
    int tape;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> p {
        { "SUBTIL", u8"Werkseinstellung", 0, {} },
        { "SUBTIL", u8"Sauberes Master", 2, { {"rec",.5f},{"wow",.06f},{"flut",.06f},{"age",0},{"hiss",.1f},{"az",0},{"out",.81f},{"speed",0},{"nr",1} } },
        { "SUBTIL", u8"Bus-Kleber", 2, { {"rec",.55f},{"wow",.08f},{"flut",.08f},{"age",.05f},{"hiss",.12f},{"sq",.15f},{"mix",.6f},{"out",.88f},{"nr",1} } },
        { "SUBTIL", u8"Chrom-Glanz", 1, { {"rec",.6f},{"bias",.4f},{"wow",.12f},{"flut",.1f},{"age",.05f},{"hiss",.2f},{"out",.74f},{"speed",0},{"nr",1} } },
        { "SUBTIL", u8"Warme Drums", 0, { {"rec",.8f},{"bias",.6f},{"wow",.1f},{"flut",.1f},{"age",.15f},{"hiss",.15f} } },
        { "SUBTIL", u8"Sonntagsjazz", 3, { {"rec",.6f},{"wow",.3f},{"flut",.25f},{"age",.35f},{"hiss",.35f},{"mix",.85f},{"out",.81f} } },

        { "LOFI", u8"Lofi Beat Tape", 0, { {"rec",.68f},{"wow",.45f},{"flut",.32f},{"age",.4f},{"hiss",.45f},{"sq",.25f},{"narrow",.12f},{"out",.53f} } },
        { "LOFI", u8"Mixtape ’89", 0, { {"rec",.7f},{"wow",.4f},{"flut",.35f},{"age",.45f},{"hiss",.5f},{"az",.35f},{"out",.73f},{"nr",1} } },
        { "LOFI", u8"Walkman im Winter", 1, { {"wow",.75f},{"flut",.6f},{"age",.2f},{"hiss",.3f},{"nr",1} } },
        { "LOFI", u8"Halbe Geschwindigkeit", 0, { {"wow",.5f},{"flut",.4f},{"age",.45f},{"hiss",.4f},{"out",.79f},{"speed",2} } },
        { "LOFI", u8"Sonnenstich", 4, { {"wow",.55f},{"age",.5f},{"hiss",.35f},{"tilt",.4f},{"out",.79f} } },
        { "LOFI", u8"Autoradio", 4, { {"wow",.4f},{"age",.35f},{"hiss",.3f},{"narrow",.3f},{"sq",.4f},{"out",.5f} } },
        { "LOFI", u8"Vaporwave", 1, { {"wow",.85f},{"flut",.2f},{"age",.3f},{"hiss",.25f},{"speed",2} } },
        { "LOFI", u8"Proberaum", 0, { {"rec",.9f},{"pre",.3f},{"sq",.45f},{"age",.3f},{"hiss",.45f},{"narrow",.1f},{"out",.54f},{"mono",1} } },

        { "KAPUTT", u8"Flohmarkt-Fund", 6, { {"age",.5f},{"hiss",.55f},{"salat",.4f},{"grit",.3f},{"out",.78f} } },
        { "KAPUTT", u8"Kopie von Kopie", 5, { {"age",.6f},{"hiss",.6f},{"wow",.4f},{"sq",.3f},{"out",.55f} } },
        { "KAPUTT", u8"Bandsalat Deluxe", 6, { {"salat",.85f},{"grit",.45f},{"age",.6f},{"wow",.5f},{"out",.74f} } },
        { "KAPUTT", u8"Sterbende Batterie", 0, { {"wow",.9f},{"flut",.7f},{"age",.4f},{"salat",.35f},{"pre",.3f},{"grit",.2f},{"speed",2} } },
        { "KAPUTT", u8"Waschmaschine", 6, { {"age",1},{"salat",.7f},{"grit",.6f},{"crush",.25f},{"wow",.7f},{"flut",.7f},{"hiss",.6f},{"out",.76f} } },
        { "KAPUTT", u8"Anrufbeantworter", 7, { {"pre",.35f},{"crush",.3f},{"narrow",.35f},{"salat",.2f},{"grit",.3f},{"hiss",.5f},{"out",.47f} } },
        { "KAPUTT", u8"Walkie-Talkie", 7, { {"narrow",.65f},{"pre",.6f},{"crush",.2f},{"sq",.6f},{"hiss",.4f},{"out",.07f},{"dpos",1} } },
        { "KAPUTT", u8"Kaputter Sampler", 5, { {"crush",.6f},{"fold",.25f},{"narrow",.2f},{"dmix",.7f},{"out",.62f},{"mono",1} } },
        { "KAPUTT", u8"Geister-Pads", 5, { {"wow",.7f},{"age",.8f},{"fold",.3f},{"sq",.5f},{"dmix",.5f},{"out",.56f} } },

        { "EXTREM", u8"Fuzz-Kassette", 2, { {"pre",.85f},{"tilt",.68f},{"dmix",.85f},{"sq",.35f},{"out",.51f},{"dpos",1} } },
        { "EXTREM", u8"Bass-Fuzz", 0, { {"pre",.75f},{"tilt",.15f},{"dmix",.6f},{"out",.81f} } },
        { "EXTREM", u8"Kreissäge", 2, { {"pre",1},{"fold",.35f},{"tilt",.8f},{"sq",.5f},{"out",.73f},{"dpos",1} } },
        { "EXTREM", u8"8-Bit-Kassette", 1, { {"crush",.85f},{"fold",.15f},{"dmix",.9f},{"hiss",.2f},{"out",.58f} } },
        { "EXTREM", u8"Blechdose", 7, { {"narrow",.8f},{"fold",.4f},{"pre",.5f},{"out",.09f} } },
        { "EXTREM", u8"Totalschaden", 6, { {"pre",.9f},{"fold",.5f},{"crush",.6f},{"sq",.8f},{"salat",.8f},{"grit",.8f},{"narrow",.3f},{"age",1},{"out",.5f},{"dpos",1} } },
    };
    return p;
}

} // namespace nova
