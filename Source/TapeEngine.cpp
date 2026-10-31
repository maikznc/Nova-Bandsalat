#include "TapeEngine.h"

namespace nova
{

static constexpr float kBase = .03f;          // tape path base delay (room for wow and flutter)
static constexpr float kHead = 8.0f;          // saturation curve covers +-8 so hot levels never hard-clip
static constexpr float kPrintDelay = 2.35f;   // print-through: one tape wrap later
static constexpr float kMaxDrag = 1.6f;       // finger brake can hold the tape back this far
static constexpr int   kSub = 32;             // parameters and coefficients update every 32 samples

static inline float db2g (float d) noexcept { return std::pow (10.0f, d / 20.0f); }
static inline float qDb (float qdb) noexcept { return std::pow (10.0f, qdb / 20.0f); }   // Web Audio lowpass/highpass Q is in dB
static inline float clamp1 (float x) noexcept { return x < -1.0f ? -1.0f : (x > 1.0f ? 1.0f : x); }
static inline float onePole (double tauSec, double rate) { return (float) (1.0 - std::exp (-1.0 / (tauSec * rate))); }

//==============================================================================
void Biquad::set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
{
    b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
    a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
}

static inline void prep (double sr, double& f, double& cw, double& sw)
{
    f = juce::jlimit (10.0, sr * 0.49, f);
    const double w0 = juce::MathConstants<double>::twoPi * f / sr;
    cw = std::cos (w0); sw = std::sin (w0);
}

void Biquad::setLowpass (double sr, double f, double q)
{
    double cw, sw; prep (sr, f, cw, sw); const double al = sw / (2.0 * q);
    set ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al);
}
void Biquad::setHighpass (double sr, double f, double q)
{
    double cw, sw; prep (sr, f, cw, sw); const double al = sw / (2.0 * q);
    set ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
}
void Biquad::setBandpass (double sr, double f, double q)
{
    double cw, sw; prep (sr, f, cw, sw); const double al = sw / (2.0 * q);
    set (al, 0, -al, 1 + al, -2 * cw, 1 - al);
}
void Biquad::setPeak (double sr, double f, double q, double g)
{
    double cw, sw; prep (sr, f, cw, sw); const double al = sw / (2.0 * q), A = std::pow (10.0, g / 40.0);
    set (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
}
void Biquad::setLowShelf (double sr, double f, double g)
{
    double cw, sw; prep (sr, f, cw, sw); const double A = std::pow (10.0, g / 40.0), al = sw / 2.0 * std::sqrt (2.0), s = 2 * std::sqrt (A) * al;
    set (A * ((A + 1) - (A - 1) * cw + s), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - s),
         (A + 1) + (A - 1) * cw + s, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - s);
}
void Biquad::setHighShelf (double sr, double f, double g)
{
    double cw, sw; prep (sr, f, cw, sw); const double A = std::pow (10.0, g / 40.0), al = sw / 2.0 * std::sqrt (2.0), s = 2 * std::sqrt (A) * al;
    set (A * ((A + 1) + (A - 1) * cw + s), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - s),
         (A + 1) - (A - 1) * cw + s, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - s);
}

//==============================================================================
void TapeEngine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = juce::jmax (16, maxBlockSize);

    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxBlock);
    osr = sr * 2.0;
    osLatency = (int) std::lround (os->getLatencyInSamples());
    baseDelaySamples = (int) std::lround (kBase * sr);
    latency = baseDelaySamples + osLatency;

    work.setSize (2, maxBlock, false, true, true);
    inCopy.setSize (2, maxBlock, false, true, true);

    wowLine.prepare ((int) std::ceil ((kBase + kMaxDrag + .25) * sr));
    azLine.prepare ((int) std::ceil (.03 * sr));
    printLine.prepare ((int) std::ceil ((kPrintDelay + .1) * sr));
    dryLine.prepare (latency + 16);

    slotLen = juce::jmax (1, (int) std::lround (.05 * sr));
    driftA.setup (.42f, sr, .55f);  driftB.setup (.95f, sr, .55f);
    flEnv1.setup (.7f, sr, .3f);    flEnv2.setup (1.1f, sr, .3f);
    flWander.setup (1.3f, sr, .5f); flScrape.setup (1.0f / 55.0f, sr, 0.0f);

    hissHP.setHighpass (sr, 900, qDb (-3));
    hissPk.setPeak (sr, 6500, .8, 2);
    crackHP.setHighpass (sr, 500, qDb (-3));
    gritBP.setBandpass (sr, 2600, .5);
    printLP.setLowpass (sr, 2200, qDb (-3));
    humLP.setLowpass (sr, 320, qDb (0));

    auto& sys = juce::Random::getSystemRandom();
    rngEvents.seed ((std::uint32_t) sys.nextInt());
    rngMod.seed ((std::uint32_t) sys.nextInt());
    rngNoise.seed ((std::uint32_t) sys.nextInt());

    reset();
}

