-- fix transport_mode table refs commit 69e737cc
update tempus.transport_mode set speed_rule=2, toll_rule=2 where id = 8;
update tempus.transport_mode set speed_rule=4, toll_rule=4 where id = 9;
