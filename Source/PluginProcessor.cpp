#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
    const char* kIds[] = { "rec", "bias", "az", "wow", "flut", "age", "hiss", "mix", "out",
                           "pre", "fold", "crush", "sq", "salat", "grit", "tilt", "narrow", "dmix",
                           "tape", "monitor", "nr", "speed", "mono", "hum", "wear", "random", "dpos", "brake" };
    constexpr int kNumIds = (int) (sizeof (kIds) / sizeof (kIds[0]));

    String pct (float v)  { return String (roundToInt (v * 100.0f)) + "%"; }
    String offOrPct (float v) { return v < .005f ? String ("OFF") : pct (v); }
    String signedDb (float d) { return (d >= 0 ? "+" : "") + String (d, 1) + " dB"; }

    std::unique_ptr<AudioParameterFloat> knob (const char* id, const char* name, float def, std::function<String (float)> fmt)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction ([fmt] (float v, int) { return fmt (v); }));
    }
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout NovaAudioProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (knob ("rec", "Rec Level", .62f, [] (float v) { return signedDb (-12 + v * 30); }));
    l.add (knob ("bias", "Bias", .5f, [] (float v) {
        const float b = v * 2 - 1;
        if (std::abs (b) < .03f) return String ("CAL");
        return String (b < 0 ? "UNDER " : "OVER ") + String (roundToInt (std::abs (b) * 100)) + "%"; }));
    l.add (knob ("az", "Azimuth", .15f, [] (float v) { return String (v * .6f, 2) + " ms"; }));
    l.add (knob ("wow", "Wow", .35f, pct));
    l.add (knob ("flut", "Flutter", .3f, pct));
    l.add (knob ("age", "Age", .3f, pct));
    l.add (knob ("hiss", "Hiss", .4f, pct));
    l.add (knob ("mix", "Mix", 1.0f, pct));
    l.add (knob ("out", "Output", .7f, [] (float v) { return signedDb (-24 + v * 30); }));

    l.add (knob ("pre", "Preamp", 0, [] (float v) { return v < .005f ? String ("OFF") : "+" + String (roundToInt (v * 42)) + " dB"; }));
    l.add (knob ("fold", "Fold", 0, offOrPct));
    l.add (knob ("crush", "Crush", 0, [] (float v) {
        if (v < .005f) return String ("OFF");
        const float hold = std::pow (40.0f, std::pow (v, 1.3f));
        return String (12 - v * 10.2f, 1) + "b " + String (48.0f / hold, 1) + "k"; }));
    l.add (knob ("sq", "Squash", 0, [] (float v) { return v < .005f ? String ("OFF") : String (1 + v * 19, 1) + " : 1"; }));
    l.add (knob ("salat", "Salat", 0, offOrPct));
    l.add (knob ("grit", "Grit", 0, offOrPct));
    l.add (knob ("tilt", "Tilt", .5f, [] (float v) {
        const float t = v * 2 - 1;
        if (std::abs (t) < .03f) return String ("FLAT");
        return String (t < 0 ? "FAT " : "FIZZ ") + String (roundToInt (std::abs (t) * 100)) + "%"; }));
    l.add (knob ("narrow", "Narrow", 0, [] (float v) {
        if (v < .005f) return String ("OFF");
        const float h = 12 * std::pow (50.0f, v), lo = 20000 * std::pow (.1f, v);
        return String (roundToInt (h)) + "-" + String (lo / 1000.0f, 1) + "k"; }));
    l.add (knob ("dmix", "Dirt Mix", 1.0f, pct));

    StringArray tapeNames;
    for (int i = 0; i < 8; ++i)
        tapeNames.add (String (nova::tapes()[(size_t) i].name) + " " + String::fromUTF8 (nova::tapeTitle (i)));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "tape", 1 }, "Cassette", tapeNames, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "monitor", 1 }, "Monitor Source", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "nr", 1 }, "Noise Reduction", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "speed", 1 }, "Speed", StringArray { "9.5 cm/s", "4.8 cm/s", "2.4 cm/s" }, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mono", 1 }, "Mono", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "hum", 1 }, "Hum 50 Hz", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "wear", 1 }, "Wear", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "random", 1 }, "Random", StringArray { "Live", "Lock" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dpos", 1 }, "Dirt Position", StringArray { "Pre Tape", "Post Tape" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "brake", 1 }, "Finger Brake", false));
    return l;
}

