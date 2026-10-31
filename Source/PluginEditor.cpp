#include "PluginEditor.h"

namespace nova::ui
{

//==============================================================================
// Small helpers
static RangedAudioParameter* param (NovaAudioProcessor& p, const char* id) { return p.apvts.getParameter (id); }
static float value (NovaAudioProcessor& p, const char* id) { return p.apvts.getRawParameterValue (id)->load(); }
static void setParam (NovaAudioProcessor& p, const char* id, float v)
{
    if (auto* prm = param (p, id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
        prm->endChangeGesture();
    }
}
static int currentTape (NovaAudioProcessor& p) { return jlimit (0, 7, (int) std::lround (value (p, "tape"))); }
static float sideMinutes (int tape) { static const float m[] = { 30, 45, 23, 30, 45, 30, 30, 15 }; return m[jlimit (0, 7, tape)]; }

/** Shared animation state of the deck: reels, counter, needles. */
struct DeckState
{
    float angL = 0, angR = 180, count = 0;
    float packPos[8] { .12f, .49f, .36f, .23f, .6f, .47f, .34f, .21f };
    float needleL = -.08f, needleR = -.08f, kick = 0;
    bool sat = false;
    float dirtGlow = 0;
};

//==============================================================================
/** Small chrome button used in the preset bar. */
class ChromeButton : public Button
{
public:
    ChromeButton (const String& name, int iconType = 0) : Button (name), icon (iconType) {}
    void paintButton (Graphics& g, bool over, bool down) override
    {
        const bool pressed = down || getToggleState();
        auto r = getLocalBounds().toFloat().reduced (1, 0).withTrimmedBottom (3);
        if (pressed) r = r.translated (0, 2);
        g.setColour (Colour (0xff55585c)); g.fillRoundedRectangle (r.translated (0, pressed ? 1.0f : 3.0f), 3);
        ColourGradient cg (pressed ? Colour (0xffe3e4e6) : Colour (0xfffdfdfd), 0, r.getY(), pressed ? Colour (0xffb8bbbf) : Colour (0xffa5a8ac), 0, r.getBottom(), false);
        if (! pressed) cg.addColour (.45, Colour (0xffd3d5d8));
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 3);
        if (over && ! pressed) { g.setColour (Colours::white.withAlpha (.15f)); g.fillRoundedRectangle (r, 3); }
        const Colour ink = getToggleState() ? Colour (0xffb25d00) : Colour (0xff2a2b2e);
        g.setColour (ink);
        if (icon != 0)
        {
            Path p; const auto c = r.getCentre();
            if (icon < 0) p.addTriangle (c.x + 3.5f, c.y - 4.5f, c.x + 3.5f, c.y + 4.5f, c.x - 3.5f, c.y);
            else          p.addTriangle (c.x - 3.5f, c.y - 4.5f, c.x - 3.5f, c.y + 4.5f, c.x + 3.5f, c.y);
            g.fillPath (p);
        }
        else
        {
            g.setFont (fonts->silkF (8.5f, .12f));
            g.drawText (getButtonText(), r, Justification::centred, false);
        }
    }
    SharedResourcePointer<Fonts> fonts;
    int icon;
};

//==============================================================================
/** Cassette shelf with green LED glow. Click a cassette to load it. */
class Rack : public Component
{
public:
    Rack (NovaAudioProcessor& p, DeckState& s) : proc (p), st (s) {}

    Rectangle<float> slotArea (int i) const
    {
        const float gap = 12, w = ((float) getWidth() - 7 * gap) / 8;
        return { (float) i * (w + gap), 30, w, 93 };
    }

    void paint (Graphics& g) override
    {
        g.setColour (col::text);
        g.setFont (fonts->silkF (10.0f, .34f));
        g.drawText ("KASSETTENREGAL", Rectangle<float> (0, 6, 400, 14), Justification::centredLeft, false);
        g.setColour (col::muted);
        g.setFont (fonts->uiF (13.0f));
        g.drawText (CharPointer_UTF8 ("Kassette anklicken, um sie einzulegen"), Rectangle<float> ((float) getWidth() - 400, 4, 400, 18), Justification::centredRight, false);

        drawGlassShelf (g, Rectangle<float> (-6, 123, (float) getWidth() + 12, 8));

        const int cur = currentTape (proc);
        for (int i = 0; i < 8; ++i)
        {
            auto r = slotArea (i);
            if (i == hover && i != cur) r = r.translated (0, -3);
            // case
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (.13f), r.getX(), r.getY(), Colours::white.withAlpha (.04f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 3);
            g.setColour (Colours::white.withAlpha (i == cur ? .22f : .16f)); g.drawRoundedRectangle (r.reduced (.5f), 3, 1);
            if (i == cur)
            {
                g.setColour (col::amber);
                g.setFont (fonts->silkF (8.5f, .3f));
                g.drawText ("IM DECK", r, Justification::centred, false);
            }
            else
            {
                auto inner = r.reduced (5);
                if (tapes()[(size_t) i].small) inner = inner.withSizeKeepingCentre (inner.getWidth() * .8f, inner.getWidth() * .8f * .64f).withY (inner.getY() + 6);
                else inner = inner.withHeight (inner.getWidth() * .64f);
                drawCassette (g, inner, i, st.packPos[i], 0, 0, {}, *fonts);
            }
            // tag
            const auto tag = Rectangle<float> (slotArea (i).getX(), 136, slotArea (i).getWidth(), 40);
            g.setColour (col::text);
            g.setFont (fonts->silkF (9.0f, .1f));
            g.drawText (tapes()[(size_t) i].name, tag.withHeight (12), Justification::centredLeft, true);
            g.setColour (col::muted);
            g.setFont (fonts->uiF (12.5f));
            g.drawText (String::fromUTF8 (tapeTitle (i)) + String::fromUTF8 (u8" · ") + tapes()[(size_t) i].type,
                        tag.withY (150).withHeight (14), Justification::centredLeft, true);
            const auto bar = Rectangle<float> (tag.getX(), 170, tag.getWidth(), 3);
            g.setColour (Colour (0xff2b2d30)); g.fillRoundedRectangle (bar, 1.5f);
            const float w = proc.wear[(size_t) i].load();
            if (w > 0)
            {
                g.setGradientFill (ColourGradient (Colour (0xffc98a3c), bar.getX(), 0, Colour (0xffe0552c), bar.getRight(), 0, false));
                g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * w), 1.5f);
            }
        }
    }

    void mouseMove (const MouseEvent& e) override { setHover (indexAt (e.position)); }
    void mouseExit (const MouseEvent&) override { setHover (-1); }
    void mouseUp (const MouseEvent& e) override
    {
        const int i = indexAt (e.position);
        if (i >= 0 && i != currentTape (proc)) setParam (proc, "tape", (float) i);
    }

