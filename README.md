# Höribert 2.0

<p align="center">
  <img src="docs/images/hoeribert.PNG" alt="Höribert 2.0" width="800">
</p>

Höribert 2.0 ist ein eigenständiger, ESP32-basierter Hörspiel- und Audioplayer in der Hardware und mit dem Bedienkonzept eines NOXON-Geräts. RFID-Karten wählen Hörspielordner aus, ein DFPlayer Mini übernimmt die Audiowiedergabe und ein ILI9341-TFT zeigt Status oder Cover an. Die vorhandene NOXON-Tastenplatine und der analoge Lautstärkeregler werden weiterverwendet.

## Funktionen

- Wiedergabe nummerierter DFPlayer-Ordner über kompatible RFID-Karten
- Album-Modus mit automatischer Folge der Tracks
- Play/Pause, Stop, nächster und vorheriger Track
- manuell speicherbare Bookmarks pro RFID-UID sowie Bookmark beim Ablauf des Sleep-Timers
- Sleep-Timer in Zehn-Minuten-Schritten
- TFT-Statusanzeige und Coverbilder aus LittleFS
- analog geregelte Lautstärke
- separater Hardware-Testmodus mit Tasten-, IR-, RFID-, Display-, DFPlayer- und OTA-Diagnose
- Unterdrückung unmittelbar doppelter DFPlayer-Finish-Meldungen

Im Normalbetrieb wird derzeit ausschließlich der TonUINO-kompatible Kartenmodus 2 (Album) abgespielt. Andere auf Karten gespeicherte Modi werden erkannt, aber nicht ausgeführt.

## Hardware

| Komponente | Aufgabe |
| --- | --- |
| ESP32 Dev Module | Steuerung |
| DFPlayer Mini | Wiedergabe von der microSD-Karte |
| MFRC522 | Lesen der RFID-Karten |
| ILI9341-TFT | Status und Coverbilder |
| 2 × 74HC165 | Einlesen der NOXON-Fronttasten |
| analoges Potentiometer | Lautstärke |
| IR-Empfänger | Diagnose im Hardware-Testmodus |

## Pinbelegung

| Funktion | ESP32-Pin |
| --- | --- |
| 74HC165 DATA | GPIO19 |
| 74HC165 CLOCK | GPIO18 |
| 74HC165 LOAD | GPIO5 |
| IR-Empfänger | GPIO27 |
| Lautstärkepotentiometer | GPIO34 |
| DFPlayer RX | GPIO16 |
| DFPlayer TX | GPIO17 |
| RFID SS/CS | GPIO21 |
| RFID RST | GPIO22 |
| RFID SCK | GPIO14 |
| RFID MISO | GPIO23 |
| RFID MOSI | GPIO13 |
| TFT CS | GPIO25 |
| TFT DC | GPIO26 |
| TFT RST | GPIO33 |
| TFT Backlight | GPIO32 |
| TFT MOSI | GPIO13 |
| TFT SCK | GPIO14 |

TFT und RFID teilen sich MOSI und SCK. Beide Geräte besitzen einen eigenen Chip-Select-Anschluss.

Beim DFPlayer bezeichnet „RX“ den Empfangspin des ESP32 (Verbindung vom DFPlayer-TX) und „TX“ den Sendepin des ESP32 (Verbindung zum DFPlayer-RX). Für die konkrete Verdrahtung ist die Hardwaredokumentation des Moduls zu beachten.

## NOXON-Tastenplatine und 74HC165-Schieberegister

Auf der NOXON-Tastenplatine sitzen zwei kaskadierte 74HC165 Parallel-in/Serial-out-Schieberegister. Zusammen stellen sie 16 parallele Eingänge bereit, obwohl der ESP32 dafür nur drei Signalleitungen benötigt:

- `DATA` an GPIO19 transportiert die Bits seriell zum ESP32.
- `CLOCK` an GPIO18 schiebt jeweils das nächste Bit heraus.
- `LOAD` an GPIO5 übernimmt gleichzeitig den aktuellen Zustand aller parallelen Eingänge.

