#ifndef __GSMODEL_HELPER_HPP__
#define __GSMODEL_HELPER_HPP__

#include "SDK/GSModel/GSModel.hpp"
#include "SDK/GSModel/IGUIModel.hpp"
#include "GSModelEvents/S2GEvents.hpp"
#include "GSModelEvents/G2SEvents.hpp"

using GSModelService = GSModel<S2GEvent::Data,
                               G2SEvent::Data,
                               IServiceModelHandler,
                               IGUIModelHandler>;

using GSModelGUI = IGUIModel<G2SEvent::Data,
                             IGUIModelHandler>;

#endif // __GSMODEL_HPP__