private:
    int indexAt (Point<float> p) const
    {
        for (int i = 0; i < 8; ++i) if (slotArea (i).withHeight (146).contains (p)) return i;
        return -1;
    }
    void setHover (int i) { if (i != hover) { hover = i; setMouseCursor (i >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); } }

    NovaAudioProcessor& proc;
    DeckState& st;
    SharedResourcePointer<Fonts> fonts;
    int hover = -1;
};

//==============================================================================
/** Cassette well: the loaded tape with turning reels. Hold the mouse on it to brake the tape. */
class Well : public Component
{
public:
    Well (NovaAudioProcessor& p, DeckState& s) : proc (p), st (s) { shown = currentTape (p); }

    void advanceAnimation (float dt)
    {
        const int want = currentTape (proc);
        if (phase == 0 && want != shown) phase = 1;
        if (phase == 1) { slide += dt / .4f; if (slide >= 1) { slide = 1; shown = want; phase = 2; } }
        else if (phase == 2) { slide -= dt / .4f; if (slide <= 0) { slide = 0; phase = 0; } }
    }
    int shownTape() const { return shown; }
    bool isSwapping() const { return phase != 0; }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawBlackFrame (g, r, 4);
        auto well = r.reduced (10);
        g.setColour (Colour (0xff050505)); g.fillRoundedRectangle (well, 5);
        {
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (well.toNearestInt());
            const float cw = well.getWidth() * .86f, chh = cw * .64f;
            const float ease = slide * slide * (3 - 2 * slide);
            auto cas = Rectangle<float> (well.getCentreX() - cw * .5f, well.getY() + (well.getHeight() - chh) * .42f, cw, chh)
                           .translated (0, -ease * (chh + 30));
            if (tapes()[(size_t) shown].small) cas = cas.withSizeKeepingCentre (cw * .82f, chh * .82f);
            drawCassette (g, cas, shown, st.packPos[shown], st.angL, st.angR, "Seite A", *fonts);

            // smoked glass
            g.setColour (Colour (0x4d16110c)); g.fillRect (well);
            ColourGradient glare (Colours::white.withAlpha (.11f), well.getX(), well.getY(), Colours::white.withAlpha (0.0f), well.getX() + well.getWidth() * .35f, well.getY() + well.getHeight() * .2f, false);
            g.setGradientFill (glare); g.fillRect (well);
            if (braking)
            {
                g.setColour (col::amber.withAlpha (.65f));
                g.drawEllipse (pressAt.x - 30, pressAt.y - 30, 60, 60, 2);
                g.setColour (col::amber.withAlpha (.15f));
                g.fillEllipse (pressAt.x - 30, pressAt.y - 30, 60, 60);
            }
        }
        g.setColour (Colours::white.withAlpha (.42f));
        g.setFont (fonts->silkF (7.5f, .2f));
        g.drawText ("HOLD = FINGER BRAKE", well.reduced (10, 6), Justification::bottomLeft, false);
        g.drawText ("SIDE A", well.reduced (10, 6), Justification::bottomRight, false);
    }

    void mouseDown (const MouseEvent& e) override
    {
        braking = true; pressAt = e.position;
        if (auto* b = param (proc, "brake")) { b->beginChangeGesture(); b->setValueNotifyingHost (1.0f); }
        repaint();
    }
    void mouseDrag (const MouseEvent& e) override { pressAt = e.position; }
    void mouseUp (const MouseEvent&) override
    {
        braking = false;
        if (auto* b = param (proc, "brake")) { b->setValueNotifyingHost (0.0f); b->endChangeGesture(); }
        repaint();
    }

private:
    NovaAudioProcessor& proc;
    DeckState& st;
    SharedResourcePointer<Fonts> fonts;
    int shown = 0, phase = 0;
    float slide = 0;
    bool braking = false;
    Point<float> pressAt;
};

//==============================================================================
/** Tape counter, VU meters, saturation lamp, tape info. */
class Meters : public Component
{
public:
    Meters (NovaAudioProcessor& p, DeckState& s) : proc (p), st (s) {}

    Rectangle<float> resetButton() const { return { 14, 18, 20, 20 }; }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawBlackFrame (g, r, 4);
        const Colour label (0xff8e9094);

        // counter
        auto rb = resetButton();
        g.setGradientFill (ColourGradient (Colours::white, rb.getX() + 6, rb.getY() + 5, Colour (0xff3d3e40), rb.getRight(), rb.getBottom(), true));
        g.fillEllipse (rb);
        const int c = ((int) std::floor (st.count) % 1000 + 1000) % 1000;
        const String digits = String (c).paddedLeft ('0', 3);
        for (int i = 0; i < 3; ++i)
        {
            auto d = Rectangle<float> (42 + (float) i * 19, 16, 17, 24);
            ColourGradient dg (Colour (0xffbdbdb8), 0, d.getY(), Colour (0xffb6b6b1), 0, d.getBottom(), false);
            dg.addColour (.38, Colour (0xfffbfbf8)); dg.addColour (.62, Colour (0xfffbfbf8));
            g.setGradientFill (dg); g.fillRoundedRectangle (d, 2);
            g.setColour (Colour (0xff121212));
            g.setFont (fonts->monoF (16.0f));
            g.drawText (digits.substring (i, i + 1), d, Justification::centred, false);
        }
        g.setColour (label);
        g.setFont (fonts->silkF (7.5f, .2f));
        g.drawText ("TAPE COUNTER", Rectangle<float> (r.getWidth() - 160, 20, 146, 16), Justification::centredRight, false);