void TapeEngine::reset()
{
    if (os != nullptr) os->reset();
    for (auto* b : { &tiltPreL, &tiltPreH, &tiltPostL, &tiltPostH, &tiltPreLb, &tiltPreHb, &tiltPostLb, &tiltPostHb, &preEmph,
                     &hp, &bump, &mid, &lp1, &lp2, &contact, &hissHP, &hissPk, &hissLP, &crackHP, &gritBP, &printLP,
                     &humLP, &nrShelf, &nHP, &nLP })
        b->reset();
    wowLine.reset(); azLine.reset(); printLine.reset(); dryLine.reset();
    for (auto* g : { &driftA, &driftB, &flEnv1, &flEnv2, &flWander, &flScrape }) g->reset();
    wowPh1 = wowPh2 = flPh1 = flPh2 = humPh = 0;
    dropGain = dropTarget = 1; dropRemain = 0; contactF = contactTarget = 20000;
    chewVal = chewTarget = 0; chewRemain = 0; crackBurst = 0;
    rate = rateTarget = 1; dragOffset = 0; dragRemain = 0; braking = false; jumpPos = 1; jumpFrom = 0;
    holdPh = 0; holdV[0] = holdV[1] = 0;
    crackLeft[0] = crackLeft[1] = 0;
    sqEnv = nrEnv = nrGainDb = 0; limGain = 1; satHold = 0;
    slotCounter = slotLen;
    firstBlock = true;
}

void TapeEngine::reseed (std::uint32_t seed)
{
    rngEvents.seed (seed * 2654435761u + 17u);
    rngMod.seed (seed * 2246822519u + 29u);
    rngNoise.seed (seed * 3266489917u + 41u);
    for (auto* g : { &driftA, &driftB, &flEnv1, &flEnv2, &flWander, &flScrape }) g->reset();
    wowPh1 = wowPh2 = flPh1 = flPh2 = 0;
    dropTarget = 1; dropRemain = 0; contactTarget = 20000;
    chewTarget = 0; chewRemain = 0;
    if (dragRemain > 0) { dragRemain = 0; startJumpBack(); }
    slotCounter = slotLen;
}

//==============================================================================
float TapeEngine::tapeShape (float x) const noexcept
{
    x = juce::jlimit (-kHead, kHead, x);
    return (std::tanh (d.k * (x + d.asym)) - d.t0) / d.slope;
}

static inline float softClipKnee (float u, float t) noexcept
{
    const float a = std::abs (u);
    if (a < t) return u;
    return std::copysign (t + (1.0f - t) * std::tanh ((a - t) / (1.0f - t)), u);
}

static inline float preampAsym (float u) noexcept
{
    return u >= 0 ? softClipKnee (u, .55f) : -softClipKnee (-u * 1.35f, .45f) / 1.1f;
}

float TapeEngine::preampShape (float x) const noexcept
{
    const float c = clamp1 (x), a = std::abs (c);
    float y = preampAsym (c * d.preG) * (.7f / d.preRef);
    if (d.preGate > 0)
        y *= a < d.preGate ? 0.0f : (a < 2 * d.preGate ? (a - d.preGate) / d.preGate : 1.0f);
    return y;
}

