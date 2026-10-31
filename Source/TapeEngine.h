#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "TapeData.h"

namespace nova
{

//==============================================================================
/** mulberry32: small, fast, seedable. Used for every random source so RANDOM LOCK is reproducible. */
struct Rng
{
    std::uint32_t state = 1;
    void seed (std::uint32_t s) noexcept { state = s; }
    float next() noexcept
    {
        std::uint32_t z = (state += 0x6D2B79F5u);
        z = (z ^ (z >> 15)) * (z | 1u);
        z ^= z + (z ^ (z >> 7)) * (z | 61u);
        return (float) ((z ^ (z >> 14)) >> 8) * (1.0f / 16777216.0f);
    }
    float bipolar() noexcept { return next() * 2.0f - 1.0f; }
};

/** Cosine-interpolated random walk: a new random point every `step` seconds. */
struct SmoothRand
{
    float a = 0, b = 0, v = 0, ph = 1, inc = 0, mom = .5f;
    void setup (float stepSeconds, double sr, float momentum) { inc = (float) (1.0 / (stepSeconds * sr)); mom = momentum; }
    void reset() { a = b = v = 0; ph = 1; }
    float next (Rng& r) noexcept
    {
        ph += inc;
        if (ph >= 1.0f) { ph -= 1.0f; a = b; v = v * mom + r.bipolar() * .8f; b = v; }
        const float mu = (1.0f - std::cos (ph * juce::MathConstants<float>::pi)) * .5f;
        return a * (1.0f - mu) + b * mu;
    }
};

/** RBJ biquad, transposed direct form II, stereo state. */
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] {}, z2[2] {};

    inline float process (float x, int ch) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }

    void setLowpass   (double sr, double f, double q);
    void setHighpass  (double sr, double f, double q);
    void setBandpass  (double sr, double f, double q);
    void setPeak      (double sr, double f, double q, double gainDb);
    void setLowShelf  (double sr, double f, double gainDb);
    void setHighShelf (double sr, double f, double gainDb);

private:
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2);
};

/** Circular stereo delay line with linear interpolation (same as a Web Audio DelayNode). */
struct DelayLine
{
    std::vector<float> buf[2];
    int size = 1, w = 0;
    void prepare (int samples) { size = juce::jmax (4, samples); for (auto& b : buf) b.assign ((size_t) size, 0.0f); w = 0; }
    void reset() { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); w = 0; }
    inline void push (float l, float r) noexcept { buf[0][(size_t) w] = l; buf[1][(size_t) w] = r; if (++w >= size) w = 0; }
    /** Returns the sample written `delaySamples` samples ago (0 = the sample just pushed). */
    inline float read (int ch, float delaySamples) const noexcept
    {
        delaySamples = juce::jlimit (0.0f, (float) (size - 3), delaySamples);
        float pos = (float) w - 1.0f - delaySamples;
        if (pos < 0) pos += (float) size;
        int i0 = (int) pos; if (i0 >= size) i0 -= size;
        const float f = pos - (float) i0;
        const int i1 = (i0 + 1) % size;
        return buf[ch][(size_t) i0] + f * (buf[ch][(size_t) i1] - buf[ch][(size_t) i0]);
    }
};

//==============================================================================
class TapeEngine
{
public:
    struct Params
    {
        float rec = .62f, bias = .5f, az = .15f, wow = .35f, flut = .3f, age = .3f, hiss = .4f, mix = 1, out = .7f;
        float pre = 0, fold = 0, crush = 0, sq = 0, salat = 0, grit = 0, tilt = .5f, narrow = 0, dmix = 1;
        int tape = 0, speed = 1;
        bool source = false, nr = false, mono = false, hum = false, lock = false, post = false, brake = false;
        float wear = 0;
    };

    struct Transport
    {
        bool hasInfo = false;      // false in the standalone app: treat as always rolling
        bool playing = true;
        juce::int64 timeInSamples = 0;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    int getLatencySamples() const noexcept { return latency; }

    void process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params&, const Transport&);

    void triggerKick() noexcept { kickPending.store (true); }