NovaAudioProcessor::NovaAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < kNumIds; ++i)
        p[i] = apvts.getRawParameterValue (kIds[i]);
    for (auto& w : wear) w.store (0.0f);
    currentPresetName = String::fromUTF8 (nova::factoryPresets()[0].name);
    lastLoadedValues = captureValues();
}

bool NovaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo()) return false;
    if (in != AudioChannelSet::mono() && in != AudioChannelSet::stereo()) return false;
    return ! (in == AudioChannelSet::stereo() && out == AudioChannelSet::mono());
}

void NovaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
}

void NovaAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels();

    nova::TapeEngine::Params pr;
    pr.rec = p[0]->load(); pr.bias = p[1]->load(); pr.az = p[2]->load(); pr.wow = p[3]->load(); pr.flut = p[4]->load();
    pr.age = p[5]->load(); pr.hiss = p[6]->load(); pr.mix = p[7]->load(); pr.out = p[8]->load();
    pr.pre = p[9]->load(); pr.fold = p[10]->load(); pr.crush = p[11]->load(); pr.sq = p[12]->load(); pr.salat = p[13]->load();
    pr.grit = p[14]->load(); pr.tilt = p[15]->load(); pr.narrow = p[16]->load(); pr.dmix = p[17]->load();
    pr.tape = jlimit (0, 7, (int) std::lround (p[18]->load()));
    pr.source = p[19]->load() > .5f;
    pr.nr = p[20]->load() > .5f;
    pr.speed = jlimit (0, 2, (int) std::lround (p[21]->load()));
    pr.mono = p[22]->load() > .5f;
    pr.hum = p[23]->load() > .5f;
    const bool wearOn = p[24]->load() > .5f;
    pr.lock = p[25]->load() > .5f;
    pr.post = p[26]->load() > .5f;
    pr.brake = p[27]->load() > .5f;
    pr.wear = wear[(size_t) pr.tape].load();

    nova::TapeEngine::Transport tr;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            tr.hasInfo = true;
            tr.playing = pos->getIsPlaying();
            if (auto t = pos->getTimeInSamples()) tr.timeInSamples = *t;
        }

    engine.process (buffer, numIn, pr, tr);

    // cassettes wear out while playing; frozen in LOCK and while rendering offline
    if (wearOn && ! pr.lock && ! pr.source && engine.isRolling() && ! isNonRealtime() && getSampleRate() > 0)
    {
        auto& w = wear[(size_t) pr.tape];
        const float add = (float) (buffer.getNumSamples() / getSampleRate() / 360.0) * engine.reelRate.load();
        w.store (jmin (1.0f, w.load() + add));
    }
}

//==============================================================================
const StringArray& NovaAudioProcessor::presetParameterIds()
{
    static const StringArray ids { "rec", "bias", "az", "wow", "flut", "age", "hiss", "mix", "out",
                                   "pre", "fold", "crush", "sq", "salat", "grit", "tilt", "narrow", "dmix",
                                   "tape", "nr", "speed", "mono", "hum", "wear", "dpos" };
    return ids;
}

NovaAudioProcessor::ValueMap NovaAudioProcessor::captureValues() const
{
    ValueMap m;
    for (auto& id : presetParameterIds())
        if (auto* v = apvts.getRawParameterValue (id))
            m[id] = v->load();
    return m;
}

void NovaAudioProcessor::applyValues (const ValueMap& values)
{
    for (auto& [id, value] : values)
        if (auto* prm = apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
            prm->endChangeGesture();
        }
}