//==============================================================================
void TapeEngine::computeDerived (const Params& p)
{
    enum { REC, BIAS, AZ, WOW, FLUT, AGE, HISS, MIX, OUT, PRE, FOLD, CRUSH, SQ, SALAT, GRIT, TILT, NARROW, DMIX };
    enum { SENS, KK, ASYM, HP, LP, BUMPF, BUMPG, MIDF, MIDG, PRES, THISS, TWOW, TFLUT, DRIFT, DROP, CHEW, TAGE, OG };
    auto K = [this] (int i) { return sk[i].cur; };
    auto T = [this] (int i) { return tc[i].cur; };
    const float sF = spd[0].cur, sLp = spd[1].cur, sWow = spd[2].cur, sFlut = spd[3].cur, sHiss = spd[4].cur, sBump = spd[5].cur;
    juce::ignoreUnused (sF);
    d.wf = spd[6].cur;

    const float bias = K (BIAS) * 2 - 1;
    const float age = juce::jlimit (0.0f, 1.6f, T (TAGE) + K (AGE) * .9f + wearS.cur * .7f), a1 = juce::jmin (age, 1.0f);
    d.age = age;

    d.drive = db2g (-12 + K (REC) * 30) * T (SENS);
    d.makeup = std::pow (d.drive, -.5f);
    d.k = T (KK) * (1 - .35f * bias);
    d.asym = T (ASYM) * .5f * (1 - .4f * bias);
    d.t0 = std::tanh (d.k * d.asym);
    d.slope = d.k * (1 - d.t0 * d.t0);

    preEmph.setHighShelf (osr, 3500, T (PRES) - bias * 5);
    hp.setHighpass (sr, T (HP), qDb (-3));
    bump.setPeak (sr, juce::jmin (T (BUMPF) * sBump, 2000.0f), 1.1, T (BUMPG));
    mid.setPeak (sr, T (MIDF), .8, T (MIDG));
    d.lp = juce::jlimit (900.0f, 20000.0f, T (LP) * sLp * (1 - .55f * a1) * (1 - .22f * bias));
    lp1.setLowpass (sr, d.lp, qDb (-3));
    lp2.setLowpass (sr, juce::jmin (d.lp * 1.15f, 21000.0f), qDb (1.5f));
    contact.setLowpass (sr, contactF, qDb (-3));
    hissLP.setLowpass (sr, juce::jlimit (2500.0f, 20000.0f, d.lp * 1.3f), qDb (-3));

    d.wowDepth = (T (TWOW) * .6f + K (WOW)) * sWow * .0042f;
    d.flutDepth = (T (TFLUT) * .6f + K (FLUT)) * sFlut * .00018f;
    d.driftDepth = (T (DRIFT) * .8f + K (WOW) * .35f + age * .2f) * sWow * .0035f;
    d.azBase = K (AZ) * .0006f;
    d.azDrift = .00004f + age * .00012f;
    d.print = age * .045f;
    d.hiss = std::pow (K (HISS), 1.5f) * T (THISS) * sHiss * .018f * (1.0f - .6f * nrAmt.cur) * (1 + age * .2f);
    d.crackle = juce::jmax (0.0f, age - .45f) * .5f + T (CHEW) * .3f + K (GRIT) * .8f;
    d.dropRate = T (DROP) + juce::jmax (0.0f, age - .15f) * 1.1f + K (SALAT) * 1.1f;
    d.chewRate = T (CHEW) + juce::jmax (0.0f, age - .5f) * .5f + K (SALAT) * 2.6f;

    // dirt section
    const float pre = K (PRE), fold = K (FOLD), crush = K (CRUSH);
    d.preOn = pre > .0005f; d.foldOn = fold > .0005f; d.crushOn = crush > .0005f;
    d.preG = db2g (pre * 42); d.preGate = .04f * pre * pre;
    d.preRef = juce::jmax (1.0e-6f, std::abs (preampAsym (.7f * d.preG)));
    d.foldK = juce::MathConstants<float>::halfPi * (1 + fold * 7); d.foldM = juce::jmin (1.0f, fold * 2.5f);
    d.crushL = std::pow (2.0f, 12 - crush * 10.2f - 1);
    d.hold = d.crushOn ? std::pow (40.0f, std::pow (crush, 1.3f)) : 1.0f;
    d.dMake = db2g (-std::pow (pre, .8f) * 13 - fold * 3 + crush * 7);
    d.anyDirt = d.preOn || d.foldOn || d.crushOn;
    d.dWet = d.anyDirt ? K (DMIX) : 1.0f;
    d.dClean = d.anyDirt ? 1 - K (DMIX) : 0.0f;

    const float tl = (K (TILT) * 2 - 1) * 12;
    tiltPreL.setLowShelf (osr, 300, -tl);  tiltPreH.setHighShelf (osr, 2200, tl);
    tiltPostL.setLowShelf (osr, 300, tl);  tiltPostH.setHighShelf (osr, 2200, -tl);
    tiltPreLb.setLowShelf (sr, 300, -tl);  tiltPreHb.setHighShelf (sr, 2200, tl);
    tiltPostLb.setLowShelf (sr, 300, tl);  tiltPostHb.setHighShelf (sr, 2200, -tl);

    const float sq = K (SQ);
    d.sqThr = -sq * 40; d.sqRatio = 1 + sq * 19; d.sqRel = .18f + sq * .25f;
    d.sqMakeup = db2g (-.6f * d.sqThr * (1 - 1 / d.sqRatio));
    d.grit = K (GRIT) * .75f;

    const float nw = K (NARROW);
    nHP.setHighpass (sr, 12 * std::pow (50.0f, nw), qDb (-3 + nw * 9));
    nLP.setLowpass (sr, juce::jmin (20000 * std::pow (.1f, nw), (float) sr * .45f), qDb (-3 + nw * 10));
    nrShelf.setHighShelf (sr, 3200, nrGainDb);

    d.hum = p.hum ? .03f * (1 + age * .5f) : 0.0f;
    const float mix = K (MIX), src = srcAmt.cur;
    d.wet = (1 - src) * mix;
    d.dry = src + (1 - src) * (1 - mix);
    d.outGain = db2g (-24 + K (OUT) * 30 + (1 - src) * (T (OG) + std::pow (nw, 1.8f) * 12 + sq * sq * 5) * mix);
}

