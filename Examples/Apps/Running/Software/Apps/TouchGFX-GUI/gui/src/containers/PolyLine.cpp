#include <touchgfx/Utils.hpp>
#include <touchgfx/widgets/canvas/Line.hpp>
#include <gui/containers/PolyLine.hpp>
#include <cstdio>
#include <touchgfx/widgets/canvas/Canvas.hpp>
#include <touchgfx/Drawable.hpp>


PolyLine::PolyLine()
    : width(1), points(nullptr), num(0),  minimalRect()
{
    painter.setColor(touchgfx::Color::getColorFromRGB(0xFF, 0xFF, 0xFF));
    setPainter(painter);
}

void PolyLine::setPoints(const uint8_t *points, uint32_t num)
{
    if (points == nullptr || num < 2) {
        return;
    }

    this->points = points;
    this->num = num;

    updateCachedShape();
}

void PolyLine::setLineWidth(int16_t w)
{
    width = w;
    updateCachedShape();
}

void PolyLine::setColor(touchgfx::colortype color)
{
    painter.setColor(color);
}

touchgfx::Rect PolyLine::getMinimalRect() const
{
    return minimalRect;
}

void PolyLine::updateCachedShape()
{
    if (points == nullptr || num < 2) {
        return;
    }

    int16_t xMin = static_cast<int16_t>(points[0]);
    int16_t yMin = static_cast<int16_t>(points[1]);
    int16_t xMax = xMin;
    int16_t yMax = yMin;

    for (uint32_t i = 1; i < num; i++) {
        int16_t x = static_cast<int16_t>(points[i * 2]);
        int16_t y = static_cast<int16_t>(points[i * 2 + 1]);

        if (x < xMin) {
            xMin =x;
        }
        if (x > xMax) {
            xMax = x;
        }
        if (y < yMin) {
            yMin = y;
        }
        if (y > yMax) {
            yMax = y;
        }
    }

    minimalRect = touchgfx::Rect(xMin, yMin, xMax - xMin + width, yMax - yMin + width);
}

bool PolyLine::drawCanvasWidget(const touchgfx::Rect &invalidatedArea) const
{
    //printf("XYWH : %d %d %d %d\n", getX(), getY(), getWidth(), getHeight());
    //printf("invalidatedArea : %d %d %d %d\n", invalidatedArea.x, invalidatedArea.y, invalidatedArea.width, invalidatedArea.height);

    touchgfx::Canvas canvas(getPainter(), getAbsoluteRect(), invalidatedArea, getAlpha());

    for (uint32_t i = 0; i < (num - 1); i++) {
        int16_t x0 = static_cast<int16_t>(points[i * 2]);
        int16_t y0 = static_cast<int16_t>(points[i * 2 + 1]);
        int16_t x1 = static_cast<int16_t>(points[i * 2 + 2]);
        int16_t y1 = static_cast<int16_t>(points[i * 2 + 3]);

        Segment line {};
        line.setPosition(getX(), getY(), getWidth(), getHeight()); // Position and size same as parent
        line.setPainter(painter);

        int16_t widthOffset = width / 2;

        line.setLine(widthOffset + x0, widthOffset + y0, widthOffset + x1, widthOffset + y1);
        line.setLineWidth(width);

        line.drawSegment(canvas, invalidatedArea);
    }

    return canvas.render();

}


PolyLine::Segment::Segment()
    : startXQ5(0), startYQ5(0), endXQ5(0), endYQ5(0),
    lineWidthQ5(touchgfx::CWRUtil::toQ5<int>(1)),
    minimalRect(),
    lineCapArcIncrement(18)
{
    Drawable::setWidthHeight(0, 0);
}

void PolyLine::Segment::setStart(touchgfx::CWRUtil::Q5 xQ5, touchgfx::CWRUtil::Q5 yQ5)
{
    if (startXQ5 == xQ5 && startYQ5 == yQ5) {
        return;
    }

    startXQ5 = xQ5;
    startYQ5 = yQ5;

    updateCachedShape();
}

void PolyLine::Segment::setEnd(touchgfx::CWRUtil::Q5 xQ5, touchgfx::CWRUtil::Q5 yQ5)
{
    if (endXQ5 == xQ5 && endYQ5 == yQ5) {
        return;
    }

    endXQ5 = xQ5;
    endYQ5 = yQ5;

    updateCachedShape();
}