        // VU meters and SAT lamp
        const float vw = (r.getWidth() - 28 - 50) / 2, vh = vw * 0.65f + 8;
        const float vy = 52;
        drawVu (g, { 14, vy, vw, vh }, st.needleL, "L", true, *fonts);
        drawVu (g, { r.getWidth() - 14 - vw, vy, vw, vh }, st.needleR, "R", true, *fonts);
        const auto lc = Point<float> (r.getCentreX(), vy + vh * .45f);
        if (st.sat)
        {
            DropShadow (col::rec.withAlpha (.8f), 14, {}).drawForPath (g, [&] { Path p; p.addEllipse (lc.x - 6, lc.y - 6, 12, 12); return p; }());
            g.setGradientFill (ColourGradient (Colour (0xfffff3ea), lc.x - 2, lc.y - 2, Colour (0xffa10f05), lc.x + 6, lc.y + 6, true));
        }
        else g.setGradientFill (ColourGradient (Colour (0xffc86a5d), lc.x - 2, lc.y - 2, Colour (0xff5e0d06), lc.x + 6, lc.y + 6, true));
        g.fillEllipse (lc.x - 5.5f, lc.y - 5.5f, 11, 11);
        g.setColour (Colour (0xff252527)); g.drawEllipse (lc.x - 6.5f, lc.y - 6.5f, 13, 13, 2);
        g.setColour (label);
        g.setFont (fonts->silkF (7.0f, .16f));
        g.drawText ("SAT", Rectangle<float> (lc.x - 20, lc.y + 10, 40, 10), Justification::centred, false);

        // info line
        const int t = currentTape (proc);
        const auto& td = tapes()[(size_t) t];
        g.setColour (Colour (0xff77797d));
        g.setFont (fonts->silkF (7.5f, .16f));
        const auto line = Rectangle<float> (14, r.getHeight() - 24, r.getWidth() - 28, 12);
        g.drawText (String (td.name) + String::fromUTF8 (u8"  ·  ") + td.type + String::fromUTF8 (u8"  ·  ") + String::fromUTF8 (tapeEq (t)),
                    line, Justification::centredLeft, false);
        static const char* speeds[] = { "9.5", "4.8", "2.4" };
        g.setColour (col::amber);
        g.drawText (String (speeds[jlimit (0, 2, (int) std::lround (value (proc, "speed")))]) + " cm/s", line, Justification::centredRight, false);
    }

    void mouseUp (const MouseEvent& e) override { if (resetButton().expanded (4).contains (e.position)) { st.count = 0; repaint(); } }

private:
    NovaAudioProcessor& proc;
    DeckState& st;
    SharedResourcePointer<Fonts> fonts;
};

//==============================================================================
/** Piano key with a printed label above it. */
class PianoKey : public Component
{
public:
    PianoKey (String labelText, int iconType) : label (std::move (labelText)), icon (iconType) {}
    std::function<void()> onPress, onRelease;

    void paint (Graphics& g) override
    {
        g.setColour (col::silk2);
        g.setFont (fonts->silkF (7.0f, .14f));
        g.drawText (label, Rectangle<float> (0, 0, (float) getWidth(), 12), Justification::centred, false);

        auto k = Rectangle<float> (2, 18, (float) getWidth() - 4, (float) getHeight() - 26);
        if (down) k = k.translated (0, 4);
        g.setColour (Colour (0xff55585c)); g.fillRoundedRectangle (k.translated (0, down ? 1.0f : 5.0f), 4);
        ColourGradient cg (down ? Colour (0xffe3e4e6) : Colour (0xfffdfdfd), 0, k.getY(), down ? Colour (0xffb8bbbf) : Colour (0xffa5a8ac), 0, k.getBottom(), false);
        if (! down) cg.addColour (.42, Colour (0xffd3d5d8));
        g.setGradientFill (cg); g.fillRoundedRectangle (k, 3);
        g.setColour (Colours::white.withAlpha (.8f)); g.drawLine (k.getX() + 3, k.getY() + .5f, k.getRight() - 3, k.getY() + .5f, 1);

        g.setColour (active ? Colour (0xffb25d00) : Colour (0xff2a2b2e));
        const auto c = k.getCentre();
        Path p;
        switch (icon)
        {
            case 0: p.addTriangle (c.x + 1, c.y - 6, c.x + 1, c.y + 6, c.x - 7, c.y); p.addTriangle (c.x + 9, c.y - 6, c.x + 9, c.y + 6, c.x + 1, c.y); break; // prev
            case 1: p.addTriangle (c.x - 9, c.y - 6, c.x - 9, c.y + 6, c.x - 1, c.y); p.addTriangle (c.x - 1, c.y - 6, c.x - 1, c.y + 6, c.x + 7, c.y); break; // next
            case 2: p.addRoundedRectangle (c.x - 8, c.y - 2.5f, 16, 5, 2.5f); p.addEllipse (c.x - 3, c.y - 7, 6, 6); break; // brake (finger)
            case 3: p.addStar (c, 7, 3.5f, 9.0f); break; // kick
            default: p.addRoundedRectangle (c.x - 9, c.y - 6, 18, 12, 2); break; // fresh tape
        }
        g.fillPath (p);
        if (icon == 4) { g.setColour (Colour (0xffd3d5d8)); g.fillEllipse (c.x - 6, c.y - 2.5f, 5, 5); g.fillEllipse (c.x + 1, c.y - 2.5f, 5, 5); }
    }

