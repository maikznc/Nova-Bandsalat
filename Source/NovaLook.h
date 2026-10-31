#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "TapeData.h"

namespace nova::ui
{
using namespace juce;

namespace col
{
    const Colour room   { 0xff121314 }, room2 { 0xff191a1c }, text { 0xffdddbd4 }, muted { 0xff93918a };
    const Colour aluHi  { 0xffeef0f2 }, alu { 0xffc9cbce }, aluLo { 0xff9ea1a6 };
    const Colour silk   { 0xff25262a }, silk2 { 0xff4a4c50 };
    const Colour amber  { 0xffffb347 }, rec { 0xffff3b24 }, led { 0xff4dffa0 }, vuFace { 0xfff1e6c6 }, dirtRed { 0xffff5a3c };
}

/** The four embedded typefaces (all SIL Open Font License). */
struct Fonts
{
    Fonts();
    Typeface::Ptr silk, ui, hand, mono;
    static Font make (const Typeface::Ptr& tf, float height, float kerning = 0.0f);
    Font silkF (float h, float kern = .2f) const { return make (silk, h, kern); }
    Font uiF   (float h, float kern = 0) const   { return make (ui, h, kern); }
    Font handF (float h) const                   { return make (hand, h); }
    Font monoF (float h) const                   { return make (mono, h); }
};

/** Brushed metal plate, rendered once into an image. */
Image makeBrushed (int w, int h, bool dark, int seed);

void drawStar (Graphics&, Rectangle<float>, Colour);
void drawCassette (Graphics&, Rectangle<float> area, int tape, float packPos, float angL, float angR,
                   const String& sourceLine, const Fonts&);
void drawVu (Graphics&, Rectangle<float> area, float needle, const String& channel, bool lit, const Fonts&);
void drawGlassShelf (Graphics&, Rectangle<float>);
void drawBlackFrame (Graphics&, Rectangle<float>, float radius = 4.0f);

//==============================================================================
/** Knurled aluminium knobs with a printed 0-10 scale. */
class KnobLook : public LookAndFeel_V4
{
public:
    KnobLook();
    void drawRotarySlider (Graphics&, int x, int y, int w, int h, float pos, float start, float end, Slider&) override;
    void drawPopupMenuBackground (Graphics&, int width, int height) override;
    Font getPopupMenuFont() override;
    void drawPopupMenuSectionHeader (Graphics&, const Rectangle<int>&, const String&) override;
    SharedResourcePointer<Fonts> fonts;
};

} // namespace nova::ui