//==============================================================================
void TapeEngine::startDropout (float depth, float durSec)
{
    dropTarget = 1 - depth;
    dropRemain = juce::jmax (1, (int) (durSec * sr));
    contactTarget = 1500 + (1 - depth) * 5000;
}

void TapeEngine::startChew (float intensity, float durSec)
{
    chewIntensity = intensity;
    chewStepCount = 6 + (int) std::lround (intensity * 14);
    chewStepLen = juce::jmax (1, (int) (durSec * sr / chewStepCount));
    chewRemain = chewStepLen;
    chewTarget = rngEvents.bipolar() * .0012f * intensity;
}

void TapeEngine::startDrag (float r, float durSec)
{
    if (braking || jumpPos < 1.0f) return;
    rateTarget = r; rateTau = .025f;
    dragRemain = juce::jmax (1, (int) (durSec * sr));
}

void TapeEngine::startJumpBack()
{
    rate = rateTarget = 1;
    if (dragOffset > .0005f) { jumpFrom = dragOffset; jumpPos = 0; }
    dragOffset = 0;
}

void TapeEngine::scheduleEvents (bool live)
{
    if (! live) return;
    const float dt = .05f, salat = sk[13].cur;
    if (rngEvents.next() < d.dropRate * dt)
    {
        const float depth = .25f + rngEvents.next() * .65f * juce::jmin (1.0f, .4f + d.age);
        startDropout (depth, .03f + rngEvents.next() * .22f * (.5f + d.age));
    }
    if (rngEvents.next() < d.chewRate * dt)
    {
        const float in = (.35f + rngEvents.next() * .65f) * (1 + salat * .8f);
        startChew (in, .12f + rngEvents.next() * .3f * (1 + salat));
    }
    if (salat > 0 && rngEvents.next() < salat * .7f * dt)
    {
        const float r = .45f + rngEvents.next() * .4f;
        startDrag (r, .08f + rngEvents.next() * .35f * salat);
    }
}