void PolyLine::Segment::drawSegment(touchgfx::Canvas &canvas, const touchgfx::Rect &invalidatedArea)
{
    touchgfx::CWRUtil::Q5 radiusQ5;
    const int angleInDegrees = touchgfx::CWRUtil::angle(cornerXQ5[0] - startXQ5, cornerYQ5[0] - startYQ5, radiusQ5);
    canvas.moveTo(cornerXQ5[0], cornerYQ5[0]);
    canvas.lineTo(cornerXQ5[1], cornerYQ5[1]);
    for (int i = lineCapArcIncrement; i < 180; i += lineCapArcIncrement) {
        canvas.lineTo(endXQ5 + radiusQ5 * touchgfx::CWRUtil::sine(angleInDegrees - i), endYQ5 - radiusQ5 * touchgfx::CWRUtil::cosine(angleInDegrees - i));
    }
    canvas.lineTo(cornerXQ5[2], cornerYQ5[2]);
    canvas.lineTo(cornerXQ5[3], cornerYQ5[3]);
    for (int i = 180 - lineCapArcIncrement; i > 0; i -= lineCapArcIncrement) {
        canvas.lineTo(startXQ5 + radiusQ5 * touchgfx::CWRUtil::sine(angleInDegrees + i), startYQ5 - radiusQ5 * touchgfx::CWRUtil::cosine(angleInDegrees + i));
    }
}

void PolyLine::Segment::updateCachedShape()
{
    touchgfx::CWRUtil::Q5 dxQ5 = endXQ5 - startXQ5;
    touchgfx::CWRUtil::Q5 dyQ5 = endYQ5 - startYQ5;
    touchgfx::CWRUtil::Q5 dQ5 = touchgfx::CWRUtil::toQ5<int>(0);
    // Look for horizontal, vertical or no-line
    if (dxQ5 == 0) {
        if (dyQ5 == 0) {
            cornerXQ5[0] = cornerXQ5[1] = cornerXQ5[2] = cornerXQ5[3] = startXQ5;
            cornerYQ5[0] = cornerYQ5[1] = cornerYQ5[2] = cornerYQ5[3] = startYQ5;
            return;
        }
        dQ5 = abs(dyQ5);
    } else if (dyQ5 == 0) {
        dQ5 = abs(dxQ5);
    } else {
        // We want to hit as close to the limit inside 32bits.
        // sqrt(0x7FFFFFFF / 2) = 46340, which is roughly toQ5(1448)
        static const int32_t MAXVAL = 46340;
        const int32_t common_divisor = touchgfx::gcd(touchgfx::abs((int32_t)dxQ5), touchgfx::abs((int32_t)dyQ5));
        // First try to scale down
        if (common_divisor != 1) {
            dxQ5 = touchgfx::CWRUtil::Q5((int32_t)dxQ5 / common_divisor);
            dyQ5 = touchgfx::CWRUtil::Q5((int32_t)dyQ5 / common_divisor);
        }
        // Neither dx or dy is zero, find the largest multiplier / smallest divisor to stay within limit
        if (touchgfx::abs((int32_t)dxQ5) <= MAXVAL || touchgfx::abs((int32_t)dyQ5) <= MAXVAL) {
            // Look for largest multiplier
            const int32_t mulx = MAXVAL / touchgfx::abs((int32_t)dxQ5);
            const int32_t muly = MAXVAL / touchgfx::abs((int32_t)dyQ5);
            const int32_t mult = MIN(mulx, muly);
            dxQ5 = touchgfx::CWRUtil::Q5(mult * (int32_t)dxQ5);
            dyQ5 = touchgfx::CWRUtil::Q5(mult * (int32_t)dyQ5);
    } else {
            // Look for smallest divisor
            const int32_t divx = touchgfx::abs((int32_t)dxQ5) / MAXVAL;
            const int32_t divy = touchgfx::abs((int32_t)dyQ5) / MAXVAL;
            const int32_t divi = MAX(divx, divy) + 1;
            dxQ5 = touchgfx::CWRUtil::Q5((int32_t)dxQ5 / divi);
            dyQ5 = touchgfx::CWRUtil::Q5((int32_t)dyQ5 / divi);
        }
        dQ5 = touchgfx::CWRUtil::length(dxQ5, dyQ5);
    }

    dyQ5 = touchgfx::CWRUtil::muldivQ5(lineWidthQ5, dyQ5, dQ5) / 2;
    dxQ5 = touchgfx::CWRUtil::muldivQ5(lineWidthQ5, dxQ5, dQ5) / 2;

    cornerXQ5[0] = startXQ5 - dyQ5;
    cornerYQ5[0] = startYQ5 + dxQ5;
    cornerXQ5[1] = endXQ5 - dyQ5;
    cornerYQ5[1] = endYQ5 + dxQ5;
    cornerXQ5[2] = endXQ5 + dyQ5;
    cornerYQ5[2] = endYQ5 - dxQ5;
    cornerXQ5[3] = startXQ5 + dyQ5;
    cornerYQ5[3] = startYQ5 - dxQ5;

}
