#pragma once

#include <juce_graphics/juce_graphics.h>

/*  Entropic Labs -- the house mark and the house textures.

    This file is shared verbatim between every Entropic Labs plugin (Doppler FX,
    Dirty Bass, ...). Change it in one place and copy it to the others.

    The mark is the studio ident: a heavy "E" whose three arms break up into
    pixels that scatter off to the right -- order on the left, entropy on the
    right -- with one ember pixel adrift among them. It is drawn as geometry,
    not loaded from an image, so it is crisp at any size and needs no asset
    pipeline. The ember pixel doubles as the instrument's activity light.

    The two textures here come from the same ident: the faint dot grid it sits
    on, and a rule that starts solid and dissolves into dust along its length.
*/
namespace Brand
{
    // ---- Palette ------------------------------------------------------------
    // Acid is anything you can move or anything currently reading true. Ember
    // is the instrument telling you about itself. Everything else is graphite.
    inline const juce::Colour ink   { 0xff08090c };
    inline const juce::Colour acid  { 0xffd9ff3d };
    inline const juce::Colour ember { 0xffff6b3d };
    inline const juce::Colour paper { 0xffd5d8d0 };

    /** Width / height of the whole mark, pixels included. */
    constexpr float markAspect = 1.40f;

    namespace detail
    {
        /** Cheap integer hash to 0..1, so the scatter is random-looking but
            identical on every paint and every platform. */
        inline float hash01 (juce::uint32 n) noexcept
        {
            n ^= n >> 16;  n *= 0x7feb352du;
            n ^= n >> 15;  n *= 0x846ca68bu;
            n ^= n >> 16;
            return (float) (n & 0xffffffu) / 16777215.0f;
        }

        struct Pixel { float cx, cy, size, alpha; };

        /** The scatter, in a space where the E is one unit tall and its left
            edge is x = 0. Measured off the ident frame by frame. */
        inline const Pixel* pixels (int& count) noexcept
        {
            static const Pixel table[] =
            {
                // top arm
                { 0.635f, 0.105f, 0.154f, 1.00f }, { 0.865f, 0.095f, 0.115f, 0.95f },
                { 1.040f, 0.085f, 0.077f, 0.90f }, { 1.190f, 0.068f, 0.042f, 0.85f },
                { 1.320f, 0.082f, 0.027f, 0.75f },
                // middle arm
                { 0.510f, 0.500f, 0.135f, 1.00f }, { 0.710f, 0.500f, 0.115f, 0.95f },
                { 0.885f, 0.505f, 0.077f, 0.90f }, { 1.020f, 0.515f, 0.042f, 0.85f },
                { 1.300f, 0.470f, 0.020f, 0.60f },
                // bottom arm
                { 0.635f, 0.895f, 0.154f, 1.00f }, { 0.865f, 0.905f, 0.115f, 0.95f },
                { 1.040f, 0.915f, 0.077f, 0.90f }, { 1.190f, 0.935f, 0.042f, 0.85f },
                { 1.320f, 0.918f, 0.027f, 0.75f },
                // dust
                { 1.230f, 0.020f, 0.014f, 0.40f }, { 1.370f, 0.130f, 0.012f, 0.35f },
                { 1.385f, 0.560f, 0.012f, 0.35f }, { 1.250f, 0.985f, 0.014f, 0.40f },
                { 1.375f, 0.860f, 0.012f, 0.35f },
            };
            count = (int) (sizeof (table) / sizeof (table[0]));
            return table;
        }

        /** Where the ember pixel sits, same space. */
        inline const Pixel& emberPixel() noexcept
        {
            static const Pixel p { 1.155f, 0.430f, 0.042f, 1.0f };
            return p;
        }
    }

