#include "common.hh"

namespace Tempus
{
    static std::map<db_id_t, TransportType> init_transport_types_()
    {
	std::map<db_id_t, TransportType> t;
	t[ TransportCar ] = TransportType( TransportCar, 0, "Car", true, false, false ); 
	t[ TransportPedestrial ] = TransportType( TransportPedestrial, 0, "Pedestrial", false, false, false ); 
	t[ TransportCycle ] = TransportType( TransportCycle, 0, "Cycle", true, false, false ); 
	t[ TransportBus ] = TransportType( TransportBus, 0, "Bus", false, false, false ); 
	t[ TransportTramway ] = TransportType( TransportTramway, 0, "Tramway", false, false, false ); 
	t[ TransportMetro ] = TransportType( TransportMetro, 0, "Metro", false, false, false ); 
	t[ TransportTrain ] = TransportType( TransportTrain, 0, "Train", false, false, false ); 
	t[ TransportSharedCycle ] = TransportType( TransportSharedCycle, TransportCycle, "Shared cycle", true, true, false ); 
	t[ TransportSharedCar ] = TransportType( TransportSharedCar, TransportCar, "Shared car", true, true, false ); 
	t[ TransportRoller ] = TransportType( TransportRoller, TransportPedestrial | TransportCycle, "Roller", false, false, false ); 
	return t;
    }

    const std::map<db_id_t, TransportType> transport_types = init_transport_types_();
};