    void mouseDown (const MouseEvent&) override { down = true; repaint(); if (onPress) onPress(); }
    void mouseUp (const MouseEvent&) override { down = false; repaint(); if (onRelease) onRelease(); }
    bool active = false;

private:
    SharedResourcePointer<Fonts> fonts;
    String label;
    int icon;
    bool down = false;
};

//==============================================================================
/** Chrome toggle lever bound to a choice or bool parameter. */
class Lever : public Component
{
public:
    Lever (NovaAudioProcessor& p, const char* paramId, String nameText, StringArray optionTexts, String unitText = {})
        : name (std::move (nameText)), opts (std::move (optionTexts)), unit (std::move (unitText)),
          attachment (*param (p, paramId), [this] (float v) { pos = jlimit (0, opts.size() - 1, (int) std::lround (v)); repaint(); })
    {
        attachment.sendInitialUpdate();
    }

    void paint (Graphics& g) override
    {
        const float w = (float) getWidth(), cx = w * .5f;
        const int last = opts.size() - 1;
        // map the parameter index to the lever position: index 0 = up
        const int visual = opts.size() == 3 ? pos : (pos == 0 ? 0 : 2);
        g.setFont (fonts->silkF (6.6f, .1f));
        g.setColour (col::silk.withAlpha (visual == 0 ? 1.0f : .45f));
        g.drawText (opts[0], Rectangle<float> (0, 0, w, 10), Justification::centred, false);
        g.setColour (col::silk.withAlpha (visual == 2 ? 1.0f : .45f));
        g.drawText (opts[last], Rectangle<float> (0, 60, w, 10), Justification::centred, false);

        const auto slot = Rectangle<float> (cx - 7, 14, 14, 42);
        g.setColour (Colours::white.withAlpha (.75f)); g.fillRoundedRectangle (slot.translated (0, 1), 7);
        g.setGradientFill (ColourGradient (Colour (0xff0d0d0e), slot.getX(), 0, Colour (0xff0d0d0e), slot.getRight(), 0, false));
        g.fillRoundedRectangle (slot, 7);
        g.setColour (Colour (0xff2c2c2f)); g.fillRoundedRectangle (slot.reduced (4, 2), 3);
        const float top = slot.getY() + 1 + (float) visual * 9.0f;
        const auto stem = Rectangle<float> (slot.getX() + 1, top, 12, 22);
        g.setColour (Colours::black.withAlpha (.5f)); g.fillRoundedRectangle (stem.translated (0, 2), 6);
        ColourGradient cg (Colour (0xff7d8084), stem.getX(), 0, Colour (0xff64676b), stem.getRight(), 0, false);
        cg.addColour (.38, Colour (0xfffdfdfd)); cg.addColour (.68, Colour (0xffb9bbbe));
        g.setGradientFill (cg); g.fillRoundedRectangle (stem, 6);

        g.setColour (col::silk);
        g.setFont (fonts->silkF (7.5f, .2f));
        g.drawText (name, Rectangle<float> (-6, 72, w + 12, 11), Justification::centred, false);
        if (unit.isNotEmpty())
        {
            g.setColour (col::silk2);
            g.setFont (fonts->monoF (10.0f));
            g.drawText (opts[pos] + unit, Rectangle<float> (-10, 85, w + 20, 12), Justification::centred, false);
        }
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getPosition())) return;
        attachment.setValueAsCompleteGesture ((float) ((pos + 1) % opts.size()));
    }

private:
    SharedResourcePointer<Fonts> fonts;
    String name;
    StringArray opts;
    String unit;
    int pos = 0;
    ParameterAttachment attachment;
};

//==============================================================================
/** Knob with label and live value readout. */
class KnobUnit : public Component
{
public:
    KnobUnit (NovaAudioProcessor& p, KnobLook& look, const char* paramId, String labelText, bool isBig, bool isDark, bool isBipolar)
        : prm (*param (p, paramId)), label (std::move (labelText)), big (isBig), dark (isDark),
          attachment (p.apvts, paramId, slider)
    {
        slider.setLookAndFeel (&look);
        slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (190);
        slider.setDoubleClickReturnValue (true, prm.convertFrom0to1 (prm.getDefaultValue()));
        slider.getProperties().set ("dark", isDark);
        slider.getProperties().set ("bip", isBipolar);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }
    ~KnobUnit() override { slider.setLookAndFeel (nullptr); }

    void resized() override
    {
        const int s = big ? 104 : 88;
        slider.setBounds ((getWidth() - s) / 2, big ? 0 : 8, s, s);
    }

    void paint (Graphics& g) override
    {
        const float y = 100;
        g.setColour (dark ? Colour (0xffe4e5e7) : col::silk);
        g.setFont (fonts->silkF (7.5f, .13f));
        g.drawText (label, Rectangle<float> (0, y, (float) getWidth(), 11), Justification::centred, false);
        g.setColour (dark ? col::amber : col::silk2);
        g.setFont (fonts->monoF (11.0f));
        g.drawText (prm.getCurrentValueAsText(), Rectangle<float> (0, y + 12, (float) getWidth(), 14), Justification::centred, false);
    }

private:
    SharedResourcePointer<Fonts> fonts;
    RangedAudioParameter& prm;
    String label;
    bool big, dark;
    Slider slider;
    AudioProcessorValueTreeState::SliderAttachment attachment;
};

//==============================================================================
/** PRE / POST TAPE slide switch of the dirt section. */
class PosToggle : public Component
{
public:
    explicit PosToggle (NovaAudioProcessor& p)
        : attachment (*param (p, "dpos"), [this] (float v) { post = v > .5f; repaint(); }) { attachment.sendInitialUpdate(); }

