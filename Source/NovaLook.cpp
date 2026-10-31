#include "NovaLook.h"
#include "BinaryData.h"

namespace nova::ui
{

//==============================================================================
Fonts::Fonts()
{
    silk = Typeface::createSystemTypefaceFor (BinaryData::Michroma_ttf, BinaryData::Michroma_ttfSize);
    ui   = Typeface::createSystemTypefaceFor (BinaryData::Archivo_ttf, BinaryData::Archivo_ttfSize);
    hand = Typeface::createSystemTypefaceFor (BinaryData::ReenieBeanie_ttf, BinaryData::ReenieBeanie_ttfSize);
    mono = Typeface::createSystemTypefaceFor (BinaryData::PlexMono_ttf, BinaryData::PlexMono_ttfSize);
}

Font Fonts::make (const Typeface::Ptr& tf, float height, float kerning)
{
    Font f = tf != nullptr ? Font (FontOptions (tf).withHeight (height)) : Font (FontOptions (height));
    return std::abs (kerning) > 1.0e-6f ? f.withExtraKerningFactor (kerning) : f;
}

//==============================================================================
Image makeBrushed (int w, int h, bool dark, int seed)
{
    w = jmax (1, w); h = jmax (1, h);
    Image img (Image::ARGB, w, h, true);
    Graphics g (img);
    if (dark)
        g.setGradientFill (ColourGradient (Colour (0xff26272a), 0, 0, Colour (0xff131416), 0, (float) h, false));
    else
    {
        ColourGradient cg (col::aluHi, 0, 0, col::aluLo, 0, (float) h, false);
        cg.addColour (.46, col::alu);
        g.setGradientFill (cg);
    }
    g.fillAll();

    Random r (seed);
    // fine horizontal brushing
    for (int y = 0; y < h; ++y)
    {
        const float a = dark ? .02f + r.nextFloat() * .035f : .04f + r.nextFloat() * .08f;
        g.setColour ((r.nextBool() ? Colours::white : Colours::black).withAlpha (a));
        g.fillRect (0, y, w, 1);
    }
    // longer streaks
    for (int i = 0; i < h * 3; ++i)
    {
        const int y = r.nextInt (h), x = r.nextInt (w), len = 40 + r.nextInt (jmax (41, w / 2));
        g.setColour ((r.nextBool() ? Colours::white : Colours::black).withAlpha (dark ? .03f : .05f));
        g.fillRect (x - len / 2, y, len, 1);
    }
    return img;
}

void drawStar (Graphics& g, Rectangle<float> r, Colour c)
{
    Path p;
    const float cx = r.getCentreX(), cy = r.getCentreY(), R = r.getWidth() * .5f, k = R * .2f;
    p.startNewSubPath (cx, cy - R); p.lineTo (cx + k, cy - k); p.lineTo (cx + R, cy); p.lineTo (cx + k, cy + k);
    p.lineTo (cx, cy + R); p.lineTo (cx - k, cy + k); p.lineTo (cx - R, cy); p.lineTo (cx - k, cy - k); p.closeSubPath();
    g.setColour (c); g.fillPath (p);
}

void drawBlackFrame (Graphics& g, Rectangle<float> r, float radius)
{
    g.setColour (Colours::white.withAlpha (.6f));
    g.fillRoundedRectangle (r.translated (0, 1), radius);
    g.setGradientFill (ColourGradient (Colour (0xff151517), 0, r.getY(), Colour (0xff070708), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (Colours::black.withAlpha (.8f));
    g.drawRoundedRectangle (r.reduced (.5f), radius, 1.0f);
    g.setGradientFill (ColourGradient (Colours::black.withAlpha (.7f), 0, r.getY(), Colours::transparentBlack, 0, r.getY() + 8, false));
    g.fillRoundedRectangle (r.withHeight (8), radius);
}

void drawGlassShelf (Graphics& g, Rectangle<float> r)
{
    Path p; p.addRoundedRectangle (r, 2.0f);
    DropShadow (col::led.withAlpha (.45f), 26, { 0, 8 }).drawForPath (g, p);
    DropShadow (col::led.withAlpha (.5f), 8, { 0, 2 }).drawForPath (g, p);
    g.setGradientFill (ColourGradient (Colour (0x80e4fff0), 0, r.getY(), Colour (0x4d46966e), 0, r.getBottom(), false));
    g.fillPath (p);
    g.setColour (Colour (0x40cdffe4));
    g.strokePath (p, PathStrokeType (1.0f));
}

//==============================================================================
static void hub (Graphics& g, float cx, float cy, float angleDeg)
{
    Graphics::ScopedSaveState ss (g);
    g.addTransform (AffineTransform::rotation (degreesToRadians (angleDeg), cx, cy));
    const Colour ivory (0xffece9e1);
    g.setColour (ivory); g.fillEllipse (cx - 10, cy - 10, 20, 20);
    g.setColour (Colour (0xff111111)); g.fillEllipse (cx - 6.3f, cy - 6.3f, 12.6f, 12.6f);
    g.setColour (ivory);
    for (int i = 0; i < 6; ++i)
    {
        Path t; t.addRectangle (cx - 1.1f, cy - 6.5f, 2.2f, 2.7f);
        g.fillPath (t, AffineTransform::rotation (degreesToRadians (i * 60.0f), cx, cy));
    }
    g.setColour (Colour (0xff8d887d));
    for (int i = 0; i < 3; ++i)
    {
        const float a = degreesToRadians (i * 120.0f);
        g.fillEllipse (cx + std::sin (a) * 8.2f - 1.05f, cy - std::cos (a) * 8.2f - 1.05f, 2.1f, 2.1f);
    }
}

void drawCassette (Graphics& g, Rectangle<float> area, int tapeIndex, float tp, float angL, float angR,
                   const String& sourceLine, const Fonts& f)
{
    const auto& t = tapes()[(size_t) jlimit (0, 7, tapeIndex)];
    Graphics::ScopedSaveState ss (g);
    const float s = area.getWidth() / 200.0f;
    auto tr = AffineTransform::scale (s).translated (area.getX(), area.getY());
    if (t.warp)
        tr = AffineTransform::rotation (degreesToRadians (-1.2f), 100, 64).followedBy (AffineTransform::shear (-.026f, 0)).followedBy (tr);
    g.addTransform (tr);

    const Colour shell (t.shell), label (t.label), stripe (t.stripe), ink (t.ink), pack (t.pack), win (t.win);

    // shell
    g.setColour (shell); g.fillRoundedRectangle (1, 1, 198, 126, 7);
    g.setColour (Colours::black.withAlpha (.6f)); g.drawRoundedRectangle (1, 1, 198, 126, 7, 1.0f);

    // screws
    for (auto pt : { Point<float> (8, 8), { 192, 8 }, { 8, 120 }, { 192, 120 }, { 100, 121 } })
    {
        g.setColour (Colour (0xff8f8f8f)); g.fillEllipse (pt.x - 2.7f, pt.y - 2.7f, 5.4f, 5.4f);
        g.setColour (Colour (0xff2a2a2a)); g.drawEllipse (pt.x - 2.7f, pt.y - 2.7f, 5.4f, 5.4f, .6f);
        g.drawLine (pt.x - 1.7f, pt.y, pt.x + 1.7f, pt.y, .7f); g.drawLine (pt.x, pt.y - 1.7f, pt.x, pt.y + 1.7f, .7f);
    }

    // label
    g.setColour (label);
    if (t.torn)
    {
        Path lp; lp.startNewSubPath (12, 10); lp.lineTo (188, 10); lp.lineTo (188, 58); lp.lineTo (185, 62); lp.lineTo (188, 67);
        lp.lineTo (183, 71); lp.lineTo (186, 77); lp.lineTo (181, 82); lp.lineTo (183, 87); lp.lineTo (40, 87); lp.lineTo (36, 84);
        lp.lineTo (30, 87); lp.lineTo (12, 87); lp.closeSubPath();
        g.fillPath (lp);
    }
    else g.fillRoundedRectangle (12, 10, 176, 78, 2.5f);
    g.setColour (stripe); g.fillRect (12.0f, 10.0f, 176.0f, 8.0f);
    g.setColour (Colours::white);
    g.setFont (f.silkF (4.6f, .15f)); g.drawText (t.brand, Rectangle<float> (17, 10, 130, 8), Justification::centredLeft, false);
    g.setColour (Colours::white.withAlpha (.85f));
    g.setFont (f.silkF (4.0f, .1f)); g.drawText (t.type, Rectangle<float> (120, 10, 63, 8), Justification::centredRight, false);

    g.setColour (ink);
    g.drawRect (Rectangle<float> (16, 22, 11, 13), .8f);
    g.setFont (f.uiF (10.0f).boldened()); g.drawText ("A", Rectangle<float> (16, 22, 11, 13), Justification::centred, false);
    g.setColour (ink.withAlpha (.22f));
    g.drawLine (32, 31.5f, 182, 31.5f, .5f); g.drawLine (32, 41, 182, 41, .5f);
    g.setColour (ink);
    g.setFont (f.handF (14.0f)); g.drawText (String::fromUTF8 (tapeHand (tapeIndex)), Rectangle<float> (33, 18, 150, 14), Justification::bottomLeft, false);
    if (sourceLine.isNotEmpty())
    {
        g.setColour (ink.withAlpha (.85f));
        g.setFont (f.handF (10.5f)); g.drawText (sourceLine, Rectangle<float> (33, 31, 150, 10), Justification::bottomLeft, false);
    }

    // window with tape packs and hubs
    g.setColour (Colours::black.withAlpha (.82f)); g.fillRoundedRectangle (35, 42.5f, 130, 38, 6.5f);
    {
        Graphics::ScopedSaveState ws (g);
        Path clip; clip.addRoundedRectangle (38, 45, 124, 33, 5);
        g.reduceClipRegion (clip);
        g.setColour (win); g.fillRect (38, 45, 124, 33);
        const float R0 = 10, RM = 34;
        const float rl = std::sqrt (R0 * R0 + (1 - tp) * (RM * RM - R0 * R0));
        const float rr = std::sqrt (R0 * R0 + RM * RM - rl * rl);
        for (auto [cx, rad] : { std::pair<float, float> (57, rl), { 143, rr } })
        {
            g.setColour (pack); g.fillEllipse (cx - rad, 61.5f - rad, rad * 2, rad * 2);
            g.setColour (Colours::black.withAlpha (.35f)); g.drawEllipse (cx - rad, 61.5f - rad, rad * 2, rad * 2, .6f);
        }
        hub (g, 57, 61.5f, angL);
        hub (g, 143, 61.5f, angR);
        g.setColour (Colours::white.withAlpha (.06f)); g.fillRect (38.0f, 45.0f, 124.0f, 11.0f);
    }

    g.setColour (ink.withAlpha (.7f));
    g.setFont (f.uiF (4.2f));
    g.drawText (CharPointer_UTF8 ("NR \xe2\x96\xa1 ON  \xe2\x96\xa1 OFF"), Rectangle<float> (40, 82, 60, 5), Justification::centredLeft, false);
    g.drawText (String (t.len) + String::fromUTF8 (u8" · EQ ") + String::fromUTF8 (tapeEq (tapeIndex)),
                Rectangle<float> (100, 82, 60, 5), Justification::centredRight, false);

    // head side
    Path trap; trap.startNewSubPath (44, 128); trap.lineTo (53, 97); trap.lineTo (147, 97); trap.lineTo (156, 128); trap.closeSubPath();
    g.setColour (Colours::black.withAlpha (.22f)); g.fillPath (trap);
    g.setColour (Colours::black.withAlpha (.35f)); g.strokePath (trap, PathStrokeType (.6f));
    g.setColour (Colour (0xff0a0a0a));
    g.fillEllipse (68 - 3.2f, 116 - 3.2f, 6.4f, 6.4f); g.fillEllipse (132 - 3.2f, 116 - 3.2f, 6.4f, 6.4f);
    g.fillRoundedRectangle (92, 113, 16, 13, 1); g.fillRect (78.0f, 118.0f, 7.0f, 8.0f); g.fillRect (115.0f, 118.0f, 7.0f, 8.0f);

    // gloss
    ColourGradient gl (Colours::white.withAlpha (.2f), 0, 0, Colours::black.withAlpha (.28f), 0, 128, false);
    gl.addColour (.45, Colours::white.withAlpha (0.0f));
    g.setGradientFill (gl); g.fillRoundedRectangle (1, 1, 198, 126, 7);
}

//==============================================================================
void drawVu (Graphics& g, Rectangle<float> area, float needle, const String& channel, bool lit, const Fonts& f)
{
    Graphics::ScopedSaveState ss (g);
    // bezel
    g.setGradientFill (ColourGradient (Colour (0xff46474a), 0, area.getY(), Colour (0xff121213), 0, area.getBottom(), false));
    g.fillRoundedRectangle (area, 4);
    g.setColour (Colour (0xff5b5c60)); g.drawRoundedRectangle (area.reduced (.5f), 4, 1);
    auto face = area.reduced (4);
    g.reduceClipRegion (face.toNearestInt());
    g.addTransform (AffineTransform::scale (face.getWidth() / 160.0f, face.getHeight() / 104.0f).translated (face.getX(), face.getY()));

    ColourGradient fg (Colour (0xfffff8e0), 80, 91, Colour (0xffc5b080), 80 + 140, 91, true);
    fg.addColour (.55, Colour (0xfff1e5c3));
    g.setGradientFill (fg); g.fillRect (0.0f, 0.0f, 160.0f, 104.0f);
    if (lit)
    {
        g.setGradientFill (ColourGradient (col::amber.withAlpha (.5f), 80, 120, col::amber.withAlpha (0.0f), 80 + 150, 120, true));
        g.fillRect (0.0f, 0.0f, 160.0f, 104.0f);
    }

    const float px = 80, py = 128;
    auto pt = [&] (float aDeg, float r) { const float a = degreesToRadians (aDeg); return Point<float> (px + r * std::sin (a), py - r * std::cos (a)); };
    auto ang = [] (float db) { return -46.0f + 92.0f * (std::pow (10.0f, db / 20.0f) - .1f) / (1.4125f - .1f); };
    const Colour ink (0xff1c1c1c), red (0xffc4271b);
    const float a0 = ang (0);
    {
        Path arc; arc.addCentredArc (px, py, 88, 88, 0, degreesToRadians (-46.0f), degreesToRadians (a0), true);
        g.setColour (ink); g.strokePath (arc, PathStrokeType (1.1f));
        Path arcR; arcR.addCentredArc (px, py, 89.6f, 89.6f, 0, degreesToRadians (a0), degreesToRadians (46.0f), true);
        g.setColour (red); g.strokePath (arcR, PathStrokeType (3.6f));
    }
    for (int db : { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 })
    {
        const float a = ang ((float) db);
        const auto c = db > 0 ? red : ink;
        g.setColour (c);
        g.drawLine (Line<float> (pt (a, 88), pt (a, db >= 0 ? 95.5f : 94)), (db == 0 || db == -10 || db == -20) ? 1.3f : .8f);
        const auto tp = pt (a, 101);
        g.setFont (f.uiF (db == 0 ? 7.6f : 6.6f).boldened());
        g.drawText (db > 0 ? "+" + String (db) : String (std::abs (db)), Rectangle<float> (tp.x - 8, tp.y - 4, 16, 8), Justification::centred, false);
    }
    g.setColour (ink);
    for (float v : { .2f, .4f, .6f, .8f, 1.0f })
    {
        const float a = -46 + 92 * (v - .1f) / 1.3125f;
        g.drawLine (Line<float> (pt (a, 84), pt (a, 80.5f)), .6f);
        const auto tp = pt (a, 75);
        g.setFont (f.uiF (4.6f));
        g.drawText (String (roundToInt (v * 100)), Rectangle<float> (tp.x - 6, tp.y - 3, 12, 6), Justification::centred, false);
    }
    g.setFont (f.silkF (12.0f, .08f)); g.drawText ("VU", Rectangle<float> (60, 66, 40, 14), Justification::centred, false);
    g.setColour (ink.withAlpha (.7f));
    g.setFont (f.silkF (4.6f, .3f)); g.drawText ("NOVA", Rectangle<float> (60, 84, 40, 6), Justification::centred, false);
    g.setColour (ink);
    g.setFont (f.silkF (7.0f)); g.drawText (channel, Rectangle<float> (8, 88, 14, 10), Justification::centredLeft, false);

    // needle
    const float na = -46 + 92 * needle;
    g.setColour (Colours::black.withAlpha (.18f));
    g.drawLine (Line<float> (Point<float> (px + 1.5f, py), pt (na, 96).translated (1.5f, 0)), 1.6f);
    g.setColour (Colour (0xff141414));
    g.drawLine (Line<float> (Point<float> (px, py), pt (na, 97)), 1.05f);

    Path cover; cover.startNewSubPath (0, 104); cover.lineTo (0, 97); cover.quadraticTo (80, 84, 160, 97); cover.lineTo (160, 104); cover.closeSubPath();
    g.setColour (Colour (0xff111111)); g.fillPath (cover);
}

//==============================================================================
KnobLook::KnobLook()
{
    setColour (PopupMenu::backgroundColourId, Colour (0xff0c0d0c));
    setColour (PopupMenu::textColourId, Colour (0xffe7bf86));
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff1d1e1c));
    setColour (PopupMenu::highlightedTextColourId, col::amber);
    setColour (PopupMenu::headerTextColourId, Colour (0xff73756f));
}

void KnobLook::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (findColour (PopupMenu::backgroundColourId));
    g.setColour (Colour (0xff2e302e)); g.drawRect (0, 0, w, h);
}