```text
NOXON-Tasten
    │
    ▼
┌─────────┐    ┌─────────┐
│ 74HC165 │───▶│ 74HC165 │─── DATA ──▶ GPIO19
└─────────┘    └─────────┘
     ▲              ▲
     └──── CLOCK ───┴──────── GPIO18
     └──── LOAD ───────────── GPIO5
```

`ButtonBoard::readRaw()` zieht `LOAD` kurz auf LOW und anschließend wieder auf HIGH. Danach erzeugt die Funktion 16 Clock-Impulse. Vor jedem Impuls liest sie den aktuellen DATA-Pegel, schiebt den bisherigen Wert um eine Stelle und setzt gegebenenfalls das niederwertigste Bit. So entsteht ein vollständiger `uint16_t`-Tastenstatus.

`ButtonBoard::update()` vergleicht diesen Status mit dem Wert des vorherigen Durchlaufs. Der Ausdruck `currentState & ~lastState` enthält nur Bits, die jetzt gesetzt waren, im vorherigen Zustand aber noch nicht. Dadurch reagiert die Bedienlogik auf neu gedrückte Tasten und nicht in jedem Programmdurchlauf erneut auf eine gehaltene Taste. Eine separate zeitbasierte Entprellung besitzt der aktuelle Code nicht.

Von den 16 möglichen Eingängen sind 14 den internen Projektbezeichnungen A bis N zugeordnet. Die physische NOXON-Beschriftung ist im aktuellen Repository nicht eindeutig abgebildet; deshalb verwendet die Dokumentation bewusst diese internen Namen.

| Interne Taste | Bit | Maske |
| --- | ---: | ---: |
| A | 9 | `0x0200` |
| B | 8 | `0x0100` |
| C | 3 | `0x0008` |
| D | 2 | `0x0004` |
| E | 1 | `0x0002` |
| F | 4 | `0x0010` |
| G | 10 | `0x0400` |
| H | 5 | `0x0020` |
| I | 7 | `0x0080` |
| J | 6 | `0x0040` |
| K | 14 | `0x4000` |
| L | 13 | `0x2000` |
| M | 12 | `0x1000` |
| N | 15 | `0x8000` |

Bit 0 und Bit 11 sind derzeit keiner Taste zugeordnet. Die jeweilige Funktion einer erkannten Taste hängt vom aktiven Betriebsmodus ab und ist im folgenden Abschnitt getrennt dokumentiert.

## Bedienung

Die Software verwendet die intern ermittelten Tastenbezeichnungen A bis N. Eine abweichende Beschriftung der physischen Front ist möglich.

### Normalbetrieb

| Taste | Funktion |
| --- | --- |
| A | Sleep-Timer um 10 Minuten verlängern; Doppeldruck löscht ihn |
| B | Display ein-/ausschalten |
| C | Cover- und Textanzeige umschalten |
| D | aktuelle Position als Bookmark speichern |
| E | Bookmark der aktiven Karte löschen |
| F | Stop |
| G | vorheriger Track |
| H | Play/Pause |
| I | nächster Track |

### Hardware-Testmodus

Taste J muss bereits beim Einschalten beziehungsweise Reset gedrückt gehalten werden. Im Testmodus gelten unter anderem:

| Taste | Funktion |
| --- | --- |
| F | Stop |
| G | vorheriger Track |
| H | Play/Pause |
| I | nächster Track |
| J | DFPlayer-Status anzeigen |
| L | nächster Ordner |
| N | vorheriger Ordner |

Zusätzlich protokolliert der Modus erkannte Tasten, IR-Daten, Lautstärke, RFID-Daten und DFPlayer-Ereignisse über TFT und serielle Schnittstelle. WLAN und OTA sind ausschließlich hier aktiv.

## RFID