    void paint (Graphics& g) override
    {
        g.setFont (fonts->silkF (7.5f, .16f));
        g.setColour (post ? Colour (0xff77797d) : Colour (0xffececed));
        g.drawText ("PRE", Rectangle<float> (0, 0, 30, 16), Justification::centredRight, false);
        g.setColour (post ? Colour (0xffececed) : Colour (0xff77797d));
        g.drawText ("POST TAPE", Rectangle<float> (78, 0, 80, 16), Justification::centredLeft, false);
        const auto trk = Rectangle<float> (36, 1, 36, 14);
        g.setColour (Colour (0xff040404)); g.fillRoundedRectangle (trk, 7);
        g.setColour (Colours::white.withAlpha (.12f)); g.drawRoundedRectangle (trk, 7, 1);
        const auto knob = Rectangle<float> (post ? 53 : 37, 2, 18, 12);
        g.setGradientFill (ColourGradient (Colour (0xfffdfdfd), 0, knob.getY(), Colour (0xffa9abae), 0, knob.getBottom(), false));
        g.fillRoundedRectangle (knob, 6);
    }
    void mouseUp (const MouseEvent&) override { attachment.setValueAsCompleteGesture (post ? 0.0f : 1.0f); }

private:
    SharedResourcePointer<Fonts> fonts;
    bool post = false;
    ParameterAttachment attachment;
};

//==============================================================================
/** Preset display with prev/next, A/B and save. */
class PresetBar : public Component
{
public:
    PresetBar (NovaAudioProcessor& p, KnobLook& l) : proc (p), look (l)
    {
        for (auto* btn : { &prev, &next, &a, &b, &copy, &save }) addAndMakeVisible (btn);
        prev.onClick = [this] { step (-1); };
        next.onClick = [this] { step (1); };
        a.onClick = [this] { switchAB (0); };
        b.onClick = [this] { switchAB (1); };
        copy.onClick = [this] { slots[1 - abCur] = proc.captureValues(); hasSlot[1 - abCur] = true; flash (abCur == 0 ? "A NACH B KOPIERT" : "B NACH A KOPIERT"); };
        save.onClick = [this] { askSave(); };
        a.setToggleState (true, dontSendNotification);
    }

    void resized() override
    {
        prev.setBounds (0, 0, 32, 32);
        display = Rectangle<int> (36, 0, getWidth() - 36 - 36 - 200, 29);
        next.setBounds (display.getRight() + 4, 0, 32, 32);
        int x = next.getRight() + 12;
        a.setBounds (x, 0, 34, 32); x += 38;
        b.setBounds (x, 0, 34, 32); x += 38;
        copy.setBounds (x, 0, 50, 32); x += 54;
        save.setBounds (x, 0, getWidth() - x, 32);
    }

    void paint (Graphics& g) override
    {
        auto d = display.toFloat();
        g.setColour (Colours::white.withAlpha (.6f)); g.fillRoundedRectangle (d.translated (0, 1), 3);
        g.setGradientFill (ColourGradient (Colour (0xff060706), 0, d.getY(), Colour (0xff121413), 0, d.getBottom(), false));
        g.fillRoundedRectangle (d, 3);
        g.setColour (Colours::black.withAlpha (.8f)); g.drawRoundedRectangle (d.reduced (.5f), 3, 1);
        auto t = d.reduced (10, 0);
        String text = flashText.isNotEmpty() ? flashText : proc.currentPresetName;
        if (flashText.isEmpty() && dirty) text << "  *";
        g.setColour (col::amber.withAlpha (.25f));
        g.setFont (fonts->monoF (15.0f));
        g.drawText (text, t.translated (0, .6f), Justification::centredLeft, true);
        g.setColour (col::amber);
        g.drawText (text, t, Justification::centredLeft, true);
        g.setColour (col::amber.withAlpha (.5f));
        g.setFont (fonts->silkF (7.0f, .2f));
        g.drawText ("PRESET", t, Justification::centredRight, false);
    }

    void mouseUp (const MouseEvent& e) override { if (display.contains (e.getPosition())) showMenu(); }

    void tick()
    {
        if (flashLeft > 0 && --flashLeft == 0) { flashText.clear(); repaint(); }
        if (++dirtyCounter >= 6)
        {
            dirtyCounter = 0;
            bool d = false;
            const auto now = proc.captureValues();
            for (auto& [id, v] : now)
            {
                auto it = proc.lastLoadedValues.find (id);
                if (it == proc.lastLoadedValues.end() || std::abs (it->second - v) > 1.0e-4f) { d = true; break; }
            }
            if (d != dirty) { dirty = d; repaint(); }
        }
    }

private:
    void flash (const String& s) { flashText = s; flashLeft = 40; repaint(); }

    void loadIndex (int i)
    {
        const int nf = proc.getNumPrograms();
        auto users = proc.getUserPresets();
        const int total = nf + users.size();
        i = ((i % total) + total) % total;
        listIndex = i;
        if (i < nf) proc.setCurrentProgram (i);
        else proc.loadUserPreset (users[i - nf]);
        proc.updateHostDisplay (AudioProcessor::ChangeDetails().withProgramChanged (true));
        dirty = false; repaint();
    }
    void step (int d) { loadIndex (listIndex + d); }

