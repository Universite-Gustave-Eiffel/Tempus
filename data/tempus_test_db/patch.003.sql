-- Fix one U-turn restriction that was set on a one-way
update tempus.road_restriction set sections=array[23885,23885] where id=3;