Unterstützt werden MIFARE Classic sowie MIFARE Ultralight/NTAG in dem vom Projekt gelesenen TonUINO-kompatiblen Kartenformat. Erwartet werden das Cookie `13 37 B3 47`, eine Ordnernummer und eine Modusnummer. Im Normalbetrieb startet eine gültige Karte mit Modus 2 den zugehörigen Ordner.

Bookmarks werden lokal im NVS des ESP32 gespeichert und anhand der Karten-UID zugeordnet. Gespeichert werden Ordner, Track und die vom ESP32 geschätzte Laufzeit. Beim Fortsetzen startet die aktuelle Implementierung den gespeicherten Track von dessen Anfang; sie führt keinen Zeitsprung innerhalb der Audiodatei aus. Ein Bookmark entsteht nur über Taste D oder beim Ablauf des Sleep-Timers. Nach dem natürlichen Ordnerende wird es gelöscht.

### RFID-Kartenformat

Höribert verwendet ein zum ursprünglichen TonUINO kompatibles RFID-Kartenformat. Auf der Karte werden unter anderem folgende Werte gespeichert bzw. gelesen:

- Magic Cookie `13 37 B3 47`
- Version
- Ordnernummer (`folder`)
- Wiedergabemodus (`mode`)
- `special`
- `special2`

Bei MIFARE-Classic-Karten liegen diese Daten im Datenblock 4 des ersten benutzten Sektors. Höribert liest dieses Format, damit bereits vorhandene TonUINO-Karten weiterverwendet werden können. Das Kartenformat und die ursprüngliche Implementierung stammen aus dem
historischen TonUINO-Projekt von Thorsten Voß:

https://github.com/tonuino/TonUINO

Die aktuelle Höribert-Implementierung besitzt eine eigene RFID- und Anwendungslogik und verwendet lediglich das kompatible Kartenformat.

### Hinweis zum RC522-Modul

Im Höribert wurde erfolgreich ein blaues RC522-Modul mit der Platinenkennzeichnung **HW-126** eingesetzt. RC522-Module existieren in zahlreichen Platinenvarianten und werden auch mit unterschiedlichen kompatiblen Chips angeboten. Obwohl sie äußerlich nahezu
identisch aussehen können, unterscheiden sie sich in der Praxis teilweise deutlich bei Lesereichweite und Zuverlässigkeit. Insbesondere die Antennen- und Matching-Beschaltung der Platine kann einen großen Einfluss darauf haben, ob RFID-Karten zuverlässig erkannt werden.

Falls Karten trotz korrekter Verkabelung und Software nicht oder nur aus sehr kurzer Entfernung erkannt werden, empfiehlt es sich daher, ein anderes RC522-Modul zu testen. Die im Projekt getestete Platinenvariante trägt die Kennzeichnung:

**HW-126**

Die Kennzeichnung allein garantiert allerdings weder einen originalen NXP MFRC522 noch eine bestimmte Chiprevision.

## SD-Karten-Struktur

Die Wiedergabe verwendet den DFPlayer-Befehl `playFolder(folder, track)`. Dafür gelten folgende Konventionen:

```text
/01/001.mp3
/01/002.mp3
/02/001.mp3
```

- Ordner werden numerisch von `01` bis `99` benannt.
- Tracks werden innerhalb des Ordners dreistellig und lückenlos ab `001` benannt.
- Pro Ordner unterstützt der verwendete Befehl höchstens 255 Tracks.
- Versteckte Dateien, Lücken oder eine nachträglich stark veränderte Kopierreihenfolge können die vom DFPlayer gemeldete Nummerierung beeinflussen.

Die microSD-Karte des DFPlayers ist unabhängig vom LittleFS-Dateisystem für Coverbilder.

## Display und Coverbilder

Cover werden vom Benutzer lokal unter `data/` als JPG-Dateien bereitgestellt. Wegen möglicher Urheberrechte gehören solche Cover nicht zum Repository und werden durch `.gitignore` vom Commit ausgeschlossen.