    void showMenu()
    {
        PopupMenu m;
        m.setLookAndFeel (&look);
        const auto& list = factoryPresets();
        String cat;
        for (int i = 0; i < (int) list.size(); ++i)
        {
            if (cat != list[(size_t) i].category) { cat = list[(size_t) i].category; m.addSectionHeader (cat); }
            m.addItem (1 + i, String::fromUTF8 (list[(size_t) i].name), true, i == listIndex);
        }
        auto users = proc.getUserPresets();
        m.addSectionHeader ("EIGENE");
        for (int j = 0; j < users.size(); ++j)
            m.addItem (1000 + j, users[j].getFileNameWithoutExtension(), true, (int) list.size() + j == listIndex);
        m.addSeparator();
        m.addItem (9000, String::fromUTF8 (u8"Aktuellen Sound speichern …"));
        m.addItem (9001, "Preset-Ordner zeigen");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (display)),
                         [safe = SafePointer<PresetBar> (this)] (int r)
                         {
                             if (safe == nullptr || r == 0) return;
                             const int nf = safe->proc.getNumPrograms();
                             if (r == 9000) safe->askSave();
                             else if (r == 9001) { auto f = safe->proc.getUserPresetFolder(); f.createDirectory(); f.revealToUser(); }
                             else if (r >= 1000) safe->loadIndex (nf + r - 1000);
                             else safe->loadIndex (r - 1);
                         });
    }

    void switchAB (int to)
    {
        if (to == abCur) { (to == 0 ? a : b).setToggleState (true, dontSendNotification); return; }
        slots[abCur] = proc.captureValues(); hasSlot[abCur] = true;
        if (! hasSlot[to]) { slots[to] = slots[abCur]; hasSlot[to] = true; }
        abCur = to;
        proc.applyValues (slots[to]);
        a.setToggleState (to == 0, dontSendNotification);
        b.setToggleState (to == 1, dontSendNotification);
        copy.setButtonText (to == 0 ? String::fromUTF8 (u8"A→B") : String::fromUTF8 (u8"B→A"));
        flash (to == 0 ? "EINSTELLUNG A" : "EINSTELLUNG B");
    }

    void askSave()
    {
        int n = proc.getUserPresets().size() + 1;
        auto* w = new AlertWindow ("Preset speichern", String::fromUTF8 (u8"Wie soll dein Sound heißen?"), MessageBoxIconType::NoIcon, this);
        w->addTextEditor ("name", "Mein Sound " + String (n));
        w->addButton ("Speichern", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Abbrechen", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([safe = SafePointer<PresetBar> (this), w] (int r)
        {
            if (safe == nullptr || r != 1) return;
            const auto name = w->getTextEditorContents ("name");
            if (safe->proc.saveUserPreset (name))
            {
                auto users = safe->proc.getUserPresets();
                for (int j = 0; j < users.size(); ++j)
                    if (users[j].getFileNameWithoutExtension() == File::createLegalFileName (name.trim()))
                        safe->listIndex = safe->proc.getNumPrograms() + j;
                safe->dirty = false;
                safe->flash ("GESPEICHERT");
            }
            else safe->flash ("SPEICHERN FEHLGESCHLAGEN");
        }), true);
    }

    NovaAudioProcessor& proc;
    KnobLook& look;
    SharedResourcePointer<Fonts> fonts;
    ChromeButton prev { "prev", -1 }, next { "next", 1 }, a { "A" }, b { "B" }, copy { String::fromUTF8 (u8"A→B") }, save { "SAVE" };
    Rectangle<int> display;
    NovaAudioProcessor::ValueMap slots[2];
    bool hasSlot[2] { false, false };
    int abCur = 0, listIndex = 0, flashLeft = 0, dirtyCounter = 0;
    String flashText;
    bool dirty = false;
};

//==============================================================================
/** Black anodised panel at the bottom of the deck. */
class DirtPanel : public Component
{
public:
    DirtPanel (NovaAudioProcessor& p, KnobLook& look, DeckState& s) : st (s), pos (p)
    {
        addAndMakeVisible (pos);
        const char* ids[] = { "pre", "fold", "crush", "sq", "salat", "grit", "tilt", "narrow", "dmix" };
        const char* names[] = { "PREAMP", "FOLD", "CRUSH", "SQUASH", "SALAT", "GRIT", "TILT", "NARROW", "DIRT MIX" };
        for (int i = 0; i < 9; ++i)
            addAndMakeVisible (knobs.add (new KnobUnit (p, look, ids[i], names[i], i == 0, true, i == 6)));
    }

    void resized() override
    {
        pos.setBounds (getWidth() - 270, 8, 160, 16);
        const float gw = ((float) getWidth() - 32 - 40) / 3;
        for (int i = 0; i < 9; ++i)
        {
            const int g = i / 3, k = i % 3;
            const float x = 16 + (float) g * (gw + 20) + (float) k * gw / 3;
            knobs[i]->setBounds ((int) x, 44, (int) (gw / 3), 128);
        }
    }

    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        if (bg.getWidth() != getWidth()) bg = makeBrushed (getWidth(), getHeight(), true, 9);
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (r, 5);
        g.setColour (Colours::white.withAlpha (.7f)); g.fillRoundedRectangle (r.translated (0, 1), 5);
        g.reduceClipRegion (clip);
        g.setOpacity (1.0f);   // images inherit the current colour's alpha
        g.drawImageAt (bg, 0, 0);
        g.setColour (Colours::white.withAlpha (.14f)); g.drawLine (r.getX() + 4, .5f, r.getRight() - 4, .5f, 1);

        g.setFont (fonts->silkF (8.5f, .3f));
        g.setColour (col::dirtRed);
        g.drawText ("DIRT SECTION", Rectangle<float> (16, 8, 200, 16), Justification::centredLeft, false);
        g.setColour (Colour (0xff7e8084));
        g.setFont (fonts->silkF (8.0f, .2f));
        g.drawText ("0 = BYPASS", Rectangle<float> (180, 8, 160, 16), Justification::centredLeft, false);

        // DIRT lamp
        const auto lc = Point<float> (r.getRight() - 78, 16);
        const float gl = st.dirtGlow;
        if (gl > .01f)
        {
            Path lp; lp.addEllipse (lc.x - 5, lc.y - 5, 10, 10);
            DropShadow (col::rec.withAlpha (.85f * gl), roundToInt (4 + 16 * gl), {}).drawForPath (g, lp);
            g.setGradientFill (ColourGradient (Colour (0xfffff1e8), lc.x - 2, lc.y - 2, Colour (0xff9a0f05), lc.x + 5, lc.y + 5, true));
        }
        else g.setColour (Colour (0xff4a0d07));
        g.fillEllipse (lc.x - 5, lc.y - 5, 10, 10);
        g.setColour (Colour (0xff050505)); g.drawEllipse (lc.x - 6, lc.y - 6, 12, 12, 2);
        g.setColour (Colour (0xff9a9ca0));
        g.setFont (fonts->silkF (8.0f, .2f));
        g.drawText ("DIRT", Rectangle<float> (lc.x + 10, 8, 50, 16), Justification::centredLeft, false);

