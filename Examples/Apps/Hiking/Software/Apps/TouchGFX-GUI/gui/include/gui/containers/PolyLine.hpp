#ifndef POLYLINE_HPP
#define POLYLINE_HPP

#include <touchgfx/widgets/canvas/PainterABGR2222.hpp>
#include <touchgfx/widgets/canvas/CWRUtil.hpp>
#include <touchgfx/widgets/canvas/Canvas.hpp>
#include <touchgfx/widgets/canvas/CanvasWidget.hpp>


class PolyLine : public touchgfx::CanvasWidget {
public:

    PolyLine();

    /**
     * @brief Sets the array of PolyLine points.
     * @param points: The array of points. 
     *        Format [x0,y0,x1,y1, ... xn, yn]
     */
    void setPoints(const uint8_t *points, uint32_t num);

    /**
     * @brief Sets the width for this Line.
     * @param w: The width of the line measured in pixels.
     */
    void setLineWidth(int16_t w);

    /**
     * @brief Sets the color for this Line.
     * @param color: The color.
     */
    void setColor(touchgfx::colortype color);

    virtual touchgfx::Rect getMinimalRect() const;

    virtual bool drawCanvasWidget(const touchgfx::Rect &invalidatedArea) const;

protected:
    touchgfx::PainterABGR2222 painter;
    int16_t width;

    const uint8_t *points;
    uint32_t num;

    touchgfx::Rect minimalRect;

    void updateCachedShape();

private:

    /**
     * @class Segment
     * @brief Partial implementation of the touchgfx::Line class with rounded 
     *        ends, used as a segment for a polyline.
     */
    class Segment : public  touchgfx::CanvasWidget {
    public:

        Segment();

        template <typename T>
        void setLine(T startX, T startY, T endX, T endY)
        {
            setStart(touchgfx::CWRUtil::toQ5<T>(startX), touchgfx::CWRUtil::toQ5<T>(startY));
            setEnd(touchgfx::CWRUtil::toQ5<T>(endX), touchgfx::CWRUtil::toQ5<T>(endY));
        }

        void setStart(touchgfx::CWRUtil::Q5 xQ5, touchgfx::CWRUtil::Q5 yQ5);

        void setEnd(touchgfx::CWRUtil::Q5 xQ5, touchgfx::CWRUtil::Q5 yQ5);

        template <typename T>
        void setLineWidth(T width)
        {
            setLineWidth(touchgfx::CWRUtil::toQ5<T>(width));
        }

        void setLineWidth(touchgfx::CWRUtil::Q5 widthQ5)
        {
            if (lineWidthQ5 == widthQ5) {
                return;
            }

            lineWidthQ5 = widthQ5;

            updateCachedShape();
        }

        void drawSegment(touchgfx::Canvas &canvas, const  touchgfx::Rect &invalidatedArea);

        bool drawCanvasWidget(const touchgfx::Rect &invalidatedArea) const
        {
            return true;
        }

    private:
        touchgfx::CWRUtil::Q5 startXQ5;
        touchgfx::CWRUtil::Q5 startYQ5;
        touchgfx::CWRUtil::Q5 endXQ5;
        touchgfx::CWRUtil::Q5 endYQ5;
        touchgfx::CWRUtil::Q5 lineWidthQ5;
        touchgfx::CWRUtil::Q5 cornerXQ5[4];
        touchgfx::CWRUtil::Q5 cornerYQ5[4];
        touchgfx::Rect minimalRect;
        int lineCapArcIncrement;

        void updateCachedShape();

    };

};





#endif // POLYLINE_HPP