Font KnobLook::getPopupMenuFont() { return fonts->monoF (15.0f); }

void KnobLook::drawPopupMenuSectionHeader (Graphics& g, const Rectangle<int>& area, const String& name)
{
    g.setColour (findColour (PopupMenu::headerTextColourId));
    g.setFont (fonts->silkF (9.0f, .26f));
    g.drawText (name, area.reduced (12, 0).toFloat(), Justification::bottomLeft, false);
}

void KnobLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const bool dark = (bool) s.getProperties()["dark"];
    const bool bip = (bool) s.getProperties()["bip"];
    const float S = (float) jmin (w, h), u = S / 100.0f;
    const float cx = (float) x + (float) w * .5f, cy = (float) y + (float) h * .5f;
    auto pt = [&] (float a, float r) { return Point<float> (cx + r * std::sin (a), cy - r * std::cos (a)); };
    const Colour scaleCol = dark ? Colour (0xffc3c4c7) : col::silk;

    // printed scale
    g.setColour (scaleCol);
    g.setFont (fonts->uiF (7.6f * u).boldened());
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        g.drawLine (Line<float> (pt (a, 35 * u), pt (a, 39 * u)), 1.1f * jmax (.8f, u));
        const auto tp = pt (a, 45.5f * u);
        const int lab = bip ? std::abs (5 - i) : i;
        g.drawText (String (lab), Rectangle<float> (tp.x - 6 * u, tp.y - 4.5f * u, 12 * u, 9 * u), Justification::centred, false);
    }

    const float R = 29.0f * u, angle = start + pos * (end - start);
    Path circle; circle.addEllipse (cx - R, cy - R, 2 * R, 2 * R);
    DropShadow (Colours::black.withAlpha (dark ? .75f : .45f), roundToInt (11 * u), { 0, roundToInt (7 * u) }).drawForPath (g, circle);

    // knurled ring (turns with the knob)
    g.setColour (Colour (0xff76787b)); g.fillPath (circle);
    Path teeth;
    for (int i = 0; i < 60; ++i)
    {
        const float a0 = angle + degreesToRadians (i * 6.0f);
        teeth.addPieSegment (cx - R, cy - R, 2 * R, 2 * R, a0, a0 + degreesToRadians (3.0f), 0);
    }
    g.setColour (Colour (0xffececec)); g.fillPath (teeth);

    // spun-metal face (light stays put)
    const float F = 21.5f * u;
    g.setGradientFill (ColourGradient (Colour (0xfff8f8f8), cx, cy, Colour (0xffb9bbbe), cx + F, cy, true));
    g.fillEllipse (cx - F, cy - F, 2 * F, 2 * F);
    for (int i = 0; i < 8; ++i)
    {
        const float a0 = degreesToRadians (18.0f + i * 45.0f);
        Path wdg; wdg.addPieSegment (cx - F, cy - F, 2 * F, 2 * F, a0, a0 + degreesToRadians (45.0f), 0);
        g.setColour ((i % 2 == 0 ? Colours::white.withAlpha (.35f) : Colours::black.withAlpha (.12f)));
        g.fillPath (wdg);
    }
    g.setColour (Colours::white.withAlpha (.75f)); g.drawEllipse (cx - F, cy - F, 2 * F, 2 * F, 1.0f);

    // pointer
    g.setColour (dark ? Colour (0xffd8261a) : Colour (0xff181818));
    g.drawLine (Line<float> (pt (angle, R * .96f), pt (angle, R * .42f)), jmax (1.6f, 2.2f * u));
}

} // namespace nova::ui
