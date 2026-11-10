#pragma once

// Klangbild Vibe – Bausteine
//  Tremolo     : optisches Amp-Tremolo (Lampe + Fotowiderstand), Form von Sinus bis Rechteck
//  Harmonisch  : "Brownface"-Tremolo – Tiefen und Höhen pulsieren gegenläufig
//  Vibe        : vier gestaffelte Allpass-Stufen, deren Frequenzen eine pulsierende Lampe
//                über Fotowiderstände verschiebt; Chorus (mit Direktsignal) oder Vibrato (nur Effekt)

#include <cmath>
#include <array>
#include <algorithm>
#include "EQDsp.h"

namespace vibe
{
constexpr double twoPi = 6.283185307179586;

// Lampe -> Fotowiderstand: schnelles Aufglühen, träges Abklingen (das typische "Pumpen")
struct PhotoCell
{
    float up = 0.0f, down = 0.0f, z = 0.0f;
    void prepare (double sr, double riseMs, double fallMs)
    {
        up   = 1.0f - (float) std::exp (-1.0 / (riseMs * 0.001 * sr));
        down = 1.0f - (float) std::exp (-1.0 / (fallMs * 0.001 * sr));
    }
    inline float process (float x) noexcept { z += (x - z) * (x > z ? up : down); return z; }
    void reset (float v = 0.0f) { z = v; }
};

// Wellenform 0..1; shape 0 = Sinus, 1 = fast Rechteck
inline float lfoShape (double phase, float shape) noexcept
{
    const float s = (float) std::sin (twoPi * phase);
    const float k = 0.6f + shape * 7.0f;
    return 0.5f + 0.5f * std::tanh (k * s) / std::tanh (k);
}

// Allpass 1. Ordnung, Eckfrequenz frei verschiebbar
struct Allpass1
{
    float z = 0.0f;
    inline float process (float x, float a) noexcept
    {
        const float y = a * x + z;
        z = x - a * y;
        return y;
    }
};

inline float allpassCoeff (double fc, double sr) noexcept
{
    const double t = std::tan (3.141592653589793 * std::min (fc, sr * 0.45) / sr);
    return (float) ((t - 1.0) / (t + 1.0));
}

class UniVibe
{
public:
    void prepare (double sr) { fs = sr; for (auto& s : st) s.z = 0.0f; setHp (sr); }
    // light 0..1 (gefiltert), depth 0..1
    inline float process (float x, float light, float depth) noexcept
    {
        // Gestaffelte Kondensatoren -> vier sehr verschiedene Eckfrequenzen
        static constexpr std::array<double, 4> base { 90.0, 320.0, 1100.0, 3300.0 };
        const double sweep = std::pow (2.0, -3.2 * (double) depth * (1.0 - (double) light));   // bis ~3 Oktaven
        float y = x;
        for (size_t i = 0; i < 4; ++i)
            y = st[i].process (y, allpassCoeff (base[i] * sweep, fs));
        // leichte Vorstufen-Sättigung wie im Original
        y = std::tanh (1.4f * y) / 1.4f;
        return hpState.process (hp, y);
    }
    void setHp (double sr) { hp = eq::makeHighPass (sr, 40.0, 0.70710678); hpState.reset(); }

private:
    double fs = 48000.0;
    std::array<Allpass1, 4> st;
    eq::Coeffs hp;
    eq::BiquadState hpState;
};
} // namespace vibe
