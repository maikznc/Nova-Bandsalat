# NOVA Bandsalat installieren

## macOS

Im Ordner liegen drei Dateien:

| Datei | Wohin |
|---|---|
| `NOVA Bandsalat.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |
| `NOVA Bandsalat.component` (Audio Unit, z. B. für Logic) | `~/Library/Audio/Plug-Ins/Components/` |
| `NOVA Bandsalat.app` (läuft ohne DAW) | `Programme` |

Den Ordner `Library` erreichst du im Finder über **Gehe zu → Gehe zum Ordner …** und die Eingabe `~/Library/Audio/Plug-Ins`.

**Wichtig:** Das Plugin ist noch nicht von Apple notarisiert. macOS blockiert es deshalb beim ersten Start. So gibst du es frei:

1. Terminal öffnen.
2. Diese Zeilen einfügen und Enter drücken:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"NOVA Bandsalat.vst3"
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"NOVA Bandsalat.component"
   xattr -dr com.apple.quarantine /Applications/"NOVA Bandsalat.app"
   ```
3. DAW neu starten. Logic prüft neue Audio Units beim Start automatisch.

## Windows

| Datei | Wohin |
|---|---|
| `NOVA Bandsalat.vst3` (Ordner) | `C:\Program Files\Common Files\VST3\` |
| `NOVA Bandsalat.exe` (läuft ohne DAW) | beliebig |

Danach in der DAW nach neuen Plugins suchen lassen.

Windows zeigt beim ersten Start der `.exe` eventuell eine SmartScreen-Warnung, weil die Datei nicht signiert ist. Klicke auf **Weitere Informationen → Trotzdem ausführen**.

## Standalone-App

Die App verarbeitet dein Audio-Interface oder Mikrofon in Echtzeit. Ein- und Ausgang stellst du über **Options → Audio/MIDI Settings** ein. Kopfhörer benutzen, sonst gibt es eine Rückkopplung.
