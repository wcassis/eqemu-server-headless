CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

SET(hc_sources
	eq.cpp
	main.cpp
	login.cpp
	world.cpp
	zone.cpp
	pathfinder_interface.cpp
	pathfinder_nav_mesh.cpp
	pathfinder_null.cpp
	pathfinder_waypoint.cpp
	map.cpp
	position.cpp
	raycast_mesh.cpp
	hc_map.cpp
)

SET(hc_headers
	eq.h
	login.h
	world.h
	zone.h
	client.h
	types.h
	pathfinder_interface.h
	pathfinder_nav_mesh.h
	pathfinder_null.h
	pathfinder_waypoint.h
	map.h
	position.h
	zone_config.h
	water_map.h
	raycast_mesh.h
	hc_map.h
)

ADD_EXECUTABLE(hc ${hc_sources} ${hc_headers})

INSTALL(TARGETS hc RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

TARGET_LINK_LIBRARIES(hc ${SERVER_LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