        // group headers
        const float gw = ((float) getWidth() - 32 - 40) / 3;
        const char* titles[] = { "OVERLOAD", "DESTROY", "COLOR" };
        for (int i = 0; i < 3; ++i)
        {
            const float x = 16 + (float) i * (gw + 20);
            g.setColour (Colours::white.withAlpha (.12f));
            g.drawLine (x, 37, x + gw * .5f - 50, 37, 1); g.drawLine (x + gw * .5f + 50, 37, x + gw, 37, 1);
            g.setColour (col::dirtRed);
            g.setFont (fonts->silkF (8.0f, .32f));
            g.drawText (titles[i], Rectangle<float> (x + gw * .5f - 50, 30, 100, 14), Justification::centred, false);
        }
    }

private:
    DeckState& st;
    SharedResourcePointer<Fonts> fonts;
    PosToggle pos;
    OwnedArray<KnobUnit> knobs;
    Image bg;
};

//==============================================================================
class Content : public Component
{
public:
    Content (NovaAudioProcessor& p, KnobLook& look)
        : proc (p), rack (p, st), presets (p, look), well (p, st), meters (p, st), dirt (p, look, st)
    {
        for (auto* c : std::initializer_list<Component*> { &rack, &presets, &well, &meters, &dirt }) addAndMakeVisible (c);

        levers.add (new Lever (p, "monitor", "MONITOR", { "TAPE", "SOURCE" }));
        levers.add (new Lever (p, "nr", "NR", { "OFF", "ON" }));
        levers.add (new Lever (p, "speed", "SPEED", { "9.5", "4.8", "2.4" }, " cm/s"));
        levers.add (new Lever (p, "mono", "MODE", { "STEREO", "MONO" }));
        levers.add (new Lever (p, "hum", "HUM", { "OFF", "50 Hz" }));
        levers.add (new Lever (p, "wear", "WEAR", { "OFF", "ON" }));
        levers.add (new Lever (p, "random", "RANDOM", { "LIVE", "LOCK" }));
        for (auto* l : levers) addAndMakeVisible (l);

        keys.add (new PianoKey ("TAPE", 0));
        keys.add (new PianoKey ("TAPE", 1));
        keys.add (new PianoKey ("BRAKE", 2));
        keys.add (new PianoKey ("KICK", 3));
        keys.add (new PianoKey ("NEU", 4));
        for (auto* k : keys) addAndMakeVisible (k);
        keys[0]->onPress = [this] { setParam (proc, "tape", (float) ((currentTape (proc) + 7) % 8)); };
        keys[1]->onPress = [this] { setParam (proc, "tape", (float) ((currentTape (proc) + 1) % 8)); };
        keys[2]->onPress = [this] { if (auto* b = param (proc, "brake")) { b->beginChangeGesture(); b->setValueNotifyingHost (1.0f); } };
        keys[2]->onRelease = [this] { if (auto* b = param (proc, "brake")) { b->setValueNotifyingHost (0.0f); b->endChangeGesture(); } };
        keys[3]->onPress = [this] { proc.engine.triggerKick(); st.kick = .35f; shake = .38f; };
        keys[4]->onPress = [this] { proc.resetWear (currentTape (proc)); rack.repaint(); };

        const char* ids[] = { "rec", "bias", "az", "wow", "flut", "age", "hiss", "mix", "out" };
        const char* names[] = { "REC LEVEL", "BIAS", "AZIMUTH", "WOW", "FLUTTER", "AGE", "HISS", "MIX", "OUTPUT" };
        for (int i = 0; i < 9; ++i)
            addAndMakeVisible (knobs.add (new KnobUnit (p, look, ids[i], names[i], i == 0, false, i == 1)));
    }

    void resized() override
    {
        rack.setBounds (20, 4, 1200, 178);
        presets.setBounds (330, 194, 580, 32);
        well.setBounds (34, 238, 340, 214);
        meters.setBounds (390, 238, 440, 214);
        for (int i = 0; i < levers.size(); ++i) levers[i]->setBounds (846 + i * 52, 242, 50, 100);
        for (int i = 0; i < keys.size(); ++i) keys[i]->setBounds (846 + i * 72, 372, 70, 80);
        const float gw = (1172.0f - 40) / 3;
        for (int i = 0; i < 9; ++i)
        {
            const int g = i / 3, k = i % 3;
            knobs[i]->setBounds ((int) (34 + g * (gw + 20) + k * gw / 3), 482, (int) (gw / 3), 128);
        }
        dirt.setBounds (34, 612, 1172, 178);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (col::room);
        g.setGradientFill (ColourGradient (Colour (0xff16241d), 620, 120, col::room, 620, 520, true));
        g.fillRect (getLocalBounds());

        const auto deck = Rectangle<float> (16, 188, 1208, 608);
        Path dp; dp.addRoundedRectangle (deck, 7);
        DropShadow (Colours::black.withAlpha (.85f), 40, { 0, 22 }).drawForPath (g, dp);
        g.setColour (Colour (0xff6b6e73)); g.fillRoundedRectangle (deck.translated (0, 5), 7);
        if (plate.isNull()) plate = makeBrushed ((int) deck.getWidth(), (int) deck.getHeight(), false, 4);
        {
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (dp);
            g.setOpacity (1.0f);
            g.drawImageAt (plate, (int) deck.getX(), (int) deck.getY());
        }
        g.setColour (Colours::white.withAlpha (.8f)); g.drawLine (deck.getX() + 6, deck.getY() + .5f, deck.getRight() - 6, deck.getY() + .5f, 1);

        // deck head
        drawStar (g, { 36, 203, 12, 12 }, col::silk);
        g.setColour (col::silk);
        g.setFont (fonts->silkF (11.0f, .45f));
        g.drawText ("NOVA", Rectangle<float> (54, 199, 120, 20), Justification::centredLeft, false);
        g.setFont (fonts->silkF (9.5f, .2f));
        g.drawText ("CASSETTE STEREO DECK   BS-77", Rectangle<float> (930, 199, 276, 20), Justification::centredRight, false);

        // knob group headers
        const float gw = (1172.0f - 40) / 3;
        const char* titles[] = { "RECORD", "CONDITION", "PLAYBACK" };
        for (int i = 0; i < 3; ++i)
        {
            const float x = 34 + (float) i * (gw + 20), y = 470;
            g.setColour (Colours::black.withAlpha (.3f));
            g.drawLine (x, y, x + gw * .5f - 52, y, 1); g.drawLine (x + gw * .5f + 52, y, x + gw, y, 1);
            g.setColour (Colours::white.withAlpha (.55f));
            g.drawLine (x, y + 1, x + gw * .5f - 52, y + 1, 1); g.drawLine (x + gw * .5f + 52, y + 1, x + gw, y + 1, 1);
            g.setColour (col::silk);
            g.setFont (fonts->silkF (8.0f, .32f));
            g.drawText (titles[i], Rectangle<float> (x + gw * .5f - 52, y - 7, 104, 14), Justification::centred, false);
        }
    }