//==============================================================================
void TapeEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels, const Params& p, const Transport& tr)
{
    const int n = buffer.getNumSamples();
    if (n <= 0 || os == nullptr) return;

    // ---- targets
    const auto& tp = tapes()[(size_t) juce::jlimit (0, 7, p.tape)].p;
    const float knobs[18] = { p.rec, p.bias, p.az, p.wow, p.flut, p.age, p.hiss, p.mix, p.out,
                              p.pre, p.fold, p.crush, p.sq, p.salat, p.grit, p.tilt, p.narrow, p.dmix };
    const float chars[18] = { tp.sens, tp.k, tp.asym, tp.hp, tp.lp, tp.bumpF, tp.bumpG, tp.midF, tp.midG, tp.pres,
                              tp.hiss, tp.wow, tp.flut, tp.drift, tp.drop, tp.chew, tp.age, tp.og };
    const auto& sc = speedChar (p.speed);
    const float spds[7] = { sc.f, sc.lp, sc.wow, sc.flut, sc.hiss, sc.bump, sc.wf };
    for (int i = 0; i < 18; ++i) { sk[i].set (knobs[i], firstBlock); tc[i].set (chars[i], firstBlock); }
    for (int i = 0; i < 7; ++i) spd[i].set (spds[i], firstBlock);
    wearS.set (p.wear, firstBlock);
    srcAmt.set (p.source ? 1.0f : 0.0f, firstBlock);
    monoAmt.set ((p.mono || tp.mono) ? 1.0f : 0.0f, firstBlock);
    nrAmt.set (p.nr ? 1.0f : 0.0f, firstBlock);
    if (firstBlock) activePost = p.post;

    // ---- transport: rolling state and RANDOM LOCK reseeding
    const bool playing = ! tr.hasInfo || tr.playing;
    if (p.lock && tr.hasInfo && tr.playing)
    {
        const bool started = ! wasPlaying || firstBlock;
        const bool jumped = expectedPos >= 0 && std::llabs (tr.timeInSamples - expectedPos) > 64;
        if (started || jumped)
            reseed ((std::uint32_t) ((std::uint64_t) juce::jmax ((juce::int64) 0, tr.timeInSamples) / 16u));
    }
    expectedPos = tr.hasInfo ? tr.timeInSamples + n : -1;
    wasPlaying = playing;
    rolling = playing;
    noiseGate.set (playing ? 1.0f : 0.0f, firstBlock);

    // ---- brake (automatable parameter) and kick (button)
    if (p.brake && ! braking && jumpPos >= 1.0f)
    {
        braking = true; dragRemain = 0; rateTarget = .38f; rateTau = .55f;
    }
    else if (! p.brake && braking)
    {
        braking = false; startJumpBack();
    }
    if (kickPending.exchange (false) && playing && ! p.source)
    {
        startDropout (.88f, .16f + rngEvents.next() * .1f);
        startChew (1.0f, .38f);
        startDrag (.82f, .12f);
    }

    firstBlock = false;
    blockSumL = blockSumR = 0; blockCount = 0;

    for (int off = 0; off < n; off += maxBlock)
        processChunk (buffer, off, juce::jmin (maxBlock, n - off), numInputChannels, p);

    // ---- meters
    if (blockCount > 0)
    {
        vuL.store (std::sqrt (blockSumL / (float) blockCount));
        vuR.store (std::sqrt (blockSumR / (float) blockCount));
    }
    satHold -= (float) n / (float) sr;
    satLed.store (satHold > 0);
    reelRate.store (rolling ? rate : 0.0f);
}

