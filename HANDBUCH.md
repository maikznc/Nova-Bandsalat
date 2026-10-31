# NOVA Bandsalat BS-77 – Benutzerhandbuch

Version 0.1 · VST3 / AU / Standalone für macOS und Windows

---

## Inhalt

1. [Was ist der Bandsalat?](#1-was-ist-der-bandsalat)
2. [Schnellstart](#2-schnellstart)
3. [Die Oberfläche im Überblick](#3-die-oberfläche-im-überblick)
4. [Die Kassetten](#4-die-kassetten)
5. [Tasten und Gesten](#5-tasten-und-gesten)
6. [Die Schalter](#6-die-schalter)
7. [Die Regler](#7-die-regler)
8. [Die Dirt Section](#8-die-dirt-section)
9. [Presets und A/B-Vergleich](#9-presets-und-ab-vergleich)
10. [Anzeigen](#10-anzeigen)
11. [In der DAW: Automation, Latenz, Export](#11-in-der-daw-automation-latenz-export)
12. [Rezepte](#12-rezepte)
13. [Signalweg](#13-signalweg)
14. [Der Web-Prototyp](#14-der-web-prototyp)

---

## 1. Was ist der Bandsalat?

Der Bandsalat ist ein **Insert-Effekt**, der deine Spur klingen lässt, als liefe sie über ein altes Kassettendeck – von leichter Bandwärme bis zum völlig zerstörten Signal. Er erzeugt selbst keine Musik, er verarbeitet und verzerrt, was hineinkommt.

Im Gegensatz zu den meisten Tape-Plugins ist die Kassette hier ein Objekt: Du wählst sie aus dem Regal, legst sie ein, und sie nutzt sich beim Abspielen ab.

Installation: siehe [INSTALL.md](INSTALL.md).

---

## 2. Schnellstart

1. Den Bandsalat als Insert auf eine Spur oder einen Bus legen.
2. Playback in der DAW starten. Die Spulen drehen sich, die VU-Meter schlagen aus.
3. Eine andere Kassette im **Kassettenregal** anklicken. Das Deck wirft aus und legt die neue ein.
4. **REC LEVEL** aufdrehen, um das Band stärker zu sättigen.
5. Für richtig dreckigen Sound: **PREAMP** in der Dirt Section aufdrehen.
6. Oder ein **Preset** wählen: Klick auf die Preset-Anzeige oben im Deck.

---

## 3. Die Oberfläche im Überblick

| Bereich | Was du dort findest |
|---|---|
| **Kassettenregal** (oben) | Die acht Kassetten mit Abnutzungsbalken. Anklicken legt sie ein. |
| **Preset-Leiste** | Preset-Anzeige, Vor/Zurück, A/B-Vergleich, Kopieren, Speichern |
| **Kassettenfach** | Die eingelegte Kassette mit drehenden Spulen. Gedrückt halten bremst das Band. |
| **Anzeigen** | Bandzählwerk, zwei VU-Meter, SAT-Lampe, Bandtyp und Geschwindigkeit |
| **Schalter** | MONITOR, NR, SPEED, MODE, HUM, WEAR, RANDOM |
| **Tasten** | TAPE ◀◀, TAPE ▶▶, BRAKE, KICK, NEU |
| **Regler** | RECORD, CONDITION, PLAYBACK – je drei Regler |
| **Dirt Section** | Die schwarze Reihe: OVERLOAD, DESTROY, COLOR – je drei Regler |

Das Fenster lässt sich an der unteren rechten Ecke größer und kleiner ziehen.

---

## 4. Die Kassetten

Jede Kassette ist ein eigener **Bandtyp** mit eigener Sättigung, eigenem Frequenzgang, Grundrauschen und mechanischem Zustand. Die Regler wirken immer zusätzlich zum Charakter der Kassette.

| Kassette | Typ | EQ | Charakter |
|---|---|---|---|
| **FE-60 Ferro** | Type I | 120 µs | Normalband. Warm, dunkel, sättigt früh und weich. Der Allrounder. |
| **CR-90 Chrome** | Type II | 70 µs | Heller, straffer Bass, weniger Rauschen. Zerrt später, dafür härter. |
| **MX-46 Metal** | Type IV | 70 µs | Riesiger Headroom, knackige Transienten, die meisten Höhen. Für sauberere Ergebnisse. |
| **FeCr-60 Ferrichrom** | Type III | 70 µs | Die vergessene Doppelschicht von 1975. Hohle Mitten, glasige Höhen. |
| **Autobahn 94** | Type I, verzogen | 120 µs | Lag einen Sommer auf dem Armaturenbrett. Eiert stark, dumpf. |
| **7. Kopie** | Type I, 7. Generation | 120 µs | Sieben Mal überspielt. Muffig, mittig, viel Rauschen. |
| **Flohmarkt** | unbekannt | ? | Dropouts, Knistern, frisst sich gern fest. |
| **MC-30 Diktiergerät** | Mikrokassette | – | Mono, Telefonband, krachige Zerre, viel Flattern. |

Die Kassette ist ein normaler Parameter („Cassette") und lässt sich automatisieren.

### Abnutzung (WEAR)

Steht der Schalter **WEAR** auf ON, nutzt sich jede Kassette ab, während die DAW abspielt. Nach etwa sechs Minuten Spielzeit ist sie komplett verbraucht: dumpfer, mehr Dropouts, mehr Knistern. Den Zustand siehst du als Balken unter jeder Kassette im Regal.

Die Taste **NEU** packt die eingelegte Kassette frisch aus und setzt sie auf 0 % zurück. Die Abnutzung wird mit deinem Projekt gespeichert.

Beim Offline-Export und bei RANDOM LOCK nutzt sich nichts ab, damit Exporte reproduzierbar bleiben.

---

## 5. Tasten und Gesten

| Taste | Funktion |
|---|---|
| **TAPE ◀◀ / TAPE ▶▶** | Vorherige bzw. nächste Kassette einlegen |
| **BRAKE** | Gedrückt halten: Fingerbremse (siehe unten) |
| **KICK** | Ein Tritt gegen das Gerät: Das Deck wackelt, die VU-Nadeln zucken, das Band setzt aus und hakt. |
| **NEU** | Die eingelegte Kassette frisch auspacken (Abnutzung auf 0 %) |

### Fingerbremse

Halte die Maus auf dem **Kassettenfenster** gedrückt (oder die Taste BRAKE). Dein Finger bremst das Band: Die Tonhöhe sinkt und das Band fällt immer weiter zurück. Beim Loslassen springt das Band zurück auf die aktuelle Position.

Die Fingerbremse ist auch ein Parameter („Finger Brake") – du kannst sie also in der DAW automatisieren, zum Beispiel für Tape-Stop-artige Effekte auf dem letzten Schlag eines Takts.

### Bandzählwerk

Zählt mit, solange das Band läuft. Der runde Knopf daneben setzt es auf 000 zurück.

---

## 6. Die Schalter

Ein Klick auf einen Hebel schaltet zur nächsten Stellung.

| Schalter | Stellungen | Funktion |
|---|---|---|
| **MONITOR** | TAPE / SOURCE | **SOURCE** hört das unbearbeitete Signal (Bypass). Zum Vergleichen. |
| **NR** | OFF / ON | Rauschunterdrückung. Senkt das Bandrauschen deutlich und dämpft in leisen Stellen die Höhen. Dadurch „atmet" der Klang, wie bei einer falsch eingemessenen Rauschunterdrückung. |
| **SPEED** | 9.5 / 4.8 / 2.4 cm/s | Bandgeschwindigkeit. **9.5:** mehr Höhen, weniger Leiern und Rauschen. **4.8:** Standard. **2.4:** dumpf, stärkeres Leiern, mehr Rauschen. |
| **MODE** | STEREO / MONO | Mono-Summe. Die MC-30 ist immer mono. |
| **HUM** | OFF / 50 Hz | Netzbrummen mit Obertönen. |
| **WEAR** | OFF / ON | Kassetten nutzen sich beim Abspielen ab (Kapitel 4). |
| **RANDOM** | LIVE / LOCK | **LIVE:** Dropouts, Bandsalat und Leiern sind bei jedem Durchlauf anders. **LOCK:** Der Zufall startet bei jedem Playback-Start an derselben Position gleich. Zwei Exporte klingen dadurch identisch. |

---

## 7. Die Regler

**Bedienung:** Hoch und runter ziehen. **Doppelklick** setzt den Regler auf den Standardwert zurück. Unter jedem Regler steht der aktuelle Wert.

### RECORD

| Regler | Bereich | Standard | Funktion |
|---|---|---|---|
| **REC LEVEL** | −12 bis +18 dB | +6.6 dB | Aufnahmepegel aufs Band. Mehr Pegel = mehr Sättigung und Kompression. Die **SAT**-Lampe zeigt, wann das Band hart angefahren wird. Auch bei Vollausschlag clippt es nicht digital, sondern läuft weich in die Sättigung. |
| **BIAS** | UNDER – CAL – OVER | CAL | Vormagnetisierung. **UNDER:** heller, mehr Verzerrung. **OVER:** dunkler, sauberer. CAL ist die korrekte Einmessung. |
| **AZIMUTH** | 0 – 0.60 ms | 0.09 ms | Schräg stehender Tonkopf. Verschiebt den rechten Kanal zeitlich: das Stereobild verschmiert, in Mono wird es phasig. |

### CONDITION

| Regler | Bereich | Standard | Funktion |
|---|---|---|---|
| **WOW** | 0 – 100 % | 35 % | Langsames Leiern der Tonhöhe, dazu ein unregelmäßiges Driften, das sich praktisch nie wiederholt. |
| **FLUTTER** | 0 – 100 % | 30 % | Schnelles, unregelmäßiges Flattern von Capstan und Andruckrolle. Macht den Ton rau. |
| **AGE** | 0 – 100 % | 30 % | Alter des Bandes. Weniger Höhen, mehr Dropouts, leises Geister-Echo (Print-Through). Ab etwa 45 % kommt Knistern dazu, ab etwa 50 % hakt das Band gelegentlich. |

### PLAYBACK

| Regler | Bereich | Standard | Funktion |
|---|---|---|---|
| **HISS** | 0 – 100 % | 40 % | Bandrauschen. **0 = kein Rauschen.** Wie stark es rauscht, hängt auch von der Kassette ab. Rauschen und Knistern laufen nur, während die DAW abspielt. |
| **MIX** | 0 – 100 % | 100 % | Verhältnis von Original- und Bandsignal. Beide Signale sind zeitlich exakt ausgerichtet. |
| **OUTPUT** | −24 bis +6 dB | −3 dB | Ausgangspegel. Ein Limiter am Ende schützt vor Übersteuerung. |

---

## 8. Die Dirt Section

Die schwarze Reihe unten ist für alle, die es richtig dreckig wollen. **Steht alles auf 0, ist die Dirt Section komplett aus** und verändert den Klang nicht. Die rote **DIRT**-Lampe leuchtet umso stärker, je mehr Dreck aktiv ist.

### PRE / POST TAPE

Der Schiebeschalter oben rechts bestimmt, wo die Verzerrung sitzt:

- **PRE:** vor dem Band. Die Verzerrung wird anschließend vom Band gesättigt, gerundet und verleiert. Klingt organischer.
- **POST TAPE:** nach dem Band. Auch Rauschen und Knistern werden mitverzerrt. Klingt deutlich härter.

### OVERLOAD

| Regler | Bereich | Funktion |
|---|---|---|
| **PREAMP** | OFF – +42 dB | Übersteuerter Vorverstärker mit asymmetrischem Clipping. Bei hohen Werten stottert er in leisen Stellen wie ein Fuzz mit schwacher Batterie. |
| **FOLD** | OFF – 100 % | Wavefolder. Faltet die Wellenform zurück, statt sie abzuschneiden. Metallisch, kaputt, obertonreich. |
| **CRUSH** | OFF – ca. 1,8 Bit | Reduziert Bittiefe **und** Sample-Rate (bis etwa 1,2 kHz). Die Anzeige zeigt beides, z. B. „4.9b 4.3k". |

### DESTROY

| Regler | Bereich | Funktion |
|---|---|---|
| **SQUASH** | OFF – 20:1 | Pumpender Kompressor wie die Aussteuerungsautomatik billiger Kassettenrekorder. Zieht leise Stellen und das Rauschen hoch. |
| **SALAT** | OFF – 100 % | Das Band frisst sich fest: häufiges Haken, Dropouts und plötzliches Abbremsen. |
| **GRIT** | OFF – 100 % | Rauschen und Kratzen, das direkt am Signal klebt (Modulationsrauschen), dazu Knistern. |

### COLOR

| Regler | Bereich | Funktion |
|---|---|---|
| **TILT** | FAT – FLAT – FIZZ | Bestimmt, welche Frequenzen verzerrt werden. **FAT:** der Bass zerrt, fett und matschig. **FIZZ:** die Höhen zerren, zischend und scharf. Der Gesamtklang bleibt dabei gleich, nur die Verzerrung ändert sich. Wirkt nur, wenn PREAMP, FOLD oder CRUSH aktiv sind. |
| **NARROW** | OFF – 600 Hz bis 2 kHz | Engt die Bandbreite ein, bis es nach Telefon oder Radio klingt. Mit leichter Resonanz an den Kanten. |
| **DIRT MIX** | 0 – 100 % | Mischt die OVERLOAD-Verzerrung parallel zum unverzerrten Signal. Praktisch, um Druck zu behalten. |

---

## 9. Presets und A/B-Vergleich

### Presets

Klick auf die **Preset-Anzeige**, um die Liste zu öffnen. Mit **◀ ▶** blätterst du durch. Hat ein Preset eine andere Kassette, wechselt das Deck sie automatisch. Ein **\*** hinter dem Namen bedeutet: Du hast seit dem Laden etwas verändert.

Die Werkspresets stehen auch in der Programmliste deiner DAW.

| Kategorie | Presets |
|---|---|
| **SUBTIL** | Werkseinstellung, Sauberes Master, Bus-Kleber, Chrom-Glanz, Warme Drums, Sonntagsjazz |
| **LOFI** | Lofi Beat Tape, Mixtape ’89, Walkman im Winter, Halbe Geschwindigkeit, Sonnenstich, Autoradio, Vaporwave, Proberaum |
| **KAPUTT** | Flohmarkt-Fund, Kopie von Kopie, Bandsalat Deluxe, Sterbende Batterie, Waschmaschine, Anrufbeantworter, Walkie-Talkie, Kaputter Sampler, Geister-Pads |
| **EXTREM** | Fuzz-Kassette, Bass-Fuzz, Kreissäge, 8-Bit-Kassette, Blechdose, Totalschaden |

Alle Werkspresets sind auf ähnliche Lautstärke abgeglichen, damit du sie fair vergleichen kannst.

### Eigene Presets

**SAVE** fragt nach einem Namen und speichert Kassette, alle Regler und Schalter (außer MONITOR und RANDOM). Eigene Presets erscheinen in der Liste unter **EIGENE** und stehen in jedem Projekt zur Verfügung. Sie liegen als XML-Dateien hier:

- **macOS:** `~/Library/NOVA/Bandsalat/Presets/`
- **Windows:** `%APPDATA%\NOVA\Bandsalat\Presets\`

Über **Preset-Ordner zeigen** im Preset-Menü öffnest du den Ordner direkt – zum Sichern oder Weitergeben.

### A/B-Vergleich

- **A** und **B** sind zwei Speicherplätze. Beim Umschalten wird die aktuelle Einstellung im aktiven Platz gemerkt und die andere geladen.
- **A→B** (bzw. **B→A**) kopiert die aktuelle Einstellung in den anderen Platz. So baust du eine Variante und vergleichst sie direkt mit dem Ausgangspunkt.

---

## 10. Anzeigen

| Anzeige | Bedeutung |
|---|---|
| **VU-Meter L / R** | Ausgangspegel mit echter VU-Trägheit. 0 VU entspricht −16 dBFS (RMS). |
| **SAT-Lampe** | Leuchtet, wenn das Signal das Band hart in die Sättigung fährt. |
| **DIRT-Lampe** | Glüht umso stärker, je mehr in der Dirt Section aktiv ist. |
| **Spulen** | Drehen mit der echten Bandgeschwindigkeit, solange die DAW abspielt. Die Wickel werden links kleiner und rechts größer. Beim Bremsen werden sie langsamer. |
| **Zeile unter den VU-Metern** | Eingelegte Kassette, Typ, Entzerrung (µs) und Bandgeschwindigkeit |

---

## 11. In der DAW: Automation, Latenz, Export

- **Automation:** Alle Regler, Schalter, die Kassette und die Fingerbremse sind automatisierbar.
- **Latenz:** Das Plugin meldet etwa 31 ms Latenz an die DAW (Platz für Leiern und Flattern). Die DAW gleicht das automatisch aus. Beim Live-Einspielen hörst du diese Verzögerung, deshalb das Plugin erst nach der Aufnahme einschalten oder den Low-Latency-Modus deiner DAW nutzen.
- **Mono und Stereo:** Funktioniert auf Mono- und Stereospuren.
- **Export:** Für reproduzierbare Exporte **RANDOM auf LOCK** stellen. Dann klingt jeder Export ab derselben Position identisch, und keine Kassette nutzt sich ab. Beim Offline-Export friert die Abnutzung auch in LIVE automatisch ein.
- **Rauschen bei gestopptem Transport:** Rauschen, Knistern und Brummen laufen nur während des Playbacks. Bei gestoppter DAW bleibt es still.
- **CPU:** Sehr sparsam – auch mit allen Effekten unter 2 % eines Prozessorkerns.

---

## 12. Rezepte

**Warme Drums ohne sichtbaren Effekt**
FE-60, REC LEVEL ca. +12 dB, WOW und FLUTTER auf 10 %, HISS auf 0, AGE auf 10 %.

**Lofi-Hip-Hop-Loop**
Preset „Lofi Beat Tape". Für mehr Nostalgie SPEED auf 2.4 stellen und AGE erhöhen.

**Vocal aus dem Anrufbeantworter**
MC-30, NARROW auf 50 %, PREAMP auf 30 %, SALAT auf 20 %.

**Gitarren-Fuzz**
MX-46, PREAMP auf 80 %, TILT Richtung FIZZ, POST TAPE, DIRT MIX auf 80 %. OUTPUT runter.

**Ambient-Pads, die zerfallen**
7. Kopie, WOW auf 70 %, AGE auf 80 %, SQUASH auf 50 %. NR auf ON für das „Atmen".

**Bass mit Druck und Dreck**
PREAMP auf 60 %, TILT Richtung FAT, DIRT MIX auf 50 %. So bleibt das Fundament sauber.

**Tape-Stop am Taktende**
„Finger Brake" in der DAW automatisieren: auf dem letzten Viertel an, auf der Eins wieder aus.

---

## 13. Signalweg

```
Eingang
  ├─ Original (latenzausgeglichen) ──────────────────────────┐ (MIX)
  └─ 2-fach Oversampling:                                    │
       [Dirt Section, Stellung PRE]                          │
         TILT → PREAMP → FOLD → CRUSH → TILT invers ↔ DIRT MIX
       → REC LEVEL → Pre-Emphasis / BIAS                     │
       → Bandsättigung (Kurve je Kassette)                   │
     → Head Bump, Mitten-EQ, Höhenverlust (AGE, SPEED)       │
     → GRIT (Modulationsrauschen)                            │
     → WOW / FLUTTER / Drift / Bandsalat / Fingerbremse      │
       (modulierte Verzögerung)                              │
     → AZIMUTH (rechter Kanal verzögert)                     │
     → MODE (Stereo / Mono)                                  │
     → Dropouts                                              │
     → + Print-Through-Echo, Rauschen (HISS), Knistern       │
     → [Dirt Section, Stellung POST TAPE]                    │
     → SQUASH → NR → NARROW                                  │
     → Bandsignal ───────────────────────────────────────────┤
                                                             ↓
                                              + HUM → OUTPUT → Limiter → Ausgang
```

---

## 14. Der Web-Prototyp

Im Ordner `docs/` liegt der ursprüngliche Web-Prototyp. Er läuft im Browser, auch auf dem Handy, und benutzt dieselbe Klang-Engine in JavaScript. Weil eine Webseite kein Audio aus der DAW bekommt, hat er Demo-Loops, einen Testton und einen Datei-Upload sowie Laufwerkstasten (PLAY, STOP, REW, FF, EJECT).

Online stellen: In den Repository-Einstellungen unter **Settings → Pages** den Branch `main` und den Ordner `/docs` wählen.