NovaAudioProcessor::ValueMap NovaAudioProcessor::factoryValues (int index) const
{
    ValueMap m;
    for (auto& id : presetParameterIds())
        if (auto* prm = apvts.getParameter (id))
            m[id] = prm->convertFrom0to1 (prm->getDefaultValue());

    const auto& list = nova::factoryPresets();
    if (isPositiveAndBelow (index, (int) list.size()))
    {
        const auto& fp = list[(size_t) index];
        m["tape"] = (float) fp.tape;
        for (auto& [id, v] : fp.values) m[String (id)] = v;
    }
    return m;
}

int NovaAudioProcessor::getNumPrograms() { return (int) nova::factoryPresets().size(); }

void NovaAudioProcessor::setCurrentProgram (int index)
{
    if (! isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;
    auto values = factoryValues (index);
    applyValues (values);
    currentPresetName = String::fromUTF8 (nova::factoryPresets()[(size_t) index].name);
    lastLoadedValues = values;
}

const String NovaAudioProcessor::getProgramName (int index)
{
    if (! isPositiveAndBelow (index, getNumPrograms())) return {};
    return String::fromUTF8 (nova::factoryPresets()[(size_t) index].name);
}

//==============================================================================
File NovaAudioProcessor::getUserPresetFolder() const
{
    return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("NOVA").getChildFile ("Bandsalat").getChildFile ("Presets");
}

Array<File> NovaAudioProcessor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (File::findFiles, false, "*.xml");
    files.sort();
    return files;
}

bool NovaAudioProcessor::saveUserPreset (const String& name)
{
    const auto clean = File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto folder = getUserPresetFolder();
    if (! folder.createDirectory()) return false;

    XmlElement xml ("NovaPreset");
    xml.setAttribute ("name", name.trim());
    for (auto& [id, v] : captureValues())
    {
        auto* e = xml.createNewChildElement ("Param");
        e->setAttribute ("id", id);
        e->setAttribute ("value", (double) v);
    }
    if (! xml.writeTo (folder.getChildFile (clean + ".xml"))) return false;
    currentPresetName = name.trim();
    lastLoadedValues = captureValues();
    return true;
}

bool NovaAudioProcessor::loadUserPreset (const File& file)
{
    auto xml = XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("NovaPreset")) return false;
    ValueMap m;
    for (auto* e : xml->getChildWithTagNameIterator ("Param"))
        m[e->getStringAttribute ("id")] = (float) e->getDoubleAttribute ("value");
    applyValues (m);
    currentPresetName = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    lastLoadedValues = captureValues();
    return true;
}

//==============================================================================
void NovaAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto old = state.getChildWithName ("NOVA"); old.isValid()) state.removeChild (old, nullptr);
    ValueTree extra ("NOVA");
    for (int i = 0; i < 8; ++i) extra.setProperty ("wear" + String (i), wear[(size_t) i].load(), nullptr);
    extra.setProperty ("program", currentProgram, nullptr);
    extra.setProperty ("presetName", currentPresetName, nullptr);
    state.appendChild (extra, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void NovaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;
    auto state = ValueTree::fromXml (*xml);
    if (! state.hasType (apvts.state.getType())) return;
    if (auto extra = state.getChildWithName ("NOVA"); extra.isValid())
    {
        for (int i = 0; i < 8; ++i) wear[(size_t) i].store ((float) extra.getProperty ("wear" + String (i), 0.0f));
        currentProgram = (int) extra.getProperty ("program", 0);
        currentPresetName = extra.getProperty ("presetName", currentPresetName).toString();
        state.removeChild (extra, nullptr);
    }
    apvts.replaceState (state);
    lastLoadedValues = captureValues();
}

//==============================================================================
AudioProcessorEditor* NovaAudioProcessor::createEditor() { return new NovaAudioProcessorEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NovaAudioProcessor(); }