    /** Fits the mark into `area`, left-aligned and vertically centred, and
        returns the rectangle it actually occupied (so text can sit beside it).

        `emberGlow` is 0..1: at 0 the ember pixel is a quiet dot; as it rises
        the pixel brightens and throws a soft halo. Feed it an output level and
        the logo becomes the activity light.
    */
    inline juce::Rectangle<float> drawMark (juce::Graphics& g, juce::Rectangle<float> area,
                                            juce::Colour body, juce::Colour emberColour,
                                            float emberGlow = 0.0f)
    {
        if (area.isEmpty())
            return {};

        const auto unit = juce::jmin (area.getHeight(), area.getWidth() / markAspect);
        const auto x0   = area.getX();
        const auto y0   = area.getCentreY() - unit * 0.5f;
        const auto to   = juce::AffineTransform::scale (unit).translated (x0, y0);

        // The letter: a stem and three arms, outer corners softened. Overlapping
        // same-direction subpaths union under non-zero winding.
        juce::Path letter;
        const auto r = 0.032f;
        letter.addRoundedRectangle (0.0f, 0.000f, 0.330f, 1.000f, r);
        letter.addRoundedRectangle (0.0f, 0.000f, 0.480f, 0.210f, r);
        letter.addRoundedRectangle (0.0f, 0.415f, 0.385f, 0.170f, r);
        letter.addRoundedRectangle (0.0f, 0.790f, 0.480f, 0.210f, r);

        g.setColour (body);
        g.fillPath (letter, to);

        int count = 0;
        const auto* px = detail::pixels (count);

        for (int i = 0; i < count; ++i)
        {
            const auto& p = px[i];
            juce::Path square;
            square.addRoundedRectangle (p.cx - p.size * 0.5f, p.cy - p.size * 0.5f,
                                        p.size, p.size, p.size * 0.18f);
            g.setColour (body.withMultipliedAlpha (p.alpha));
            g.fillPath (square, to);
        }

        // The ember pixel, and its halo when the instrument is doing something.
        {
            const auto& e  = detail::emberPixel();
            const auto glow = juce::jlimit (0.0f, 1.0f, emberGlow);
            const auto centre = juce::Point<float> (e.cx, e.cy).transformedBy (to);
            const auto size   = e.size * unit;

            if (glow > 0.01f)
            {
                const auto radius = size * (2.0f + 4.0f * glow);
                g.setGradientFill (juce::ColourGradient (emberColour.withAlpha (0.55f * glow), centre.x, centre.y,
                                                         emberColour.withAlpha (0.0f), centre.x + radius, centre.y, true));
                g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
            }

            g.setColour (emberColour.brighter (0.35f * glow).withAlpha (0.78f + 0.22f * glow));
            g.fillRoundedRectangle (juce::Rectangle<float> (size, size).withCentre (centre), size * 0.18f);
        }

        return { x0, y0, unit * markAspect, unit };
    }

    /** The ident's backdrop: a faint grid of single dots. Only the dots inside
        the current clip are generated, so a child repainting at 30 fps does not
        pay for the whole window. */
    inline void dotGrid (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour,
                         float spacing = 16.0f, float dot = 1.0f)
    {
        const auto clip = g.getClipBounds().toFloat().getIntersection (area);
        if (clip.isEmpty() || spacing < 2.0f)
            return;

        juce::RectangleList<float> dots;

        const auto startCol = std::floor ((clip.getX() - area.getX()) / spacing);
        const auto startRow = std::floor ((clip.getY() - area.getY()) / spacing);

        for (auto y = area.getY() + startRow * spacing; y <= clip.getBottom(); y += spacing)
            for (auto x = area.getX() + startCol * spacing; x <= clip.getRight(); x += spacing)
                dots.addWithoutMerging ({ x, y, dot, dot });

        g.setColour (colour);
        g.fillRectList (dots);
    }

    /** A rule that is a solid line at its left end and has dissolved into
        scattered pixels by its right end. Trim, not information: keep `alpha`
        low. `seed` varies the scatter between two rules on the same panel. */
    inline void entropyRule (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour,
                             float alpha = 0.5f, juce::uint32 seed = 1)
    {
        if (area.isEmpty())
            return;

        constexpr float solidUntil = 0.26f;     // fraction of the width that stays a clean line

        const auto px   = juce::jmax (1.5f, std::floor (area.getHeight() * 0.34f));
        const auto midY = area.getCentreY();
        const auto span = juce::jmax (1.0f, area.getHeight() - px);

        juce::RectangleList<float> solid, dust;
        solid.addWithoutMerging ({ area.getX(), midY - px * 0.5f, area.getWidth() * solidUntil, px });

        int i = 0;
        for (auto x = area.getX() + area.getWidth() * solidUntil; x < area.getRight(); x += px, ++i)
        {
            // 0 where the line ends, 1 at the far right.
            const auto u = (x - area.getX() - area.getWidth() * solidUntil) / (area.getWidth() * (1.0f - solidUntil));
            const auto h = detail::hash01 ((juce::uint32) i * 2654435761u + seed * 97u);

            // Survival falls off along the rule; what survives drifts further
            // from the centre line the further along it is.
            const auto survive = 1.0f - std::pow (u, 1.3f) * 0.95f;
            if (h > survive)
                continue;

            const auto drift = (detail::hash01 ((juce::uint32) i * 40503u + seed * 131u) - 0.5f) * 2.0f;
            const auto y     = midY - px * 0.5f + drift * span * 0.5f * juce::jlimit (0.0f, 1.0f, u * 1.6f);

            dust.addWithoutMerging ({ x, y, px, px });
        }

        g.setColour (colour.withAlpha (alpha));
        g.fillRectList (solid);
        g.setColour (colour.withAlpha (alpha * 0.85f));
        g.fillRectList (dust);
    }
}