void TapeEngine::processChunk (juce::AudioBuffer<float>& buffer, int offset, int len, int numIn, const Params& p)
{
    const int numOut = buffer.getNumChannels();

    // copy input (mono input feeds both sides)
    for (int ch = 0; ch < 2; ++ch)
    {
        if (numIn <= 0) { inCopy.clear (ch, 0, len); continue; }
        const int src = juce::jmin (ch, numIn - 1);
        inCopy.copyFrom (ch, 0, buffer, src, offset, len);
        work.copyFrom (ch, 0, buffer, src, offset, len);
    }
    if (numIn <= 0) { work.clear(); }

    const float ksm = onePole (.03, sr / kSub);       // knob smoothing per sub-block
    const float kNoise = onePole (.05, sr / kSub);
    const float cDropDown = onePole (.008, sr), cDropUp = onePole (.035, sr);
    const float cContDown = onePole (.01, sr), cContUp = onePole (.06, sr);
    const float cChew = onePole (.004, sr), cChewEnd = onePole (.03, sr);
    const float cBurstUp = onePole (.005, sr), cBurstDown = onePole (.05, sr);
    const float cNr = onePole (.05, sr), cJump = (float) (1.0 / (.04 * sr));
    const float cRouteFade = (float) (1.0 / (.006 * sr));
    const float cLimRel = onePole (.12, sr), cSqAtt = (float) std::exp (-1.0 / (.003 * sr));
    const float crackProb = .0007f * (float) (48000.0 / sr);
    const float limThr = db2g (-2);
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float invSr = (float) (1.0 / sr);

    for (int start = 0; start < len; start += kSub)
    {
        const int sub = juce::jmin (kSub, len - start);

        // ---- smoothing and derived values
        for (auto& s : sk) s.step (ksm);
        for (auto& s : tc) s.step (ksm);
        for (auto& s : spd) s.step (ksm);
        wearS.step (ksm); srcAmt.step (ksm); monoAmt.step (ksm); nrAmt.step (ksm); noiseGate.step (kNoise);
        {
            const float lv = std::sqrt (nrEnv) * .7f;
            const float target = nrAmt.cur * -16.0f * (1.0f - juce::jlimit (0.0f, 1.0f, lv * 7.0f));
            nrGainDb += (target - nrGainDb) * onePole (.07, sr / kSub);
        }
        computeDerived (p);

        // ---- oversampled stage: dirt (PRE), record level, pre-emphasis, tape saturation
        juce::dsp::AudioBlock<float> wb (work);
        auto subBlock = wb.getSubBlock ((size_t) start, (size_t) sub);
        auto up = os->processSamplesUp (subBlock);
        float* u0 = up.getChannelPointer (0);
        float* u1 = up.getChannelPointer (1);
        const int un = (int) up.getNumSamples();
        const bool preDirt = ! activePost;
        const float holdStepOs = 1.0f / (d.hold * 2.0f);
        float satPeak = 0;

        for (int i = 0; i < un; ++i)
        {
            float x[2] = { u0[i], u1[i] };
            bool takeHold = true;
            if (preDirt && d.crushOn)
            {
                holdPh += holdStepOs;
                takeHold = d.hold <= 1.001f || holdPh >= 1.0f;
                if (holdPh >= 1.0f) holdPh -= 1.0f;
            }
            for (int ch = 0; ch < 2; ++ch)
            {
                float y = x[ch];
                if (preDirt)
                {
                    const float clean = y;
                    float t = tiltPreH.process (tiltPreL.process (y, ch), ch);
                    if (d.preOn) t = preampShape (t);
                    if (d.foldOn) { const float c = clamp1 (t); t = (1 - d.foldM) * c + d.foldM * std::sin (d.foldK * c) * .75f; }
                    if (d.crushOn)
                    {
                        const float c = clamp1 (t);
                        const float q = std::round (c * d.crushL) / d.crushL;
                        if (takeHold) holdV[ch] = q;
                        t = holdV[ch];
                    }
                    t = tiltPostH.process (tiltPostL.process (t, ch), ch) * d.dMake;
                    y = t * d.dWet + clean * d.dClean;
                }
                y *= d.drive;
                satPeak = juce::jmax (satPeak, std::abs (y));
                y = preEmph.process (y, ch);
                x[ch] = tapeShape (y) * d.makeup;
            }
            u0[i] = x[0]; u1[i] = x[1];
        }
        os->processSamplesDown (subBlock);
        if (satPeak > .92f && srcAmt.cur < .5f) satHold = .14f;

        // ---- base-rate stage
        float* w0 = work.getWritePointer (0, start);
        float* w1 = work.getWritePointer (1, start);
        const float* in0 = inCopy.getReadPointer (0, start);
        const float* in1 = inCopy.getReadPointer (1, start);
        const bool live = rolling && srcAmt.target < .5f;
        const float holdStep = 1.0f / d.hold;

        for (int i = 0; i < sub; ++i)
        {
            // events on a fixed 50 ms grid
            if (--slotCounter <= 0) { slotCounter = slotLen; scheduleEvents (live); }

            // dropout envelope
            if (dropRemain > 0 && --dropRemain == 0) { dropTarget = 1; contactTarget = 20000; }
            dropGain += (dropTarget - dropGain) * (dropTarget < dropGain ? cDropDown : cDropUp);
            contactF += (contactTarget - contactF) * (contactTarget < contactF ? cContDown : cContUp);

            // chew (tape eating)
            if (chewStepCount > 0)
            {
                if (--chewRemain <= 0)
                {
                    if (--chewStepCount > 0) { chewRemain = chewStepLen; chewTarget = rngEvents.bipolar() * .0012f * chewIntensity; }
                    else chewTarget = 0;
                }
                chewVal += (chewTarget - chewVal) * cChew;
                crackBurst += (.25f * chewIntensity - crackBurst) * cBurstUp;
            }
            else
            {
                chewVal += (0 - chewVal) * cChewEnd;
                crackBurst += (0 - crackBurst) * cBurstDown;
            }

            // drag and finger brake: the tape is held back, the delay grows
            rate += (rateTarget - rate) * onePole (rateTau, sr);
            if (braking || dragRemain > 0)
            {
                dragOffset += (1.0f - rate) * invSr;
                if (dragOffset >= kMaxDrag) { dragOffset = kMaxDrag; rateTarget = 1; }
                if (dragRemain > 0 && --dragRemain == 0) startJumpBack();
            }

            // tape transport filters
            float v[2] = { w0[i], w1[i] };
            for (int ch = 0; ch < 2; ++ch)
                v[ch] = contact.process (lp2.process (lp1.process (mid.process (bump.process (hp.process (v[ch], ch), ch), ch), ch), ch), ch);

            // grit: modulation noise riding on the signal
            if (d.grit > 0)
            {
                const float m = 1.0f + gritBP.process (rngNoise.bipolar(), 0) * d.grit;
                v[0] *= m; v[1] *= m;
            }
            wowLine.push (v[0], v[1]);

            // wow, flutter, drift
            const float dA = driftA.next (rngMod) * .72f, dB = driftB.next (rngMod) * .72f;
            wowPh1 += (.55f * d.wf + .05f * dB) * invSr; if (wowPh1 >= 1) wowPh1 -= 1;
            wowPh2 += (.23f * d.wf + .03f * dA) * invSr; if (wowPh2 >= 1) wowPh2 -= 1;
            const float fw = flWander.next (rngMod);
            flPh1 += 8.6f * (1 + .07f * fw) * d.wf * invSr; if (flPh1 >= 1) flPh1 -= 1;
            flPh2 += 13.3f * (1 - .05f * fw) * d.wf * invSr; if (flPh2 >= 1) flPh2 -= 1;
            const float fl = ((.55f + .45f * flEnv1.next (rngMod)) * std::sin (twoPi * flPh1) * .7f
                              + (.4f + .6f * flEnv2.next (rngMod)) * std::sin (twoPi * flPh2) * .3f
                              + flScrape.next (rngMod) * .3f) * .85f;
            const float mod = d.wowDepth * (std::sin (twoPi * wowPh1) + .6f * std::sin (twoPi * wowPh2))
                            + d.flutDepth * 1.25f * fl
                            + d.driftDepth * (.75f * dA + .55f * dB)
                            + chewVal + dragOffset;
            const float delay = (float) baseDelaySamples + mod * (float) sr;

            float l = wowLine.read (0, delay), r = wowLine.read (1, delay);
            if (jumpPos < 1.0f)
            {
                const float dj = delay + jumpFrom * (float) sr;
                l = wowLine.read (0, dj) * (1 - jumpPos) + l * jumpPos;
                r = wowLine.read (1, dj) * (1 - jumpPos) + r * jumpPos;
                jumpPos += cJump;
            }

            // azimuth: right channel slightly late
            azLine.push (l, r);
            r = azLine.read (1, juce::jmax (0.0f, d.azBase + dA * d.azDrift) * (float) sr);

            // mono
            if (monoAmt.cur > .001f) { const float m = (l + r) * .5f; l += (m - l) * monoAmt.cur; r += (m - r) * monoAmt.cur; }

            // dropouts
            l *= dropGain; r *= dropGain;

            // print-through
            printLine.push (l, r);
            const float pl = printLP.process (printLine.read (0, kPrintDelay * (float) sr), 0) * d.print;
            const float pr = printLP.process (printLine.read (1, kPrintDelay * (float) sr), 1) * d.print;

            // noise floor (only while the tape is rolling)
            const float ng = noiseGate.cur;
            float hissL = 0, hissR = 0, crk[2] = { 0, 0 };
            if (d.hiss > 0 && ng > 0)
            {
                const float hg = d.hiss * ng * juce::jmin (1.0f, rate);
                hissL = hissLP.process (hissPk.process (hissHP.process (rngNoise.bipolar(), 0), 0), 0) * hg;
                hissR = hissLP.process (hissPk.process (hissHP.process (rngNoise.bipolar(), 1), 1), 1) * hg;
            }
            const float crG = (d.crackle * .15f + crackBurst) * ng;
            if (crG > 0)
            {
                for (int ch = 0; ch < 2; ++ch)
                {
                    if (crackLeft[ch] <= 0 && rngNoise.next() < crackProb)
                    {
                        crackAmp[ch] = rngNoise.bipolar() * (rngNoise.next() < .12f ? 1.0f : .35f);
                        crackLeft[ch] = 2 + (int) (rngNoise.next() * 22); crackK[ch] = 0;
                    }
                    float c = 0;
                    if (crackLeft[ch] > 0)
                    {
                        c = crackAmp[ch] * std::exp (-(float) crackK[ch] / 4.0f) * (.4f + .6f * rngNoise.next());
                        ++crackK[ch]; --crackLeft[ch];
                    }
                    crk[ch] = crackHP.process (c, ch) * crG;
                }
            }

            float t[2] = { l + pl + hissL + crk[0], r + pr + hissR + crk[1] };

            // dirt section after the tape
            if (activePost)
            {
                bool takeHold = true;
                if (d.crushOn)
                {
                    holdPh += holdStep;
                    takeHold = d.hold <= 1.001f || holdPh >= 1.0f;
                    if (holdPh >= 1.0f) holdPh -= 1.0f;
                }
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float clean = t[ch];
                    float y = tiltPreHb.process (tiltPreLb.process (t[ch], ch), ch);
                    if (d.preOn) y = preampShape (y);
                    if (d.foldOn) { const float c = clamp1 (y); y = (1 - d.foldM) * c + d.foldM * std::sin (d.foldK * c) * .75f; }
                    if (d.crushOn)
                    {
                        const float q = std::round (clamp1 (y) * d.crushL) / d.crushL;
                        if (takeHold) holdV[ch] = q;
                        y = holdV[ch];
                    }
                    y = tiltPostHb.process (tiltPostLb.process (y, ch), ch) * d.dMake;
                    t[ch] = y * d.dWet + clean * d.dClean;
                }
            }

            // routing switch PRE/POST with a short fade
            if (p.post != activePost)
            {
                routeFade -= cRouteFade;
                if (routeFade <= 0) { routeFade = 0; activePost = p.post; }
            }
            else if (routeFade < 1) routeFade = juce::jmin (1.0f, routeFade + cRouteFade);

            // squash
            if (d.sqRatio > 1.001f)
            {
                const float lvl = juce::jmax (std::abs (t[0]), std::abs (t[1]));
                const float cRel = (float) std::exp (-1.0 / (d.sqRel * sr));
                sqEnv = lvl > sqEnv ? cSqAtt * sqEnv + (1 - cSqAtt) * lvl : cRel * sqEnv + (1 - cRel) * lvl;
                const float over = 20.0f * std::log10 (sqEnv + 1.0e-9f) - d.sqThr;
                const float slope = 1.0f / d.sqRatio - 1.0f;
                float gr = 0;
                if (over >= 3) gr = over * slope;
                else if (over > -3) { const float xk = over + 3; gr = slope * xk * xk / 12.0f; }
                const float g = db2g (gr) * d.sqMakeup;
                t[0] *= g; t[1] *= g;
            }

            // NR (level follower drives the shelf, updated per sub-block)
            nrEnv += ((t[0] * t[0] + t[1] * t[1]) * .5f - nrEnv) * cNr;

            float out[2];
            dryLine.push (in0[i], in1[i]);
            const float dryL = dryLine.read (0, (float) latency), dryR = dryLine.read (1, (float) latency);
            const float hum = d.hum > 0 ? humLP.process (2.0f * humPh - 1.0f, 0) * d.hum * noiseGate.cur : 0.0f;
            humPh += 50.0f * invSr; if (humPh >= 1) humPh -= 1;

            for (int ch = 0; ch < 2; ++ch)
            {
                float y = nrShelf.process (t[ch], ch);
                y = nLP.process (nHP.process (y, ch), ch);
                out[ch] = (y * d.wet * routeFade + (ch == 0 ? dryL : dryR) * d.dry + hum) * d.outGain;
            }

            // limiter
            const float pk = juce::jmax (std::abs (out[0]), std::abs (out[1]));
            const float lt = pk > limThr ? limThr / pk : 1.0f;
            limGain = lt < limGain ? lt : limGain + (1.0f - limGain) * cLimRel;
            out[0] = juce::jlimit (-1.0f, 1.0f, out[0] * limGain);
            out[1] = juce::jlimit (-1.0f, 1.0f, out[1] * limGain);

            blockSumL += out[0] * out[0]; blockSumR += out[1] * out[1]; ++blockCount;
            w0[i] = out[0]; w1[i] = out[1];
        }
    }

    // write back
    if (numOut == 1)
    {
        auto* o = buffer.getWritePointer (0, offset);
        const float* a = work.getReadPointer (0);
        const float* b = work.getReadPointer (1);
        for (int i = 0; i < len; ++i) o[i] = .5f * (a[i] + b[i]);
    }
    else
    {
        for (int ch = 0; ch < numOut; ++ch)
            buffer.copyFrom (ch, offset, work, juce::jmin (ch, 1), 0, len);
    }
}

} // namespace nova
