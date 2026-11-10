# Klangbild Vibe

Tremolo und Uni-Vibe im Klangbild-Vintage-Look – AU, VST3 und Standalone für macOS (Logic Pro).

## Modi

| Modus | Klang |
|---|---|
| **Tremolo** | Optisches Amp-Tremolo: Lampe und Fotowiderstand, Form von weichem Sinus bis hartem Rechteck |
| **Harmonisch** | „Brownface“-Tremolo: Tiefen und Höhen pulsieren gegenläufig – weicher, fast phasig |
| **Vibe Chorus** | Vier gestaffelte Phasenstufen, von einer pulsierenden Glühbirne gesteuert, gemischt mit dem Direktsignal |
| **Vibe Vibrato** | Nur das Effektsignal – echtes Tonhöhen-Vibrato |

Das Fenster in der Mitte zeigt die Glühbirne und die beiden Fotowiderstände; die Lampe pulsiert mit der Modulation.

## Regler

| Regler | Aufgabe |
|---|---|
| **Geschwindigkeit** | 0,3 – 12 Hz (frei) |
| **Tiefe** | Stärke der Modulation |
| **Modus** | Drehschalter: Tremolo · Harmonisch · Chorus · Vibrato |
| **Form** | Wellenform beim Tremolo (Sinus → Rechteck) |
| **Notenwert** | Bei „Im Takt“: 1/2, 1/4, 1/4 punktiert, 1/8, 1/8 Triole, 1/16 – folgt dem Logic-Tempo und der Song-Position |
| **Tempo** | Frei oder im Takt |
| **Stereo-Bild** | Mono oder Stereo (Tremolo pendelt links/rechts, Vibe versetzt) |
| **Ausgang** | Ausgangspegel |

## Presets

Standard · Amp-Tremolo – klassisch · Surf – schnell & tief · Harmonisch – Brownface · Vibe – Chorus ·
Vibe – Vibrato · Soul – langsamer Vibe · Stereo-Pendel · Im Takt – Achtel

Alle Presets sind auf etwa gleiche Lautstärke eingepegelt.

## Installation

Über den [Klangbild Installer](https://github.com/Guitoli1/KlangbildInstaller):

```
curl -fsSL https://raw.githubusercontent.com/Guitoli1/KlangbildInstaller/main/install.sh | bash
```

## Build

JUCE 8 / CMake. GitHub Actions baut auf macOS 15 ein Universal Binary (arm64 + x86_64), signiert ad-hoc und prüft mit `auval`.
