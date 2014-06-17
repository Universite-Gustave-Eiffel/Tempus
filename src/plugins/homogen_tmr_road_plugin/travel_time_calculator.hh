namespace Tempus {

	class TravelTimeCalculator
	
	{
		public:
			TravelTimeCalculator( unsigned int mode, double walking_speed, double cycling_speed ) : 
				mode_(mode), walking_speed_(walking_speed), cycling_speed_(cycling_speed) {} // constructeur
			
			double operator() ( Multimodal::Graph& graph, Multimodal::Edge& e ) { 
				Road::Graph& road_graph = graph.road; 
				
                double travel_time;

		        switch ( e.connection_type() ) {  
					case Multimodal::Edge::Road2Road: {
						Road::Edge road_e;
						bool found;
						boost::tie( road_e, found ) = road_edge( e );
						if ( ( (road_graph[ road_e ].transport_type) & (mode_) ) > 0 ) {
							double speed; 
							if ( ( mode_ == 1 ) || ( mode_ == 256 ) ) {
								if ( road_graph[ road_e ].car_average_speed > 0 )
									speed = road_graph[ road_e ].car_average_speed ; 
								else if ( road_graph[ road_e ].car_speed_limit > 0 )
									speed = road_graph[ road_e ].car_speed_limit ; 
							}
							else if ( mode_ == 2 ) {
								speed = walking_speed_ ;
							}
							else if ( ( mode_ == 4 ) || ( mode_ == 128 ) ) {
								speed = cycling_speed_ ;
							}
                        travel_time = road_graph[ road_e ].length / ( speed * 1000 ) * 60 ;
						}
					}
					break ;  
					
                    default: {
                        travel_time = std::numeric_limits<double>::max();
                    }
                }
                return travel_time;
            }
				
		private:
			unsigned int mode_; // Transport mode
			double walking_speed_; 
			double cycling_speed_; 

    };
	
} // end namespace