    // called ~30 times per second
    void tick (float dt)
    {
        const float rate = proc.engine.reelRate.load();
        const int speed = jlimit (0, 2, (int) std::lround (value (proc, "speed")));
        const float sf = speedChar (speed).f;

        well.advanceAnimation (dt);
        if (! well.isSwapping())
        {
            const int t = well.shownTape();
            const float R0 = 10, RM = 34, tp = st.packPos[t];
            const float rl = std::sqrt (R0 * R0 + (1 - tp) * (RM * RM - R0 * R0)), rr = std::sqrt (R0 * R0 + RM * RM - rl * rl);
            const float v = 47.6f * sf;
            st.angL = std::fmod (st.angL - v * rate / rl * 57.3f * dt, 360.0f);
            st.angR = std::fmod (st.angR - v * rate / rr * 57.3f * dt, 360.0f);
            st.packPos[t] = jlimit (0.0f, 1.0f, tp + dt * rate * sf / (sideMinutes (t) * 60));
            st.count += dt * rate * 1.35f;
        }

        // needles with VU ballistics
        const float ref = std::pow (10.0f, -16.0f / 20.0f);
        auto frac = [ref] (float x) { return jlimit (-.08f, 1.07f, (x / ref - .1f) / 1.3125f); };
        const float k = 1 - std::exp (-dt / .09f);
        st.needleL += (frac (proc.engine.vuL.load()) + st.kick * (rng.nextFloat() + .4f) - st.needleL) * k;
        st.needleR += (frac (proc.engine.vuR.load()) + st.kick * (rng.nextFloat() + .4f) - st.needleR) * k;
        st.kick *= std::exp (-dt / .06f);
        st.sat = proc.engine.satLed.load();

        // dirt lamp
        auto v = [this] (const char* id) { return value (proc, id); };
        const float glow = jlimit (0.0f, 1.0f, jmax (jmax (v ("pre"), v ("fold"), v ("crush")) * v ("dmix"), jmax (v ("sq"), v ("salat"), v ("grit")), v ("narrow") * .8f) * 1.25f);
        if (std::abs (glow - st.dirtGlow) > .005f) { st.dirtGlow = glow; dirt.repaint (dirt.getWidth() - 110, 0, 110, 32); }

        const bool braking = value (proc, "brake") > .5f;
        if (keys[2]->active != braking) { keys[2]->active = braking; keys[2]->repaint(); }

        // rack: repaint when tape or wear changed
        const int cur = currentTape (proc);
        float wsum = 0; for (auto& w : proc.wear) wsum += w.load();
        if (cur != lastTape || std::abs (wsum - lastWear) > .003f) { lastTape = cur; lastWear = wsum; rack.repaint(); }

        presets.tick();
        well.repaint();
        meters.repaint();
        if (shake > 0) shake = jmax (0.0f, shake - dt);
    }

    Point<float> shakeOffset() const
    {
        if (shake <= 0) return {};
        const float ph = 1 - shake / .38f, a = (1 - ph) * 7;
        return { std::sin (ph * 40) * a, std::cos (ph * 33) * a * .4f };
    }

private:
    NovaAudioProcessor& proc;
    DeckState st;
    SharedResourcePointer<Fonts> fonts;
    Rack rack;
    PresetBar presets;
    Well well;
    Meters meters;
    DirtPanel dirt;
    OwnedArray<Lever> levers;
    OwnedArray<PianoKey> keys;
    OwnedArray<KnobUnit> knobs;
    Image plate;
    Random rng;
    int lastTape = -1;
    float lastWear = -1, shake = 0;
};

} // namespace nova::ui

//==============================================================================
NovaAudioProcessorEditor::NovaAudioProcessorEditor (NovaAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    content = std::make_unique<nova::ui::Content> (p, look);
    content->setBounds (0, 0, designW, designH);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (designW / 2, designH / 2, designW * 3 / 2, designH * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) designW / (double) designH);
    setSize (designW * 4 / 5, designH * 4 / 5);

    lastTime = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

NovaAudioProcessorEditor::~NovaAudioProcessorEditor() { stopTimer(); }

void NovaAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (nova::ui::col::room); }

void NovaAudioProcessorEditor::resized() { applyTransform(); }

void NovaAudioProcessorEditor::applyTransform()
{
    const float s = (float) getWidth() / (float) designW;
    const auto off = content->shakeOffset();
    content->setTransform (juce::AffineTransform::translation (off.x, off.y).scaled (s));
}

void NovaAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = (float) juce::jlimit (0.0, .1, (now - lastTime) / 1000.0);
    lastTime = now;
    content->tick (dt);
    applyTransform();
}