    // Meters, read by the editor
    std::atomic<float> vuL { 0 }, vuR { 0 }, reelRate { 0 };
    std::atomic<bool> satLed { false };
    bool isRolling() const noexcept { return rolling; }

private:
    struct Smoothed
    {
        float cur = 0, target = 0;
        void set (float t, bool snap) { target = t; if (snap) cur = t; }
        inline void step (float coef) noexcept { cur += (target - cur) * coef; }
    };

    void computeDerived (const Params&);
    void processChunk (juce::AudioBuffer<float>& buffer, int offset, int len, int numInputChannels, const Params&);
    void reseed (std::uint32_t seed);
    void scheduleEvents (bool live);
    void startDropout (float depth, float durSec);
    void startChew (float intensity, float durSec);
    void startDrag (float rate, float durSec);
    void startJumpBack();

    float tapeShape (float x) const noexcept;
    float preampShape (float x) const noexcept;

    double sr = 48000, osr = 96000;
    int maxBlock = 512, latency = 0, osLatency = 0, baseDelaySamples = 0;
    bool firstBlock = true, rolling = false, wasPlaying = false;
    juce::int64 expectedPos = -1;

    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::AudioBuffer<float> work, inCopy;

    // smoothed knob values (0..1) and tape character
    Smoothed sk[18];
    Smoothed tc[18];
    Smoothed spd[7];
    Smoothed wearS;

    // derived per sub-block
    struct D
    {
        float drive = 1, makeup = 1, k = 2.4f, asym = .07f, t0 = 0, slope = 1;
        float lp = 9500, wowDepth = 0, flutDepth = 0, driftDepth = 0, azBase = 0, azDrift = 0;
        float print = 0, hiss = 0, crackle = 0, dropRate = 0, chewRate = 0, age = 0;
        float preG = 1, preRef = 1, preGate = 0, foldK = 1, foldM = 0, crushL = 1, hold = 1, dMake = 1, dWet = 1, dClean = 0;
        float sqThr = 0, sqRatio = 1, sqRel = .2f, sqMakeup = 1, grit = 0, outGain = 1, wet = 1, dry = 0, hum = 0;
        float wf = 1;
        bool preOn = false, foldOn = false, crushOn = false, anyDirt = false, mono = false;
    } d;

    // filters
    Biquad tiltPreL, tiltPreH, tiltPostL, tiltPostH;           // oversampled dirt stage
    Biquad tiltPreLb, tiltPreHb, tiltPostLb, tiltPostHb;       // base-rate copies for POST mode
    Biquad preEmph;                                           // oversampled
    Biquad hp, bump, mid, lp1, lp2, contact;
    Biquad hissHP, hissPk, hissLP, crackHP, gritBP, printLP, humLP, nrShelf, nHP, nLP;

    DelayLine wowLine, azLine, printLine, dryLine;

    // modulation
    Rng rngEvents, rngMod, rngNoise;
    SmoothRand driftA, driftB, flEnv1, flEnv2, flWander, flScrape;
    float wowPh1 = 0, wowPh2 = 0, flPh1 = 0, flPh2 = 0, humPh = 0;

    // events
    int slotLen = 2400, slotCounter = 0;
    float dropGain = 1, dropTarget = 1, dropCoefDown = 0, dropCoefUp = 0; int dropRemain = 0;
    float contactF = 20000, contactTarget = 20000;
    float chewVal = 0, chewTarget = 0; int chewRemain = 0, chewStepLen = 1, chewStepCount = 0; float chewIntensity = 0;
    float crackBurst = 0;
    float rate = 1, rateTarget = 1, rateTau = .03f, dragOffset = 0; int dragRemain = 0;
    bool braking = false;
    float jumpFrom = 0, jumpPos = 1;
    std::atomic<bool> kickPending { false };

    // crush hold
    float holdPh = 0, holdV[2] {};
    // crackle
    float crackAmp[2] {}; int crackLeft[2] {}, crackK[2] {};
    // squash + NR + limiter + meters
    float sqEnv = 0, nrEnv = 0, nrGainDb = 0, limGain = 1, satHold = 0;
    Smoothed noiseGate, srcAmt, monoAmt, nrAmt;
    bool activePost = false; float routeFade = 1;
    float blockSumL = 0, blockSumR = 0; int blockCount = 0;
};

} // namespace nova