Für Ordner 1 sucht die Firmware zunächst `/1.jpg` und danach `/01.jpg`; entsprechend können lokal beispielsweise `1.jpg` oder `01.jpg` verwendet werden. Die Zahl muss der Ordnernummer der zugehörigen RFID-Karte entsprechen. Fehlt ein Bild, zeigt das TFT die textbasierte Ordneransicht.

LittleFS lässt sich per USB hochladen:

```bash
pio run -e esp32dev_usb -t uploadfs
```

Ein Firmware-Upload überträgt die LittleFS-Daten nicht automatisch.

## Installation und Build

Benötigt werden VS Code mit PlatformIO oder eine lokale PlatformIO-CLI sowie ein ESP32-Toolchain-Setup.

```bash
git clone https://github.com/toor0001/Hoeribert-2.0.git
cd Hoeribert-2.0
cp include/secrets.example.h include/secrets.h
```

Anschließend in `include/secrets.h` die lokalen WLAN-Daten eintragen. Diese Datei ist ignoriert und darf nicht committed werden.

Build:

```bash
pio run -e esp32dev_usb
```

USB-Upload:

```bash
pio run -e esp32dev_usb -t upload
```

## WLAN und OTA

OTA ist nur im Hardware-Testmodus verfügbar:

1. Taste J gedrückt halten und Höribert einschalten oder zurücksetzen.
2. Auf die erfolgreiche WLAN-Verbindung und IP-Ausgabe warten.
3. OTA über den Hostnamen `noxon` starten.

```bash
pio run -e esp32dev -t upload
```

Falls mDNS im lokalen Netz nicht funktioniert, kann die Zieladresse explizit angegeben werden:

```bash
pio run -e esp32dev -t upload --upload-port <IP-ADRESSE>
```

Im Normalbetrieb wird WLAN bewusst abgeschaltet. Ein dort gestartetes Gerät ist daher nicht per OTA erreichbar.

## Projektstruktur

```text
src/          Firmware und Hardware-Abstraktionen
include/      lokale Konfigurationsvorlage
data/         LittleFS-Coverbilder
docs/         ergänzende aktuelle Dokumentation
test/         PlatformIO-Tests, sofern künftig ergänzt
```

## Credits / Herkunft

Höribert 2.0 ist eine eigenständige ESP32-/NOXON-Implementierung. Das kompatible RFID-Kartenformat und die grundlegende Idee der kartengesteuerten Ordnerwahl gehen auf das historische [TonUINO-Projekt](https://github.com/tonuino/TonUINO) von Thorsten Voß zurück. Das heutige Community-Projekt wird als [TonUINO-TNG](https://github.com/tonuino/TonUINO-TNG) weiterentwickelt. In der bereinigten, erreichbaren Git-Historie dieses Repositorys befindet sich kein direkt übernommener TonUINO-Quellcode.

Die über PlatformIO eingebundenen Bibliotheken, darunter DFRobotDFPlayerMini, MFRC522, Adafruit GFX/ILI9341, IRremote und TJpg_Decoder, unterliegen jeweils ihren eigenen Lizenzen. Lokal verwendete Coverbilder sind nicht Bestandteil des Projekts oder seiner Lizenz.

## Entwicklung

Bei Analyse, Refactoring, Fehlersuche und Dokumentation kamen ChatGPT und OpenAI Codex als KI-gestützte Entwicklungswerkzeuge zum Einsatz. Konzept, Hardwareaufbau, Anforderungen, Integration und Tests am realen Gerät stammen vom Projektbetreiber.

## Lizenz

Der eigenständige Projektcode und die beiden Projektdokumentationsfotos stehen unter der [MIT-Lizenz](LICENSE). Lizenzen externer Bibliotheken und lokal verwendeter Medien bleiben davon unberührt.

## Einblicke ins Gerät

<p align="center">
  <img src="docs/images/hoeribert-innenraum.jpg" alt="Innenraum des Höribert 2.0 mit Lautsprecher und Elektronik" width="45%">
  <img src="docs/images/hoeribert-elektronik.jpg" alt="Elektronik und NOXON-Tastenplatine im Höribert 2.0" width="45%">
</p>
