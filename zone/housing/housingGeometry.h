#ifndef EQEMU_ZONE_HOUSING_HOUSING_GEOMETRY_H
#define EQEMU_ZONE_HOUSING_HOUSING_GEOMETRY_H

#include <array>
#include <cmath>

namespace housing {
  struct RegionBox {
    float x;
    float y;
    float z;
    float sizeX;
    float sizeY;
    float sizeZ;
    float heading;
  };

  // heading in EQ units (512 per turn), 0 facing +y; rows in the client's axis order Y, X, Z (playerHousing/tools/regionMatrix.py)
  inline std::array<float, 16> regionMatrix(const RegionBox &box) {
    const double angle = box.heading / 512.0 * 2.0 * M_PI;
    const double sine = std::sin(angle);
    const double cosine = std::cos(angle);
    const double halfX = box.sizeX / 2.0;
    const double halfY = box.sizeY / 2.0;
    const double halfZ = box.sizeZ / 2.0;
    return {
      static_cast<float>(-sine * halfX), static_cast<float>(cosine * halfX), 0.0f, 0.0f,
      static_cast<float>(cosine * halfY), static_cast<float>(sine * halfY), 0.0f, 0.0f,
      0.0f, 0.0f, static_cast<float>(halfZ), 0.0f,
      box.y, box.x, box.z, 1.0f
    };
  }

  inline bool regionContains(const RegionBox &box, float x, float y, float z) {
    const double angle = box.heading / 512.0 * 2.0 * M_PI;
    const double dx = x - box.x;
    const double dy = y - box.y;
    const double along = dx * std::sin(angle) + dy * std::cos(angle);
    const double across = dx * std::cos(angle) - dy * std::sin(angle);
    return std::fabs(across) <= box.sizeX / 2.0 && std::fabs(along) <= box.sizeY / 2.0 && std::fabs(z - box.z) <= box.sizeZ / 2.0;
  }
}

#endif
