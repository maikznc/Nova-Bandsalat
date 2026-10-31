# NOVA Bandsalat BS-77

**Kassettendeck-Effekt, der kaputt klingen darf.** VST3 · AU · Standalone für macOS und Windows.

Der Bandsalat lässt deine Spur klingen, als liefe sie über ein altes Kassettendeck – von leichter Bandwärme bis zum völlig zerstörten Signal. Die Kassette ist dabei ein Objekt: Du wählst sie aus dem Regal, legst sie ein, und sie nutzt sich beim Abspielen ab.

## Features

- **8 Kassetten als Bandtypen:** Ferro (Type I), Chrome (Type II), Ferrichrom (Type III), Metal (Type IV), dazu eine sonnenverzogene, eine 7. Kopie, eine Flohmarkt-Kassette und eine Diktiergerät-Mikrokassette
- **Abnutzung:** Kassetten werden beim Abspielen dumpfer und löchriger
- **Tape-Engine:** Sättigung mit Oversampling, Bias, Azimuth, unregelmäßiges Wow und Flutter, Drift, Dropouts, Print-Through, Rauschen, Knistern, Netzbrummen, atmende Rauschunterdrückung, drei Bandgeschwindigkeiten
- **Dirt Section:** Preamp-Fuzz, Wavefolder, Bit- und Sample-Rate-Reduktion, pumpender Kompressor, Bandsalat, Modulationsrauschen, Tilt, Bandbreite, Parallel-Mix – wahlweise vor oder nach dem Band
- **Fingerbremse** (automatisierbar) und **KICK** gegen das Gerät
- **29 Werkspresets** in vier Kategorien, eigene Presets, A/B-Vergleich
- **RANDOM LOCK** für identische Exporte
- Sehr sparsam: unter 2 % eines CPU-Kerns

Bedienung: **[HANDBUCH.md](HANDBUCH.md)** · Installation: **[INSTALL.md](INSTALL.md)**

## Plugin herunterladen

GitHub baut das Plugin bei jedem Push automatisch für macOS und Windows.

1. Im Repository oben auf **Actions** gehen.
2. Den neuesten Lauf von **Build plugin** öffnen (grüner Haken = fertig, dauert etwa 10–15 Minuten).
3. Unten unter **Artifacts** `NOVA-Bandsalat-macOS` oder `NOVA-Bandsalat-Windows` herunterladen.

Für einen öffentlichen Download-Link ein Release erstellen: unter **Releases → Draft a new release** einen Tag wie `v0.1.0` anlegen und veröffentlichen. Der Build hängt die ZIP-Dateien dann automatisch an das Release.

| Download | Inhalt |
|---|---|
| macOS | `NOVA Bandsalat.vst3`, `NOVA Bandsalat.component` (AU), `NOVA Bandsalat.app` – Universal (Apple Silicon und Intel), ab macOS 10.13 |
| Windows | `NOVA Bandsalat.vst3`, `NOVA Bandsalat.exe` – 64 Bit |

## Selbst bauen

Voraussetzungen: CMake 3.22+, ein C++17-Compiler (Xcode auf dem Mac, Visual Studio 2022 unter Windows) und Git. JUCE wird beim ersten Konfigurieren automatisch heruntergeladen.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Die fertigen Plugins liegen danach in `build/NovaBandsalat_artefacts/Release/`.

Auf dem Mac als Universal Binary: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` beim ersten Befehl ergänzen.

## Projektstruktur

```
.github/workflows/build.yml   Automatischer Build für macOS und Windows
CMakeLists.txt                Build-Konfiguration (JUCE 8.0.12)
Source/
  TapeEngine.h/.cpp           Die komplette Klang-Engine
  TapeData.h                  Kassetten, Bandgeschwindigkeiten, Werkspresets
  PluginProcessor.h/.cpp      Parameter, Presets, Projektspeicherung, Abnutzung
  PluginEditor.h/.cpp         Oberfläche: Regal, Deck, Regler, Dirt Section
  NovaLook.h/.cpp             Zeichnen: Kassetten, VU-Meter, Regler, gebürstetes Metall
Resources/Fonts/              Schriften (SIL Open Font License)
docs/                         Web-Prototyp (läuft im Browser, auch über GitHub Pages)
HANDBUCH.md                   Benutzerhandbuch
INSTALL.md                    Installationsanleitung (liegt auch jedem Download bei)
```

## Web-Prototyp

Der ursprüngliche Prototyp liegt in `docs/` und läuft direkt im Browser. Unter **Settings → Pages** Branch `main` und Ordner `/docs` wählen, dann ist er unter `https://<name>.github.io/<repo>/` erreichbar.

## Hinweise

- **Nicht signiert:** Die Downloads sind weder von Apple notarisiert noch für Windows signiert. Wie du sie trotzdem freigibst, steht in [INSTALL.md](INSTALL.md). Für den Verkauf brauchst du ein Apple-Developer-Konto (Notarisierung) und ein Code-Signing-Zertifikat für Windows.
- **AAX (Pro Tools)** ist noch nicht dabei. Dafür braucht es das AAX-SDK und eine Signierung über Avid.

## Lizenzen

- **Code:** Noch keine Lizenz festgelegt. Ohne Lizenzdatei liegen alle Rechte bei NOVA.
- **JUCE** ist unter AGPLv3 oder einer kommerziellen Lizenz erhältlich. Bevor du das Plugin verkaufst oder ohne Quellcode weitergibst, prüfe die Lizenzbedingungen auf [juce.com](https://juce.com). Für VST3 gelten zusätzlich die Bedingungen von Steinberg.
- **Schriften** (Michroma, Archivo, Reenie Beanie, IBM Plex Mono): SIL Open Font License, siehe `Resources/Fonts/OFL-LICENSES.txt`.
