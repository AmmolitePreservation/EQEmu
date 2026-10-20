#ifndef EQEMU_ZONE_HOUSING_HOUSING_WRITS_H
#define EQEMU_ZONE_HOUSING_HOUSING_WRITS_H

#include "../../common/housingEventTypes.h"

namespace housing {
  void loadWrits();
  void announceGuildNeighborhood(const GuildNeighborhoodCreatedPayload &created);
}

#endif
