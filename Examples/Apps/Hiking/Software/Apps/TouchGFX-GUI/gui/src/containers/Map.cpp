#include <gui/containers/Map.hpp>

Map::Map()
{
    track.setPosition(4, 4, 142, 142);
    track.setLineWidth(3);
    track.setColor(0xC08000);  // yellow-dark
    insert(nullptr, track);    // insert behind the dots
}

void Map::initialize()
{
    MapBase::initialize();
}

void Map::setMap(const SDK::TrackMapScreen& map)
{
    if (map.points.empty()) {
        return;
    }

    const int16_t offsetX  = -static_cast<int16_t>(map.minx);
    const int16_t offsetY  = -static_cast<int16_t>(map.miny);
    const int16_t contentW = static_cast<int16_t>(map.maxx - map.minx);
    const int16_t contentH = static_cast<int16_t>(map.maxy - map.miny);
    const int16_t centerX  = (142 - contentW) / 2;
    const int16_t centerY  = (142 - contentH) / 2;

    track.setXY(4 + centerX + offsetX, 4 + centerY + offsetY);
    track.setPoints(reinterpret_cast<const uint8_t*>(map.points.data()), map.points.size());

    dotStart.setXY(centerX + offsetX + map.points.front().x,
                   centerY + offsetY + map.points.front().y);
    dotEnd.setXY(centerX + offsetX + map.points.back().x,
                 centerY + offsetY + map.points.back().y);

    invalidate();
}
